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

#include <gtest/gtest.h>
#include <arpa/inet.h>
#include <string>
#include <sdl/asyncstorage.hpp>
#include "private/createlogger.hpp"
#include "private/hostandport.hpp"
#include "private/timer.hpp"
#include "private/redis/asyncsentineldatabasediscovery.hpp"
#include "private/tst/asynccommanddispatchermock.hpp"
#include "private/tst/contentsbuildermock.hpp"
#include "private/tst/enginemock.hpp"
#include "private/tst/replymock.hpp"
#include "private/tst/wellknownerrorcode.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class AsyncSentinelDatabaseDiscoveryBaseTest: public testing::Test
    {
    public:
        std::unique_ptr<AsyncSentinelDatabaseDiscovery> asyncSentinelDatabaseDiscovery;
        std::shared_ptr<StrictMock<EngineMock>> engineMock;
        std::shared_ptr<StrictMock<AsyncCommandDispatcherMock>> subscriberMock;
        std::shared_ptr<StrictMock<AsyncCommandDispatcherMock>> dispatcherMock;
        std::shared_ptr<StrictMock<ContentsBuilderMock>> contentsBuilderMock;
        std::shared_ptr<Logger> logger;
        Contents contents;
        AsyncCommandDispatcher::ConnectAck subscriberConnectAck;
        AsyncCommandDispatcher::DisconnectCb subscriberDisconnectCb;
        AsyncCommandDispatcher::ConnectAck dispatcherConnectAck;
        AsyncCommandDispatcher::CommandCb savedSubscriberCommandCb;
        AsyncCommandDispatcher::CommandCb savedDispatcherCommandCb;
        ReplyMock masterInquiryReplyMock;
        std::string someHost;
        uint16_t somePort;
        std::string someOtherHost;
        uint16_t someOtherPort;
        Reply::DataItem hostDataItem;
        Reply::DataItem portDataItem;
        std::shared_ptr<ReplyMock> masterInquiryReplyHost;
        std::shared_ptr<ReplyMock> masterInquiryReplyPort;
        Reply::ReplyVector masterInquiryReply;
        Timer::Duration expectedMasterInquiryRetryTimerDuration;
        Timer::Callback savedMasterInquiryRetryTimerCallback;
        // Mocks for SUBSCRIBE command replies are a bit complicated, because reply might have several
        // meanings/structures: https://redis.io/topics/pubsub#format-of-pushed-messages
        ReplyMock subscribeReplyMock;
        std::shared_ptr<ReplyMock> subscribeReplyArrayElement0;
        std::shared_ptr<ReplyMock> subscribeReplyArrayElement1;
        std::shared_ptr<ReplyMock> subscribeReplyArrayElement2;
        Reply::ReplyVector subscribeReplyVector;
        Reply::DataItem subscribeDataItem;
        ReplyMock notificationReplyMock;
        std::shared_ptr<ReplyMock> notificationReplyArrayElement0;
        std::shared_ptr<ReplyMock> notificationReplyArrayElement1;
        std::shared_ptr<ReplyMock> notificationReplyArrayElement2;
        Reply::ReplyVector notificationReplyVector;
        Reply::DataItem notificationDataItem;
        std::string notificationMessage;
        Reply::DataItem notificationMessageDataItem;
        Timer::Duration expectedSubscribeRetryTimerDuration;
        Timer::Callback savedSubscribeRetryTimerCallback;

        AsyncSentinelDatabaseDiscoveryBaseTest():
            engineMock(std::make_shared<StrictMock<EngineMock>>()),
            contentsBuilderMock(std::make_shared<StrictMock<ContentsBuilderMock>>(AsyncStorage::SEPARATOR)),
            logger(createLogger(SDL_LOG_PREFIX)),
            contents({{"aaa","bbb"},{3,3}}),
            someHost("somehost"),
            somePort(1234),
            someOtherHost("someotherhost"),
            someOtherPort(5678),
            hostDataItem({someHost,ReplyStringLength(someHost.length())}),
            portDataItem({std::to_string(somePort),ReplyStringLength(std::to_string(somePort).length())}),
            masterInquiryReplyHost(std::make_shared<ReplyMock>()),
            masterInquiryReplyPort(std::make_shared<ReplyMock>()),
            expectedMasterInquiryRetryTimerDuration(std::chrono::seconds(1)),
            subscribeReplyArrayElement0(std::make_shared<ReplyMock>()),
            subscribeReplyArrayElement1(std::make_shared<ReplyMock>()),
            subscribeReplyArrayElement2(std::make_shared<ReplyMock>()),
            subscribeDataItem({"subscribe",9}),
            notificationReplyArrayElement0(std::make_shared<ReplyMock>()),
            notificationReplyArrayElement1(std::make_shared<ReplyMock>()),
            notificationReplyArrayElement2(std::make_shared<ReplyMock>()),
            notificationDataItem({"message",7}),
            notificationMessage("mymaster " + someHost + " " + std::to_string(somePort) + " " + someOtherHost + " " + std::to_string(someOtherPort)),
            notificationMessageDataItem({notificationMessage, ReplyStringLength(notificationMessage.length())}),
            expectedSubscribeRetryTimerDuration(std::chrono::seconds(1))
        {
            masterInquiryReply.push_back(masterInquiryReplyHost);
            masterInquiryReply.push_back(masterInquiryReplyPort);
            subscribeReplyVector.push_back(subscribeReplyArrayElement0);
            subscribeReplyVector.push_back(subscribeReplyArrayElement1);
            subscribeReplyVector.push_back(subscribeReplyArrayElement2);
            notificationReplyVector.push_back(notificationReplyArrayElement0);
            notificationReplyVector.push_back(notificationReplyArrayElement1);
            notificationReplyVector.push_back(notificationReplyArrayElement2);
        }

        virtual ~AsyncSentinelDatabaseDiscoveryBaseTest()
        {
        }

        std::shared_ptr<AsyncCommandDispatcher> asyncCommandDispatcherCreator()
        {
            // @TODO Add database info checking when configuration support for sentinel is added.
            if (!subscriberMock)
            {
                subscriberMock = std::make_shared<StrictMock<AsyncCommandDispatcherMock>>();
                newDispatcherCreated();
                return subscriberMock;
            }
            if (!dispatcherMock)
            {
                dispatcherMock = std::make_shared<StrictMock<AsyncCommandDispatcherMock>>();
                newDispatcherCreated();
                return dispatcherMock;
            }
            return nullptr;
        }

        MOCK_METHOD0(newDispatcherCreated, void());

        void expectDispatchersCreated()
        {
            EXPECT_CALL(*this, newDispatcherCreated())
                .Times(2);
        }

        void expectSubscriberWaitConnectedAsync()
        {
            EXPECT_CALL(*subscriberMock, waitConnectedAsync(_))
                .Times(1)
                .WillOnce(Invoke([this](const AsyncCommandDispatcher::ConnectAck& connectAck)
                        {
                            subscriberConnectAck = connectAck;
                        }));
        }

        void expectSubscriberRegisterDisconnectCb()
        {
            EXPECT_CALL(*subscriberMock, registerDisconnectCb(_))
                .Times(1)
                .WillOnce(Invoke([this](const AsyncCommandDispatcher::DisconnectCb& disconnectCb)
                        {
                            subscriberDisconnectCb = disconnectCb;
                        }));
        }

        void expectDispatcherWaitConnectedAsync()
        {
            EXPECT_CALL(*dispatcherMock, waitConnectedAsync(_))
                .Times(1)
                .WillOnce(Invoke([this](const AsyncCommandDispatcher::ConnectAck& connectAck)
                        {
                            dispatcherConnectAck = connectAck;
                        }));
        }

        void expectContentsBuild(const std::string& string,
                                 const std::string& string2)
        {
            EXPECT_CALL(*contentsBuilderMock, build(string, string2))
                .Times(1)
                .WillOnce(Return(contents));
        }

        void expectContentsBuild(const std::string& string,
                                 const std::string& string2,
                                 const std::string& string3)
        {
            EXPECT_CALL(*contentsBuilderMock, build(string, string2, string3))
                .Times(1)
                .WillOnce(Return(contents));
        }

        void expectSubscriberDispatchAsync()
        {
            EXPECT_CALL(*subscriberMock, dispatchAsync(_, _, contents))
                .Times(1)
                .WillOnce(SaveArg<0>(&savedSubscriberCommandCb));
        }

        void expectDispatcherDispatchAsync()
        {
            EXPECT_CALL(*dispatcherMock, dispatchAsync(_, _, contents))
                .Times(1)
                .WillOnce(SaveArg<0>(&savedDispatcherCommandCb));
        }

        void expectSubscribeNotifications()
        {
            expectContentsBuild("SUBSCRIBE", "+switch-master");
            expectSubscriberDispatchAsync();
        }

        void expectMasterInquiry()
        {
            expectContentsBuild("SENTINEL", "get-master-addr-by-name", "mymaster");
            expectDispatcherDispatchAsync();
        }

        MOCK_METHOD1(stateChangedCb, void(const DatabaseInfo&));

        void expectStateChangedCb(const std::string& host, uint16_t port)
        {
            EXPECT_CALL(*this, stateChangedCb(_))
                .Times(1)
                .WillOnce(Invoke([this, host, port](const DatabaseInfo& databaseInfo)
                                 {
                                     EXPECT_THAT(DatabaseConfiguration::Addresses({ HostAndPort(host, htons(port)) }),
                                                 ContainerEq(databaseInfo.hosts));
                                     EXPECT_EQ(DatabaseInfo::Type::SINGLE, databaseInfo.type);
                                     EXPECT_EQ(boost::none, databaseInfo.ns);
                                     EXPECT_EQ(DatabaseInfo::Discovery::SENTINEL, databaseInfo.discovery);
                                 }));
        }

        void expectMasterIquiryReply()
        {
            expectGetType(masterInquiryReplyMock, Reply::Type::ARRAY);
            expectGetArray(masterInquiryReplyMock, masterInquiryReply);
            expectGetType(*masterInquiryReplyHost, Reply::Type::STRING);
            expectGetString(*masterInquiryReplyHost, hostDataItem);
            expectGetType(*masterInquiryReplyPort, Reply::Type::STRING);
            expectGetString(*masterInquiryReplyPort, portDataItem);
        }

        void expectMasterInquiryRetryTimer()
        {
            EXPECT_CALL(*engineMock, armTimer(_, expectedMasterInquiryRetryTimerDuration, _))
                .Times(1)
                .WillOnce(SaveArg<2>(&savedMasterInquiryRetryTimerCallback));
        }

        void expectSubscribeRetryTimer()
        {
            EXPECT_CALL(*engineMock, armTimer(_, expectedSubscribeRetryTimerDuration, _))
                .Times(1)
                .WillOnce(SaveArg<2>(&savedSubscribeRetryTimerCallback));
        }

        void setStateChangedCbExpectsBeforeMasterInquiry()
        {
            expectSubscriberRegisterDisconnectCb();
            expectSubscriberWaitConnectedAsync();
            asyncSentinelDatabaseDiscovery->setStateChangedCb(std::bind(&AsyncSentinelDatabaseDiscoveryBaseTest::stateChangedCb,
                    this,
                    std::placeholders::_1));
            expectSubscribeNotifications();
            subscriberConnectAck();
            expectSubscribeReply();
            expectDispatcherWaitConnectedAsync();
            savedSubscriberCommandCb(std::error_code(), subscribeReplyMock);
            expectMasterInquiry();
        }

        void setDefaultResponsesForMasterInquiryReplyParsing()
        {
            ON_CALL(masterInquiryReplyMock, getType())
                .WillByDefault(Return(Reply::Type::ARRAY));
            ON_CALL(masterInquiryReplyMock, getArray())
                .WillByDefault(Return(&masterInquiryReply));
            ON_CALL(*masterInquiryReplyHost, getType())
                .WillByDefault(Return(Reply::Type::STRING));
            ON_CALL(*masterInquiryReplyHost, getString())
                .WillByDefault(Return(&hostDataItem));
            ON_CALL(*masterInquiryReplyPort, getType())
                .WillByDefault(Return(Reply::Type::STRING));
            ON_CALL(*masterInquiryReplyHost, getString())
                .WillByDefault(Return(&portDataItem));
        }

        void expectGetType(ReplyMock& mock, const Reply::Type& type)
        {
            EXPECT_CALL(mock, getType())
                .Times(1)
                .WillOnce(Return(type));
        }

        void expectGetString(ReplyMock& mock, const Reply::DataItem& item)
        {
            EXPECT_CALL(mock, getString())
                .Times(1)
                .WillOnce(Return(&item));
        }

        void expectGetInteger(ReplyMock& mock, int value)
        {
            EXPECT_CALL(mock, getInteger())
                .Times(1)
                .WillOnce(Return(value));
        }

        void expectGetArray(ReplyMock& mock, Reply::ReplyVector& replyVector)
        {
            EXPECT_CALL(mock, getArray())
                .Times(1)
                .WillOnce(Return(&replyVector));
        }

        void expectSubscribeReply()
        {
            expectGetType(subscribeReplyMock, Reply::Type::ARRAY);
            expectGetArray(subscribeReplyMock, subscribeReplyVector);
            expectGetType(*subscribeReplyArrayElement0, Reply::Type::STRING);
            expectGetString(*subscribeReplyArrayElement0, subscribeDataItem);
        }

        void expectNotificationReply()
        {
            expectGetType(notificationReplyMock, Reply::Type::ARRAY);
            expectGetArray(notificationReplyMock, notificationReplyVector);
            expectGetType(*notificationReplyArrayElement0, Reply::Type::STRING);
            expectGetString(*notificationReplyArrayElement0, notificationDataItem);
            expectGetType(*notificationReplyArrayElement2, Reply::Type::STRING);
            expectGetString(*notificationReplyArrayElement2, notificationMessageDataItem);
        }

        void setDefaultResponsesForNotificationReplyParsing()
        {
            ON_CALL(notificationReplyMock, getType())
                .WillByDefault(Return(Reply::Type::ARRAY));
            ON_CALL(notificationReplyMock, getArray())
                .WillByDefault(Return(&notificationReplyVector));
            ON_CALL(*notificationReplyArrayElement0, getType())
                .WillByDefault(Return(Reply::Type::STRING));
            ON_CALL(*notificationReplyArrayElement0, getString())
                .WillByDefault(Return(&notificationDataItem));
            ON_CALL(*notificationReplyArrayElement2, getType())
                .WillByDefault(Return(Reply::Type::STRING));
            ON_CALL(*notificationReplyArrayElement2, getString())
                .WillByDefault(Return(&notificationMessageDataItem));
        }
    };

    class AsyncSentinelDatabaseDiscoveryTest: public AsyncSentinelDatabaseDiscoveryBaseTest
    {
    public:
        AsyncSentinelDatabaseDiscoveryTest()
        {
            expectDispatchersCreated();
            asyncSentinelDatabaseDiscovery.reset(
                    new AsyncSentinelDatabaseDiscovery(
                            engineMock,
                            logger,
                            HostAndPort(someHost, somePort),
                            "mymaster",
                            std::bind(&AsyncSentinelDatabaseDiscoveryBaseTest::asyncCommandDispatcherCreator,
                                      this),
                            contentsBuilderMock));
        }

        ~AsyncSentinelDatabaseDiscoveryTest()
        {
            EXPECT_CALL(*subscriberMock, disableCommandCallbacks())
                .Times(1);
            EXPECT_CALL(*dispatcherMock, disableCommandCallbacks())
                .Times(1);
        }
    };

    class AsyncSentinelDatabaseDiscoveryInListeningModeTest: public AsyncSentinelDatabaseDiscoveryTest
    {
    public:
        AsyncSentinelDatabaseDiscoveryInListeningModeTest()
        {
            InSequence dummy;
            setStateChangedCbExpectsBeforeMasterInquiry();
            dispatcherConnectAck();
            expectMasterIquiryReply();
            expectStateChangedCb(someHost, somePort);
            savedDispatcherCommandCb(std::error_code(), masterInquiryReplyMock);
        }
    };

    using AsyncSentinelDatabaseDiscoveryDeathTest = AsyncSentinelDatabaseDiscoveryTest;

    using AsyncSentinelDatabaseDiscoveryInListeningModeDeathTest = AsyncSentinelDatabaseDiscoveryInListeningModeTest;
}

