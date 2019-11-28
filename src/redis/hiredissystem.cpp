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

#include "private/redis/hiredissystem.hpp"

using namespace shareddatalayer::redis;

redisContext* HiredisSystem::redisConnect(const char* ip, int port)
{
    return ::redisConnect(ip, port);
}

void* HiredisSystem::redisCommandArgv(redisContext* context, int argc, const char** argv, const size_t* argvlen)
{
    return ::redisCommandArgv(context, argc, argv, argvlen);
}

void HiredisSystem::freeReplyObject(void* reply)
{
    ::freeReplyObject(reply);
}

void HiredisSystem::redisFree(redisContext* context)
{
    ::redisFree(context);
}

redisAsyncContext* HiredisSystem::redisAsyncConnect(const char* ip, int port)
{
    return ::redisAsyncConnect(ip, port);
}

int HiredisSystem::redisAsyncSetConnectCallback(redisAsyncContext* ac, redisConnectCallback* fn)
{
    return ::redisAsyncSetConnectCallback(ac, fn);
}

int HiredisSystem::redisAsyncSetDisconnectCallback(redisAsyncContext* ac, redisDisconnectCallback* fn)
{
    return ::redisAsyncSetDisconnectCallback(ac, fn);
}

int HiredisSystem::redisAsyncCommandArgv(redisAsyncContext* ac,
                                         redisCallbackFn* fn,
                                         void* privdata,
                                         int argc,
                                         const char** argv,
                                         const size_t* argvlen)
{
    return ::redisAsyncCommandArgv(ac, fn, privdata, argc, argv, argvlen);
}

void HiredisSystem::redisAsyncHandleRead(redisAsyncContext* ac)
{
    ::redisAsyncHandleRead(ac);
}

void HiredisSystem::redisAsyncHandleWrite(redisAsyncContext* ac)
{
    ::redisAsyncHandleWrite(ac);
}

void HiredisSystem::redisAsyncDisconnect(redisAsyncContext* ac)
{
    ::redisAsyncDisconnect(ac);
}

void HiredisSystem::redisAsyncFree(redisAsyncContext* ac)
{
    ::redisAsyncFree(ac);
}

HiredisSystem& HiredisSystem::getHiredisSystem() noexcept
{
    static HiredisSystem hiredisSystem;
    return hiredisSystem;
}
