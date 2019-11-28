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
#include "private/redis/asynchirediscommanddispatcher.hpp"
#include "private/redis/reply.hpp"
#include "private/redis/contents.hpp"
#include "private/timer.hpp"
#include "private/tst/contentsbuildermock.hpp"
#include "private/tst/enginemock.hpp"
#include "private/tst/hiredissystemmock.hpp"
#include "private/tst/hiredisepolladaptermock.hpp"
#include "private/tst/redisreplybuilder.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class AsyncHiredisCommandDispatcherBaseTest: public testing::Test
    {
    public:
        std::shared_ptr<ContentsBuilderMock> contentsBuilderMock;
        StrictMock<EngineMock> engineMock;
        HiredisSystemMock hiredisSystemMock;
        std::shared_ptr<HiredisEpollAdapterMock> adapterMock;
        redisAsyncContext ac;
        int hiredisFd;
        std::unique_ptr<AsyncHiredisCommandDispatcher> dispatcher;
        void (*connected)(const redisAsyncContext*, int);
        void (*disconnected)(const redisAsyncContext*, int);
        Timer::Callback savedConnectionRetryTimerCallback;
        Timer::Duration expectedConnectionRetryTimerDuration;
        Timer::Duration expectedConnectionVerificationRetryTimerDuration;
        RedisReplyBuilder redisReplyBuilder;
        const AsyncConnection::Namespace defaultNamespace;
        std::shared_ptr<Logger> logger;

        AsyncHiredisCommandDispatcherBaseTest():
            contentsBuilderMock(std::make_shared<ContentsBuilderMock>(AsyncConnection::SEPARATOR)),
            adapterMock(std::make_shared<HiredisEpollAdapterMock>(engineMock, hiredisSystemMock)),
            ac { },
            hiredisFd(3),
            connected(nullptr),
            disconnected(nullptr),
            expectedConnectionRetryTimerDuration(std::chrono::seconds(1)),
            expectedConnectionVerificationRetryTimerDuration(std::chrono::seconds(10)),
            redisReplyBuilder { },
            defaultNamespace("namespace"),
            logger(createLogger(SDL_LOG_PREFIX))
        {
        }

        virtual ~AsyncHiredisCommandDispatcherBaseTest()
        {
        }

        MOCK_METHOD0(connectAck, void());

        MOCK_METHOD0(disconnectCallback, void());

        MOCK_METHOD2(ack, void(const std::error_code&, const Reply&));

        void expectationsUntilConnect()
        {
            expectationsUntilConnect(ac);
        }

        void expectationsUntilConnect(redisAsyncContext& ac)
        {
            expectRedisAsyncConnect(ac);
        }

        void expectRedisAsyncConnect()
        {
            expectRedisAsyncConnect(ac);
        }

        void expectRedisAsyncConnect(redisAsyncContext& ac)
        {
            EXPECT_CALL(hiredisSystemMock, redisAsyncConnect(StrEq("host"), 6379U))
                .Times(1)
                .WillOnce(InvokeWithoutArgs([this, &ac]()
                                            {
                                                ac.c.fd = hiredisFd;
                                                return &ac;
                                            }));
        }

        void expectRedisAsyncConnectReturnNullptr()
        {
            EXPECT_CALL(hiredisSystemMock, redisAsyncConnect(StrEq("host"), 6379U))
                .Times(1)
                .WillOnce(InvokeWithoutArgs([this]()
                                            {
                                                ac.c.fd = hiredisFd;
                                                return nullptr;
                                            }));
        }

        void expectRedisAsyncSetConnectCallback()
        {
            expectRedisAsyncSetConnectCallback(ac);
        }

        void expectRedisAsyncSetConnectCallback(redisAsyncContext& ac)
        {
            EXPECT_CALL(hiredisSystemMock, redisAsyncSetConnectCallback(&ac, _))
                .Times(1)
                .WillOnce(Invoke([this](const redisAsyncContext*, redisConnectCallback* cb)
                                 {
                                     connected = cb;
                                     return REDIS_OK;
                                 }));
        }

        void expectRedisAsyncSetDisconnectCallback()
        {
            expectRedisAsyncSetDisconnectCallback(ac);
        }

        void expectRedisAsyncSetDisconnectCallback(redisAsyncContext& ac)
        {
            EXPECT_CALL(hiredisSystemMock, redisAsyncSetDisconnectCallback(&ac, _))
                .Times(1)
                .WillOnce(Invoke([this](const redisAsyncContext*, redisDisconnectCallback* cb)
                                 {
                                     disconnected = cb;
                                     return REDIS_OK;
                                 }));
        }

        void expectAdapterAttach()
        {
            expectAdapterAttach(ac);
        }

        void expectAdapterAttach(redisAsyncContext& ac)
        {
            EXPECT_CALL(*adapterMock, attach(&ac))
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

        void expectRedisAsyncFree()
        {
            EXPECT_CALL(hiredisSystemMock, redisAsyncFree(&ac))
                .Times(1);
        }

        void expectRedisAsyncDisconnect()
        {
            EXPECT_CALL(hiredisSystemMock, redisAsyncDisconnect(&ac))
                .Times(1);
        }

        void expectRedisAsyncCommandArgv(redisReply& rr)
        {
            EXPECT_CALL(hiredisSystemMock, redisAsyncCommandArgv(&ac, _, _, _, _, _))
                .Times(1)
                .WillOnce(Invoke([&rr](redisAsyncContext* ac, redisCallbackFn* cb, void* pd,
                                       int, const char**, const size_t*)
                                 {
                                     cb(ac, &rr, pd);
                                     return REDIS_OK;
                                 }));
        }

        void expectCommandListQuery()
        {
            expectRedisAsyncCommandArgv(redisReplyBuilder.buildCommandListQueryReply());
        }

        void expectCommandListQueryReturnError()
        {
            expectRedisAsyncCommandArgv(redisReplyBuilder.buildErrorReply("SomeErrorForCommandListQuery"));
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
            EXPECT_CALL(engineMock, armTimer(_, expectedConnectionRetryTimerDuration, _))
                .Times(1)
                .WillOnce(SaveArg<2>(&savedConnectionRetryTimerCallback));
        }

        void expectArmConnectionVerificationRetryTimer()
        {
            EXPECT_CALL(engineMock, armTimer(_, expectedConnectionVerificationRetryTimerDuration, _))
                .Times(1)
                .WillOnce(SaveArg<2>(&savedConnectionRetryTimerCallback));
        }

        void expectDisarmConnectionRetryTimer()
        {
            EXPECT_CALL(engineMock, disarmTimer(_))
                .Times(1);
        }
    };

    class AsyncHiredisCommandDispatcherDisconnectedTest: public AsyncHiredisCommandDispatcherBaseTest
    {
    public:
        AsyncHiredisCommandDispatcherDisconnectedTest()
        {
                InSequence dummy;
                expectationsUntilConnect();
                expectAdapterAttach();
                expectRedisAsyncSetConnectCallback();
                expectRedisAsyncSetDisconnectCallback();
                dispatcher.reset(new AsyncHiredisCommandDispatcher(engineMock,
                                                                   "host",
                                                                   htons(6379U),
                                                                   contentsBuilderMock,
                                                                   false,
                                                                   hiredisSystemMock,
                                                                   adapterMock,
                                                                   logger,
                                                                   false));
        }

        ~AsyncHiredisCommandDispatcherDisconnectedTest()
        {
        }
    };

    class AsyncHiredisCommandDispatcherWithPermanentCommandCallbacksTest: public AsyncHiredisCommandDispatcherBaseTest
    {
    public:
        AsyncHiredisCommandDispatcherWithPermanentCommandCallbacksTest()
        {
                InSequence dummy;
                expectationsUntilConnect();
                expectAdapterAttach();
                expectRedisAsyncSetConnectCallback();
                expectRedisAsyncSetDisconnectCallback();
                dispatcher.reset(new AsyncHiredisCommandDispatcher(engineMock,
                                                                   "host",
                                                                   htons(6379U),
                                                                   contentsBuilderMock,
                                                                   true,
                                                                   hiredisSystemMock,
                                                                   adapterMock,
                                                                   logger,
                                                                   false));
        }

        ~AsyncHiredisCommandDispatcherWithPermanentCommandCallbacksTest()
        {
            expectRedisAsyncFree();
        }
    };

    class AsyncHiredisCommandDispatcherConnectedTest: public AsyncHiredisCommandDispatcherDisconnectedTest
    {
    public:
        Contents contents;
        redisCallbackFn* savedCb;
        void* savedPd;

        AsyncHiredisCommandDispatcherConnectedTest():
            contents { { "CMD", "key1", "value1", "key2", "value2" },
                       { 3, 4, 6, 4, 6 } },
            savedCb(nullptr),
            savedPd(nullptr)
        {
            expectCommandListQuery();
            connected(&ac, 0);
        }

        ~AsyncHiredisCommandDispatcherConnectedTest()
        {
            expectRedisAsyncFree();
        }

        void expectAck()
        {
            EXPECT_CALL(*this, ack(std::error_code(), _))
                .Times(1);
        }

        void expectReplyError(const std::string& msg)
        {
            EXPECT_CALL(hiredisSystemMock, redisAsyncCommandArgv(&ac, _, _, _, _, _))
                .Times(1)
                .WillOnce(Invoke([this, msg](redisAsyncContext* ac, redisCallbackFn* cb, void* pd,
                                    int, const char**, const size_t*)
                                 {
                                     cb(ac, &redisReplyBuilder.buildErrorReply(msg), pd);
                                     return REDIS_OK;
                                 }));
        }

        void expectContextError(int code)
        {
            EXPECT_CALL(hiredisSystemMock, redisAsyncCommandArgv(&ac, _, _, _, _, _))
                .Times(1)
                .WillOnce(Invoke([code](redisAsyncContext* ac, redisCallbackFn* cb, void* pd, int, const char**, const size_t*)
                                 {
                                     ac->err = code;
                                     cb(ac, nullptr, pd);
                                     return REDIS_OK;
                                 }));
        }

        void expectRedisAsyncFreeCallPendingCallback(redisCallbackFn* cb, void* pd)
        {
            EXPECT_CALL(hiredisSystemMock, redisAsyncFree(&ac))
                .Times(1)
                .WillOnce(Invoke([cb, pd](redisAsyncContext* ac)
                                 {
                                     cb(ac, nullptr, pd);
                                 }));
        }

        void expectAckNotCalled()
        {
            EXPECT_CALL(*this, ack(_,_))
                .Times(0);
        }

        void expectRedisAsyncCommandArgv_SaveCb()
        {
            EXPECT_CALL(hiredisSystemMock, redisAsyncCommandArgv(&ac, _, _, _, _, _))
                .Times(1)
                .WillRepeatedly(Invoke([this](redisAsyncContext*, redisCallbackFn* cb, void* pd,
                                              int, const char**, const size_t*)
                                              {
                                                  savedCb = cb;
                                                  savedPd = pd;
                                                  return REDIS_OK;
                                              }));
        }
    };

    using AsyncHiredisCommandDispatcherDeathTest = AsyncHiredisCommandDispatcherConnectedTest;

    class AsyncHiredisCommandDispatcherForSentinelTest: public AsyncHiredisCommandDispatcherBaseTest
    {
    public:
        AsyncHiredisCommandDispatcherForSentinelTest()
        {
                InSequence dummy;
                expectationsUntilConnect();
                expectAdapterAttach();
                expectRedisAsyncSetConnectCallback();
                expectRedisAsyncSetDisconnectCallback();
                dispatcher.reset(new AsyncHiredisCommandDispatcher(engineMock,
                                                                   "host",
                                                                   htons(6379U),
                                                                   contentsBuilderMock,
                                                                   true,
                                                                   hiredisSystemMock,
                                                                   adapterMock,
                                                                   logger,
                                                                   true));
        }

        ~AsyncHiredisCommandDispatcherForSentinelTest()
        {
            expectRedisAsyncFree();
        }
    };
}