TEST_F(AsyncSentinelDatabaseDiscoveryBaseTest, IsNotCopyable)
{
    InSequence dummy;
    EXPECT_FALSE(std::is_copy_constructible<AsyncSentinelDatabaseDiscovery>::value);
    EXPECT_FALSE(std::is_copy_assignable<AsyncSentinelDatabaseDiscovery>::value);
}

TEST_F(AsyncSentinelDatabaseDiscoveryBaseTest, ImplementsAsyncDatabaseDiscovery)
{
    InSequence dummy;
    EXPECT_TRUE((std::is_base_of<AsyncDatabaseDiscovery, AsyncSentinelDatabaseDiscovery>::value));
}

TEST_F(AsyncSentinelDatabaseDiscoveryTest, SettingChangedCallbackTriggersSentinelNotificationsSubscriptionAndMasterInquiry)
{
    InSequence dummy;
    setStateChangedCbExpectsBeforeMasterInquiry();
    dispatcherConnectAck();
    expectMasterIquiryReply();
    expectStateChangedCb(someHost, somePort);
    savedDispatcherCommandCb(std::error_code(), masterInquiryReplyMock);
}

TEST_F(AsyncSentinelDatabaseDiscoveryTest, MasterInquiryErrorTriggersRetry)
{
    InSequence dummy;
    setStateChangedCbExpectsBeforeMasterInquiry();
    dispatcherConnectAck();
    expectMasterInquiryRetryTimer();
    savedDispatcherCommandCb(getWellKnownErrorCode(), masterInquiryReplyMock);
    expectMasterInquiry();
    savedMasterInquiryRetryTimerCallback();
    expectMasterIquiryReply();
    expectStateChangedCb(someHost, somePort);
    savedDispatcherCommandCb(std::error_code(), masterInquiryReplyMock);
}

