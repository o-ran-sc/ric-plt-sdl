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

#ifndef SHAREDDATALAYER_REDIS_ASYNCCOMMANDDISPATCHER_HPP_
#define SHAREDDATALAYER_REDIS_ASYNCCOMMANDDISPATCHER_HPP_

#include <functional>
#include <system_error>
#include <string>
#include <memory>
#include <vector>
#include "private/asyncconnection.hpp"
#include "private/logger.hpp"

namespace shareddatalayer
{
    class Engine;
    namespace redis
    {
        class ContentsBuilder;
        class Reply;
        struct Contents;
        struct DatabaseInfo;

        class AsyncCommandDispatcher
        {
        public:
            AsyncCommandDispatcher(const AsyncCommandDispatcher&) = delete;

            AsyncCommandDispatcher& operator = (const AsyncCommandDispatcher&) = delete;

            virtual ~AsyncCommandDispatcher() = default;

            using ConnectAck = std::function<void()>;

            virtual void waitConnectedAsync(const ConnectAck& connectAck) = 0;

            // NOTE: If there are connections open to several database instances, callback
            // does not identify that which one was disconnected.
            using DisconnectCb = std::function<void()>;

            virtual void registerDisconnectCb(const DisconnectCb& disconnectCb) = 0;

            using CommandCb = std::function<void(const std::error_code&, const Reply&)>;

            virtual void dispatchAsync(const CommandCb& commandCb,
                                       const AsyncConnection::Namespace& ns,
                                       const Contents& contents) = 0;

            virtual void disableCommandCallbacks() = 0;

            static std::shared_ptr<AsyncCommandDispatcher> create(Engine& engine,
                                                                  const DatabaseInfo& databaseInfo,
                                                                  std::shared_ptr<ContentsBuilder> contentsBuilder,
                                                                  bool usePermanentCommandCallbacks,
                                                                  std::shared_ptr<Logger> logger,
                                                                  bool usedForSentinel);

        protected:
            AsyncCommandDispatcher() = default;
        };
    }
}

#endif
