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

#ifndef SHAREDDATALAYER_REDIS_HIREDISCLUSTEREPOLLADAPTER_HPP_
#define SHAREDDATALAYER_REDIS_HIREDISCLUSTEREPOLLADAPTER_HPP_

#include <map>
#include <memory>

extern "C"
{
    struct redisClusterAsyncContext;
    struct redisAsyncContext;
}

namespace shareddatalayer
{
    class Engine;

    namespace redis
    {
        class HiredisClusterSystem;

        class HiredisClusterEpollAdapter
        {
        public:
            class Node;

            explicit HiredisClusterEpollAdapter(Engine& engine);

            HiredisClusterEpollAdapter(Engine& engine, HiredisClusterSystem& hiredisClusterSystem);

            virtual ~HiredisClusterEpollAdapter() = default;

            virtual void setup(redisClusterAsyncContext* acc);

            virtual void attach(redisAsyncContext* ac);

            virtual void detach(const redisAsyncContext* ac);

            HiredisClusterEpollAdapter(const HiredisClusterEpollAdapter&) = delete;
            HiredisClusterEpollAdapter(HiredisClusterEpollAdapter&&) = delete;
            HiredisClusterEpollAdapter& operator = (const HiredisClusterEpollAdapter&) = delete;
            HiredisClusterEpollAdapter& operator = (HiredisClusterEpollAdapter&&) = delete;

        private:
            Engine& engine;
            HiredisClusterSystem& hiredisClusterSystem;
            std::map<int, std::unique_ptr<Node>> nodes;
        };

        class HiredisClusterEpollAdapter::Node
        {
        public:
            Node(Engine& engine,
                 redisAsyncContext* ac,
                 HiredisClusterSystem& hiredisClusterSystem);

            ~Node();

            void addRead();

            void addWrite();

            void delRead();

            void delWrite();

            void cleanup();

            Node(const Node&) = delete;
            Node(Node&&) = delete;
            Node& operator = (const Node&) = delete;
            Node& operator = (Node&&) = delete;

        private:
            HiredisClusterSystem& hiredisClusterSystem;
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
