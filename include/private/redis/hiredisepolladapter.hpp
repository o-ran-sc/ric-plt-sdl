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

#ifndef SHAREDDATALAYER_REDIS_HIREDISEPOLLADAPTER_HPP_
#define SHAREDDATALAYER_REDIS_HIREDISEPOLLADAPTER_HPP_

extern "C"
{
    struct redisAsyncContext;
}

namespace shareddatalayer
{
    class Engine;

    namespace redis
    {
        class HiredisSystem;

        class HiredisEpollAdapter
        {
        public:
            explicit HiredisEpollAdapter(Engine& engine);

            HiredisEpollAdapter(Engine& engine, HiredisSystem& hiredisSystem);

            HiredisEpollAdapter(const HiredisEpollAdapter&) = delete;

            HiredisEpollAdapter& operator = (const HiredisEpollAdapter&) = delete;

            virtual ~HiredisEpollAdapter();

            virtual void attach(redisAsyncContext* ac);

            virtual void addRead();

            virtual void delRead();

            virtual void addWrite();

            virtual void delWrite();

            virtual void cleanUp();

        private:
            HiredisSystem& hiredisSystem;
            Engine& engine;
            redisAsyncContext* ac;
            unsigned int eventState;
            bool reading;
            bool writing;
            bool isMonitoring;

            void eventHandler(unsigned int events);
        };
    }
}

#endif
