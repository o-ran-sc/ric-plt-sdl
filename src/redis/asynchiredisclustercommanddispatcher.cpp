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

#include "private/redis/asynchiredisclustercommanddispatcher.hpp"
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <sstream>
#include "private/abort.hpp"
#include "private/createlogger.hpp"
#include "private/error.hpp"
#include "private/logger.hpp"
#include "private/redis/asyncredisreply.hpp"
#include "private/redis/reply.hpp"
#include "private/redis/hiredisclustersystem.hpp"
#include "private/engine.hpp"
#include "private/redis/hiredisclusterepolladapter.hpp"
#include "private/redis/contents.hpp"
#include "private/redis/redisgeneral.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;

namespace
{
    void connectCb(const redisClusterAsyncContext*, const redisAsyncContext* ac, int status)
    {
        if (!status)
        {
            std::ostringstream msg;
            msg << "redis cluster instance connected, fd: " << ac->c.fd;
            logDebugOnce(msg.str());
        }
    }

    void disconnectCb(const redisClusterAsyncContext* acc, const redisAsyncContext* ac, int status)
    {
        std::ostringstream msg;
        msg << "redis cluster instance disconnected, status: " << ac->err
            << ", " << ac->errstr << ", fd: " << ac->c.fd << std::endl;
        logDebugOnce(msg.str());

        auto instance(static_cast<AsyncHiredisClusterCommandDispatcher*>(acc->data));
        instance->handleDisconnect(ac);
    }

    void cb(redisClusterAsyncContext* acc, void* rr, void* pd)
    {
        auto instance(static_cast<AsyncHiredisClusterCommandDispatcher*>(acc->data));
        auto reply(static_cast<redisReply*>(rr));
        auto cb(static_cast<AsyncHiredisClusterCommandDispatcher::CommandCb*>(pd));
        if (instance->isClientCallbacksEnabled())
            instance->handleReply(*cb, getRedisError(acc->err, acc->errstr, reply), reply);
    }
}

AsyncHiredisClusterCommandDispatcher::AsyncHiredisClusterCommandDispatcher(Engine& engine,
                                                                           const boost::optional<std::string>& ns,
                                                                           const DatabaseConfiguration::Addresses& addresses,
                                                                           std::shared_ptr<ContentsBuilder> contentsBuilder,
                                                                           bool usePermanentCommandCallbacks,
                                                                           std::shared_ptr<Logger> logger):
    AsyncHiredisClusterCommandDispatcher(engine,
                                         ns,
                                         addresses,
                                         contentsBuilder,
                                         usePermanentCommandCallbacks,
                                         HiredisClusterSystem::getInstance(),
                                         std::make_shared<HiredisClusterEpollAdapter>(engine),
                                         logger)
{
}

AsyncHiredisClusterCommandDispatcher::AsyncHiredisClusterCommandDispatcher(Engine& engine,
                                                                           const boost::optional<std::string>& ns,
                                                                           const DatabaseConfiguration::Addresses& addresses,
                                                                           std::shared_ptr<ContentsBuilder> contentsBuilder,
                                                                           bool usePermanentCommandCallbacks,
                                                                           HiredisClusterSystem& hiredisClusterSystem,
                                                                           std::shared_ptr<HiredisClusterEpollAdapter> adapter,
                                                                           std::shared_ptr<Logger> logger):
    engine(engine),
    initialNamespace(ns),
    addresses(addresses),
    contentsBuilder(contentsBuilder),
    usePermanentCommandCallbacks(usePermanentCommandCallbacks),
    hiredisClusterSystem(hiredisClusterSystem),
    adapter(adapter),
    acc(nullptr),
    serviceState(ServiceState::DISCONNECTED),
    clientCallbacksEnabled(true),
    connectionRetryTimer(engine),
    connectionRetryTimerDuration(std::chrono::seconds(1)),
    logger(logger)
{
    connect();
}

AsyncHiredisClusterCommandDispatcher::~AsyncHiredisClusterCommandDispatcher()
{
    disconnectHiredisCluster();
}

void AsyncHiredisClusterCommandDispatcher::connect()
{
    // Disconnect and free possibly (in re-connecting scenarios) already existing Redis cluster context.
    disconnectHiredisCluster();
    acc = hiredisClusterSystem.redisClusterAsyncConnect(formatToClusterSyntax(addresses).c_str(),
                                                        HIRCLUSTER_FLAG_ROUTE_USE_SLOTS);
    if (acc == nullptr)
    {
        logger->error() << "SDL: connecting to redis cluster failed, null context returned";
        armConnectionRetryTimer();
        return;
    }
    if (acc->err)
    {
        logger->error() << "SDL: connecting to redis cluster failed, error: " << acc->err;
        armConnectionRetryTimer();
        return;
    }
    acc->data = this;
    adapter->setup(acc);
    hiredisClusterSystem.redisClusterAsyncSetConnectCallback(acc, connectCb);
    hiredisClusterSystem.redisClusterAsyncSetDisconnectCallback(acc, disconnectCb);
    verifyConnection();
}

