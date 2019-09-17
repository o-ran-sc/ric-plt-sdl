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

#include <gtest/gtest.h>
#include <arpa/inet.h>
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
        std::shared_ptr<StrictMock<AsyncCommandDispatcherMock>> dispatcherMock;
        std::shared_ptr<StrictMock<ContentsBuilderMock>> contentsBuilderMock;
        std::shared_ptr<Logger> logger;
        Contents contents;
        AsyncCommandDispatcher::ConnectAck dispatcherConnectAck;
        AsyncCommandDispatcher::CommandCb savedCommandCb;
        ReplyMock replyMock;
        std::string someHost;
        uint16_t somePort;
        Reply::DataItem hostDataItem;
        Reply::DataItem portDataItem;
        std::shared_ptr<ReplyMock> masterInquiryReplyHost;
        std::shared_ptr<ReplyMock> masterInquiryReplyPort;
        Reply::ReplyVector masterInquiryReply;
        Timer::Duration expectedMasterInquiryRetryTimerDuration;
        Timer::Callback savedConnectionRetryTimerCallback;

        AsyncSentinelDatabaseDiscoveryBaseTest():
            engineMock(std::make_shared<StrictMock<EngineMock>>()),
            dispatcherMock(std::make_shared<StrictMock<AsyncCommandDispatcherMock>>()),
            contentsBuilderMock(std::make_shared<StrictMock<ContentsBuilderMock>>(AsyncStorage::SEPARATOR)),
            logger(createLogger(SDL_LOG_PREFIX)),
            contents({{"aaa","bbb"},{3,3}}),
            someHost("somehost"),
            somePort(1234),
            hostDataItem({someHost,ReplyStringLength(someHost.length())}),
            portDataItem({std::to_string(somePort),ReplyStringLength(std::to_string(somePort).length())}),
            masterInquiryReplyHost(std::make_shared<ReplyMock>()),
            masterInquiryReplyPort(std::make_shared<ReplyMock>()),
            expectedMasterInquiryRetryTimerDuration(std::chrono::seconds(1))
        {
            masterInquiryReply.push_back(masterInquiryReplyHost);
            masterInquiryReply.push_back(masterInquiryReplyPort);
        }

        virtual ~AsyncSentinelDatabaseDiscoveryBaseTest()
        {
        }

        std::shared_ptr<AsyncCommandDispatcher> asyncCommandDispatcherCreator(Engine&,
                                                                              const DatabaseInfo&,
                                                                              std::shared_ptr<ContentsBuilder>)
        {
            // @TODO Add database info checking when configuration support for sentinel is added.
            newDispatcherCreated();
            return dispatcherMock;
        }

        MOCK_METHOD0(newDispatcherCreated, void());

        void expectNewDispatcherCreated()
        {
            EXPECT_CALL(*this, newDispatcherCreated())
                .Times(1);
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
                                 const std::string& string2,
                                 const std::string& string3)
        {
            EXPECT_CALL(*contentsBuilderMock, build(string, string2, string3))
                .Times(1)
                .WillOnce(Return(contents));
        }

        void expectDispatchAsync()
        {
            EXPECT_CALL(*dispatcherMock, dispatchAsync(_, _, contents))
                .Times(1)
                .WillOnce(SaveArg<0>(&savedCommandCb));
        }

        void expectMasterInquiry()
        {
            expectContentsBuild("SENTINEL", "get-master-addr-by-name", "mymaster");
            expectDispatchAsync();
        }

        MOCK_METHOD1(stateChangedCb, void(const DatabaseInfo&));

        void expectStateChangedCb()
        {
            EXPECT_CALL(*this, stateChangedCb(_))
                .Times(1)
                .WillOnce(Invoke([this](const DatabaseInfo& databaseInfo)
                                 {
                                     EXPECT_THAT(DatabaseConfiguration::Addresses({ HostAndPort(someHost, htons(somePort)) }),
                                                 ContainerEq(databaseInfo.hosts));
                                     EXPECT_EQ(DatabaseInfo::Type::SINGLE, databaseInfo.type);
                                     EXPECT_EQ(boost::none, databaseInfo.ns);
                                     EXPECT_EQ(DatabaseInfo::Discovery::SENTINEL, databaseInfo.discovery);
                                 }));
        }

        void expectGetReplyType(ReplyMock& mock, const Reply::Type& type)
        {
            EXPECT_CALL(mock, getType())
                .Times(1)
                .WillOnce(Return(type));
        }

        void expectGetReplyArray_ReturnMasterInquiryReply()
        {
            EXPECT_CALL(replyMock, getArray())
                .Times(1)
                .WillOnce(Return(&masterInquiryReply));
        }

        void expectGetReplyString(ReplyMock& mock, const Reply::DataItem& item)
        {
            EXPECT_CALL(mock, getString())
                .Times(1)
                .WillOnce(Return(&item));
        }

        void expectMasterIquiryReply()
        {
            expectGetReplyType(replyMock, Reply::Type::ARRAY);
            expectGetReplyArray_ReturnMasterInquiryReply();
            expectGetReplyType(*masterInquiryReplyHost, Reply::Type::STRING);
            expectGetReplyString(*masterInquiryReplyHost, hostDataItem);
            expectGetReplyType(*masterInquiryReplyPort, Reply::Type::STRING);
            expectGetReplyString(*masterInquiryReplyPort, portDataItem);
        }

        void expectMasterInquiryRetryTimer()
        {
            EXPECT_CALL(*engineMock, armTimer(_, expectedMasterInquiryRetryTimerDuration, _))
                .Times(1)
                .WillOnce(SaveArg<2>(&savedConnectionRetryTimerCallback));
        }

        void setDefaultResponsesForMasterInquiryReplyParsing()
        {
            ON_CALL(replyMock, getType())
                .WillByDefault(Return(Reply::Type::ARRAY));
            ON_CALL(replyMock, getArray())
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
    };

    class AsyncSentinelDatabaseDiscoveryTest: public AsyncSentinelDatabaseDiscoveryBaseTest
    {
    public:
        AsyncSentinelDatabaseDiscoveryTest()
        {
            expectNewDispatcherCreated();
            asyncSentinelDatabaseDiscovery.reset(
                    new AsyncSentinelDatabaseDiscovery(
                            engineMock,
                            logger,
                            std::bind(&AsyncSentinelDatabaseDiscoveryBaseTest::asyncCommandDispatcherCreator,
                                      this,
                                      std::placeholders::_1,
                                      std::placeholders::_2,
                                      std::placeholders::_3),
                            contentsBuilderMock));
        }
    };

    using AsyncSentinelDatabaseDiscoveryDeathTest = AsyncSentinelDatabaseDiscoveryTest;
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