TEST_F(AsyncSentinelDatabaseDiscoveryDeathTest, MasterInquiryParsingErrorAborts_InvalidReplyType)
{
    InSequence dummy;
    setStateChangedCbExpectsBeforeMasterInquiry();
    dispatcherConnectAck();
    ON_CALL(masterInquiryReplyMock, getType())
        .WillByDefault(Return(Reply::Type::NIL));
    EXPECT_EXIT(savedDispatcherCommandCb(std::error_code(), masterInquiryReplyMock), KilledBySignal(SIGABRT), ".*Master inquiry reply parsing error");
}

TEST_F(AsyncSentinelDatabaseDiscoveryDeathTest, MasterInquiryParsingErrorAborts_InvalidHostElementType)
{
    InSequence dummy;
    setStateChangedCbExpectsBeforeMasterInquiry();
    dispatcherConnectAck();
    setDefaultResponsesForMasterInquiryReplyParsing();
    ON_CALL(*masterInquiryReplyHost, getType())
        .WillByDefault(Return(Reply::Type::NIL));
    EXPECT_EXIT(savedDispatcherCommandCb(std::error_code(), masterInquiryReplyMock), KilledBySignal(SIGABRT), ".*Master inquiry reply parsing error");
}

TEST_F(AsyncSentinelDatabaseDiscoveryDeathTest, MasterInquiryParsingErrorAborts_InvalidPortElementType)
{
    InSequence dummy;
    setStateChangedCbExpectsBeforeMasterInquiry();
    dispatcherConnectAck();
    setDefaultResponsesForMasterInquiryReplyParsing();
    ON_CALL(*masterInquiryReplyPort, getType())
        .WillByDefault(Return(Reply::Type::NIL));
    EXPECT_EXIT(savedDispatcherCommandCb(std::error_code(), masterInquiryReplyMock), KilledBySignal(SIGABRT), ".*Master inquiry reply parsing error");
}

