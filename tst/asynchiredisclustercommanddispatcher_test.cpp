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
#include <cstring>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/eventfd.h>
#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <async.h>
#include "private/createlogger.hpp"
#include "private/error.hpp"
#include "private/logger.hpp"
#include "private/redis/asynchiredisclustercommanddispatcher.hpp"
#include "private/redis/redisgeneral.hpp"
#include "private/redis/reply.hpp"
#include "private/redis/contents.hpp"
#include "private/tst/hiredisclustersystemmock.hpp"
#include "private/timer.hpp"
#include "private/tst/contentsbuildermock.hpp"
#include "private/tst/enginemock.hpp"
#include "private/tst/hiredisclusterepolladaptermock.hpp"
#include "private/tst/redisreplybuilder.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class AsyncHiredisClusterCommandDispatcherBaseTest: public testing::Test
    {
    public:
        std::shared_ptr<ContentsBuilderMock> contentsBuilderMock;
        StrictMock<EngineMock> engineMock;
        HiredisClusterSystemMock hiredisClusterSystemMock;
        std::shared_ptr<HiredisClusterEpollAdapterMock> adapterMock;
        redisClusterAsyncContext acc;
        redisAsyncContext ac;
        int hiredisFd;
        std::unique_ptr<AsyncHiredisClusterCommandDispatcher> dispatcher;
        void (*connected)(const redisClusterAsyncContext*, const redisAsyncContext*, int);
        void (*disconnected)(const redisClusterAsyncContext*, const redisAsyncContext*, int);
        Timer::Callback savedConnectionRetryTimerCallback;
        Timer::Duration expectedRetryTimerDuration;
        Contents contents;
        Contents clusterConnectionSetupContents;
        RedisReplyBuilder redisReplyBuilder;
        const AsyncConnection::Namespace defaultNamespace;
        std::shared_ptr<Logger> logger;

        AsyncHiredisClusterCommandDispatcherBaseTest():
            contentsBuilderMock(std::make_shared<ContentsBuilderMock>(AsyncConnection::SEPARATOR)),
            adapterMock(std::make_shared<HiredisClusterEpollAdapterMock>(engineMock, hiredisClusterSystemMock)),
            acc { },
            ac { },
            hiredisFd(3),
            connected(nullptr),
            disconnected(nullptr),
            expectedRetryTimerDuration(std::chrono::seconds(1)),
            contents { { "CMD", "key1", "value1", "key2", "value2" },
                       { 3, 4, 6, 4, 6 } },
            redisReplyBuilder { },
            defaultNamespace("namespace"),
            logger(createLogger(SDL_LOG_PREFIX))
        {
        }

        virtual ~AsyncHiredisClusterCommandDispatcherBaseTest()
        {
        }

        MOCK_METHOD0(connectAck, void());

        MOCK_METHOD0(disconnectCallback, void());

        MOCK_METHOD2(ack, void(const std::error_code&, const Reply&));

        void expectationsUntilConnect()
        {
            expectationsUntilConnect(acc);
        }

        void expectationsUntilConnect(redisClusterAsyncContext& acc)
        {
            expectRedisClusterAsyncConnect(acc);
        }

        void expectRedisClusterAsyncConnect()
        {
            expectRedisClusterAsyncConnect(acc);
        }

        void expectRedisClusterAsyncConnect(redisClusterAsyncContext& acc)
        {
            EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncConnect(StrEq("addr1:28416,addr2:56832"),
                                                                           HIRCLUSTER_FLAG_ROUTE_USE_SLOTS))
                .Times(1)
                .WillOnce(InvokeWithoutArgs([this, &acc]()
                                            {
                                                return &acc;
                                            }));
        }

        void expectRedisClusterAsyncConnectReturnNullptr()
        {
            EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncConnect(StrEq("addr1:28416,addr2:56832"),
                                                                           HIRCLUSTER_FLAG_ROUTE_USE_SLOTS))
                .Times(1)
                .WillOnce(InvokeWithoutArgs([this]()
                                            {
                                                return nullptr;
                                            }));
        }

        void expectRedisClusterAsyncSetConnectCallback()
        {
            expectRedisClusterAsyncSetConnectCallback(acc);
        }

        void expectRedisClusterAsyncSetConnectCallback(redisClusterAsyncContext& acc)
        {
            EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncSetConnectCallback(&acc, _))
                .Times(1)
                .WillOnce(Invoke([this](const redisClusterAsyncContext*, redisClusterInstanceConnectCallback* cb)
                                 {
                                     connected = cb;
                                     return REDIS_OK;
                                 }));
        }

        void expectRedisClusterAsyncSetDisconnectCallback()
        {
            expectRedisClusterAsyncSetDisconnectCallback(acc);
        }

        void expectRedisClusterAsyncSetDisconnectCallback(redisClusterAsyncContext& acc)
        {
            EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncSetDisconnectCallback(&acc, _))
                .Times(1)
                .WillOnce(Invoke([this](const redisClusterAsyncContext*, redisClusterInstanceDisconnectCallback* cb)
                                 {
                                     disconnected = cb;
                                     return REDIS_OK;
                                 }));
        }

        void expectCommandListQuery()
        {
            expectRedisClusterAsyncCommandArgv(redisReplyBuilder.buildCommandListQueryReply());
        }

        void expectCommandListQueryReturnError()
        {
            expectRedisClusterAsyncCommandArgv(redisReplyBuilder.buildErrorReply("SomeErrorForCommandListQuery"));
        }

        void expectAdapterSetup()
        {
            expectAdapterSetup(acc);
        }

        void expectAdapterSetup(redisClusterAsyncContext& acc)
        {
            EXPECT_CALL(*adapterMock, setup(&acc))
                .Times(1);
        }

        void expectAdapterDetach()
        {
            EXPECT_CALL(*adapterMock, detach(&ac))
                .Times(1);
        }

        void expectConnectAck()
        {
            EXPECT_CALL(*this, connectAck())
                .Times(1);
        }

        void expectDisconnectCallback()
        {
            EXPECT_CALL(*this, disconnectCallback())
                .Times(1);
        }

        void expectRedisClusterAsyncFree()
        {
            EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncFree(&acc))
                .Times(1);
        }

        void expectRedisClusterAsyncDisconnect()
        {
            EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncDisconnect(&acc))
                .Times(1);
        }

        void verifyAckErrorReply(const Reply& reply)
        {
            EXPECT_EQ(Reply::Type::NIL, reply.getType());
            EXPECT_EQ(0, reply.getInteger());
            EXPECT_TRUE(reply.getString()->str.empty());
            EXPECT_EQ(static_cast<ReplyStringLength>(0), reply.getString()->len);
            EXPECT_TRUE(reply.getArray()->empty());
        }

        void expectAckError()
        {
            EXPECT_CALL(*this, ack(Ne(std::error_code()), _))
                .Times(1)
                .WillOnce(Invoke([this](const std::error_code&, const Reply& reply)
                                 {
                                     verifyAckErrorReply(reply);
                                 }));
        }

        void expectAckError(const std::error_code& ec)
        {
            EXPECT_CALL(*this, ack(ec, _))
                .Times(1)
                .WillOnce(Invoke([this](const std::error_code&, const Reply& reply)
                                 {
                                     verifyAckErrorReply(reply);
                                 }));
        }

        void expectArmConnectionRetryTimer()
        {
            EXPECT_CALL(engineMock, armTimer(_, expectedRetryTimerDuration, _))
                .Times(1)
                .WillOnce(SaveArg<2>(&savedConnectionRetryTimerCallback));
        }

        void expectDisarmConnectionRetryTimer()
        {
            EXPECT_CALL(engineMock, disarmTimer(_))
                .Times(1);
        }

        void expectRedisClusterAsyncCommandArgv(redisReply& rr)
        {
            EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncCommandArgvWithKey(&acc, _, _, _, _, _, _, _))
                .Times(1)
                .WillOnce(Invoke([&rr](redisClusterAsyncContext* acc, redisClusterCallbackFn* cb, void* pd, const char*, int,
                                       int, const char**, const size_t*)
                                 {
                                     cb(acc, &rr, pd);
                                     return REDIS_OK;
                                 }));
        }

        void expectAck()
        {
            EXPECT_CALL(*this, ack(std::error_code(), _))
                .Times(1);
        }

        void expectReplyError(const std::string& msg)
        {
            EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncCommandArgvWithKey(&acc, _, _, _, _, _, _, _))
                .Times(1)
                .WillOnce(Invoke([this, msg](redisClusterAsyncContext* acc, redisClusterCallbackFn* cb, void* pd, const char*,
                                    int, int, const char**, const size_t*)
                                 {
                                     cb(acc, &redisReplyBuilder.buildErrorReply(msg), pd);
                                     return REDIS_OK;
                                 }));
        }

        void expectContextError(int code)
        {
            EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncCommandArgvWithKey(&acc, _, _, _, _, _, _, _))
                .Times(1)
                .WillOnce(Invoke([code](redisClusterAsyncContext* acc, redisClusterCallbackFn* cb, void* pd, const char*, int,
                                        int, const char**, const size_t*)
                                 {
                                     acc->err = code;
                                     cb(acc, nullptr, pd);
                                     return REDIS_OK;
                                 }));
        }

        void expectRedisClusterAsyncFreeCallPendingCallback(redisClusterCallbackFn* cb, void* pd)
        {
            EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncFree(&acc))
                .Times(1)
                .WillOnce(Invoke([this, cb, pd](redisClusterAsyncContext* acc)
                                 {
                                     cb(acc, &redisReplyBuilder.buildNilReply(), pd);
                                 }));
        }

        void expectAckNotCalled()
        {
            EXPECT_CALL(*this, ack(_,_))
                .Times(0);
        }

        void expectionsForSuccessfullConnectionSetup()
        {
            expectationsUntilConnect();
            expectAdapterSetup();
            expectRedisClusterAsyncSetConnectCallback();
            expectRedisClusterAsyncSetDisconnectCallback();
            expectCommandListQuery();
        }

        void callConnectionRetryTimerCallback()
        {
            ASSERT_NE(savedConnectionRetryTimerCallback, nullptr);
            savedConnectionRetryTimerCallback();
        }
    };

    class AsyncHiredisClusterCommandDispatcherDisconnectedTest: public AsyncHiredisClusterCommandDispatcherBaseTest
    {
    public:
        AsyncHiredisClusterCommandDispatcherDisconnectedTest()
        {
            InSequence dummy;
            expectationsUntilConnect();
            expectAdapterSetup();
            expectRedisClusterAsyncSetConnectCallback();
            expectRedisClusterAsyncSetDisconnectCallback();
            EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncCommandArgvWithKey(&acc, _, _, _, _, _, _, _))
                .Times(1);
            dispatcher.reset(new AsyncHiredisClusterCommandDispatcher(engineMock,
                                                                      defaultNamespace,
                                                                      { { "addr1", 111 }, { "addr2", 222 } },
                                                                      contentsBuilderMock,
                                                                      false,
                                                                      hiredisClusterSystemMock,
                                                                      adapterMock,
                                                                      logger));
        }

        ~AsyncHiredisClusterCommandDispatcherDisconnectedTest()
        {
        }
    };

    class AsyncHiredisClusterCommandDispatcherWithPermanentCommandCallbacksTest: public AsyncHiredisClusterCommandDispatcherBaseTest
    {
    public:
        AsyncHiredisClusterCommandDispatcherWithPermanentCommandCallbacksTest()
        {
            InSequence dummy;
            expectionsForSuccessfullConnectionSetup();
            dispatcher.reset(new AsyncHiredisClusterCommandDispatcher(engineMock,
                                                                      defaultNamespace,
                                                                      { { "addr1", 111 }, { "addr2", 222 } },
                                                                      contentsBuilderMock,
                                                                      true,
                                                                      hiredisClusterSystemMock,
                                                                      adapterMock,
                                                                      logger));
        }

        ~AsyncHiredisClusterCommandDispatcherWithPermanentCommandCallbacksTest()
        {
            expectRedisClusterAsyncFree();
        }
    };

    class AsyncHiredisClusterCommandDispatcherConnectedTest: public AsyncHiredisClusterCommandDispatcherBaseTest
    {
    public:
        redisClusterCallbackFn* savedCb;
        void* savedPd;

        AsyncHiredisClusterCommandDispatcherConnectedTest():
            savedCb(nullptr),
            savedPd(nullptr)
        {
            InSequence dummy;
            expectionsForSuccessfullConnectionSetup();
            dispatcher.reset(new AsyncHiredisClusterCommandDispatcher(engineMock,
                                                                      defaultNamespace,
                                                                      { { "addr1", 111 }, { "addr2", 222 } },
                                                                      contentsBuilderMock,
                                                                      false,
                                                                      hiredisClusterSystemMock,
                                                                      adapterMock,
                                                                      logger));
            connected(&acc, &ac, 0);
        }

        ~AsyncHiredisClusterCommandDispatcherConnectedTest()
        {
            expectRedisClusterAsyncFree();
        }

        void expectRedisClusterAsyncCommandArgvWithKey_SaveCb()
        {
            EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncCommandArgvWithKey(&acc, _, _, _, _, _, _, _))
                .Times(1)
                .WillOnce(Invoke([this](redisClusterAsyncContext*, redisClusterCallbackFn* cb, void* pd,
                                        const char*, int, int, const char**, const size_t*)
                                 {
                                     savedCb = cb;
                                     savedPd = pd;
                                     return REDIS_OK;
                                 }));
        }
    };

    using AsyncHiredisClusterCommandDispatcherDeathTest = AsyncHiredisClusterCommandDispatcherConnectedTest;
}

