/*
   Copyright (c) 2018-2019 Nokia.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/*
 * This source code is part of the near-RT RIC (RAN Intelligent Controller)
 * platform project (RICP).
*/

#include "config.h"
#include "private/createlogger.hpp"
#include "private/error.hpp"
#include "private/redis/redisgeneral.hpp"
#include <arpa/inet.h>
#include <cstring>
#if HAVE_HIREDIS_VIP
#include <hircluster.h>
#endif
#include <hiredis/hiredis.h>
#include <sstream>

using namespace shareddatalayer;
using namespace shareddatalayer::redis;

namespace
{
    bool equals(const std::string& s1, const char* s2, size_t s2Len)
    {
        if (s2 == nullptr)
        {
            logErrorOnce("redisGeneral: null pointer passed to equals function");
            return false;
        }

        return ((s1.size() == s2Len) && (std::memcmp(s1.data(), s2, s2Len) == 0));
    }

    bool startsWith(const std::string& s1, const char* s2, size_t s2Len)
    {
        if (s2 == nullptr)
        {
            logErrorOnce("redisGeneral: null pointer passed to startsWith function");
            return false;
        }

        return ((s1.size() <= s2Len) && (std::memcmp(s1.data(), s2, s1.size()) == 0));
    }

    AsyncRedisCommandDispatcherErrorCode mapRedisReplyErrorToSdlError(const redisReply* rr)
    {
        if (equals("LOADING Redis is loading the dataset in memory", rr->str, static_cast<size_t>(rr->len)))
            return AsyncRedisCommandDispatcherErrorCode::DATASET_LOADING;

        /* This error reply comes when some cluster node(s) is down and rest of the cluster
         * nodes cannot operate due to that. This error reply typically comes from nodes
         * which are working but cannot handle requests because other node(s) are down.
         * Nodes which are actually down (under failover handling) typically return
         * CLUSTER_ERROR_NOT_CONNECTED or CLUSTER_ERROR_CONNECTION_LOST.
         */
        if (startsWith("CLUSTERDOWN", rr->str, static_cast<size_t>(rr->len)))
            return AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED;

        if (startsWith("ERR Protocol error", rr->str, static_cast<size_t>(rr->len)))
            return AsyncRedisCommandDispatcherErrorCode::PROTOCOL_ERROR;

        if (startsWith("READONLY", rr->str, static_cast<size_t>(rr->len)))
            return AsyncRedisCommandDispatcherErrorCode::WRITING_TO_SLAVE;

        std::ostringstream oss;
        oss << "redis reply error: " << std::string(rr->str, static_cast<size_t>(rr->len));
        logErrorOnce(oss.str());
        return AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR;
    }

    AsyncRedisCommandDispatcherErrorCode mapRedisContextErrorToSdlError(int redisContextErr, const char* redisContextErrstr)
    {
        switch (redisContextErr)
        {
            /* From hiredis/read.h:
             * When an error occurs, the err flag in a context is set to hold the type of
             * error that occurred. REDIS_ERR_IO means there was an I/O error and you
             * should use the "errno" variable to find out what is wrong.
             * For other values, the "errstr" field will hold a description. */
            case REDIS_ERR_IO:
                if (errno == ECONNRESET)
                    return AsyncRedisCommandDispatcherErrorCode::CONNECTION_LOST;
                logErrorOnce("redis io error. Errno: " + std::to_string(errno));
                return AsyncRedisCommandDispatcherErrorCode::IO_ERROR;
            case REDIS_ERR_EOF:
                return AsyncRedisCommandDispatcherErrorCode::CONNECTION_LOST;
            case REDIS_ERR_PROTOCOL:
                return AsyncRedisCommandDispatcherErrorCode::PROTOCOL_ERROR;
            case REDIS_ERR_OOM:
                return AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY;
#if HAVE_HIREDIS_VIP
            /* hiredis_vip returns CLUSTER_ERROR_NOT_CONNECTED when cluster node is disconnected
             * but failover handling has not started yet (node_timeout not elapsed yet). In
             * this situation hiredis_vip does not send request to redis (as it is clear that
             * request cannot succeed), therefore we can map this error to NOT_CONNECTED error
             * as it best descripes this situation.
             */
            case CLUSTER_ERROR_NOT_CONNECTED:
                return AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED;
             /* hiredis_vip returns CLUSTER_ERROR_CONNECTION_LOST when connection is lost while
              * hiredis is waiting for reply from redis.
              */
            case CLUSTER_ERROR_CONNECTION_LOST:
                return AsyncRedisCommandDispatcherErrorCode::CONNECTION_LOST;
#endif
            default:
                std::ostringstream oss;
                oss << "redis error: "
                    << redisContextErrstr
                    << " (" << redisContextErr << ")";
                logErrorOnce(oss.str());
                return AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR;
        }
    }
}

namespace shareddatalayer
{
    namespace redis
    {
        std::string formatToClusterSyntax(const DatabaseConfiguration::Addresses& addresses)
        {
            std::ostringstream oss;
            for (auto i(addresses.begin()); i != addresses.end(); ++i)
            {
                oss << i->getHost() << ':' << ntohs(i->getPort());
                if (i == --addresses.end())
                    break;
                else
                    oss << ',';
            }
            return oss.str();
        }

        const std::set<std::string>& getRequiredRedisModuleCommands()
        {
            static const std::set<std::string> requiredRedisModuleCommands({"msetpub","setie","setiepub","setnxpub","delpub","delie","deliepub"});
            return requiredRedisModuleCommands;
        }

        std::error_code getRedisError(int redisContextErr, const char* redisContextErrstr, const redisReply* rr)
        {
            if (rr != nullptr)
            {
                if (rr->type != REDIS_REPLY_ERROR)
                    return std::error_code();

                return std::error_code(mapRedisReplyErrorToSdlError(rr));
            }

            return std::error_code(mapRedisContextErrorToSdlError(redisContextErr, redisContextErrstr));
        }

        std::set<std::string> parseCommandListReply(const redis::Reply& reply)
        {
            std::set<std::string> availableCommands;
            auto replyArray(reply.getArray());
            for (const auto& j : *replyArray)
            {
                auto element = j->getArray();
                auto command = element->front()->getString();
                availableCommands.insert(command->str);
            }
            return availableCommands;
        }

        bool checkRedisModuleCommands(const std::set<std::string>& availableCommands)
        {
            std::set<std::string> missingCommands;

            for (const auto& i : getRequiredRedisModuleCommands())
            {
                const auto it = availableCommands.find(i);
                if (it == availableCommands.end())
                    missingCommands.insert(i);
            }
            if (!missingCommands.empty())
            {
                logErrorOnce("Missing Redis module extension commands:");
                for (const auto& i : missingCommands)
                    logErrorOnce(i);
            }
            return missingCommands.empty();
        }
    }
}
