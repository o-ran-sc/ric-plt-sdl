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

#include <arpa/inet.h>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <string>
#include <vector>
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
                                                                          std::shared_ptr<Logger> logger,
                                                                          bool usePermanentCommandCallbacks);

    struct SubscribeReply
    {
        enum class Type { UNKNOWN, SUBSCRIBE_REPLY, NOTIFICATION };
        Type type;
        std::string message;

        SubscribeReply(): type(Type::UNKNOWN) { }
    };

    std::unique_ptr<SubscribeReply> parseSubscribeReply(const Reply& reply, Logger& logger);

    std::unique_ptr<HostAndPort> parseMasterInquiryReply(const Reply& reply, Logger& logger);

    std::unique_ptr<HostAndPort> parseNotificationMessage(const std::string& message, Logger& logger);
}

AsyncSentinelDatabaseDiscovery::AsyncSentinelDatabaseDiscovery(std::shared_ptr<Engine> engine,
                                                               std::shared_ptr<Logger> logger,
                                                               const HostAndPort& sentinelAddress,
                                                               const std::string& sentinelMasterName):
        AsyncSentinelDatabaseDiscovery(engine,
                                       logger,
                                       sentinelAddress,
                                       sentinelMasterName,
                                       ::asyncCommandDispatcherCreator,
                                       std::make_shared<redis::ContentsBuilder>(AsyncStorage::SEPARATOR))
{
}

AsyncSentinelDatabaseDiscovery::AsyncSentinelDatabaseDiscovery(std::shared_ptr<Engine> engine,
                                                               std::shared_ptr<Logger> logger,
                                                               const HostAndPort& sentinelAddress,
                                                               const std::string& sentinelMasterName,
                                                               const AsyncCommandDispatcherCreator& asyncCommandDispatcherCreator,
                                                               std::shared_ptr<redis::ContentsBuilder> contentsBuilder):
        engine(engine),
        logger(logger),
        databaseInfo(DatabaseInfo({DatabaseConfiguration::Addresses({sentinelAddress}),
                     DatabaseInfo::Type::SINGLE,
                     boost::none,
                     DatabaseInfo::Discovery::SENTINEL})),
        sentinelMasterName(sentinelMasterName),
        contentsBuilder(contentsBuilder),
        subscribeRetryTimer(*engine),
        subscribeRetryTimerDuration(std::chrono::seconds(1)),
        masterInquiryRetryTimer(*engine),
        masterInquiryRetryTimerDuration(std::chrono::seconds(1))
{
    subscriber = asyncCommandDispatcherCreator(*engine,
                                               databaseInfo,
                                               contentsBuilder,
                                               logger,
                                               true);
    dispatcher = asyncCommandDispatcherCreator(*engine,
                                               databaseInfo,
                                               contentsBuilder,
                                               logger,
                                               false);
}

AsyncSentinelDatabaseDiscovery::~AsyncSentinelDatabaseDiscovery()
{
    if (subscriber)
        subscriber->disableCommandCallbacks();
    if (dispatcher)
        dispatcher->disableCommandCallbacks();
    stateChangedCb = nullptr;
}

void AsyncSentinelDatabaseDiscovery::setStateChangedCb(const StateChangedCb& cb)
{
    stateChangedCb = cb;
    subscriber->registerDisconnectCb([this]()
            {
                subscriber->waitConnectedAsync(std::bind(&AsyncSentinelDatabaseDiscovery::subscribeNotifications, this));
            });
    subscriber->waitConnectedAsync(std::bind(&AsyncSentinelDatabaseDiscovery::subscribeNotifications, this));
}

void AsyncSentinelDatabaseDiscovery::clearStateChangedCb()
{
    stateChangedCb = nullptr;
}