TEST_F(AsyncSentinelDatabaseDiscoveryDeathTest, MasterInquiryParsingErrorAborts_PortCantBeCastedToInt)
{
    InSequence dummy;
    setStateChangedCbExpectsBeforeMasterInquiry();
    dispatcherConnectAck();
    setDefaultResponsesForMasterInquiryReplyParsing();
    std::string invalidPort("invalidPort");
    Reply::DataItem invalidPortDataItem({invalidPort,ReplyStringLength(invalidPort.length())});
    ON_CALL(*masterInquiryReplyPort, getString())
        .WillByDefault(Return(&invalidPortDataItem));
    EXPECT_EXIT(savedDispatcherCommandCb(std::error_code(), masterInquiryReplyMock), KilledBySignal(SIGABRT), ".*Master inquiry reply parsing error");
}

TEST_F(AsyncSentinelDatabaseDiscoveryTest, CallbackIsNotCalledAfterCleared)
{
    InSequence dummy;
    setStateChangedCbExpectsBeforeMasterInquiry();
    dispatcherConnectAck();
    expectMasterInquiryRetryTimer();
    savedDispatcherCommandCb(getWellKnownErrorCode(), masterInquiryReplyMock);
    expectMasterInquiry();
    savedMasterInquiryRetryTimerCallback();
    expectMasterIquiryReply();
    asyncSentinelDatabaseDiscovery->clearStateChangedCb();
    EXPECT_CALL(*this, stateChangedCb(_))
        .Times(0);
    savedDispatcherCommandCb(std::error_code(), masterInquiryReplyMock);
}