TEST_F(AsyncSentinelDatabaseDiscoveryTest, RedisMasterIsInquiredFromSentinel)
{
    InSequence dummy;
    expectDispatcherWaitConnectedAsync();
    asyncSentinelDatabaseDiscovery->setStateChangedCb(std::bind(&AsyncSentinelDatabaseDiscoveryTest::stateChangedCb,
            this,
            std::placeholders::_1));
    expectMasterInquiry();
    dispatcherConnectAck();
    expectMasterIquiryReply();
    expectStateChangedCb();
    savedCommandCb(std::error_code(), replyMock);
}

TEST_F(AsyncSentinelDatabaseDiscoveryTest, RedisMasterInquiryErrorTriggersRetry)
{
    InSequence dummy;
    expectDispatcherWaitConnectedAsync();
    asyncSentinelDatabaseDiscovery->setStateChangedCb(std::bind(&AsyncSentinelDatabaseDiscoveryTest::stateChangedCb,
            this,
            std::placeholders::_1));
    expectMasterInquiry();
    dispatcherConnectAck();
    expectMasterInquiryRetryTimer();
    savedCommandCb(getWellKnownErrorCode(), replyMock);
    expectMasterInquiry();
    savedConnectionRetryTimerCallback();
    expectMasterIquiryReply();
    expectStateChangedCb();
    savedCommandCb(std::error_code(), replyMock);
}