TEST_F(AsyncHiredisClusterCommandDispatcherDisconnectedTest, IsNotCopyable)
{
    EXPECT_FALSE(std::is_copy_constructible<AsyncHiredisClusterCommandDispatcher>::value);
    EXPECT_FALSE(std::is_copy_assignable<AsyncHiredisClusterCommandDispatcher>::value);
}

TEST_F(AsyncHiredisClusterCommandDispatcherDisconnectedTest, ImplementsAsyncRedisCommandDispatcher)
{
    EXPECT_TRUE((std::is_base_of<AsyncCommandDispatcher, AsyncHiredisClusterCommandDispatcher>::value));
}

TEST_F(AsyncHiredisClusterCommandDispatcherDisconnectedTest, CannotDispatchCommandsIfDisconnected)
{
    Engine::Callback storedCallback;
    EXPECT_CALL(engineMock, postCallback(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&storedCallback));
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherDisconnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              defaultNamespace,
                              { });
    expectAckError(AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED);
    storedCallback();
}

TEST_F(AsyncHiredisClusterCommandDispatcherBaseTest, ContextErrorInConnectArmsRetryTimer)
{
    InSequence dummy;
    acc.err = 123;
    expectationsUntilConnect();
    expectArmConnectionRetryTimer();
    expectDisarmConnectionRetryTimer();

    dispatcher.reset(new AsyncHiredisClusterCommandDispatcher(engineMock,
                                                              defaultNamespace,
                                                              { { "addr1", 111 }, { "addr2", 222 } },
                                                              contentsBuilderMock,
                                                              false,
                                                              hiredisClusterSystemMock,
                                                              adapterMock,
                                                              logger));
}

