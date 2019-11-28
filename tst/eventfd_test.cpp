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
#include <cstdint>
#include <sys/eventfd.h>
#include <gmock/gmock.h>
#include "private/eventfd.hpp"
#include "private/timerfd.hpp"
#include "private/tst/systemmock.hpp"
#include "private/tst/enginemock.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class EventFDTest: public testing::Test
    {
    public:
        const int efd;
        NiceMock<SystemMock> systemMock;
        EngineMock engineMock;
        std::unique_ptr<EventFD> eventFD;
        Engine::EventHandler savedEventHandler;

        EventFDTest(): efd(123)
        {
            InSequence dummy;
            EXPECT_CALL(systemMock, eventfd(0U, EFD_CLOEXEC | EFD_NONBLOCK))
                .Times(1)
                .WillOnce(Return(efd));
            EXPECT_CALL(engineMock, addMonitoredFD(Matcher<FileDescriptor&>(_), Engine::EVENT_IN, _))
                .Times(1)
                .WillOnce(Invoke([this] (FileDescriptor& fd, unsigned int, const Engine::EventHandler& eh)
                                 {
                                     EXPECT_EQ(efd, static_cast<int>(fd));
                                     savedEventHandler = eh;
                                 }));
            eventFD.reset(new EventFD(systemMock, engineMock));
            Mock::VerifyAndClear(&systemMock);
            Mock::VerifyAndClear(&engineMock);
        }

        void expectWrite()
        {
            EXPECT_CALL(systemMock, write(efd, NotNull(), sizeof(uint64_t)))
                .Times(1)
                .WillOnce(Invoke([] (int, const void* buf, size_t) -> ssize_t
                                 {
                                     EXPECT_EQ(1U, *static_cast<const uint64_t*>(buf));
                                     return sizeof(uint64_t);
                                 }));
        }

        void expectRead()
        {
            EXPECT_CALL(systemMock, read(efd, NotNull(), sizeof(uint64_t)))
                .Times(1)
                .WillOnce(Return(sizeof(uint64_t)));
        }

        void post(const EventFD::Callback& callback)
        {
            eventFD->post(callback);
        }

        void post(int i)
        {
            post(std::bind(&EventFDTest::callback, this, i));
        }

        MOCK_METHOD1(callback, void(int i));
    };
}

TEST_F(EventFDTest, IsNotCopyableAndIsNotMovable)
{
    EXPECT_FALSE(std::is_copy_assignable<EventFD>::value);
    EXPECT_FALSE(std::is_move_assignable<EventFD>::value);
    EXPECT_FALSE(std::is_copy_constructible<EventFD>::value);
    EXPECT_FALSE(std::is_move_constructible<EventFD>::value);
}

TEST_F(EventFDTest, PostWritesToEventFD)
{
    expectWrite();
    post(1);
}

TEST_F(EventFDTest, HandleEventsExecutesAllCallbacksInFIFOOrder)
{
    post(1);
    post(2);
    InSequence dummy;
    expectRead();
    EXPECT_CALL(*this, callback(1))
        .Times(1);
    EXPECT_CALL(*this, callback(2))
        .Times(1);
    savedEventHandler(Engine::EVENT_IN);
}

TEST_F(EventFDTest, CallbacksAddedInPostAreNotExecutedDuringTheSameHandleEvents)
{
    post([this] () { post(1); });
    InSequence dummy;
    expectRead();
    EXPECT_CALL(*this, callback(_))
        .Times(0);
    savedEventHandler(Engine::EVENT_IN);
}

TEST_F(EventFDTest, ExecutedCallbackIsDestroyedBeforeExecutingTheNextCallback)
{
    std::shared_ptr<int> data(std::make_shared<int>(1));
    std::weak_ptr<int> weak(data);
    post([data] () { static_cast<void>(data); });
    data.reset();
    post([weak] () { EXPECT_EQ(nullptr, weak.lock()); });
    savedEventHandler(Engine::EVENT_IN);
}

TEST_F(EventFDTest, PostingNullCallbackCallsSHAREDDATALAYER_ABORT)
{
    EXPECT_EXIT(post(EventFD::Callback()),
         KilledBySignal(SIGABRT), "ABORT.*eventfd\\.cpp");
}
