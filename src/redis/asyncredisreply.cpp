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

#include "private/redis/asyncredisreply.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;

AsyncRedisReply::AsyncRedisReply():
    type(Type::NIL),
    integer(0),
    dataItem { { }, 0 }
{
}

AsyncRedisReply::AsyncRedisReply(const redisReply& rr):
    integer(0),
    dataItem { { }, 0 },
    typeMap { { REDIS_REPLY_NIL, Type::NIL }, { REDIS_REPLY_INTEGER, Type::INTEGER },
              { REDIS_REPLY_STATUS, Type::STATUS }, { REDIS_REPLY_STRING, Type::STRING },
              { REDIS_REPLY_ARRAY, Type::ARRAY } }
{
    auto res(typeMap.find(rr.type));
    if (res != typeMap.end())
    {
        type = res->second;
        parseReply(rr);
    }
}

AsyncRedisReply::Type AsyncRedisReply::getType() const
{
    return type;
}

long long AsyncRedisReply::getInteger() const
{
    return integer;
}

const AsyncRedisReply::DataItem* AsyncRedisReply::getString() const
{
    return &dataItem;
}

const AsyncRedisReply::ReplyVector* AsyncRedisReply::getArray() const
{
    return &replyVector;
}

void AsyncRedisReply::parseReply(const redisReply& rr)
{
    switch (type)
    {
        case Type::INTEGER:
            integer = rr.integer;
            break;
        case Type::STATUS:
        case Type::STRING:
            dataItem.str = std::string(rr.str, static_cast<size_t>(rr.len));
            dataItem.len = rr.len;
            break;
        case Type::ARRAY:
            parseArray(rr);
            break;
        case Type::NIL:
        default:
            break;
    }
}

void AsyncRedisReply::parseArray(const redisReply& rr)
{
    for (auto i(0U); i < rr.elements; ++i)
        replyVector.push_back(std::make_shared<AsyncRedisReply>(*rr.element[i]));
}