TEST_F(AsyncHiredisCommandDispatcherDisconnectedTest, IsNotCopyable)
{
    EXPECT_FALSE(std::is_copy_constructible<AsyncHiredisCommandDispatcher>::value);
    EXPECT_FALSE(std::is_copy_assignable<AsyncHiredisCommandDispatcher>::value);
}

TEST_F(AsyncHiredisCommandDispatcherDisconnectedTest, ImplementsAsyncRedisCommandDispatcher)
{
    EXPECT_TRUE((std::is_base_of<AsyncCommandDispatcher, AsyncHiredisCommandDispatcher>::value));
}

TEST_F(AsyncHiredisCommandDispatcherDisconnectedTest, CannotDispatchCommandsIfDisconnected)
{
    Engine::Callback storedCallback;
    EXPECT_CALL(engineMock, postCallback(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&storedCallback));
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherDisconnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              defaultNamespace,
                              { });
    expectAckError(std::error_code(AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED));
    storedCallback();
}

TEST_F(AsyncHiredisCommandDispatcherDisconnectedTest, ContextErrorInConnectArmsRetryTimer)
{
    InSequence dummy;
    expectationsUntilConnect();
    expectArmConnectionRetryTimer();
    ac.err = 123;
    dispatcher.reset(new AsyncHiredisCommandDispatcher(engineMock,
                                                       "host",
                                                       htons(6379U),
                                                       contentsBuilderMock,
                                                       false,
                                                       hiredisSystemMock,
                                                       adapterMock,
                                                       logger,
                                                       false));
    expectDisarmConnectionRetryTimer();
}