void AsyncSentinelDatabaseDiscovery::subscribeNotifications()
{
    subscriber->dispatchAsync(std::bind(&AsyncSentinelDatabaseDiscovery::subscribeAck,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              "dummyNamespace", // Not meaningful for Sentinel
                              contentsBuilder->build("SUBSCRIBE", "+switch-master"));
}

void AsyncSentinelDatabaseDiscovery::subscribeAck(const std::error_code& error,
                                                  const Reply& reply)
{
    if (!error)
    {
        auto subscribeReply = parseSubscribeReply(reply, *logger);
        if (subscribeReply)
        {
            switch (subscribeReply->type)
            {
                case (SubscribeReply::Type::SUBSCRIBE_REPLY):
                {
                    dispatcher->waitConnectedAsync(std::bind(&AsyncSentinelDatabaseDiscovery::sendMasterInquiry, this));
                    break;
                }
                case (SubscribeReply::Type::NOTIFICATION):
                {
                    auto hostAndPort = parseNotificationMessage(subscribeReply->message, *logger);
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
                        SHAREDDATALAYER_ABORT("Notification message parsing error.");
                    break;
                }
                case (SubscribeReply::Type::UNKNOWN):
                {
                    logger->debug() << "Invalid SUBSCRIBE reply type." << std::endl;
                    SHAREDDATALAYER_ABORT("Invalid SUBSCRIBE command reply type.");
                }
            }
        }
        else
            SHAREDDATALAYER_ABORT("SUBSCRIBE command reply parsing error.");
    }
    else
        subscribeRetryTimer.arm(
                subscribeRetryTimerDuration,
                std::bind(&AsyncSentinelDatabaseDiscovery::subscribeNotifications, this));
}

void AsyncSentinelDatabaseDiscovery::sendMasterInquiry()
{
    dispatcher->dispatchAsync(std::bind(&AsyncSentinelDatabaseDiscovery::masterInquiryAck,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              "dummyNamespace", // Not meaningful for Sentinel
                              contentsBuilder->build("SENTINEL", "get-master-addr-by-name", sentinelMasterName));
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
                                                                          std::shared_ptr<Logger> logger,
                                                                          bool usePermanentCommandCallbacks)
    {
        return AsyncCommandDispatcher::create(engine,
                                              databaseInfo,
                                              contentsBuilder,
                                              usePermanentCommandCallbacks,
                                              logger,
                                              true);
    }

    std::unique_ptr<SubscribeReply> parseSubscribeReply(const Reply& reply, Logger& logger)
    {
        // refer to: https://redis.io/topics/pubsub#format-of-pushed-messages
        auto replyType = reply.getType();
        if (replyType == Reply::Type::ARRAY)
        {
            auto& replyVector(*reply.getArray());
            auto firstElementType = replyVector[0]->getType();
            if (firstElementType == Reply::Type::STRING)
            {
                auto subscribeReply = std::unique_ptr<SubscribeReply>(new SubscribeReply());
                auto kind(replyVector[0]->getString()->str);
                if (kind == "subscribe")
                {
                    subscribeReply->type = SubscribeReply::Type::SUBSCRIBE_REPLY;
                    return subscribeReply;
                }
                else if (kind == "message")
                {
                    subscribeReply->type = SubscribeReply::Type::NOTIFICATION;
                    auto thirdElementType = replyVector[2]->getType();
                    if (thirdElementType == Reply::Type::STRING)
                    {
                        subscribeReply->message = replyVector[2]->getString()->str;
                        return subscribeReply;
                    }
                    else
                        logger.debug() << "Invalid message field type in SUBSCRIBE reply: " << kind << std::endl;
                }
                else
                    logger.debug() << "Invalid kind field in SUBSCRIBE reply: " << kind << std::endl;
            }
            else
                logger.debug() << "Invalid first element type in SUBSCRIBE reply: "
                               << static_cast<int>(firstElementType) << std::endl;
        }
        else
            logger.debug() << "Invalid SUBSCRIBE reply type: "
                           << static_cast<int>(replyType) << std::endl;
        return nullptr;
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

    std::unique_ptr<HostAndPort> parseNotificationMessage(const std::string& message, Logger& logger)
    {
        std::vector<std::string> splittedMessage;
        boost::split(splittedMessage, message, boost::is_any_of(" "));
        if (splittedMessage.size() == 5)
        {
            auto host = splittedMessage[3];
            auto port = splittedMessage[4];
            try
            {
                return std::unique_ptr<HostAndPort>(new HostAndPort(host+":"+port, 0));;
            }
            catch (const std::exception& e)
            {
                logger.debug() << "Invalid host or port in notification message, host: "
                               << host << ", port: " << port
                               << ", exception: " << e.what() << std::endl;
            }
        }
        else
            logger.debug() << "Invalid structure in notification message, size: " << splittedMessage.size() << std::endl;
        return nullptr;
    }
}
