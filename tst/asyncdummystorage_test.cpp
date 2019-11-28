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
#include <type_traits>
#include <memory>
#include <sys/eventfd.h>
#include "private/asyncdummystorage.hpp"
#include "private/tst/enginemock.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class AsyncDummyStorageTest: public testing::Test
    {
    public:
        std::shared_ptr<StrictMock<EngineMock>> engineMock;
        int fd;
        AsyncStorage::Namespace ns;
        Engine::Callback storedCallback;
        std::unique_ptr<AsyncDummyStorage> dummyStorage;

        AsyncDummyStorageTest():
            engineMock(std::make_shared<StrictMock<EngineMock>>()),
            fd(10),
            ns("someKnownNamespace")
        {
            dummyStorage.reset(new AsyncDummyStorage(engineMock));
        }

        MOCK_METHOD1(ack1, void(const std::error_code&));

        MOCK_METHOD2(ack2, void(const std::error_code&, bool));

        MOCK_METHOD2(ack3, void(const std::error_code&, const AsyncStorage::DataMap&));

        MOCK_METHOD2(ack4, void(const std::error_code&, const AsyncStorage::Keys&));

        void expectAck1()
        {
            EXPECT_CALL(*this, ack1(std::error_code()))
                .Times(1);
        }

        void expectAck2()
        {
            EXPECT_CALL(*this, ack2(std::error_code(), true))
                .Times(1);
        }

        void expectAck3()
        {
            EXPECT_CALL(*this, ack3(std::error_code(), IsEmpty()))
                .Times(1);
        }

        void expectAck4()
        {
            EXPECT_CALL(*this, ack4(std::error_code(), IsEmpty()))
                .Times(1);
        }

        void expectPostCallback()
        {
            EXPECT_CALL(*engineMock, postCallback(_))
                .Times(1)
                .WillOnce(SaveArg<0>(&storedCallback));
        }
    };
}

TEST_F(AsyncDummyStorageTest, IsNotCopyableAndIsNotMovable)
{
    EXPECT_FALSE(std::is_copy_assignable<AsyncDummyStorage>::value);
    EXPECT_FALSE(std::is_move_assignable<AsyncDummyStorage>::value);
    EXPECT_FALSE(std::is_copy_constructible<AsyncDummyStorage>::value);
    EXPECT_FALSE(std::is_move_constructible<AsyncDummyStorage>::value);
}

TEST_F(AsyncDummyStorageTest, ImplementsAsyncStorage)
{
    EXPECT_TRUE((std::is_base_of<AsyncStorage, AsyncDummyStorage>::value));
}

TEST_F(AsyncDummyStorageTest, CanGetFd)
{
    EXPECT_CALL(*engineMock, fd())
        .Times(1)
        .WillOnce(Return(fd));
    EXPECT_EQ(fd, dummyStorage->fd());
}

TEST_F(AsyncDummyStorageTest, CanHandleEvents)
{
    EXPECT_CALL(*engineMock, handleEvents())
        .Times(1);
    dummyStorage->handleEvents();
}

TEST_F(AsyncDummyStorageTest, AcksAreImmediatelyScheduled)
{
    InSequence dummy;

    expectPostCallback();
    dummyStorage->waitReadyAsync(ns, std::bind(&AsyncDummyStorageTest::ack1,
                                               this,
                                               std::placeholders::_1));
    expectAck1();
    storedCallback();

    expectPostCallback();
    dummyStorage->setAsync(ns, { }, std::bind(&AsyncDummyStorageTest::ack1,
                                              this,
                                              std::placeholders::_1));
    expectAck1();
    storedCallback();

    expectPostCallback();
    dummyStorage->setIfAsync(ns, { }, { }, { }, std::bind(&AsyncDummyStorageTest::ack2,
                                                          this,
                                                          std::placeholders::_1,
                                                          std::placeholders::_2));
    expectAck2();
    storedCallback();

    expectPostCallback();
    dummyStorage->setIfNotExistsAsync(ns, { }, { }, std::bind(&AsyncDummyStorageTest::ack2,
                                                              this,
                                                              std::placeholders::_1,
                                                              std::placeholders::_2));
    expectAck2();
    storedCallback();

    expectPostCallback();
    dummyStorage->getAsync(ns, { }, std::bind(&AsyncDummyStorageTest::ack3,
                                              this,
                                              std::placeholders::_1,
                                              std::placeholders::_2));
    expectAck3();
    storedCallback();

    expectPostCallback();
    dummyStorage->removeAsync(ns, { }, std::bind(&AsyncDummyStorageTest::ack1,
                                                 this,
                                                 std::placeholders::_1));
    expectAck1();
    storedCallback();

    expectPostCallback();
    dummyStorage->removeIfAsync(ns, { }, { }, std::bind(&AsyncDummyStorageTest::ack2,
                                                        this,
                                                        std::placeholders::_1,
                                                        std::placeholders::_2));
    expectAck2();
    storedCallback();

    expectPostCallback();
    dummyStorage->findKeysAsync(ns,
                                "*",
                                std::bind(&AsyncDummyStorageTest::ack4,
                                          this,
                                          std::placeholders::_1,
                                          std::placeholders::_2));
    expectAck4();
    storedCallback();

    expectPostCallback();
    dummyStorage->removeAllAsync(ns, std::bind(&AsyncDummyStorageTest::ack1,
                                               this,
                                               std::placeholders::_1));
    expectAck1();
    storedCallback();
}