TEST_F(AsyncHiredisCommandDispatcherBaseTest, NullRedisContextInConnectArmsRetryTimer)
{
    InSequence dummy;
    expectRedisAsyncConnectReturnNullptr();
    expectArmConnectionRetryTimer();
    expectDisarmConnectionRetryTimer();

    dispatcher.reset(new AsyncHiredisCommandDispatcher(engineMock,
                                                       "host",
                                                       htons(6379U),
                                                       contentsBuilderMock,
                                                       false,
                                                       hiredisSystemMock,
                                                       adapterMock,
                                                       logger,
                                                       false));
}

TEST_F(AsyncHiredisCommandDispatcherDisconnectedTest, FailedCommandListQueryArmsRetryTimer)
{
    InSequence dummy;
    expectCommandListQueryReturnError();
    expectArmConnectionVerificationRetryTimer();
    expectRedisAsyncFree();
    connected(&ac, 0);
    expectDisarmConnectionRetryTimer();
}

TEST_F(AsyncHiredisCommandDispatcherDisconnectedTest, ErrorInConnectedCallbackArmsRetryTimer)
{
    InSequence dummy;
    expectArmConnectionRetryTimer();
    connected(&ac, -1);
    expectDisarmConnectionRetryTimer();
}

TEST_F(AsyncHiredisCommandDispatcherDisconnectedTest, ConnectionSucceedsWithRetryTimer)
{
    InSequence dummy;
    expectArmConnectionRetryTimer();

    connected(&ac, -1);

    expectationsUntilConnect();
    expectAdapterAttach();
    expectRedisAsyncSetConnectCallback();
    expectRedisAsyncSetDisconnectCallback();

    savedConnectionRetryTimerCallback();

    expectCommandListQuery();
    expectConnectAck();

    dispatcher->waitConnectedAsync(std::bind(&AsyncHiredisCommandDispatcherDisconnectedTest::connectAck, this));
    connected(&ac, 0);

    expectRedisAsyncFree();
}

