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

#ifndef SHAREDDATALAYER_REDIS_HIREDISSYSTEM_HPP_
#define SHAREDDATALAYER_REDIS_HIREDISSYSTEM_HPP_

#include <hiredis/hiredis.h>
#include <async.h>

namespace shareddatalayer
{
    namespace redis
    {
        class HiredisSystem
        {
        public:
            HiredisSystem() = default;

            HiredisSystem(const HiredisSystem&) = delete;

            HiredisSystem& operator = (const HiredisSystem&) = delete;

            virtual ~HiredisSystem() = default;

            virtual redisContext* redisConnect(const char* ip, int port);

            virtual void* redisCommandArgv(redisContext* context, int argc, const char** argv, const size_t* argvlen);

            virtual void freeReplyObject(void* reply);

            virtual void redisFree(redisContext* context);

            virtual redisAsyncContext* redisAsyncConnect(const char* ip, int port);

            virtual int redisAsyncSetConnectCallback(redisAsyncContext* ac, redisConnectCallback* fn);

            virtual int redisAsyncSetDisconnectCallback(redisAsyncContext* ac, redisDisconnectCallback* fn);

            virtual int redisAsyncCommandArgv(redisAsyncContext* ac,
                                              redisCallbackFn* fn,
                                              void* privdata,
                                              int argc,
                                              const char** argv,
                                              const size_t* argvlen);

            virtual void redisAsyncHandleRead(redisAsyncContext* ac);

            virtual void redisAsyncHandleWrite(redisAsyncContext* ac);

            virtual void redisAsyncDisconnect(redisAsyncContext* ac);

            virtual void redisAsyncFree(redisAsyncContext* ac);

            static HiredisSystem& getHiredisSystem() noexcept;
        };
    }
}

#endif
