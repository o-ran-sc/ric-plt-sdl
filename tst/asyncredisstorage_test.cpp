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

#include <type_traits>
#include <memory>
#include <cstdlib>
#include <gtest/gtest.h>
#include <arpa/inet.h>
#include <sdl/emptynamespace.hpp>
#include <sdl/invalidnamespace.hpp>
#include <sdl/publisherid.hpp>
#include "private/createlogger.hpp"
#include "private/error.hpp"
#include "private/logger.hpp"
#include "private/redis/asyncredisstorage.hpp"
#include "private/redis/contents.hpp"
#include "private/redis/databaseinfo.hpp"
#include "private/redis/reply.hpp"
#include "private/tst/asynccommanddispatchermock.hpp"
#include "private/tst/asyncdatabasediscoverymock.hpp"
#include "private/tst/contentsbuildermock.hpp"
#include "private/tst/enginemock.hpp"
#include "private/tst/namespaceconfigurationsmock.hpp"
#include "private/tst/replymock.hpp"
#include "private/tst/wellknownerrorcode.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    std::string getErrorCodeMessage(std::error_code ec)
    {
        return ec.message();
    }

    class AsyncRedisStorageErrorCodeTest: public testing::Test
    {
    public:
        AsyncRedisStorageErrorCodeTest()
        {
        }

        virtual ~AsyncRedisStorageErrorCodeTest()
        {
        }
    };

    static const std::string defaultAddress = "address";
    static const uint16_t defaultPort = 3333;
    class AsyncRedisStorageTestBase: public testing::Test
    {
    public:
        std::shared_ptr<StrictMock<EngineMock>> engineMock;
        std::shared_ptr<StrictMock<AsyncDatabaseDiscoveryMock>> discoveryMock;
        std::shared_ptr<StrictMock<AsyncCommandDispatcherMock>> dispatcherMock;
        std::unique_ptr<AsyncRedisStorage> sdlStorage;
        AsyncStorage::Namespace ns;
        std::shared_ptr<StrictMock<ContentsBuilderMock>> contentsBuilderMock;
        std::shared_ptr<StrictMock<NamespaceConfigurationsMock>> namespaceConfigurationsMock;
        Contents contents;
        Contents publishContentsWithoutPubId;
        int fd;
        int discoveryFd;
        int dispatcherFd;
        Engine::EventHandler savedDiscoveryEventHandler;
        Engine::EventHandler savedDispatcherEventHandler;
        AsyncDatabaseDiscovery::StateChangedCb stateChangedCb;
        AsyncCommandDispatcher::ConnectAck dispatcherConnectAck;
        Engine::Callback storedCallback;
        AsyncCommandDispatcher::CommandCb savedCommandCb;
        AsyncCommandDispatcher::CommandCb savedPublishCommandCb;
        AsyncCommandDispatcher::CommandCb savedCommandListQueryCb;
        ReplyMock replyMock;
        Reply::ReplyVector replyVector;
        Reply::ReplyVector commandListReplyVector;
        Reply::ReplyVector commandListReplyElementVector;
        std::string expectedStr1;
        std::string expectedStr2;
        AsyncStorage::Key key1;
        AsyncStorage::Key key2;
        AsyncStorage::Keys keys;
        AsyncStorage::Keys keysWithNonExistKey;
        AsyncStorage::Data data1;
        AsyncStorage::Data data2;
        AsyncStorage::DataMap dataMap;
        std::string keyPrefix;
        std::shared_ptr<Logger> logger;

        AsyncRedisStorageTestBase():
            engineMock(std::make_shared<StrictMock<EngineMock>>()),
            discoveryMock(std::make_shared<StrictMock<AsyncDatabaseDiscoveryMock>>()),
            dispatcherMock(std::make_shared<StrictMock<AsyncCommandDispatcherMock>>()),
            ns("tag1"),
            contentsBuilderMock(std::make_shared<StrictMock<ContentsBuilderMock>>(AsyncStorage::SEPARATOR)),
            namespaceConfigurationsMock(std::make_shared<StrictMock<NamespaceConfigurationsMock>>()),
            contents({{"aaa","bbb"},{3,3}}),
            publishContentsWithoutPubId({ { "PUBLISH", ns, "shareddatalayer::NO_PUBLISHER" }, { 7, 4, 29 } }),
            fd(10),
            discoveryFd(20),
            dispatcherFd(30),
            key1("key1"),
            key2("key2"),
            keys({key1,key2}),
            keysWithNonExistKey({key1,"notfound",key2}),
            data1({1,2,3}),
            data2({4,5,6}),
            dataMap({{key1,data1},{key2,data2}}),
            keyPrefix("{tag1},*"),
            logger(createLogger(SDL_LOG_PREFIX))
        {
        }

        virtual ~AsyncRedisStorageTestBase() = default;

        std::shared_ptr<AsyncCommandDispatcher> asyncCommandDispatcherCreator(Engine&,
                                                                              const DatabaseInfo&,
                                                                              std::shared_ptr<ContentsBuilder>)
        {
            newDispatcherCreated();
            return dispatcherMock;
        }

        MOCK_METHOD0(newDispatcherCreated, void());

        MOCK_METHOD1(readyAck, void(const std::error_code&));

        MOCK_METHOD1(modifyAck, void(const std::error_code&));

        MOCK_METHOD2(modifyIfAck, void(const std::error_code&, bool status));

        MOCK_METHOD2(getAck, void(const std::error_code&, const AsyncStorage::DataMap&));

        MOCK_METHOD2(findKeysAck, void(const std::error_code&, const AsyncStorage::Keys&));

        DatabaseInfo getDatabaseInfo(DatabaseInfo::Type type = DatabaseInfo::Type::SINGLE,
                                     DatabaseInfo::Discovery discovery = DatabaseInfo::Discovery::HIREDIS,
                                     std::string address = defaultAddress,
                                     uint16_t port = defaultPort)
        {
            DatabaseInfo databaseInfo;
            databaseInfo.type = type;
            databaseInfo.discovery = discovery;
            databaseInfo.hosts.push_back({address, htons(port)});
            databaseInfo.ns = ns;
            return databaseInfo;
        }

        void expectDiscoverySetStateChangedCb()
        {
            EXPECT_CALL(*discoveryMock, setStateChangedCb(_))
                .Times(1)
                .WillOnce(Invoke([this](const AsyncDatabaseDiscovery::StateChangedCb& cb)
                        {
                            stateChangedCb = cb;
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

        void expectDispatcherCreation()
        {
            expectDispatcherWaitConnectedAsync();
        }

        void expectNewDispatcherCreated()
        {
            EXPECT_CALL(*this, newDispatcherCreated)
                .Times(1);
        }

        void expectNewDispatcherNotCreated()
        {
            EXPECT_CALL(*this, newDispatcherCreated)
                .Times(0);
        }

        void expectReadyAck(const std::error_code& error)
        {
            EXPECT_CALL(*this, readyAck(error))
                .Times(1);
        }

        void expectModifyAck(const std::error_code& error)
        {
            EXPECT_CALL(*this, modifyAck(error))
                .Times(1);
        }

        void expectModifyIfAck(const std::error_code& error, bool status)
        {
            EXPECT_CALL(*this, modifyIfAck(error, status))
                .Times(1);
        }

        void expectGetAck(const std::error_code& error, const AsyncStorage::DataMap& dataMap)
        {
            EXPECT_CALL(*this, getAck(error, dataMap))
                .Times(1);
        }

        void expectFindKeysAck(const std::error_code& error, const AsyncStorage::Keys& keys)
        {
            EXPECT_CALL(*this, findKeysAck(error, keys))
                .Times(1);
        }

        void expectPostCallback()
        {
            EXPECT_CALL(*engineMock, postCallback(_))
                .Times(1)
                .WillOnce(SaveArg<0>(&storedCallback));
        }

        void createAsyncStorageInstance(const boost::optional<PublisherId>& pId)
        {
            expectDiscoverySetStateChangedCb();
            if (sdlStorage)
                expectClearStateChangedCb();
            sdlStorage.reset(new AsyncRedisStorage(engineMock,
                                                   discoveryMock,
                                                   pId,
                                                   namespaceConfigurationsMock,
                                                   std::bind(&AsyncRedisStorageTestBase::asyncCommandDispatcherCreator,
                                                             this,
                                                             std::placeholders::_1,
                                                             std::placeholders::_2,
                                                             std::placeholders::_3),
                                                   contentsBuilderMock,
                                                   logger));
        }

        void createAndConnectAsyncStorageInstance(const boost::optional<PublisherId>& pId)
        {
            InSequence dummy;
            createAsyncStorageInstance(pId);
            sdlStorage->waitReadyAsync(ns, std::bind(&AsyncRedisStorageTestBase::readyAck, this, std::placeholders::_1));
            expectNewDispatcherCreated();
            expectDispatcherCreation();
            stateChangedCb(getDatabaseInfo());
            EXPECT_CALL(*this, readyAck(std::error_code()))
                .Times(1);
            dispatcherConnectAck();
        }

        void expectClearStateChangedCb()
        {
            EXPECT_CALL(*discoveryMock, clearStateChangedCb())
                .Times(1);
        }

        void expectNoDispatchAsync()
        {
            EXPECT_CALL(*dispatcherMock, dispatchAsync(_, ns, _))
                .Times(0);
        }

        void expectDispatchAsync()
        {
            EXPECT_CALL(*dispatcherMock, dispatchAsync(_, ns, contents))
                .Times(1)
                .WillOnce(SaveArg<0>(&savedCommandCb));
        }

        void expectPublishDispatch()
        {
            EXPECT_CALL(*dispatcherMock, dispatchAsync(_, ns, contents))
                .Times(1)
                .WillOnce(SaveArg<0>(&savedPublishCommandCb));
        }

        std::shared_ptr<Reply> getMockPtr()
        {
            return std::shared_ptr<Reply>(&replyMock, [](Reply*){ });
        }

        void expectGetType(const Reply::Type& type)
        {
            EXPECT_CALL(replyMock, getType())
                .Times(1)
                .WillOnce(Return(type));
        }

        void expectGetDataString(const Reply::DataItem& item)
        {
            expectGetType(Reply::Type::STRING);
            EXPECT_CALL(replyMock, getString())
                .Times(1)
                .WillOnce(Return(&item));
        }

        void expectGetArray()
        {
            EXPECT_CALL(replyMock, getArray())
                .Times(1)
                .WillOnce(Return(&replyVector));
        }

        void expectGetInteger(int value)
        {
            EXPECT_CALL(replyMock, getInteger())
                .Times(1)
                .WillOnce(Return(value));
        }

        void expectContentsBuild(const std::string& string,
                                 const AsyncStorage::Key& key,
                                 const AsyncStorage::Data& data)
        {
            EXPECT_CALL(*contentsBuilderMock, build(string, ns, key, data))
                .Times(1)
                .WillOnce(Return(contents));
        }


        void expectContentsBuild(const std::string& string,
                                 const AsyncStorage::Key& key,
                                 const AsyncStorage::Data& data,
                                 const std::string& string2,
                                 const std::string& string3)
        {
            EXPECT_CALL(*contentsBuilderMock, build(string, ns, key, data, string2, string3))
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

        void expectContentsBuild(const std::string& string,
                                 const AsyncStorage::DataMap& dataMap)
        {
            EXPECT_CALL(*contentsBuilderMock, build(string, ns, dataMap))
                .Times(1)
                .WillOnce(Return(contents));
        }

        void expectContentsBuild(const std::string& string,
                                 const AsyncStorage::DataMap& dataMap,
                                 const std::string& string2,
                                 const std::string& string3)
        {
            EXPECT_CALL(*contentsBuilderMock, build(string, ns, dataMap, string2, string3))
                .Times(1)
                .WillOnce(Return(contents));
        }

        void expectContentsBuild(const std::string& string,
                                 const AsyncStorage::Keys& keys)
        {
            EXPECT_CALL(*contentsBuilderMock, build(string, ns, keys))
                .Times(1)
                .WillOnce(Return(contents));
        }

        void expectContentsBuild(const std::string& string,
                                 const AsyncStorage::Keys& keys,
                                 const std::string& string2,
                                 const std::string& string3)
        {
            EXPECT_CALL(*contentsBuilderMock, build(string, ns, keys, string2, string3))
                .Times(1)
                .WillOnce(Return(contents));
        }

        void expectContentsBuild(const std::string& string,
                                 const std::string& string2)
        {
            EXPECT_CALL(*contentsBuilderMock, build(string, string2))
                .Times(1)
                .WillOnce(Return(contents));
        }
    };

    class AsyncRedisStorageTest: public AsyncRedisStorageTestBase
    {
    public:
        AsyncRedisStorageTest()
        {
            InSequence dummy;
            for (auto i(0U); i < 3; ++i)
                replyVector.push_back(getMockPtr());
            createAndConnectAsyncStorageInstance(boost::none);
            EXPECT_CALL(*namespaceConfigurationsMock, areNotificationsEnabled(_)).WillRepeatedly(Return(true));
        }

        ~AsyncRedisStorageTest()
        {
            expectClearStateChangedCb();
            EXPECT_CALL(*dispatcherMock, disableCommandCallbacks())
                .Times(1);
        }
    };


    class AsyncRedisStorageTestNotificationsDisabled: public AsyncRedisStorageTestBase
    {
    public:
        AsyncRedisStorageTestNotificationsDisabled()
        {
            InSequence dummy;
            for (auto i(0U); i < 3; ++i)
                replyVector.push_back(getMockPtr());
            createAndConnectAsyncStorageInstance(boost::none);
            EXPECT_CALL(*namespaceConfigurationsMock, areNotificationsEnabled(_)).WillRepeatedly(Return(false));
        }

        ~AsyncRedisStorageTestNotificationsDisabled()
        {
            expectClearStateChangedCb();
            EXPECT_CALL(*dispatcherMock, disableCommandCallbacks())
                .Times(1);
        }
    };


    class AsyncRedisStorageTestDispatcherNotCreated: public AsyncRedisStorageTestBase
    {
    public:
        AsyncRedisStorageTestDispatcherNotCreated()
        {
            InSequence dummy;
            for (auto i(0U); i < 3; ++i)
                replyVector.push_back(getMockPtr());
            createAsyncStorageInstance(boost::none);
        }

        ~AsyncRedisStorageTestDispatcherNotCreated()
        {
            expectClearStateChangedCb();
        }
    };

    class AsyncRedisStorageDeathTest: public AsyncRedisStorageTestBase
    {
    public:
        AsyncRedisStorageDeathTest()
        {
        }
    };
}

TEST_F(AsyncRedisStorageErrorCodeTest, AllErrorCodeEnumsHaveCorrectDescriptionMessage)
{
    std::error_code ec;

    for (AsyncRedisStorage::ErrorCode aec = AsyncRedisStorage::ErrorCode::SUCCESS; aec != AsyncRedisStorage::ErrorCode::END_MARKER; ++aec)
    {
        switch (aec)
        {
            case AsyncRedisStorage::ErrorCode::SUCCESS:
                ec = aec;
                EXPECT_EQ(std::error_code().message(), getErrorCodeMessage(ec));
                break;
            case AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED:
                ec = aec;
                EXPECT_EQ("connection to the underlying data storage not yet available", getErrorCodeMessage(ec));
                break;
            case AsyncRedisStorage::ErrorCode::INVALID_NAMESPACE:
                ec = aec;
                EXPECT_EQ("invalid namespace identifier passed to SDL API", getErrorCodeMessage(ec));
                break;
            case AsyncRedisStorage::ErrorCode::END_MARKER:
                ec = aec;
                EXPECT_EQ("unsupported error code for message()", getErrorCodeMessage(ec));
                break;
            default:
                FAIL() << "No mapping for AsyncRedisStorage value: " << aec;
                break;
        }
    }
}

TEST_F(AsyncRedisStorageErrorCodeTest, AllErrorCodeEnumsAreMappedToCorrectSDLInternalError)
{
    /* If this test case detects missing error code, remember to add new error code also to AllErrorCodeEnumsAreMappedToCorrectClientErrorCode
     * test case (and add also mapping implementation from InternalError to Error if needed).
     */
    std::error_code ec;

    for (AsyncRedisStorage::ErrorCode aec = AsyncRedisStorage::ErrorCode::SUCCESS; aec != AsyncRedisStorage::ErrorCode::END_MARKER; ++aec)
    {
        switch (aec)
        {
            case AsyncRedisStorage::ErrorCode::SUCCESS:
                ec = aec;
                EXPECT_TRUE(ec == InternalError::SUCCESS);
                break;
            case AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED:
                ec = aec;
                EXPECT_TRUE(ec == InternalError::SDL_NOT_READY);
                break;
            case AsyncRedisStorage::ErrorCode::INVALID_NAMESPACE:
                ec = aec;
                EXPECT_TRUE(ec == InternalError::SDL_RECEIVED_INVALID_PARAMETER);
                break;
            case AsyncRedisStorage::ErrorCode::END_MARKER:
                ec = aec;
                EXPECT_TRUE(ec == InternalError::SDL_ERROR_CODE_LOGIC_ERROR);
                break;
            default:
                FAIL() << "No mapping for AsyncRedisStorage value: " << aec;
                break;
        }
    }
}

TEST_F(AsyncRedisStorageErrorCodeTest, AllErrorCodeEnumsAreMappedToCorrectClientErrorCode)
{
    std::error_code ec;

    ec = AsyncRedisStorage::ErrorCode::SUCCESS;
    EXPECT_TRUE(ec == shareddatalayer::Error::SUCCESS);
    ec = AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED;
    EXPECT_TRUE(ec == shareddatalayer::Error::NOT_CONNECTED);
    ec = AsyncRedisStorage::ErrorCode::END_MARKER;
    EXPECT_TRUE(ec == shareddatalayer::Error::BACKEND_FAILURE);
}

TEST_F(AsyncRedisStorageTest, IsNotCopyable)
{
    EXPECT_FALSE(std::is_copy_constructible<AsyncRedisStorage>::value);
    EXPECT_FALSE(std::is_copy_assignable<AsyncRedisStorage>::value);
}
TEST_F(AsyncRedisStorageTest, ImplementsAsyncStorage)
{
    EXPECT_TRUE((std::is_base_of<AsyncStorage, AsyncRedisStorage>::value));
}

TEST_F(AsyncRedisStorageTest, CanGetFd)
{
    EXPECT_CALL(*engineMock, fd())
        .Times(1)
        .WillOnce(Return(fd));
    EXPECT_EQ(fd, sdlStorage->fd());
}

TEST_F(AsyncRedisStorageTest, CanHandleEvents)
{
    EXPECT_CALL(*engineMock, handleEvents())
        .Times(1);
    sdlStorage->handleEvents();
}

TEST_F(AsyncRedisStorageTest, ReadyAckIsPassedToAsyncRedisCommandDispatcher)
{
    InSequence dummy;
    EXPECT_CALL(*dispatcherMock, waitConnectedAsync(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&dispatcherConnectAck));
    sdlStorage->waitReadyAsync(ns, std::bind(&AsyncRedisStorageTest::readyAck, this, std::placeholders::_1));
    expectReadyAck(std::error_code());
    dispatcherConnectAck();
}

TEST_F(AsyncRedisStorageTest, PassingEmptyPublisherIdThrows)
{
    EXPECT_THROW(sdlStorage.reset(new AsyncRedisStorage(
                     engineMock,
                     discoveryMock,
                     std::string(""),
                     namespaceConfigurationsMock,
                     std::bind(&AsyncRedisStorageTest::asyncCommandDispatcherCreator,
                         this,
                         std::placeholders::_1,
                         std::placeholders::_2,
                         std::placeholders::_3),
                     contentsBuilderMock,
                     logger)),
                 std::invalid_argument);
}

TEST_F(AsyncRedisStorageTest, PassingInvalidNamespaceToSetAsyncNacks)
{
    InSequence dummy;
    expectPostCallback();
    sdlStorage->setAsync("ns1,2",
                         { { key1, { } } },
                         std::bind(&AsyncRedisStorageTestDispatcherNotCreated::modifyAck,
                                   this,
                                   std::placeholders::_1));
    expectModifyAck(std::error_code(AsyncRedisStorage::ErrorCode::INVALID_NAMESPACE));
    storedCallback();
}

TEST_F(AsyncRedisStorageTest, PassingEmptyNamespaceToSetAsyncNacks)
{
    InSequence dummy;
    expectPostCallback();
    sdlStorage->setAsync("",
                         { { key1, { } } },
                         std::bind(&AsyncRedisStorageTestDispatcherNotCreated::modifyAck,
                                   this,
                                   std::placeholders::_1));
    expectModifyAck(std::error_code(AsyncRedisStorage::ErrorCode::INVALID_NAMESPACE));
    storedCallback();
}

TEST_F(AsyncRedisStorageTest, SetAsyncSuccessfullyAndErrorIsForwarded)
{
    InSequence dummy;
    expectContentsBuild("MSETPUB", dataMap, ns, shareddatalayer::NO_PUBLISHER);
    expectDispatchAsync();
    sdlStorage->setAsync(ns,
                         dataMap,
                         std::bind(&AsyncRedisStorageTest::modifyAck, this, std::placeholders::_1));
    expectModifyAck(std::error_code());
    savedCommandCb(std::error_code(), replyMock);
    expectModifyAck(getWellKnownErrorCode());
    savedCommandCb(getWellKnownErrorCode(), replyMock);
}

TEST_F(AsyncRedisStorageTestNotificationsDisabled, SetAsyncSuccessfullyAndErrorIsForwardedNoPublish)
{
    InSequence dummy;
    expectContentsBuild("MSET", dataMap);
    expectDispatchAsync();
    sdlStorage->setAsync(ns,
                         dataMap,
                         std::bind(&AsyncRedisStorageTest::modifyAck, this, std::placeholders::_1));
    expectModifyAck(std::error_code());
    savedCommandCb(std::error_code(), replyMock);
    expectModifyAck(getWellKnownErrorCode());
    savedCommandCb(getWellKnownErrorCode(), replyMock);
}

TEST_F(AsyncRedisStorageTest, EmptyMapIsCheckedInSetAsyncAndAckIsScheduled)
{
    InSequence dummy;
    expectNoDispatchAsync();
    expectPostCallback();
    sdlStorage->setAsync(ns,
                         { },
                         std::bind(&AsyncRedisStorageTest::modifyAck, this, std::placeholders::_1));
    expectModifyAck(std::error_code());
    storedCallback();
}

TEST_F(AsyncRedisStorageTest, GetAsyncSuccessfullyAndErrorIsForwarded)
{
    InSequence dummy;
    expectContentsBuild("MGET", keysWithNonExistKey);
    expectDispatchAsync();
    sdlStorage->getAsync(ns,
                         keysWithNonExistKey,
                         std::bind(&AsyncRedisStorageTest::getAck,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2));
    expectGetArray();
    auto expectedDataItem1(Reply::DataItem { std::string(data1.begin(),data1.end()), ReplyStringLength(data1.size()) });
    auto expectedDataItem2(Reply::DataItem { std::string(data2.begin(),data2.end()), ReplyStringLength(data2.size()) });
    expectGetDataString(expectedDataItem1);
    expectGetDataString(expectedDataItem2);
    expectGetType(Reply::Type::NIL);
    expectGetAck(std::error_code(), { { key1, data1 }, { key2, data2 } });
    savedCommandCb(std::error_code(), replyMock);
    expectGetAck(getWellKnownErrorCode(), { });
    savedCommandCb(getWellKnownErrorCode(), replyMock);
}

TEST_F(AsyncRedisStorageTest, EmptyEntriesIsCheckedInGetAsyncAndAckIsScheduled)
{
    InSequence dummy;
    expectNoDispatchAsync();
    expectPostCallback();
    sdlStorage->getAsync(ns,
                         { },
                         std::bind(&AsyncRedisStorageTest::getAck,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2));
    expectGetAck(std::error_code(), { });
    storedCallback();
}

TEST_F(AsyncRedisStorageTest, RemoveAsyncSuccessfullyAndErrorIsForwarded)
{
    InSequence dummy;
    expectContentsBuild("DELPUB", keys, ns, shareddatalayer::NO_PUBLISHER);
    expectDispatchAsync();
    sdlStorage->removeAsync(ns,
                            keys,
                            std::bind(&AsyncRedisStorageTest::modifyAck,
                                      this,
                                      std::placeholders::_1));
    expectModifyAck(std::error_code());
    savedCommandCb(std::error_code(), replyMock);
    expectModifyAck(getWellKnownErrorCode());
    savedCommandCb(getWellKnownErrorCode(), replyMock);
}

TEST_F(AsyncRedisStorageTestNotificationsDisabled, RemoveAsyncSuccessfullyAndErrorIsForwardedNoPublish)
{
    InSequence dummy;
    expectContentsBuild("DEL", keys);
    expectDispatchAsync();
    sdlStorage->removeAsync(ns,
                            keys,
                            std::bind(&AsyncRedisStorageTest::modifyAck,
                                      this,
                                      std::placeholders::_1));
    expectModifyAck(std::error_code());
    savedCommandCb(std::error_code(), replyMock);
    expectModifyAck(getWellKnownErrorCode());
    savedCommandCb(getWellKnownErrorCode(), replyMock);
}

TEST_F(AsyncRedisStorageTest, EmptyEntriesIsCheckedInRemoveAsyncAndAckIsScheduled)
{
    InSequence dummy;
    expectNoDispatchAsync();
    expectPostCallback();
    sdlStorage->removeAsync(ns,
                            { },
                            std::bind(&AsyncRedisStorageTest::modifyAck, this, std::placeholders::_1));
    expectModifyAck(std::error_code());
    storedCallback();
}

TEST_F(AsyncRedisStorageTest, FindKeysAsyncSuccessfullyAndErrorIsTranslated)
{
    InSequence dummy;
    expectContentsBuild("KEYS", keyPrefix);
    expectDispatchAsync();
    sdlStorage->findKeysAsync(ns,
                              "",
                              std::bind(&AsyncRedisStorageTest::findKeysAck,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2));
    expectGetArray();
    auto expectedDataItem1(Reply::DataItem { key1, ReplyStringLength(key1.size()) });
    auto expectedDataItem2(Reply::DataItem { key2, ReplyStringLength(key2.size()) });
    expectGetDataString(expectedDataItem1);
    expectGetType(Reply::Type::NIL);
    expectGetDataString(expectedDataItem2);
    expectFindKeysAck(std::error_code(), { key1, key2 });
    savedCommandCb(std::error_code(), replyMock);
    expectFindKeysAck(getWellKnownErrorCode(), { });
    savedCommandCb(getWellKnownErrorCode(), replyMock);
}

TEST_F(AsyncRedisStorageTest, ListKeysPatternSuccessfullyAndErrorIsTranslated)
{
    InSequence dummy;
    expectContentsBuild("KEYS", "{tag1},key[12]");
    expectDispatchAsync();
    sdlStorage->listKeys(ns,
                         "key[12]",
                         std::bind(&AsyncRedisStorageTest::findKeysAck,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2));
    expectGetArray();
    auto expectedDataItem1(Reply::DataItem { key1, ReplyStringLength(key1.size()) });
    auto expectedDataItem2(Reply::DataItem { key2, ReplyStringLength(key2.size()) });
    expectGetDataString(expectedDataItem1);
    expectGetType(Reply::Type::NIL);
    expectGetDataString(expectedDataItem2);
    expectFindKeysAck(std::error_code(), { key1, key2 });
    savedCommandCb(std::error_code(), replyMock);
    expectFindKeysAck(getWellKnownErrorCode(), { });
    savedCommandCb(getWellKnownErrorCode(), replyMock);
}

TEST_F(AsyncRedisStorageTest, RemoveAllAsyncSuccessfully)
{
    InSequence dummy;
    expectContentsBuild("KEYS", keyPrefix);
    expectDispatchAsync();
    sdlStorage->removeAllAsync(ns,
                               std::bind(&AsyncRedisStorageTest::modifyAck, this, std::placeholders::_1));
    expectGetArray();
    auto expectedDataItem1(Reply::DataItem { key1, ReplyStringLength(key1.size()) });
    auto expectedDataItem2(Reply::DataItem { key2, ReplyStringLength(key2.size()) });
    expectGetDataString(expectedDataItem1);
    expectGetType(Reply::Type::NIL);
    expectGetDataString(expectedDataItem2);
    expectContentsBuild("DELPUB", keys, ns, shareddatalayer::NO_PUBLISHER);
    expectDispatchAsync();
    savedCommandCb(std::error_code(), replyMock);
    expectModifyAck(std::error_code());
    savedCommandCb(std::error_code(), replyMock);
}

TEST_F(AsyncRedisStorageTestNotificationsDisabled, RemoveAllAsyncSuccessfullyNoPublish)
{
    InSequence dummy;
    expectContentsBuild("KEYS", keyPrefix);
    expectDispatchAsync();
    sdlStorage->removeAllAsync(ns,
                               std::bind(&AsyncRedisStorageTest::modifyAck, this, std::placeholders::_1));
    expectGetArray();
    auto expectedDataItem1(Reply::DataItem { key1, ReplyStringLength(key1.size()) });
    auto expectedDataItem2(Reply::DataItem { key2, ReplyStringLength(key2.size()) });
    expectGetDataString(expectedDataItem1);
    expectGetType(Reply::Type::NIL);
    expectGetDataString(expectedDataItem2);
    expectContentsBuild("DEL", keys);
    expectDispatchAsync();
    savedCommandCb(std::error_code(), replyMock);
}

TEST_F(AsyncRedisStorageTest, NothingIsIssuedToBeRemovedIfNoKeysAreFoundUnderNamespace)
{
    InSequence dummy;
    Reply::ReplyVector empty;
    expectContentsBuild("KEYS", keyPrefix);
    expectDispatchAsync();
    sdlStorage->removeAllAsync(ns,
                               std::bind(&AsyncRedisStorageTest::modifyAck, this, std::placeholders::_1));
    EXPECT_CALL(replyMock, getArray())
        .Times(1)
        .WillOnce(Return(&empty));
    EXPECT_CALL(*dispatcherMock, dispatchAsync(_, ns, _))
        .Times(0);
    expectModifyAck(std::error_code());
    savedCommandCb(std::error_code(), replyMock);
}

TEST_F(AsyncRedisStorageTest, RemoveAllAsyncErrorIsForwarded)
{
    InSequence dummy;
    expectContentsBuild("KEYS", keyPrefix);
    expectDispatchAsync();
    sdlStorage->removeAllAsync(ns,
                               std::bind(&AsyncRedisStorageTest::modifyAck,
                                         this,
                                         std::placeholders::_1));
    expectModifyAck(getWellKnownErrorCode());
    savedCommandCb(getWellKnownErrorCode(), replyMock);
}

TEST_F(AsyncRedisStorageTest, SetIfNotExistsAsyncSuccess)
{
    InSequence dummy;
    expectContentsBuild("SETNXPUB", key1, data1, ns, shareddatalayer::NO_PUBLISHER);
    expectDispatchAsync();
    sdlStorage->setIfNotExistsAsync(ns,
                                    key1,
                                    data1,
                                    std::bind(&AsyncRedisStorageTest::modifyIfAck,
                                              this, std::placeholders::_1,  std::placeholders::_2));
    expectGetType(Reply::Type::INTEGER);
    expectGetInteger(1);
    expectModifyIfAck(std::error_code(), true);
    savedCommandCb(std::error_code(), replyMock);
}

TEST_F(AsyncRedisStorageTestNotificationsDisabled, SetIfNotExistsAsyncSuccessNoPublish)
{
    InSequence dummy;
    expectContentsBuild("SETNX", key1, data1);
    expectDispatchAsync();
    sdlStorage->setIfNotExistsAsync(ns,
                                    key1,
                                    data1,
                                    std::bind(&AsyncRedisStorageTest::modifyIfAck,
                                              this, std::placeholders::_1,  std::placeholders::_2));
    expectGetType(Reply::Type::INTEGER);
    expectGetInteger(1);
    expectModifyIfAck(std::error_code(), true);
    savedCommandCb(std::error_code(), replyMock);
}

TEST_F(AsyncRedisStorageTest, SetIfNotExistsAsyncKeyAlreadyExists)
{
    InSequence dummy;
    expectContentsBuild("SETNXPUB", key1, data1, ns, shareddatalayer::NO_PUBLISHER);
    expectDispatchAsync();
    sdlStorage->setIfNotExistsAsync(ns,
                                    key1,
                                    data1,
                                    std::bind(&AsyncRedisStorageTest::modifyIfAck,
                                              this, std::placeholders::_1,  std::placeholders::_2));
    expectGetType(Reply::Type::INTEGER);
    expectGetInteger(0);
    expectModifyIfAck(std::error_code(), false);
    savedCommandCb(std::error_code(), replyMock);
}

TEST_F(AsyncRedisStorageTest, SetIfNotExistsAsyncErrorResponse)
{
    InSequence dummy;
    expectContentsBuild("SETNXPUB", key1, data1, ns, shareddatalayer::NO_PUBLISHER);
    expectDispatchAsync();
    sdlStorage->setIfNotExistsAsync(ns,
                                    key1,
                                    data1,
                                    std::bind(&AsyncRedisStorageTest::modifyIfAck,
                                              this, std::placeholders::_1,  std::placeholders::_2));
    expectGetType(Reply::Type::INTEGER);
    expectModifyIfAck(getWellKnownErrorCode(), false);
    savedCommandCb(getWellKnownErrorCode(), replyMock);
}

TEST_F(AsyncRedisStorageTest, RedisSearchPatternCharactersAreCorrectlyEscapedInKeyPrefixSearch)
{
    InSequence dummy;
    const std::string keyPrefixSearchPatternPrefix = '{' + ns + '}' + AsyncStorage::SEPARATOR;
    std::string builtKeyPrefixSearchPattern;
    std::string expectedKeyPrefixSearchPattern;

    // empty prefix will not be escaped and it will search all keys
    expectedKeyPrefixSearchPattern = keyPrefixSearchPatternPrefix + '*';
    builtKeyPrefixSearchPattern = sdlStorage->buildKeyPrefixSearchPattern(ns, "");
    ASSERT_STREQ(expectedKeyPrefixSearchPattern.c_str(), builtKeyPrefixSearchPattern.c_str());

    // prefix without search characters is not escaped
    expectedKeyPrefixSearchPattern = keyPrefixSearchPatternPrefix + "someKnownKeyPrefix" + '*';
    builtKeyPrefixSearchPattern = sdlStorage->buildKeyPrefixSearchPattern(ns, "someKnownKeyPrefix");
    ASSERT_STREQ(expectedKeyPrefixSearchPattern.c_str(), builtKeyPrefixSearchPattern.c_str());

    expectedKeyPrefixSearchPattern = keyPrefixSearchPatternPrefix + R"("someKnownKeyPrefix")" + '*';
    builtKeyPrefixSearchPattern = sdlStorage->buildKeyPrefixSearchPattern(ns, R"("someKnownKeyPrefix")");
    ASSERT_STREQ(expectedKeyPrefixSearchPattern.c_str(), builtKeyPrefixSearchPattern.c_str());

    // all search characters are correctly escaped with backslash
    expectedKeyPrefixSearchPattern = keyPrefixSearchPatternPrefix + R"(someKnownKeyPrefix\*)" + '*';
    builtKeyPrefixSearchPattern = sdlStorage->buildKeyPrefixSearchPattern(ns, "someKnownKeyPrefix*");
    ASSERT_STREQ(expectedKeyPrefixSearchPattern.c_str(), builtKeyPrefixSearchPattern.c_str());

    expectedKeyPrefixSearchPattern = keyPrefixSearchPatternPrefix + R"(\?some\]Known\[Key\\Prefix\*)" + '*';
    builtKeyPrefixSearchPattern = sdlStorage->buildKeyPrefixSearchPattern(ns, "?some]Known[Key\\Prefix*");
    ASSERT_STREQ(expectedKeyPrefixSearchPattern.c_str(), builtKeyPrefixSearchPattern.c_str());

    expectedKeyPrefixSearchPattern = keyPrefixSearchPatternPrefix + R"(\?\*some\[\]Known\[\*\?\\\]Key\\\*Prefix\*\*\*\*)" + '*';
    builtKeyPrefixSearchPattern = sdlStorage->buildKeyPrefixSearchPattern(ns, "?*some[]Known[*?\\]Key\\*Prefix****");
    ASSERT_STREQ(expectedKeyPrefixSearchPattern.c_str(), builtKeyPrefixSearchPattern.c_str());
}

TEST_F(AsyncRedisStorageTest, BuildNamespaceKeySearchPatternIsCorrect)
{
    InSequence dummy;
    const std::string nsPrefix = '{' + ns + '}' + AsyncStorage::SEPARATOR;
    std::string buildPattern;
    std::string expectedPattern;

    expectedPattern = nsPrefix;
    buildPattern = sdlStorage->buildNamespaceKeySearchPattern(ns, "");
    ASSERT_STREQ(expectedPattern.c_str(), buildPattern.c_str());

    expectedPattern = nsPrefix + '*';
    buildPattern = sdlStorage->buildNamespaceKeySearchPattern(ns, "*");
    ASSERT_STREQ(expectedPattern.c_str(), buildPattern.c_str());

    expectedPattern = nsPrefix + "h?llo";
    buildPattern = sdlStorage->buildNamespaceKeySearchPattern(ns, "h?llo");
    ASSERT_STREQ(expectedPattern.c_str(), buildPattern.c_str());
}

TEST_F(AsyncRedisStorageTestDispatcherNotCreated, ReadyAckNotForwardedIfDispatcherNotYetCreated)
{
    InSequence dummy;
    EXPECT_CALL(*dispatcherMock, waitConnectedAsync(_))
        .Times(0);
    sdlStorage->waitReadyAsync(ns, std::bind(&AsyncRedisStorageTestDispatcherNotCreated::readyAck, this, std::placeholders::_1));
}

TEST_F(AsyncRedisStorageTestDispatcherNotCreated, SetAsyncWithoutDispatcherInstanceNacksWithREDIS_NOT_YET_DISCOVERED)
{
    InSequence dummy;
    expectPostCallback();
    sdlStorage->setAsync(ns,
                         { { key1, { } } },
                         std::bind(&AsyncRedisStorageTestDispatcherNotCreated::modifyAck,
                                   this,
                                   std::placeholders::_1));
    expectModifyAck(std::error_code(AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED));
    storedCallback();
}

TEST_F(AsyncRedisStorageTestDispatcherNotCreated, SetIfAsyncWithoutDispatcherInstanceNacksWithREDIS_NOT_YET_DISCOVERED)
{
    InSequence dummy;
    expectPostCallback();
    sdlStorage->setIfAsync(ns,
                           key1,
                           data1,
                           data2,
                           std::bind(&AsyncRedisStorageTestDispatcherNotCreated::modifyIfAck,
                                     this, std::placeholders::_1,  std::placeholders::_2));
    expectModifyIfAck(std::error_code(AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED), false);
    storedCallback();
}

TEST_F(AsyncRedisStorageTestDispatcherNotCreated, SetIfNotExistsAsyncWithoutDispatcherInstanceNacksWithREDIS_NOT_YET_DISCOVERED)
{
    InSequence dummy;
    expectPostCallback();
    sdlStorage->setIfNotExistsAsync(ns,
                                    key1,
                                    data1,
                                    std::bind(&AsyncRedisStorageTestDispatcherNotCreated::modifyIfAck,
                                              this, std::placeholders::_1,  std::placeholders::_2));
    expectModifyIfAck(std::error_code(AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED), false);
    storedCallback();
}

TEST_F(AsyncRedisStorageTestDispatcherNotCreated, GetAsyncWithoutDispatcherInstanceNacksWithREDIS_NOT_YET_DISCOVERED)
{
    InSequence dummy;
    expectPostCallback();
    sdlStorage->getAsync(ns,
                         {key1},
                         std::bind(&AsyncRedisStorageTestDispatcherNotCreated::getAck,
                                   this,
                                   std::placeholders::_1,
                                   std::placeholders::_2));
    expectGetAck(std::error_code(AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED), { });
    storedCallback();
}

TEST_F(AsyncRedisStorageTestDispatcherNotCreated, RemoveAsyncWithoutDispatcherInstanceNacksWithREDIS_NOT_YET_DISCOVERED)
{
    InSequence dummy;
    expectPostCallback();
    sdlStorage->removeAsync(ns,
                            {key1},
                            std::bind(&AsyncRedisStorageTestDispatcherNotCreated::modifyAck, this, std::placeholders::_1));
    expectModifyAck(std::error_code(AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED));
    storedCallback();
}

TEST_F(AsyncRedisStorageTestDispatcherNotCreated, FindKeysAsyncWithoutDispatcherInstanceNacksWithREDIS_NOT_YET_DISCOVERED)
{
    InSequence dummy;
    expectPostCallback();
    sdlStorage->findKeysAsync(ns,
                              "*",
                              std::bind(&AsyncRedisStorageTestDispatcherNotCreated::findKeysAck,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2));
    expectFindKeysAck(std::error_code(AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED), { });
    storedCallback();
}

TEST_F(AsyncRedisStorageTestDispatcherNotCreated, RemoveAllAsyncWithoutDispatcherInstanceNacksWithREDIS_NOT_YET_DISCOVERED)
{
    InSequence dummy;
    expectPostCallback();
    sdlStorage->removeAllAsync(ns,
                               std::bind(&AsyncRedisStorageTestDispatcherNotCreated::modifyAck, this, std::placeholders::_1));
    expectModifyAck(std::error_code(AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED));
    storedCallback();
}
