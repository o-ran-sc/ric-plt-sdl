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

#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <sdl/asyncstorage.hpp>
#include "private/abort.hpp"
#include "private/hostandport.hpp"
#include "private/redis/asyncsentineldatabasediscovery.hpp"
#include "private/redis/asynccommanddispatcher.hpp"
#include "private/redis/contents.hpp"
#include "private/redis/contentsbuilder.hpp"
#include "private/redis/reply.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;

namespace
{
    std::shared_ptr<AsyncCommandDispatcher> asyncCommandDispatcherCreator(Engine& engine,
                                                                          const DatabaseInfo& databaseInfo,
                                                                          std::shared_ptr<ContentsBuilder> contentsBuilder,
                                                                          std::shared_ptr<Logger> logger);

    std::unique_ptr<HostAndPort> parseMasterInquiryReply(const Reply& reply, Logger& logger);
}

AsyncSentinelDatabaseDiscovery::AsyncSentinelDatabaseDiscovery(std::shared_ptr<Engine> engine,
                                                               std::shared_ptr<Logger> logger):
        AsyncSentinelDatabaseDiscovery(engine,
                                       logger,
                                       ::asyncCommandDispatcherCreator,
                                       std::make_shared<redis::ContentsBuilder>(AsyncStorage::SEPARATOR))
{
}

AsyncSentinelDatabaseDiscovery::AsyncSentinelDatabaseDiscovery(std::shared_ptr<Engine> engine,
                                                               std::shared_ptr<Logger> logger,
                                                               const AsyncCommandDispatcherCreator& asyncCommandDispatcherCreator,
                                                               std::shared_ptr<redis::ContentsBuilder> contentsBuilder):
        engine(engine),
        logger(logger),
        // @TODO Make configurable.
        databaseInfo(DatabaseInfo({DatabaseConfiguration::Addresses({HostAndPort("dbaas-ha", htons(26379U))}),
                     DatabaseInfo::Type::SINGLE,
                     boost::none,
                     DatabaseInfo::Discovery::SENTINEL})),
        contentsBuilder(contentsBuilder),
        masterInquiryRetryTimer(*engine),
        masterInquiryRetryTimerDuration(std::chrono::seconds(1))
{
    dispatcher = asyncCommandDispatcherCreator(*engine,
                                               databaseInfo,
                                               contentsBuilder,
                                               logger);
}

void AsyncSentinelDatabaseDiscovery::setStateChangedCb(const StateChangedCb& cb)
{
    stateChangedCb = cb;
    dispatcher->waitConnectedAsync(std::bind(&AsyncSentinelDatabaseDiscovery::sendMasterInquiry, this));
}

void AsyncSentinelDatabaseDiscovery::clearStateChangedCb()
{
    stateChangedCb = nullptr;
}

void AsyncSentinelDatabaseDiscovery::sendMasterInquiry()
{
    dispatcher->dispatchAsync(std::bind(&AsyncSentinelDatabaseDiscovery::masterInquiryAck,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              "dummyNamespace", // Not meaningful for SENTINEL commands
                              contentsBuilder->build("SENTINEL", "get-master-addr-by-name", "mymaster")); //@TODO Make master name configurable
}

void AsyncSentinelDatabaseDiscovery::masterInquiryAck(const std::error_code& error,
                                                      const Reply& reply)
{
    if (!error)
    {
        auto hostAndPort = parseMasterInquiryReply(reply, *logger);
        if (hostAndPort)
        {
            auto databaseInfo(DatabaseInfo({DatabaseConfiguration::Addresses({*hostAndPort}),
                                           DatabaseInfo::Type::SINGLE,
                                           boost::none,
                                           DatabaseInfo::Discovery::SENTINEL}));
            if (stateChangedCb)
                stateChangedCb(databaseInfo);
        }
        else
            SHAREDDATALAYER_ABORT("Master inquiry reply parsing error.");
    }
    else
    {
        masterInquiryRetryTimer.arm(
                masterInquiryRetryTimerDuration,
                std::bind(&AsyncSentinelDatabaseDiscovery::sendMasterInquiry, this));
    }
}

namespace
{
    std::shared_ptr<AsyncCommandDispatcher> asyncCommandDispatcherCreator(Engine& engine,
                                                                          const DatabaseInfo& databaseInfo,
                                                                          std::shared_ptr<ContentsBuilder> contentsBuilder,
                                                                          std::shared_ptr<Logger> logger)
    {
        return AsyncCommandDispatcher::create(engine,
                                              databaseInfo,
                                              contentsBuilder,
                                              false,
                                              logger,
                                              true);
    }

    std::unique_ptr<HostAndPort> parseMasterInquiryReply(const Reply& reply, Logger& logger)
    {
        auto replyType = reply.getType();
        if (replyType == Reply::Type::ARRAY)
        {
            auto& replyVector(*reply.getArray());
            auto hostElementType = replyVector[0]->getType();
            if (hostElementType == Reply::Type::STRING)
            {
                auto host(replyVector[0]->getString()->str);
                auto portElementType = replyVector[1]->getType();
                if (portElementType == Reply::Type::STRING)
                {
                    auto port(replyVector[1]->getString()->str);
                    try
                    {
                        return std::unique_ptr<HostAndPort>(new HostAndPort(host+":"+port, 0));;
                    }
                    catch (const std::exception& e)
                    {
                        logger.debug() << "Invalid host or port in master inquiry reply, host: "
                                       << host << ", port: " << port
                                       << ", exception: " << e.what() << std::endl;
                    }
                }
                else
                    logger.debug() << "Invalid port element type in master inquiry reply: "
                                   << static_cast<int>(portElementType) << std::endl;
            }
            else
                logger.debug() << "Invalid host element type in master inquiry reply: "
                               << static_cast<int>(hostElementType) << std::endl;
        }
        else
            logger.debug() << "Invalid master inquiry reply type: "
                           << static_cast<int>(replyType) << std::endl;
        return nullptr;
    }
}