TEST_F(AsyncHiredisClusterCommandDispatcherBaseTest, NullRedisContextInConnectArmsRetryTimer)
{
    InSequence dummy;
    expectRedisClusterAsyncConnectReturnNullptr();
    expectArmConnectionRetryTimer();
    expectDisarmConnectionRetryTimer();

    dispatcher.reset(new AsyncHiredisClusterCommandDispatcher(engineMock,
                                                              defaultNamespace,
                                                              { { "addr1", 111 }, { "addr2", 222 } },
                                                              contentsBuilderMock,
                                                              false,
                                                              hiredisClusterSystemMock,
                                                              adapterMock,
                                                              logger));
}

TEST_F(AsyncHiredisClusterCommandDispatcherBaseTest, FailedCommandListQueryArmsRetryTimer)
{
    InSequence dummy;
    Engine::Callback storedCallback;
    expectationsUntilConnect();
    expectAdapterSetup();
    expectRedisClusterAsyncSetConnectCallback();
    expectRedisClusterAsyncSetDisconnectCallback();
    expectCommandListQueryReturnError();
    expectArmConnectionRetryTimer();

    dispatcher.reset(new AsyncHiredisClusterCommandDispatcher(engineMock,
                                                              defaultNamespace,
                                                              { { "addr1", 111 }, { "addr2", 222 } },
                                                              contentsBuilderMock,
                                                              false,
                                                              hiredisClusterSystemMock,
                                                              adapterMock,
                                                              logger));

    expectDisarmConnectionRetryTimer();
}

