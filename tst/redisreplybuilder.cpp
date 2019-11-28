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

#include <cstring>
#include "private/abort.hpp"
#include "private/tst/redisreplybuilder.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;
using namespace shareddatalayer::tst;

RedisReplyBuilder::RedisReplyBuilder():
    builtRedisReplies({}),
    defaultStringValue({ 'a', 'b', 'c' }),
    errorMessage(nullptr),
    requiredRedisModuleCommands(getRequiredRedisModuleCommands()),
    commandItems({}),
    commandListQueryElements({})
{
    initArrayReplyContent();
    initCommandListQueryReplyContent();
}

RedisReplyBuilder::~RedisReplyBuilder()
{
    delete[] errorMessage;

    for (auto& redisReplyPtr : commandListQueryElements)
    {
        delete *(redisReplyPtr->element);
        delete redisReplyPtr;
    }

    for (auto& redisReplyPtr : builtRedisReplies)
    {
        delete redisReplyPtr;
    }
}

void RedisReplyBuilder::initArrayReplyContent()
{
    if (!arrayReplyElements.empty())
        SHAREDDATALAYER_ABORT("ArrayReplyContent initialization should done only once");

    arrayReplyElement1.type = REDIS_REPLY_STRING;
    arrayReplyElement1.integer = 0;
    arrayReplyElement1.str = const_cast<char*>(&defaultStringValue[0]);
    arrayReplyElement1.len = static_cast<int>(defaultStringValue.size());
    arrayReplyElements.push_back(&arrayReplyElement1);
    arrayReplyElement2.type = REDIS_REPLY_NIL;
    arrayReplyElement2.integer = 0;
    arrayReplyElement2.str = nullptr;
    arrayReplyElement2.len = 0;
    arrayReplyElements.push_back(&arrayReplyElement2);
}

void RedisReplyBuilder::initCommandListQueryReplyContent()
{
    if (!commandItems.empty() || !commandListQueryElements.empty())
        SHAREDDATALAYER_ABORT("CommandListQueryReplyContent initialization should done only once");

    redisReply * commandItem = nullptr;
    redisReply * arrayItem = nullptr;

    for (auto& requiredRedisModuleCommand : requiredRedisModuleCommands)
    {
        commandItem = new redisReply();
        commandItem->type = REDIS_REPLY_STRING;
        commandItem->integer = 0;
        commandItem->str = const_cast<char*>(requiredRedisModuleCommand.c_str());
        commandItem->len = static_cast<int>(requiredRedisModuleCommand.size());
        commandItems.push_back(commandItem);
    }

    for (auto& commandItem : commandItems)
    {
        arrayItem = new redisReply();
        arrayItem->type = REDIS_REPLY_ARRAY;
        arrayItem->elements = 1;
        arrayItem->element = &commandItem;

        commandListQueryElements.push_back(arrayItem);
    }
}

redisReply& RedisReplyBuilder::buildNilReply()
{
    auto rr = new redisReply();
    rr->type = REDIS_REPLY_NIL;
    builtRedisReplies.push_back(rr);
    return std::ref(*rr);
}

redisReply& RedisReplyBuilder::buildIntegerReply()
{
    auto rr = new redisReply();
    rr->type = REDIS_REPLY_INTEGER;
    rr->integer = 10;
    builtRedisReplies.push_back(rr);
    return std::ref(*rr);
}

redisReply& RedisReplyBuilder::buildStatusReply()
{
    auto rr = new redisReply();
    rr->type = REDIS_REPLY_STATUS;
    rr->str = const_cast<char*>(&defaultStringValue[0]);
    rr->len = static_cast<int>(defaultStringValue.size());
    builtRedisReplies.push_back(rr);
    return std::ref(*rr);
}

redisReply& RedisReplyBuilder::buildStringReply()
{
    auto rr = new redisReply();
    rr->type = REDIS_REPLY_STRING;
    rr->str = const_cast<char*>(&defaultStringValue[0]);
    rr->len = static_cast<int>(defaultStringValue.size());
    builtRedisReplies.push_back(rr);
    return std::ref(*rr);
}

redisReply& RedisReplyBuilder::buildArrayReply()
{
    auto rr = new redisReply();
    rr->type = REDIS_REPLY_ARRAY;
    rr->elements = 2;
    rr->element = &arrayReplyElements[0];
    builtRedisReplies.push_back(rr);
    return std::ref(*rr);
}

redisReply& RedisReplyBuilder::buildCommandListQueryReply()
{
    if (commandListQueryElements.empty())
        SHAREDDATALAYER_ABORT("Cannot built command list query reply");

    auto rr = new redisReply();
    rr->type = REDIS_REPLY_ARRAY;
    rr->elements = commandListQueryElements.size();
    rr->element = &commandListQueryElements[0];
    builtRedisReplies.push_back(rr);
    return std::ref(*rr);
}

redisReply& RedisReplyBuilder::buildIncompleteCommandListQueryReply()
{
    if (commandListQueryElements.empty())
        SHAREDDATALAYER_ABORT("Cannot built incomplete command list query reply");

    auto rr = new redisReply();
    rr->type = REDIS_REPLY_ARRAY;
    rr->elements = commandListQueryElements.size() - 1;
    rr->element = &commandListQueryElements[0];
    builtRedisReplies.push_back(rr);
    return std::ref(*rr);
}

redisReply& RedisReplyBuilder::buildErrorReply(const std::string& msg)
{
    auto rr = new redisReply();
    auto len(msg.size());
    delete[] errorMessage;
    errorMessage = new char[len];
    std::memcpy(errorMessage, msg.c_str(), len);
    rr->type = REDIS_REPLY_ERROR;
    rr->str = errorMessage;
    rr->len = static_cast<int>(len);
    builtRedisReplies.push_back(rr);
    return std::ref(*rr);
}