TEST_F(AsyncSentinelDatabaseDiscoveryTest, ChangeNotificationFromSentinel)
{
    InSequence dummy;
    setStateChangedCbExpectsBeforeMasterInquiry();
    dispatcherConnectAck();
    expectMasterIquiryReply();
    expectStateChangedCb(someHost, somePort);
    savedDispatcherCommandCb(std::error_code(), masterInquiryReplyMock);
    expectNotificationReply();
    expectStateChangedCb(someOtherHost, someOtherPort);
    savedSubscriberCommandCb(std::error_code(), notificationReplyMock);
}

TEST_F(AsyncSentinelDatabaseDiscoveryInListeningModeTest, SubscribeCommandErrorTriggersRetry)
{
    InSequence dummy;
    expectSubscribeRetryTimer();
    savedSubscriberCommandCb(getWellKnownErrorCode(), subscribeReplyMock);
    expectSubscribeNotifications();
    savedSubscribeRetryTimerCallback();
}

TEST_F(AsyncSentinelDatabaseDiscoveryInListeningModeDeathTest, SubscribeReplyParsingErrorAborts_InvalidReplyType)
{
    InSequence dummy;
    ON_CALL(notificationReplyMock, getType())
        .WillByDefault(Return(Reply::Type::NIL));
    EXPECT_EXIT(savedSubscriberCommandCb(std::error_code(), notificationReplyMock), KilledBySignal(SIGABRT), ".*SUBSCRIBE command reply parsing error");
}

