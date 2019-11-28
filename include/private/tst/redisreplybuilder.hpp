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

#ifndef SHAREDDATALAYER_TST_REDISREPLYBUILDER_HPP_
#define SHAREDDATALAYER_TST_REDISREPLYBUILDER_HPP_

#include "private/redis/redisgeneral.hpp"
#include "private/redis/reply.hpp"

namespace shareddatalayer
{
    namespace tst
    {
        class RedisReplyBuilder
        {
        public:
            RedisReplyBuilder();

            RedisReplyBuilder(const RedisReplyBuilder&) = delete;

            RedisReplyBuilder& operator = (const RedisReplyBuilder&) = delete;

            virtual ~RedisReplyBuilder();

            redisReply& buildNilReply();

            redisReply& buildIntegerReply();

            redisReply& buildStatusReply();

            redisReply& buildStringReply();

            redisReply& buildArrayReply();

            redisReply& buildCommandListQueryReply();

            redisReply& buildIncompleteCommandListQueryReply();

            redisReply& buildErrorReply(const std::string& msg);

        private:
            std::vector<redisReply*> builtRedisReplies;
            const std::vector<char> defaultStringValue;
            std::vector<redisReply*> arrayReplyElements;
            redisReply arrayReplyElement1;
            redisReply arrayReplyElement2;
            char* errorMessage;
            const std::set<std::string>& requiredRedisModuleCommands;
            std::vector<redisReply*> commandItems;
            std::vector<redisReply*> commandListQueryElements;

            void initArrayReplyContent();
            void initCommandListQueryReplyContent();
        };
    }
}

#endif
