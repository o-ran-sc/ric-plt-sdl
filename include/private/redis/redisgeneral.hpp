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

#ifndef SHAREDDATALAYER_REDIS_REDISGENERAL_HPP_
#define SHAREDDATALAYER_REDIS_REDISGENERAL_HPP_

#include "private/databaseconfiguration.hpp"
#include "private/redis/asynccommanddispatcher.hpp"
#include "private/redis/contents.hpp"
#include "private/redis/contentsbuilder.hpp"
#include "private/redis/reply.hpp"
#include <system_error>

extern "C"
{
    struct redisReply;
}

namespace shareddatalayer
{
    namespace redis
    {
        std::string formatToClusterSyntax(const DatabaseConfiguration::Addresses& addresses);
        const std::set<std::string>& getRequiredRedisModuleCommands();
        std::error_code getRedisError(int redisContextErr, const char* redisContextErrstr, const redisReply* rr);
        std::set<std::string> parseCommandListReply(const redis::Reply& reply);
        bool checkRedisModuleCommands(const std::set<std::string>& availableCommands);
    }
}

#endif