TEST_F(AsyncSentinelDatabaseDiscoveryInListeningModeDeathTest, SubscribeReplyParsingErrorAborts_InvalidKindElementType)
{
    InSequence dummy;
    setDefaultResponsesForNotificationReplyParsing();
    ON_CALL(*notificationReplyArrayElement0, getType())
        .WillByDefault(Return(Reply::Type::NIL));
    EXPECT_EXIT(savedSubscriberCommandCb(std::error_code(), notificationReplyMock), KilledBySignal(SIGABRT), ".*SUBSCRIBE command reply parsing error");
}

TEST_F(AsyncSentinelDatabaseDiscoveryInListeningModeDeathTest, SubscribeReplyParsingErrorAborts_InvalidKind)
{
    InSequence dummy;
    setDefaultResponsesForNotificationReplyParsing();
    std::string invalidKind("invalidKind");
    Reply::DataItem invalidKindDataItem({invalidKind,ReplyStringLength(invalidKind.length())});
    ON_CALL(*notificationReplyArrayElement0, getString())
        .WillByDefault(Return(&invalidKindDataItem));
    EXPECT_EXIT(savedSubscriberCommandCb(std::error_code(), notificationReplyMock), KilledBySignal(SIGABRT), ".*SUBSCRIBE command reply parsing error");
}

