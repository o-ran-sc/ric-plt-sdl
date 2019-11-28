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

#ifndef SHAREDDATALAYER_REDIS_ASYNCHIREDISCOMMANDDISPATCHER_HPP_
#define SHAREDDATALAYER_REDIS_ASYNCHIREDISCOMMANDDISPATCHER_HPP_

#include "private/redis/asynccommanddispatcher.hpp"
#include <string>
#include <list>
#include <vector>
#include <map>
#include <memory>
#include <queue>
#include "private/logger.hpp"
#include "private/timer.hpp"

extern "C"
{
    struct redisReply;
    struct redisAsyncContext;
}

namespace shareddatalayer
{
    class Engine;

    namespace redis
    {
        class HiredisSystem;
        class HiredisEpollAdapter;
        class Reply;

        class AsyncHiredisCommandDispatcher: public AsyncCommandDispatcher
        {
        public:
            AsyncHiredisCommandDispatcher(const AsyncHiredisCommandDispatcher&) = delete;

            AsyncHiredisCommandDispatcher& operator = (const AsyncHiredisCommandDispatcher&) = delete;

            AsyncHiredisCommandDispatcher(Engine& engine,
                                          const std::string& address,
                                          uint16_t port,
                                          std::shared_ptr<ContentsBuilder> contentsBuilder,
                                          bool usePermanentCommandCallbacks,
                                          std::shared_ptr<Logger> logger,
                                          bool usedForSentinel);

            AsyncHiredisCommandDispatcher(Engine& engine,
                                          const std::string& address,
                                          uint16_t port,
                                          std::shared_ptr<ContentsBuilder> contentsBuilder,
                                          bool usePermanentCommandCallbacks,
                                          HiredisSystem& hiredisSystem,
                                          std::shared_ptr<HiredisEpollAdapter> adapter,
                                          std::shared_ptr<Logger> logger,
                                          bool usedForSentinel);

            ~AsyncHiredisCommandDispatcher() override;

            void waitConnectedAsync(const ConnectAck& connectAck) override;

            void registerDisconnectCb(const DisconnectCb& disconnectCb) override;

            void dispatchAsync(const CommandCb& commandCb, const AsyncConnection::Namespace& ns,
                               const Contents& contents) override;

            void disableCommandCallbacks() override;

            void setConnected();

            void setDisconnected();

            void handleReply(const CommandCb& commandCb,
                             const std::error_code& error,
                             const redisReply* rr);

            bool isClientCallbacksEnabled() const;

            void verifyConnection();

            void disconnectHiredis();

            void armConnectionRetryTimer(Timer::Duration duration,
                                         std::function<void()> retryAction);

        private:
            enum class ServiceState
            {   DISCONNECTED,
                CONNECTION_VERIFICATION,
                CONNECTED
            };

            using Callback = std::function<void(const Reply&)>;

            Engine& engine;
            std::string address;
            uint16_t port;
            std::shared_ptr<ContentsBuilder> contentsBuilder;
            bool usePermanentCommandCallbacks;
            HiredisSystem& hiredisSystem;
            std::shared_ptr<HiredisEpollAdapter> adapter;
            redisAsyncContext* ac;
            ConnectAck connectAck;
            DisconnectCb disconnectCallback;
            ServiceState serviceState;
            std::list<CommandCb> cbs;
            bool clientCallbacksEnabled;
            Timer connectionRetryTimer;
            Timer::Duration connectionRetryTimerDuration;
            Timer::Duration connectionVerificationRetryTimerDuration;
            std::shared_ptr<Logger> logger;
            bool usedForSentinel;

            void connect();

            bool isValidCb(const CommandCb& commandCb);

            void removeCb(const CommandCb& commandCb);

            void callCommandCbWithError(const CommandCb& commandCb, const std::error_code& error);

            void dispatchAsync(const CommandCb& commandCb, const Contents& contents, bool checkConnectionState);

            void verifyConnectionReply(const std::error_code& error, const redis::Reply& reply);
        };
    }
}

#endif