TEST_F(AsyncSentinelDatabaseDiscoveryDeathTest, MasterInquiryParsingErrorAborts_InvalidReplyType)
{
    InSequence dummy;
    expectDispatcherWaitConnectedAsync();
    asyncSentinelDatabaseDiscovery->setStateChangedCb(std::bind(&AsyncSentinelDatabaseDiscoveryTest::stateChangedCb,
            this,
            std::placeholders::_1));
    expectMasterInquiry();
    dispatcherConnectAck();
    ON_CALL(replyMock, getType())
        .WillByDefault(Return(Reply::Type::NIL));
    EXPECT_EXIT(savedCommandCb(std::error_code(), replyMock), KilledBySignal(SIGABRT), ".*Master inquiry reply parsing error");
}

TEST_F(AsyncSentinelDatabaseDiscoveryDeathTest, MasterInquiryParsingErrorAborts_InvalidHostElementType)
{
    InSequence dummy;
    expectDispatcherWaitConnectedAsync();
    asyncSentinelDatabaseDiscovery->setStateChangedCb(std::bind(&AsyncSentinelDatabaseDiscoveryTest::stateChangedCb,
            this,
            std::placeholders::_1));
    expectMasterInquiry();
    dispatcherConnectAck();
    setDefaultResponsesForMasterInquiryReplyParsing();
    ON_CALL(*masterInquiryReplyHost, getType())
        .WillByDefault(Return(Reply::Type::NIL));
    EXPECT_EXIT(savedCommandCb(std::error_code(), replyMock), KilledBySignal(SIGABRT), ".*Master inquiry reply parsing error");
}

TEST_F(AsyncSentinelDatabaseDiscoveryDeathTest, MasterInquiryParsingErrorAborts_InvalidPortElementType)
{
    InSequence dummy;
    expectDispatcherWaitConnectedAsync();
    asyncSentinelDatabaseDiscovery->setStateChangedCb(std::bind(&AsyncSentinelDatabaseDiscoveryTest::stateChangedCb,
            this,
            std::placeholders::_1));
    expectMasterInquiry();
    dispatcherConnectAck();
    setDefaultResponsesForMasterInquiryReplyParsing();
    ON_CALL(*masterInquiryReplyPort, getType())
        .WillByDefault(Return(Reply::Type::NIL));
    EXPECT_EXIT(savedCommandCb(std::error_code(), replyMock), KilledBySignal(SIGABRT), ".*Master inquiry reply parsing error");
}

TEST_F(AsyncSentinelDatabaseDiscoveryDeathTest, MasterInquiryParsingErrorAborts_PortCantBeCastedToInt)
{
    InSequence dummy;
    expectDispatcherWaitConnectedAsync();
    asyncSentinelDatabaseDiscovery->setStateChangedCb(std::bind(&AsyncSentinelDatabaseDiscoveryTest::stateChangedCb,
            this,
            std::placeholders::_1));
    expectMasterInquiry();
    dispatcherConnectAck();
    setDefaultResponsesForMasterInquiryReplyParsing();
    std::string invalidPort("invalidPort");
    Reply::DataItem invalidPortDataItem({invalidPort,ReplyStringLength(invalidPort.length())});
    ON_CALL(*masterInquiryReplyPort, getString())
        .WillByDefault(Return(&invalidPortDataItem));
    EXPECT_EXIT(savedCommandCb(std::error_code(), replyMock), KilledBySignal(SIGABRT), ".*Master inquiry reply parsing error");
}

TEST_F(AsyncSentinelDatabaseDiscoveryTest, CallbackIsNotCalledAfterCleared)
{
    InSequence dummy;
    expectDispatcherWaitConnectedAsync();
    asyncSentinelDatabaseDiscovery->setStateChangedCb(std::bind(&AsyncSentinelDatabaseDiscoveryTest::stateChangedCb,
            this,
            std::placeholders::_1));
    expectMasterInquiry();
    dispatcherConnectAck();
    expectMasterInquiryRetryTimer();
    savedCommandCb(getWellKnownErrorCode(), replyMock);
    expectMasterInquiry();
    savedConnectionRetryTimerCallback();
    expectMasterIquiryReply();
    asyncSentinelDatabaseDiscovery->clearStateChangedCb();
    EXPECT_CALL(*this, stateChangedCb(_))
        .Times(0);
    savedCommandCb(std::error_code(), replyMock);
}