void AsyncHiredisClusterCommandDispatcher::verifyConnection()
{
    /* redisClusterAsyncConnect only queries available cluster nodes but it does
     * not connect to any cluster node (as it does not know to which node it should connect to).
     * As we use same cluster node for one SDL namespace (namespace is a hash tag), it is already
     * determined to which cluster node this instance will connect to. We do initial operation
     * to get connection to right redis node established already now. This also verifies that
     * connection really works. When Redis has max amount of users, it will still accept new
     * connections but is will close them immediately. Therefore, we need to verify that just
     * established connection really works.
     */
    /* Connection setup/verification is now done by doing redis command list query. Because we anyway
     * need to verify that Redis has required commands, we can now combine these two operations
     * (command list query and connection setup/verification). If either one of the functionalities
     * is not needed in the future and it is removed, remember to still leave the other one.
     */
    /* Non namespace-specific command list query can be used for connection setup purposes,
     * because SDL uses redisClusterAsyncCommandArgvWithKey which adds namespace to all
     * commands dispacthed.
     */

    /* If initial namespace was not given during dispatcher creation (multi namespace API),
     * verification is sent to hardcoded namespace. This works for verification purposes
     * because in our environment cluster is configured to operate only if all nodes
     * are working (so we can do verification to any node). However, this is not optimal
     * because we do not necessarily connect to such cluster node which will later be
     * used by client. Also our cluster configuration can change. This needs to be
     * optimized later (perhaps to connect to all nodes). */
    std::string nsForVerification;
    if (initialNamespace)
        nsForVerification = *initialNamespace;
    else
        nsForVerification = "namespace";

    dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcher::verifyConnectionReply,
                            this,
                            std::placeholders::_1,
                            std::placeholders::_2),
                  nsForVerification,
                  contentsBuilder->build("COMMAND"),
                  false);
}

void AsyncHiredisClusterCommandDispatcher::verifyConnectionReply(const std::error_code& error, const redis::Reply& reply)
{
    if(error)
    {
        logger->error() << "AsyncHiredisClusterCommandDispatcher: connection verification failed: "
                        << error.message();
        armConnectionRetryTimer();
    }
    else
    {
        if (checkRedisModuleCommands(parseCommandListReply(reply)))
            setConnected();
        else
            SHAREDDATALAYER_ABORT("Required Redis module extension commands not available.");
    }
}

void AsyncHiredisClusterCommandDispatcher::waitConnectedAsync(const ConnectAck& connectAck)
{
    this->connectAck = connectAck;
    if (serviceState == ServiceState::CONNECTED)
        engine.postCallback(connectAck);
}

void AsyncHiredisClusterCommandDispatcher::registerDisconnectCb(const DisconnectCb& disconnectCb)
{
    disconnectCallback = disconnectCb;
}

void AsyncHiredisClusterCommandDispatcher::dispatchAsync(const CommandCb& commandCb,
                                                         const AsyncConnection::Namespace& ns,
                                                         const Contents& contents)
{
    dispatchAsync(commandCb, ns, contents, true);
}

void AsyncHiredisClusterCommandDispatcher::dispatchAsync(const CommandCb& commandCb,
                                                         const AsyncConnection::Namespace& ns,
                                                         const Contents& contents,
                                                         bool checkConnectionState)
{
    if (checkConnectionState && serviceState != ServiceState::CONNECTED)
    {
        engine.postCallback(std::bind(&AsyncHiredisClusterCommandDispatcher::callCommandCbWithError,
                                       this,
                                       commandCb,
                                       std::error_code(AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED)));
        return;
    }
    cbs.push_back(commandCb);
    std::vector<const char*> chars;
    std::transform(contents.stack.begin(), contents.stack.end(),
                   std::back_inserter(chars), [](const std::string& str){ return str.c_str(); });
    if (hiredisClusterSystem.redisClusterAsyncCommandArgvWithKey(acc, cb, &cbs.back(), ns.c_str(), static_cast<int>(ns.size()),
                                                                 static_cast<int>(contents.stack.size()), &chars[0],
                                                                 &contents.sizes[0]) != REDIS_OK)
    {
        removeCb(cbs.back());
        engine.postCallback(std::bind(&AsyncHiredisClusterCommandDispatcher::callCommandCbWithError,
                                       this,
                                       commandCb,
                                       getRedisError(acc->err, acc->errstr, nullptr)));
    }
}

void AsyncHiredisClusterCommandDispatcher::disableCommandCallbacks()
{
    clientCallbacksEnabled = false;
}

void AsyncHiredisClusterCommandDispatcher::callCommandCbWithError(const CommandCb& commandCb, const std::error_code& error)
{
    commandCb(error, AsyncRedisReply());
}

void AsyncHiredisClusterCommandDispatcher::setConnected()
{
    serviceState = ServiceState::CONNECTED;

    if (connectAck)
    {
        connectAck();
        connectAck = ConnectAck();
    }
}

void AsyncHiredisClusterCommandDispatcher::armConnectionRetryTimer()
{
    connectionRetryTimer.arm(connectionRetryTimerDuration,
                             [this] () { connect(); });

}

void AsyncHiredisClusterCommandDispatcher::handleReply(const CommandCb& commandCb,
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

bool AsyncHiredisClusterCommandDispatcher::isClientCallbacksEnabled() const
{
    return clientCallbacksEnabled;
}

bool AsyncHiredisClusterCommandDispatcher::isValidCb(const CommandCb& commandCb)
{
    for (auto i(cbs.begin()); i != cbs.end(); ++i)
        if (&*i == &commandCb)
            return true;
    return false;
}

void AsyncHiredisClusterCommandDispatcher::removeCb(const CommandCb& commandCb)
{
    for (auto i(cbs.begin()); i != cbs.end(); ++i)
        if (&*i == &commandCb)
        {
            cbs.erase(i);
            break;
        }
}

void AsyncHiredisClusterCommandDispatcher::handleDisconnect(const redisAsyncContext* ac)
{
    adapter->detach(ac);

    if (disconnectCallback)
        disconnectCallback();
}

void AsyncHiredisClusterCommandDispatcher::disconnectHiredisCluster()
{
    /* hiredis sometimes crashes if redisClusterAsyncFree is called without being connected (even
     * if acc is a valid pointer).
     */
    if (serviceState == ServiceState::CONNECTED)
        hiredisClusterSystem.redisClusterAsyncFree(acc);

    serviceState = ServiceState::DISCONNECTED;
}
