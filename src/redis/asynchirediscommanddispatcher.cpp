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

#include "private/redis/asynchirediscommanddispatcher.hpp"
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <arpa/inet.h>
#include "private/abort.hpp"
#include "private/createlogger.hpp"
#include "private/engine.hpp"
#include "private/error.hpp"
#include "private/logger.hpp"
#include "private/redis/asyncredisreply.hpp"
#include "private/redis/reply.hpp"
#include "private/redis/hiredissystem.hpp"
#include "private/redis/hiredisepolladapter.hpp"
#include "private/redis/contents.hpp"
#include "private/redis/redisgeneral.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;

namespace
{
    void connectCb(const redisAsyncContext* ac, int status)
    {
        bool isConnected = !status;
        auto instance(static_cast<AsyncHiredisCommandDispatcher*>(ac->data));

        if (isConnected)
        {
            std::ostringstream msg;
            msg << "redis connected, fd: " << ac->c.fd;
            logInfoOnce(msg.str());
            instance->verifyConnection();
        }
        else
            instance->setDisconnected();

    }

    void disconnectCb(const redisAsyncContext* ac, int status)
    {
        if (status) {
            std::ostringstream msg;
            msg << "redis disconnected, status: " << ac->err << ", " << ac->errstr << ", fd: " << ac->c.fd;
            logErrorOnce(msg.str());
        }
        auto instance(static_cast<AsyncHiredisCommandDispatcher*>(ac->data));
        instance->setDisconnected();
    }

    void cb(redisAsyncContext* ac, void* rr, void* pd)
    {
        auto instance(static_cast<AsyncHiredisCommandDispatcher*>(ac->data));
        auto reply(static_cast<redisReply*>(rr));
        auto cb(static_cast<AsyncHiredisCommandDispatcher::CommandCb*>(pd));
        if (instance->isClientCallbacksEnabled())
            instance->handleReply(*cb, getRedisError(ac->err, ac->errstr, reply), reply);
    }
}

AsyncHiredisCommandDispatcher::AsyncHiredisCommandDispatcher(Engine& engine,
                                                             const std::string& address,
                                                             uint16_t port,
                                                             std::shared_ptr<ContentsBuilder> contentsBuilder,
                                                             bool usePermanentCommandCallbacks,
                                                             std::shared_ptr<Logger> logger,
                                                             bool usedForSentinel):
    AsyncHiredisCommandDispatcher(engine,
                                  address,
                                  port,
                                  contentsBuilder,
                                  usePermanentCommandCallbacks,
                                  HiredisSystem::getHiredisSystem(),
                                  std::make_shared<HiredisEpollAdapter>(engine),
                                  logger,
                                  usedForSentinel)
{
}

AsyncHiredisCommandDispatcher::AsyncHiredisCommandDispatcher(Engine& engine,
                                                             const std::string& address,
                                                             uint16_t port,
                                                             std::shared_ptr<ContentsBuilder> contentsBuilder,
                                                             bool usePermanentCommandCallbacks,
                                                             HiredisSystem& hiredisSystem,
                                                             std::shared_ptr<HiredisEpollAdapter> adapter,
                                                             std::shared_ptr<Logger> logger,
                                                             bool usedForSentinel):
    engine(engine),
    address(address),
    port(ntohs(port)),
    contentsBuilder(contentsBuilder),
    usePermanentCommandCallbacks(usePermanentCommandCallbacks),
    hiredisSystem(hiredisSystem),
    adapter(adapter),
    ac(nullptr),
    serviceState(ServiceState::DISCONNECTED),
    clientCallbacksEnabled(true),
    connectionRetryTimer(engine),
    connectionRetryTimerDuration(std::chrono::seconds(1)),
    connectionVerificationRetryTimerDuration(std::chrono::seconds(10)),
    logger(logger),
    usedForSentinel(usedForSentinel)

{
    connect();
}

AsyncHiredisCommandDispatcher::~AsyncHiredisCommandDispatcher()
{
    disconnectHiredis();
}

void AsyncHiredisCommandDispatcher::connect()
{
    ac = hiredisSystem.redisAsyncConnect(address.c_str(), port);
    if (ac == nullptr || ac->err)
    {
        setDisconnected();
        return;
    }
    ac->data = this;
    adapter->attach(ac);
    hiredisSystem.redisAsyncSetConnectCallback(ac, connectCb);
    hiredisSystem.redisAsyncSetDisconnectCallback(ac, disconnectCb);
}

void AsyncHiredisCommandDispatcher::verifyConnection()
{
    if (usedForSentinel)
        setConnected();
    else
    {
       /* When Redis has max amount of users, it will still accept new connections but will
        * close them immediately. Therefore, we need to verify that just established connection
        * really works. This prevents calling client readyAck callback for a connection that
        * will be terminated immediately.
        */
        /* Connection verification is now done by doing redis command list query. Because we anyway
         * need to verify that Redis has required commands, we can now combine these two operations
         * (command list query and connection verification). If either one of the functionalities
         * is not needed in the future and it is removed, remember to still leave the other one.
         */
        serviceState = ServiceState::CONNECTION_VERIFICATION;
        /* Disarm retry timer as now we are connected to hiredis. This ensures timer disarm if
         * we are spontaneously connected to redis while timer is running. If connection verification
         * fails, timer is armed again (normal handling in connection verification).
         */
        connectionRetryTimer.disarm();
        dispatchAsync(std::bind(&AsyncHiredisCommandDispatcher::verifyConnectionReply,
                                this,
                                std::placeholders::_1,
                                std::placeholders::_2),
                      contentsBuilder->build("COMMAND"),
                      false);
    }
}