TEST_F(AsyncSentinelDatabaseDiscoveryInListeningModeDeathTest, SubscribeReplyParsingErrorAborts_InvalidMessageElementType)
{
    InSequence dummy;
    setDefaultResponsesForNotificationReplyParsing();
    ON_CALL(*notificationReplyArrayElement2, getType())
        .WillByDefault(Return(Reply::Type::NIL));
    EXPECT_EXIT(savedSubscriberCommandCb(std::error_code(), notificationReplyMock), KilledBySignal(SIGABRT), ".*SUBSCRIBE command reply parsing error");
}

TEST_F(AsyncSentinelDatabaseDiscoveryInListeningModeDeathTest, SubscribeReplyParsingErrorAborts_InvalidMessageStructure)
{
    InSequence dummy;
    setDefaultResponsesForNotificationReplyParsing();
    std::string invalidMessage("mymaster oldHost 1234 5678");
    auto invalidMessageDataItem(Reply::DataItem({invalidMessage, ReplyStringLength(invalidMessage.length())}));
    ON_CALL(*notificationReplyArrayElement2, getString())
        .WillByDefault(Return(&invalidMessageDataItem));
    EXPECT_EXIT(savedSubscriberCommandCb(std::error_code(), notificationReplyMock), KilledBySignal(SIGABRT), ".*Notification message parsing error");
}

TEST_F(AsyncSentinelDatabaseDiscoveryInListeningModeDeathTest, SubscribeReplyParsingErrorAborts_InvalidPort)
{
    InSequence dummy;
    setDefaultResponsesForNotificationReplyParsing();
    std::string invalidMessage("mymaster oldHost 1234 newHost invalidPort");
    auto invalidMessageDataItem(Reply::DataItem({invalidMessage, ReplyStringLength(invalidMessage.length())}));
    ON_CALL(*notificationReplyArrayElement2, getString())
        .WillByDefault(Return(&invalidMessageDataItem));
    EXPECT_EXIT(savedSubscriberCommandCb(std::error_code(), notificationReplyMock), KilledBySignal(SIGABRT), ".*Notification message parsing error");
}

TEST_F(AsyncSentinelDatabaseDiscoveryInListeningModeTest, SubscriberDisconnectCallbackTriggersSubscriptionRenewal)
{
    InSequence dummy;
    expectSubscriberWaitConnectedAsync();
    subscriberDisconnectCb();
    expectSubscribeNotifications();
    subscriberConnectAck();
    expectSubscribeReply();
    expectDispatcherWaitConnectedAsync();
    savedSubscriberCommandCb(std::error_code(), subscribeReplyMock);
    expectMasterInquiry();
    dispatcherConnectAck();
    expectMasterIquiryReply();
    expectStateChangedCb(someHost, somePort);
    savedDispatcherCommandCb(std::error_code(), masterInquiryReplyMock);
}
