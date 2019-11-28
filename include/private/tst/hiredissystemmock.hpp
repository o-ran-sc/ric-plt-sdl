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

#ifndef SHAREDDATALAYER_TST_HIREDISSYSTEMMOCK_HPP_
#define SHAREDDATALAYER_TST_HIREDISSYSTEMMOCK_HPP_

#include <gmock/gmock.h>
#include "private/redis/hiredissystem.hpp"

namespace shareddatalayer
{
    namespace tst
    {
        class HiredisSystemMock: public redis::HiredisSystem
        {
        public:
            MOCK_METHOD2(redisConnect, redisContext*(const char* ip, int port));

            MOCK_METHOD4(redisCommandArgv, void*(redisContext* context,
                                                 int argc,
                                                 const char** argv,
                                                 const size_t* argvlen));

            MOCK_METHOD1(freeReplyObject, void(void* reply));

            MOCK_METHOD1(redisFree, void(redisContext* context));

            MOCK_METHOD2(redisAsyncConnect, redisAsyncContext*(const char* ip, int port));

            MOCK_METHOD2(redisAsyncSetConnectCallback, int(redisAsyncContext* ac, redisConnectCallback* fn));

            MOCK_METHOD2(redisAsyncSetDisconnectCallback, int(redisAsyncContext* ac, redisDisconnectCallback* fn));

            MOCK_METHOD6(redisAsyncCommandArgv, int(redisAsyncContext* ac,
                                                    redisCallbackFn* fn,
                                                    void* privdata,
                                                    int argc,
                                                    const char** argv,
                                                    const size_t* argvlen));

            MOCK_METHOD1(redisAsyncHandleRead, void(redisAsyncContext* ac));

            MOCK_METHOD1(redisAsyncHandleWrite, void(redisAsyncContext* ac));

            MOCK_METHOD1(redisAsyncDisconnect, void(redisAsyncContext* ac));

            MOCK_METHOD1(redisAsyncFree, void(redisAsyncContext* ac));
        };
    }
}

#endif
