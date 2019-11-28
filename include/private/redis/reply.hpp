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

#ifndef SHAREDDATALAYER_REDIS_REPLY_HPP_
#define SHAREDDATALAYER_REDIS_REPLY_HPP_

#include <string>
#include <vector>
#include <memory>
#include <hiredis/hiredis.h>

namespace shareddatalayer
{
    namespace redis
    {
        using ReplyStringLength = decltype(::redisReply::len);

        class Reply
        {
        public:
            Reply(const Reply&) = delete;

            Reply& operator = (const Reply&) = delete;

            virtual ~Reply() = default;

            enum class Type
            {
                NIL,
                INTEGER,
                STATUS,
                STRING,
                ARRAY
            };

            struct DataItem
            {
                std::string str;
                ReplyStringLength len;
            };

            virtual Type getType() const = 0;

            virtual long long getInteger() const = 0;

            virtual const DataItem* getString() const = 0;

            using ReplyVector = std::vector<std::shared_ptr<Reply>>;

            virtual const ReplyVector* getArray() const = 0;

        protected:
            Reply() = default;
        };
    }
}

#endif
