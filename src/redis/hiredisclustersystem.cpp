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

#include "private/redis/hiredisclustersystem.hpp"

using namespace shareddatalayer::redis;

redisClusterAsyncContext* HiredisClusterSystem::redisClusterAsyncConnect(const char* addrs, int flags)
{
    return ::redisClusterAsyncConnect(addrs, flags);
}

int HiredisClusterSystem::redisClusterAsyncSetConnectCallback(redisClusterAsyncContext* acc,
                                                              redisClusterInstanceConnectCallback* fn)
{
    return ::redisClusterAsyncSetConnectCallback(acc, fn);
}

int HiredisClusterSystem::redisClusterAsyncSetDisconnectCallback(redisClusterAsyncContext* acc,
                                                                 redisClusterInstanceDisconnectCallback* fn)
{
    return ::redisClusterAsyncSetDisconnectCallback(acc, fn);
}

int HiredisClusterSystem::redisClusterAsyncCommandArgv(redisClusterAsyncContext* acc, redisClusterCallbackFn* fn,
                                                       void* privdata, int argc, const char** argv,
                                                       const size_t* argvlen)
{
    return ::redisClusterAsyncCommandArgv(acc, fn, privdata, argc, argv, argvlen);
}

int HiredisClusterSystem::redisClusterAsyncCommandArgvWithKey(redisClusterAsyncContext *acc, redisClusterCallbackFn *fn,
                                                              void *privdata, const char *key, int keylen, int argc,
                                                              const char **argv, const size_t *argvlen)
{
    return ::redisClusterAsyncCommandArgvWithKey(acc, fn, privdata, key, keylen, argc, argv, argvlen);
}

void HiredisClusterSystem::redisAsyncHandleRead(redisAsyncContext* ac)
{
    ::redisAsyncHandleRead(ac);
}

void HiredisClusterSystem::redisAsyncHandleWrite(redisAsyncContext* ac)
{
    ::redisAsyncHandleWrite(ac);
}

void HiredisClusterSystem::redisClusterAsyncDisconnect(redisClusterAsyncContext* acc)
{
    ::redisClusterAsyncDisconnect(acc);
}

void HiredisClusterSystem::redisClusterAsyncFree(redisClusterAsyncContext* acc)
{
    ::redisClusterAsyncFree(acc);
}

HiredisClusterSystem& HiredisClusterSystem::getInstance()
{
    static HiredisClusterSystem instance;
    return instance;
}
