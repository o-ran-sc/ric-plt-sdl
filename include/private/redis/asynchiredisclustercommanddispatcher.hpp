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

#ifndef SHAREDDATALAYER_REDIS_ASYNCHIREDISCLUSTERCOMMANDDISPATCHER_HPP_
#define SHAREDDATALAYER_REDIS_ASYNCHIREDISCLUSTERCOMMANDDISPATCHER_HPP_

#include "private/redis/asynccommanddispatcher.hpp"
#include "private/databaseconfiguration.hpp"
#include "private/logger.hpp"
#include "private/timer.hpp"
#include <string>
#include <set>
#include <list>
#include <vector>
#include <map>
#include <memory>
#include <queue>
#include <boost/optional.hpp>

extern "C"
{
    struct redisReply;
    struct redisClusterAsyncContext;
    struct redisAsyncContext;
}

namespace shareddatalayer
{
    class Engine;

    namespace redis
    {
        class HiredisClusterSystem;
        class HiredisClusterEpollAdapter;
        class Reply;

        class AsyncHiredisClusterCommandDispatcher: public AsyncCommandDispatcher
        {
        public:
            AsyncHiredisClusterCommandDispatcher(const AsyncHiredisClusterCommandDispatcher&) = delete;

            AsyncHiredisClusterCommandDispatcher& operator = (const AsyncHiredisClusterCommandDispatcher&) = delete;

            AsyncHiredisClusterCommandDispatcher(Engine& engine,
                                                 const boost::optional<std::string>& ns,
                                                 const DatabaseConfiguration::Addresses& addresses,
                                                 std::shared_ptr<ContentsBuilder> contentsBuilder,
                                                 bool usePermanentCommandCallbacks,
                                                 std::shared_ptr<Logger> logger);

            AsyncHiredisClusterCommandDispatcher(Engine& engine,
                                                 const boost::optional<std::string>& ns,
                                                 const DatabaseConfiguration::Addresses& addresses,
                                                 std::shared_ptr<ContentsBuilder> contentsBuilder,
                                                 bool usePermanentCommandCallbacks,
                                                 HiredisClusterSystem& hiredisClusterSystem,
                                                 std::shared_ptr<HiredisClusterEpollAdapter> adapter,
                                                 std::shared_ptr<Logger> logger);

            ~AsyncHiredisClusterCommandDispatcher() override;

            void waitConnectedAsync(const ConnectAck& connectAck) override;

            void registerDisconnectCb(const DisconnectCb& disconnectCb) override;

            void dispatchAsync(const CommandCb& commandCb, const AsyncConnection::Namespace& ns, const Contents& contents) override;

            void disableCommandCallbacks() override;

            void handleReply(const CommandCb& commandCb, const std::error_code& error, const redisReply* rr);

            bool isClientCallbacksEnabled() const;

            void handleDisconnect(const redisAsyncContext* ac);

        private:
            enum class ServiceState
            {
                DISCONNECTED,
                CONNECTED
            };

            using Callback = std::function<void(const Reply&)>;

            Engine& engine;
            const boost::optional<std::string> initialNamespace;
            const DatabaseConfiguration::Addresses addresses;
            std::shared_ptr<ContentsBuilder> contentsBuilder;
            bool usePermanentCommandCallbacks;
            HiredisClusterSystem& hiredisClusterSystem;
            std::shared_ptr<HiredisClusterEpollAdapter> adapter;
            redisClusterAsyncContext* acc;
            ConnectAck connectAck;
            DisconnectCb disconnectCallback;
            ServiceState serviceState;
            std::list<CommandCb> cbs;
            bool clientCallbacksEnabled;
            Timer connectionRetryTimer;
            Timer::Duration connectionRetryTimerDuration;
            std::shared_ptr<Logger> logger;

            void connect();

            bool isValidCb(const CommandCb& commandCb);

            void removeCb(const CommandCb& commandCb);

            void callCommandCbWithError(const CommandCb& commandCb, const std::error_code& error);

            void dispatchAsync(const CommandCb& commandCb, const AsyncConnection::Namespace& ns, const Contents& contents, bool checkConnectionState);

            void verifyConnection();

            void verifyConnectionReply(const std::error_code& error, const redis::Reply& reply);

            void setConnected();

            void armConnectionRetryTimer();

            void disconnectHiredisCluster();
        };
    }
}

#endif