TEST_F(AsyncHiredisClusterCommandDispatcherBaseTest, ConnectionSucceedsWithRetryTimer)
{
    InSequence dummy;
    expectRedisClusterAsyncConnectReturnNullptr();
    expectArmConnectionRetryTimer();

    dispatcher.reset(new AsyncHiredisClusterCommandDispatcher(engineMock,
                                                              defaultNamespace,
                                                              { { "addr1", 111 }, { "addr2", 222 } },
                                                              contentsBuilderMock,
                                                              false,
                                                              hiredisClusterSystemMock,
                                                              adapterMock,
                                                              logger));

    expectionsForSuccessfullConnectionSetup();
    expectRedisClusterAsyncFree();

    callConnectionRetryTimerCallback();
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, ConnectAckCalledIfConnected)
{
    Engine::Callback storedCallback;
    EXPECT_CALL(engineMock, postCallback(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&storedCallback));
    dispatcher->waitConnectedAsync(std::bind(&AsyncHiredisClusterCommandDispatcherDisconnectedTest::connectAck,
                                             this));
    expectConnectAck();
    storedCallback();
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, CanDispatchCommands)
{
    EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncCommandArgvWithKey(&acc, _, _, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([this](redisClusterAsyncContext* acc, redisClusterCallbackFn* cb, void* pd, const char *key,
                                int keylen, int argc, const char** argv, const size_t* argvlen)
                         {
                             EXPECT_STREQ(defaultNamespace.c_str(), key);
                             EXPECT_EQ(9, keylen);
                             EXPECT_EQ((int)contents.stack.size(), argc);
                             EXPECT_EQ(contents.sizes[0], argvlen[0]);
                             EXPECT_EQ(contents.sizes[1], argvlen[1]);
                             EXPECT_EQ(contents.sizes[2], argvlen[2]);
                             EXPECT_EQ(contents.sizes[3], argvlen[3]);
                             EXPECT_EQ(contents.sizes[4], argvlen[4]);
                             EXPECT_FALSE(std::memcmp(argv[0], contents.stack[0].c_str(), contents.sizes[0]));
                             EXPECT_FALSE(std::memcmp(argv[1], contents.stack[1].c_str(), contents.sizes[1]));
                             EXPECT_FALSE(std::memcmp(argv[2], contents.stack[2].c_str(), contents.sizes[2]));
                             EXPECT_FALSE(std::memcmp(argv[3], contents.stack[3].c_str(), contents.sizes[3]));
                             EXPECT_FALSE(std::memcmp(argv[4], contents.stack[4].c_str(), contents.sizes[4]));
                             cb(acc, &redisReplyBuilder.buildNilReply(), pd);
                             return REDIS_OK;
                         }));
    expectAck();
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              defaultNamespace,
                              contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, CanParseNilReply)
{
    expectRedisClusterAsyncCommandArgv(redisReplyBuilder.buildNilReply());
    EXPECT_CALL(*this, ack(std::error_code(), _))
        .Times(1)
        .WillOnce(Invoke([](const std::error_code&, const Reply& reply)
                         {
                             EXPECT_EQ(Reply::Type::NIL, reply.getType());
                             EXPECT_EQ(0, reply.getInteger());
                             EXPECT_TRUE(reply.getString()->str.empty());
                             EXPECT_EQ(static_cast<ReplyStringLength>(0), reply.getString()->len);
                             EXPECT_TRUE(reply.getArray()->empty());
                         }));
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              defaultNamespace,
                              contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, CanParseIntegerReply)
{
    expectRedisClusterAsyncCommandArgv(redisReplyBuilder.buildIntegerReply());
    EXPECT_CALL(*this, ack(std::error_code(), _))
        .Times(1)
        .WillOnce(Invoke([this](const std::error_code&, const Reply& reply)
                         {
                             auto expected(redisReplyBuilder.buildIntegerReply());
                             EXPECT_EQ(Reply::Type::INTEGER, reply.getType());
                             EXPECT_EQ(expected.integer, reply.getInteger());
                         }));
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              defaultNamespace,
                              contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, CanParseStatusReply)
{
    expectRedisClusterAsyncCommandArgv(redisReplyBuilder.buildStatusReply());
    EXPECT_CALL(*this, ack(std::error_code(), _))
        .Times(1)
        .WillOnce(Invoke([this](const std::error_code&, const Reply& reply)
                         {
                             auto expected(redisReplyBuilder.buildStatusReply());
                             EXPECT_EQ(Reply::Type::STATUS, reply.getType());
                             EXPECT_EQ(expected.len, reply.getString()->len);
                             EXPECT_FALSE(std::memcmp(reply.getString()->str.c_str(), expected.str, expected.len));
                         }));
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              defaultNamespace,
                              contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, CanParseStringReply)
{
    expectRedisClusterAsyncCommandArgv(redisReplyBuilder.buildStringReply());
    EXPECT_CALL(*this, ack(std::error_code(), _))
        .Times(1)
        .WillOnce(Invoke([this](const std::error_code&, const Reply& reply)
                         {
                             auto expected(redisReplyBuilder.buildStringReply());
                             EXPECT_EQ(Reply::Type::STRING, reply.getType());
                             EXPECT_EQ(expected.len, reply.getString()->len);
                             EXPECT_FALSE(std::memcmp(reply.getString()->str.c_str(), expected.str, expected.len));
                         }));
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              defaultNamespace,
                              contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, CanParseArrayReply)
{
    expectRedisClusterAsyncCommandArgv(redisReplyBuilder.buildArrayReply());
    EXPECT_CALL(*this, ack(std::error_code(), _))
        .Times(1)
        .WillOnce(Invoke([this](const std::error_code&, const Reply& reply)
                         {
                             auto array(reply.getArray());
                             EXPECT_EQ(Reply::Type::ARRAY, reply.getType());
                             EXPECT_EQ(Reply::Type::STRING, (*array)[0]->getType());
                             EXPECT_EQ(Reply::Type::NIL, (*array)[1]->getType());
                         }));
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              defaultNamespace,
                              contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, CanHandleDispatchHiredisBufferErrors)
{
    EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncCommandArgvWithKey(&acc, _, _, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([](redisClusterAsyncContext* acc, redisClusterCallbackFn*, void*, const char*, int, int,
                            const char**, const size_t*)
                         {
                             acc->err = REDIS_ERR;
                             return REDIS_ERR;
                         }));
    Engine::Callback storedCallback;
    EXPECT_CALL(engineMock, postCallback(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&storedCallback));
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              defaultNamespace,
                              contents);
    expectAckError();
    storedCallback();
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, CanHandleDispatchHiredisCbErrors)
{
    EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncCommandArgvWithKey(&acc, _, _, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([](redisClusterAsyncContext* acc, redisClusterCallbackFn* cb, void* pd, const char*, int, int,
                            const char**, const size_t*)
                         {
                             cb(acc, nullptr, pd);
                             return REDIS_OK;
                         }));
    expectAckError();
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              defaultNamespace,
                              contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, DatasetStillBeingLoadedInMemoryIsRecognizedFromReply)
{
    expectReplyError("LOADING Redis is loading the dataset in memory");
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::DATASET_LOADING), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              defaultNamespace,
                              contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, ClusterDownIsRecognizedFromReply)
{
    //SDL checks only that reply starts with CLUSTERDOWN string
    expectReplyError("CLUSTERDOWN");
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);

    expectReplyError("CLUSTERDOWN The cluster is down");
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);

    expectReplyError("CLUSTERDOW");
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, ProtocolErrorIsRecognizedFromReply)
{
    expectReplyError("ERR Protocol error: invalid bulk length");
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::PROTOCOL_ERROR), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, UnrecognizedReplyErrorIsConvertedToUnknownError)
{
    expectReplyError("something sinister");
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, EmptyReplyErrorIsConvertedToUnknownError)
{
    expectReplyError("");
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, IOErrorInContext)
{
    EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncCommandArgvWithKey(&acc, _, _, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([](redisClusterAsyncContext* acc, redisClusterCallbackFn* cb, void* pd, const char*, int, int,
                            const char**, const size_t*)
                         {
                             acc->err = REDIS_ERR_IO;
                             errno = EINVAL;
                             cb(acc, nullptr, pd);
                             return REDIS_OK;
                         }));
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::IO_ERROR), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, EofErrorInContext)
{
    expectContextError(REDIS_ERR_EOF);
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::CONNECTION_LOST), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, ProtocolErrorInContext)
{
    expectContextError(REDIS_ERR_PROTOCOL);
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::PROTOCOL_ERROR), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, OomErrorInContext)
{
    expectContextError(REDIS_ERR_OOM);
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, ClusterErrorNotConnectedInContext)
{
    expectContextError(CLUSTER_ERROR_NOT_CONNECTED);
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, ClusterErrorConnectionLostInContext)
{
    expectContextError(CLUSTER_ERROR_CONNECTION_LOST);
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::CONNECTION_LOST), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, UnrecognizedContextErrorIsConvertedToUnknownError)
{
    expectContextError(REDIS_ERR_OTHER);
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, PendingClientCallbacksAreNotCalledAfterDisabled)
{
    InSequence dummy;
    expectRedisClusterAsyncCommandArgvWithKey_SaveCb();
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherDeathTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
    expectAck();
    savedCb(&acc, &redisReplyBuilder.buildStringReply(), savedPd);
    dispatcher->disableCommandCallbacks();
    expectRedisClusterAsyncCommandArgvWithKey_SaveCb();
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherDeathTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
    expectAckNotCalled();
    savedCb(&acc, &redisReplyBuilder.buildStringReply(), savedPd);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, DisconnectCallbackDetachesContextFromAdapter)
{
    InSequence dummy;
    expectAdapterDetach();
    disconnected(&acc, &ac, 0);
}