TEST_F(AsyncHiredisCommandDispatcherDisconnectedTest, ConnectAckCalledOnceConnected)
{
    InSequence dummy;
    expectCommandListQuery();
    expectConnectAck();
    dispatcher->waitConnectedAsync(std::bind(&AsyncHiredisCommandDispatcherDisconnectedTest::connectAck, this));
    connected(&ac, 0);
    expectRedisAsyncFree();
}

TEST_F(AsyncHiredisCommandDispatcherDisconnectedTest, ConnectAckCalledIfConnected)
{
    Engine::Callback storedCallback;
    EXPECT_CALL(engineMock, postCallback(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&storedCallback));
    expectCommandListQuery();
    connected(&ac, 0);
    dispatcher->waitConnectedAsync(std::bind(&AsyncHiredisCommandDispatcherDisconnectedTest::connectAck, this));
    expectConnectAck();
    storedCallback();
    expectRedisAsyncFree();
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, CanDispatchCommands)
{
    EXPECT_CALL(hiredisSystemMock, redisAsyncCommandArgv(&ac, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([this](redisAsyncContext* ac, redisCallbackFn* cb, void* pd,
                                int argc, const char** argv, const size_t* argvlen)
                         {
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
                             cb(ac, &redisReplyBuilder.buildNilReply(), pd);
                             return REDIS_OK;
                         }));
    expectAck();
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              defaultNamespace,
                              contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, CanParseNilReply)
{
    expectRedisAsyncCommandArgv(redisReplyBuilder.buildNilReply());
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
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, CanParseIntegerReply)
{
    expectRedisAsyncCommandArgv(redisReplyBuilder.buildIntegerReply());
    EXPECT_CALL(*this, ack(std::error_code(), _))
        .Times(1)
        .WillOnce(Invoke([this](const std::error_code&, const Reply& reply)
                         {
                             auto expected(redisReplyBuilder.buildIntegerReply());
                             EXPECT_EQ(Reply::Type::INTEGER, reply.getType());
                             EXPECT_EQ(expected.integer, reply.getInteger());
                         }));
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, CanParseStatusReply)
{
    expectRedisAsyncCommandArgv(redisReplyBuilder.buildStatusReply());
    EXPECT_CALL(*this, ack(std::error_code(), _))
        .Times(1)
        .WillOnce(Invoke([this](const std::error_code&, const Reply& reply)
                         {
                             auto expected(redisReplyBuilder.buildStatusReply());
                             EXPECT_EQ(Reply::Type::STATUS, reply.getType());
                             EXPECT_EQ(expected.len, reply.getString()->len);
                             EXPECT_FALSE(std::memcmp(reply.getString()->str.c_str(), expected.str, expected.len));
                         }));
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, CanParseStringReply)
{
    expectRedisAsyncCommandArgv(redisReplyBuilder.buildStringReply());
    EXPECT_CALL(*this, ack(std::error_code(), _))
        .Times(1)
        .WillOnce(Invoke([this](const std::error_code&, const Reply& reply)
                         {
                             auto expected(redisReplyBuilder.buildStringReply());
                             EXPECT_EQ(Reply::Type::STRING, reply.getType());
                             EXPECT_EQ(expected.len, reply.getString()->len);
                             EXPECT_FALSE(std::memcmp(reply.getString()->str.c_str(), expected.str, expected.len));
                         }));
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, CanParseArrayReply)
{
    expectRedisAsyncCommandArgv(redisReplyBuilder.buildArrayReply());
    EXPECT_CALL(*this, ack(std::error_code(), _))
        .Times(1)
        .WillOnce(Invoke([](const std::error_code&, const Reply& reply)
                         {
                             auto array(reply.getArray());
                             EXPECT_EQ(Reply::Type::ARRAY, reply.getType());
                             EXPECT_EQ(Reply::Type::STRING, (*array)[0]->getType());
                             EXPECT_EQ(Reply::Type::NIL, (*array)[1]->getType());
                         }));
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, CanHandleDispatchHiredisBufferErrors)
{
    EXPECT_CALL(hiredisSystemMock, redisAsyncCommandArgv(&ac, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([](redisAsyncContext* ac, redisCallbackFn*, void*, int, const char**, const size_t*)
                         {
                             ac->err = REDIS_ERR;
                             return REDIS_ERR;
                         }));
    Engine::Callback storedCallback;
    EXPECT_CALL(engineMock, postCallback(_))
        .Times(1)
        .WillOnce(SaveArg<0>(&storedCallback));
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
    expectAckError();
    storedCallback();
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, CanHandleDispatchHiredisCbErrors)
{
    EXPECT_CALL(hiredisSystemMock, redisAsyncCommandArgv(&ac, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([](redisAsyncContext* ac, redisCallbackFn* cb, void* pd, int, const char**, const size_t*)
                         {
                             cb(ac, nullptr, pd);
                             return REDIS_OK;
                         }));
    expectAckError();
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, DatasetStillBeingLoadedInMemoryIsRecognizedFromReply)
{
    expectReplyError("LOADING Redis is loading the dataset in memory");
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::DATASET_LOADING), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, ProtocolErrorIsRecognizedFromReply)
{
    expectReplyError("ERR Protocol error: invalid bulk length");
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::PROTOCOL_ERROR), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, UnrecognizedReplyErrorIsConvertedToUnknownError)
{
    expectReplyError("something sinister");
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, IOErrorInContext)
{
    EXPECT_CALL(hiredisSystemMock, redisAsyncCommandArgv(&ac, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([](redisAsyncContext* ac, redisCallbackFn* cb, void* pd, int, const char**, const size_t*)
                         {
                             ac->err = REDIS_ERR_IO;
                             errno = EINVAL;
                             cb(ac, nullptr, pd);
                             return REDIS_OK;
                         }));
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::IO_ERROR), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, IOErrorInContextWithECONNRESETerrnoValue)
{
    EXPECT_CALL(hiredisSystemMock, redisAsyncCommandArgv(&ac, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([](redisAsyncContext* ac, redisCallbackFn* cb, void* pd, int, const char**, const size_t*)
                         {
                             ac->err = REDIS_ERR_IO;
                             errno = ECONNRESET;
                             cb(ac, nullptr, pd);
                             return REDIS_OK;
                         }));
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::CONNECTION_LOST), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, EofErrorInContext)
{
    expectContextError(REDIS_ERR_EOF);
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::CONNECTION_LOST), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, ProtocolErrorInContext)
{
    expectContextError(REDIS_ERR_PROTOCOL);
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::PROTOCOL_ERROR), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, OomErrorInContext)
{
    expectContextError(REDIS_ERR_OOM);
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, UnrecognizedContextErrorIsConvertedToUnknownError)
{
    expectContextError(REDIS_ERR_OTHER);
    EXPECT_CALL(*this, ack(std::error_code(AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR), _))
        .Times(1);
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, PendingClientCallbacksAreNotCalledAfterDisabled)
{
    InSequence dummy;
    expectRedisAsyncCommandArgv_SaveCb();
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
    expectAck();
    savedCb(&ac, &redisReplyBuilder.buildStringReply(), savedPd);
    dispatcher->disableCommandCallbacks();
    expectRedisAsyncCommandArgv_SaveCb();
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
    expectAckNotCalled();
    savedCb(&ac, &redisReplyBuilder.buildStringReply(), savedPd);
}

