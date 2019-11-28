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

#ifndef SHAREDDATALAYER_TST_HIREDISCLUSTERSYSTEMMOCK_HPP_
#define SHAREDDATALAYER_TST_HIREDISCLUSTERSYSTEMMOCK_HPP_

#include <gmock/gmock.h>
#include "private/redis/hiredisclustersystem.hpp"

namespace shareddatalayer
{
    namespace tst
    {
        class HiredisClusterSystemMock: public redis::HiredisClusterSystem
        {
        public:
            MOCK_METHOD2(redisClusterAsyncConnect, redisClusterAsyncContext*(const char* addrs, int flags));

            MOCK_METHOD2(redisClusterAsyncSetConnectCallback, int(redisClusterAsyncContext* acc,
                                                                  redisClusterInstanceConnectCallback* fn));

            MOCK_METHOD2(redisClusterAsyncSetDisconnectCallback, int(redisClusterAsyncContext* acc,
                                                                     redisClusterInstanceDisconnectCallback* fn));

            MOCK_METHOD6(redisClusterAsyncCommandArgv, int(redisClusterAsyncContext* acc, redisClusterCallbackFn* fn,
                                                           void* privdata, int argc, const char** argv,
                                                           const size_t* argvlen));

            MOCK_METHOD8(redisClusterAsyncCommandArgvWithKey, int(redisClusterAsyncContext *acc,
                                                                  redisClusterCallbackFn *fn,
                                                                  void *privdata, const char *key, int keylen, int argc,
                                                                  const char **argv, const size_t *argvlen));

            MOCK_METHOD1(redisAsyncHandleRead, void(redisAsyncContext* ac));

            MOCK_METHOD1(redisAsyncHandleWrite, void(redisAsyncContext* ac));

            MOCK_METHOD1(redisClusterAsyncDisconnect, void(redisClusterAsyncContext* acc));

            MOCK_METHOD1(redisClusterAsyncFree, void(redisClusterAsyncContext* acc));
        };
    }
}

#endif

