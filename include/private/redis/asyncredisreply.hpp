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

#ifndef SHAREDDATALAYER_REDIS_ASYNCREDISREPLY_HPP_
#define SHAREDDATALAYER_REDIS_ASYNCREDISREPLY_HPP_

#include "private/redis/reply.hpp"
#include <map>

extern "C"
{
    struct redisReply;
}

namespace shareddatalayer
{
    namespace redis
    {
        class AsyncRedisReply: public Reply
        {
        public:
            AsyncRedisReply();

            explicit AsyncRedisReply(const redisReply& rr);

            Type getType() const override;

            long long getInteger() const override;

            const DataItem* getString() const override;

            const ReplyVector* getArray() const override;

        private:
            Type type;
            long long integer;
            DataItem dataItem;
            ReplyVector replyVector;
            std::map<int, Type> typeMap;

            void parseReply(const redisReply& rr);

            void parseArray(const redisReply& rr);
        };
    }
}

#endif