TEST_F(AsyncHiredisCommandDispatcherConnectedTest, RegisteredClientDisconnectCallbackIsCalled)
{
    InSequence dummy;
    dispatcher->registerDisconnectCb(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::disconnectCallback,
                                             this));
    expectDisconnectCallback();
    expectArmConnectionRetryTimer();
    disconnected(&ac, 0);
    expectDisarmConnectionRetryTimer();
    expectCommandListQuery();
    connected(&ac, 0); // restore connection to meet destructor expectations
}

TEST_F(AsyncHiredisCommandDispatcherWithPermanentCommandCallbacksTest, CanHandleMultipleRepliesForSameRedisCommand)
{
    InSequence dummy;
    redisCallbackFn* savedCb;
    void* savedPd;
    Contents contents({ { "cmd", "key", "value" }, { 3, 3, 5 } });
    expectCommandListQuery();
    connected(&ac, 0);
    EXPECT_CALL(hiredisSystemMock, redisAsyncCommandArgv(&ac, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([&savedCb, &savedPd](redisAsyncContext*, redisCallbackFn* cb, void* pd,
                                              int, const char**, const size_t*)
                         {
                             savedCb = cb;
                             savedPd = pd;
                             return REDIS_OK;
                         }));
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
    EXPECT_CALL(*this, ack(std::error_code(), _))
            .Times(3);
    redisReply rr;
    rr.type = REDIS_REPLY_NIL;
    savedCb(&ac, &rr, savedPd);
    savedCb(&ac, &rr, savedPd);
    savedCb(&ac, &rr, savedPd);
}

TEST_F(AsyncHiredisCommandDispatcherDeathTest, CbRemovedAfterHiredisCb)
{
    redisCallbackFn* savedCb;
    void* savedPd;
    EXPECT_CALL(hiredisSystemMock, redisAsyncCommandArgv(&ac, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([this, &savedCb, &savedPd](redisAsyncContext* ac, redisCallbackFn* cb, void* pd,
                                                    int, const char**, const size_t*)
                         {
                             savedCb = cb;
                             savedPd = pd;
                             cb(ac, &redisReplyBuilder.buildNilReply(), pd);
                             return REDIS_OK;
                         }));
    expectAck();
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherDeathTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
    EXPECT_EXIT(savedCb(&ac, &redisReplyBuilder.buildNilReply(), savedPd), KilledBySignal(SIGABRT), "");
}

TEST_F(AsyncHiredisCommandDispatcherDeathTest, TooManyRepliesAborts)
{
    InSequence dummy;
    redisCallbackFn* savedCb;
    void* savedPd;
    Contents contents({ { "cmd", "key", "value" }, { 3, 3, 5 } });
    EXPECT_CALL(hiredisSystemMock, redisAsyncCommandArgv(&ac, _, _, _, _, _))
        .Times(1)
        .WillOnce(Invoke([&savedCb, &savedPd](redisAsyncContext*, redisCallbackFn* cb, void* pd,
                                              int, const char**, const size_t*)
                         {
                             savedCb = cb;
                             savedPd = pd;
                             return REDIS_OK;
                         }));
    expectAck();
    dispatcher->dispatchAsync(std::bind(&AsyncHiredisCommandDispatcherConnectedTest::ack,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                                        defaultNamespace,
                                        contents);
    savedCb(&ac, &redisReplyBuilder.buildNilReply(), savedPd);
    EXPECT_EXIT(savedCb(&ac, &redisReplyBuilder.buildNilReply(), savedPd), KilledBySignal(SIGABRT), "");
}

TEST_F(AsyncHiredisCommandDispatcherForSentinelTest, CommandListInquiryIsNotSent)
{
    EXPECT_CALL(hiredisSystemMock, redisAsyncCommandArgv(_, _, _, _, _, _))
        .Times(0);
    connected(&ac, 0);
}