TEST_F(AsyncHiredisClusterCommandDispatcherConnectedTest, RegisteredClientDisconnectCallbackIsCalled)
{
    InSequence dummy;
    dispatcher->registerDisconnectCb(std::bind(&AsyncHiredisClusterCommandDispatcherConnectedTest::disconnectCallback,
                                             this));
    expectAdapterDetach();
    expectDisconnectCallback();
    disconnected(&acc, &ac, 0);
}

TEST_F(AsyncHiredisClusterCommandDispatcherWithPermanentCommandCallbacksTest, CanHandleMultipleRepliesForSameRedisCommand)
{
    InSequence dummy;
    redisClusterCallbackFn* savedCb;
    void* savedPd;
    EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncCommandArgvWithKey(&acc, _, _, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([&savedCb, &savedPd](redisClusterAsyncContext*, redisClusterCallbackFn* cb, void* pd,
                                              const char*, int, int, const char**, const size_t*)
                         {
                             savedCb = cb;
                             savedPd = pd;
                             return REDIS_OK;
                         }));
    Contents contents({ { "cmd", "key", "value" }, { 3, 3, 5 } });
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherDeathTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              defaultNamespace,
                              contents);
    EXPECT_CALL(*this, ack(std::error_code(), _))
            .Times(3);
    redisReply rr;
    rr.type = REDIS_REPLY_NIL;
    savedCb(&acc, &rr, savedPd);
    savedCb(&acc, &rr, savedPd);
    savedCb(&acc, &rr, savedPd);
}