void AsyncHiredisCommandDispatcher::verifyConnectionReply(const std::error_code& error,
                                                          const redis::Reply& reply)
{
    if(error)
    {
        logger->error() << "AsyncHiredisCommandDispatcher: connection verification failed: "
                        << error.message();

        if (!connectionRetryTimer.isArmed())
        {
            /* Typically if connection verification fails, hiredis will call disconnect callback and
             * whole connection establishment procedure will be restarted via that. To ensure that
             * we will retry verification even if connection would not be disconnected this timer
             * is set. If connection is later disconnected, this timer is disarmed (when disconnect
             * callback handling arms this timer again).
             */
            armConnectionRetryTimer(connectionVerificationRetryTimerDuration,
                                    std::bind(&AsyncHiredisCommandDispatcher::verifyConnection, this));
        }
    }
    else
    {
        if (checkRedisModuleCommands(parseCommandListReply(reply)))
            setConnected();
        else
            SHAREDDATALAYER_ABORT("Required Redis module extension commands not available.");
    }
}

void AsyncHiredisCommandDispatcher::waitConnectedAsync(const ConnectAck& connectAck)
{
    this->connectAck = connectAck;
    if (serviceState == ServiceState::CONNECTED)
        engine.postCallback(connectAck);
}

void AsyncHiredisCommandDispatcher::registerDisconnectCb(const DisconnectCb& disconnectCb)
{
    disconnectCallback = disconnectCb;
}

void AsyncHiredisCommandDispatcher::dispatchAsync(const CommandCb& commandCb,
                                                  const AsyncConnection::Namespace&,
                                                  const Contents& contents)
{
    dispatchAsync(commandCb, contents, true);
}

void AsyncHiredisCommandDispatcher::dispatchAsync(const CommandCb& commandCb,
                                                  const Contents& contents,
                                                  bool checkConnectionState)
{
    if (checkConnectionState && serviceState != ServiceState::CONNECTED)
    {
        engine.postCallback(std::bind(&AsyncHiredisCommandDispatcher::callCommandCbWithError,
                                       this,
                                       commandCb,
                                       std::error_code(AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED)));
        return;
    }
    cbs.push_back(commandCb);
    std::vector<const char*> chars;
    std::transform(contents.stack.begin(), contents.stack.end(),
                   std::back_inserter(chars), [](const std::string& str){ return str.c_str(); });
    if (hiredisSystem.redisAsyncCommandArgv(ac, cb, &cbs.back(), static_cast<int>(contents.stack.size()),
                                            &chars[0], &contents.sizes[0]) != REDIS_OK)
    {
        removeCb(cbs.back());
        engine.postCallback(std::bind(&AsyncHiredisCommandDispatcher::callCommandCbWithError,
                                       this,
                                       commandCb,
                                       getRedisError(ac->err, ac->errstr, nullptr)));
    }
}

void AsyncHiredisCommandDispatcher::disableCommandCallbacks()
{
    clientCallbacksEnabled = false;
}

void AsyncHiredisCommandDispatcher::callCommandCbWithError(const CommandCb& commandCb,
                                                           const std::error_code& error)
{
    commandCb(error, AsyncRedisReply());
}

void AsyncHiredisCommandDispatcher::setConnected()
{
    serviceState = ServiceState::CONNECTED;

    if (connectAck)
    {
        connectAck();
        connectAck = ConnectAck();
    }
}

void AsyncHiredisCommandDispatcher::setDisconnected()
{
    serviceState = ServiceState::DISCONNECTED;

    if (disconnectCallback)
        disconnectCallback();

    armConnectionRetryTimer(connectionRetryTimerDuration,
                            std::bind(&AsyncHiredisCommandDispatcher::connect, this));
}

void AsyncHiredisCommandDispatcher::handleReply(const CommandCb& commandCb,
                                                const std::error_code& error,
                                                const redisReply* rr)
{
    if (!isValidCb(commandCb))
        SHAREDDATALAYER_ABORT("Invalid callback function.");
    if (error)
        commandCb(error, AsyncRedisReply());
    else
        commandCb(error, AsyncRedisReply(*rr));
    if (!usePermanentCommandCallbacks)
        removeCb(commandCb);
}

bool AsyncHiredisCommandDispatcher::isClientCallbacksEnabled() const
{
    return clientCallbacksEnabled;
}

bool AsyncHiredisCommandDispatcher::isValidCb(const CommandCb& commandCb)
{
    for (auto i(cbs.begin()); i != cbs.end(); ++i)
        if (&*i == &commandCb)
            return true;
    return false;
}

void AsyncHiredisCommandDispatcher::removeCb(const CommandCb& commandCb)
{
    for (auto i(cbs.begin()); i != cbs.end(); ++i)
        if (&*i == &commandCb)
        {
            cbs.erase(i);
            break;
        }
}

void AsyncHiredisCommandDispatcher::disconnectHiredis()
{
    /* hiredis sometimes crashes if redisAsyncFree is called without being connected (even
     * if ac is a valid pointer).
     */
    if (serviceState == ServiceState::CONNECTED || serviceState == ServiceState::CONNECTION_VERIFICATION)
        hiredisSystem.redisAsyncFree(ac);

    //disconnect callback handler will update serviceState
}

void AsyncHiredisCommandDispatcher::armConnectionRetryTimer(Timer::Duration duration,
                                                            std::function<void()> retryAction)
{
    connectionRetryTimer.arm(duration,
                             [retryAction] () { retryAction(); });
}
