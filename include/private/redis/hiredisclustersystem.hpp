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

#ifndef SHAREDDATALAYER_REDIS_HIREDISCLUSTERSYSTEM_HPP_
#define SHAREDDATALAYER_REDIS_HIREDISCLUSTERSYSTEM_HPP_

#include <hircluster.h>

namespace shareddatalayer
{
    namespace redis
    {
        class HiredisClusterSystem
        {
        public:
            HiredisClusterSystem() = default;

            HiredisClusterSystem(const HiredisClusterSystem&) = delete;

            HiredisClusterSystem& operator = (const HiredisClusterSystem&) = delete;

            virtual ~HiredisClusterSystem() = default;

            virtual redisClusterAsyncContext* redisClusterAsyncConnect(const char* addrs, int flags);

            virtual int redisClusterAsyncSetConnectCallback(redisClusterAsyncContext* acc,
                                                            redisClusterInstanceConnectCallback* fn);

            virtual int redisClusterAsyncSetDisconnectCallback(redisClusterAsyncContext* acc,
                                                               redisClusterInstanceDisconnectCallback* fn);

            virtual int redisClusterAsyncCommandArgv(redisClusterAsyncContext* acc, redisClusterCallbackFn* fn,
                                                     void* privdata, int argc, const char** argv,
                                                     const size_t* argvlen);

            virtual int redisClusterAsyncCommandArgvWithKey(redisClusterAsyncContext* acc, redisClusterCallbackFn* fn,
                                                            void* privdata, const char* key, int keylen, int argc,
                                                            const char** argv, const size_t* argvlen);

            virtual void redisAsyncHandleRead(redisAsyncContext* ac);

            virtual void redisAsyncHandleWrite(redisAsyncContext* ac);

            virtual void redisClusterAsyncDisconnect(redisClusterAsyncContext* acc);

            virtual void redisClusterAsyncFree(redisClusterAsyncContext* acc);

            static HiredisClusterSystem& getInstance();
        };
    }
}

#endif