TEST_F(AsyncHiredisClusterCommandDispatcherDeathTest, CbRemovedAfterHiredisCb)
{
    InSequence dummy;
    redisClusterCallbackFn* savedCb;
    void* savedPd;
    EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncCommandArgvWithKey(&acc, _, _, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([this, &savedCb, &savedPd](redisClusterAsyncContext* acc, redisClusterCallbackFn* cb, void* pd,
                                                    const char*, int, int, const char**, const size_t*)
                         {
                             savedCb = cb;
                             savedPd = pd;
                             cb(acc, &redisReplyBuilder.buildNilReply(), pd);
                             return REDIS_OK;
                         }));
    expectAck();
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherDeathTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
    EXPECT_EXIT(savedCb(&acc, &redisReplyBuilder.buildNilReply(), savedPd), KilledBySignal(SIGABRT), "");
}

TEST_F(AsyncHiredisClusterCommandDispatcherDeathTest, TooManyRepliesAborts)
{
    InSequence dummy;
    redisClusterCallbackFn* savedCb;
    void* savedPd;
    EXPECT_CALL(hiredisClusterSystemMock, redisClusterAsyncCommandArgvWithKey(&acc, _, _, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([&savedCb, &savedPd](redisClusterAsyncContext*, redisClusterCallbackFn* cb, void* pd,
                                              const char*, int, int, const char**, const size_t*)
                         {
                             savedCb = cb;
                             savedPd = pd;
                             return REDIS_OK;
                         }));
    Contents contents({ { "cmd", "key", "value" }, { 3, 3, 5 } });
    expectAck();
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisClusterCommandDispatcherDeathTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
    savedCb(&acc, &redisReplyBuilder.buildNilReply(), savedPd);
    EXPECT_EXIT(savedCb(&acc, &redisReplyBuilder.buildNilReply(), savedPd), KilledBySignal(SIGABRT), "");
}
