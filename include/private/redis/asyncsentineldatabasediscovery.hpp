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

#ifndef SHAREDDATALAYER_REDIS_ASYNCSENTINELDATABASEDISCOVERY_HPP_
#define SHAREDDATALAYER_REDIS_ASYNCSENTINELDATABASEDISCOVERY_HPP_

#include <functional>
#include <system_error>
#include "private/redis/asyncdatabasediscovery.hpp"
#include "private/redis/databaseinfo.hpp"
#include "private/logger.hpp"
#include "private/timer.hpp"

namespace shareddatalayer
{
    namespace redis
    {
        class AsyncCommandDispatcher;
        struct Contents;
        class ContentsBuilder;
        class Reply;
    }

    class Engine;

    namespace redis
    {
        class AsyncSentinelDatabaseDiscovery: public AsyncDatabaseDiscovery
        {
        public:
            using AsyncCommandDispatcherCreator = std::function<std::shared_ptr<redis::AsyncCommandDispatcher>(Engine& engine,
                                                                                                               const redis::DatabaseInfo& databaseInfo,
                                                                                                               std::shared_ptr<redis::ContentsBuilder> contentsBuilder,
                                                                                                               std::shared_ptr<Logger> logger,
                                                                                                               bool usePermanentCommandCallbacks)>;

            AsyncSentinelDatabaseDiscovery(const AsyncSentinelDatabaseDiscovery&) = delete;

            AsyncSentinelDatabaseDiscovery& operator = (const AsyncSentinelDatabaseDiscovery&) = delete;

            AsyncSentinelDatabaseDiscovery(std::shared_ptr<Engine> engine,
                                           std::shared_ptr<Logger> logger,
                                           const HostAndPort& sentinelAddress,
                                           const std::string& sentinelMasterName);

            AsyncSentinelDatabaseDiscovery(std::shared_ptr<Engine> engine,
                                           std::shared_ptr<Logger> logger,
                                           const HostAndPort& sentinelAddress,
                                           const std::string& sentinelMasterName,
                                           const AsyncCommandDispatcherCreator& asyncCommandDispatcherCreator,
                                           std::shared_ptr<redis::ContentsBuilder> contentsBuilder);

            ~AsyncSentinelDatabaseDiscovery() override;

            void setStateChangedCb(const StateChangedCb& stateChangedCb) override;

            void clearStateChangedCb() override;

            void setConnected(bool state);
        private:
            std::shared_ptr<Engine> engine;
            std::shared_ptr<Logger> logger;
            StateChangedCb stateChangedCb;
            DatabaseInfo databaseInfo;
            std::string sentinelMasterName;
            std::shared_ptr<redis::AsyncCommandDispatcher> subscriber;
            std::shared_ptr<redis::AsyncCommandDispatcher> dispatcher;
            std::shared_ptr<redis::ContentsBuilder> contentsBuilder;
            Timer subscribeRetryTimer;
            Timer::Duration subscribeRetryTimerDuration;
            Timer masterInquiryRetryTimer;
            Timer::Duration masterInquiryRetryTimerDuration;

            void subscribeNotifications();

            void subscribeAck(const std::error_code& error, const Reply& reply);

            void sendMasterInquiry();

            void masterInquiryAck(const std::error_code& error, const Reply& reply);
        };
    }
}

#endif
