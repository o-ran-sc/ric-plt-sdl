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
#include <boost/optional.hpp>
#include <sys/timerfd.h>
#include <gmock/gmock.h>
#include "private/timerfd.hpp"
#include "private/tst/systemmock.hpp"
#include "private/tst/enginemock.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class TimerFDTest: public testing::Test
    {
    public:
        const int tfd;
        NiceMock<SystemMock> systemMock;
        EngineMock engineMock;
        std::unique_ptr<TimerFD> timerFD;
        std::unique_ptr<Timer> timer1;
        std::unique_ptr<Timer> timer2;
        Engine::EventHandler savedEventHandler;

        TimerFDTest(): tfd(123)
        {
            InSequence dummy;
            EXPECT_CALL(systemMock, timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC))
                .Times(1)
                .WillOnce(Return(tfd));
            EXPECT_CALL(engineMock, addMonitoredFD(Matcher<FileDescriptor&>(_), Engine::EVENT_IN, _))
                .Times(1)
                .WillOnce(Invoke([this] (FileDescriptor& fd, unsigned int, const Engine::EventHandler& eh)
                                 {
                                     EXPECT_EQ(tfd, static_cast<int>(fd));
                                     savedEventHandler = eh;
                                 }));
            timerFD.reset(new TimerFD(systemMock, engineMock));

            Mock::VerifyAndClear(&systemMock);
            Mock::VerifyAndClear(&engineMock);

            EXPECT_CALL(engineMock, armTimer(_, _, _))
                 .Times(AnyNumber())
                 .WillRepeatedly(Invoke([this] (Timer& timer, const Timer::Duration& duration, const Timer::Callback& cb)
                      {
                          timerFD.get()->arm(timer, duration, cb);
                      }));

            EXPECT_CALL(engineMock, disarmTimer(_))
                 .Times(AnyNumber())
                 .WillRepeatedly(Invoke([this](const Timer& timer)
                     {
                         timerFD.get()->disarm(timer);
                     }));

            ON_CALL(systemMock, time_since_epoch())
                .WillByDefault(Return(std::chrono::steady_clock::duration(0)));

            timer1.reset(new Timer(engineMock));
            timer2.reset(new Timer(engineMock));
        }

        void expectSetTime(int seconds, int nanoseconds)
        {
            EXPECT_CALL(systemMock, timerfd_settime(tfd, TFD_TIMER_ABSTIME, NotNull(), nullptr))
                .Times(1)
                .WillOnce(Invoke([seconds, nanoseconds] (int, int, const itimerspec* new_value, itimerspec*)
                                 {
                                     EXPECT_EQ(0, new_value->it_interval.tv_sec);
                                     EXPECT_EQ(0, new_value->it_interval.tv_nsec);
                                     EXPECT_EQ(seconds, new_value->it_value.tv_sec);
                                     EXPECT_EQ(nanoseconds, new_value->it_value.tv_nsec);
                                 }));
        }

        void expectRead(const boost::optional<uint64_t>& count)
        {
            EXPECT_CALL(systemMock, read(tfd, NotNull(), sizeof(uint64_t)))
                .Times(1)
                .WillOnce(Invoke([count] (int, void* buf, size_t) -> ssize_t
                                 {
                                     if (count)
                                     {
                                         *static_cast<uint64_t *>(buf) = *count;
                                         return sizeof(uint64_t);
                                     }
                                     return -1;
                                 }));
        }

        void expectTimeSinceEpoch(const std::chrono::steady_clock::duration& ret)
        {
            EXPECT_CALL(systemMock, time_since_epoch())
                .Times(1)
                .WillOnce(Return(ret));
        }

        void arm(Timer& timer, int seconds, int nanoseconds, const std::string& param = std::string())
        {
            timer.arm(std::chrono::duration_cast<Timer::Duration>(std::chrono::seconds(seconds) + std::chrono::nanoseconds(nanoseconds)),
                      std::bind(&TimerFDTest::callback, this, param));
        }

        void arm(int seconds, int nanoseconds, const std::string& param = std::string())
        {
            arm(*timer1, seconds, nanoseconds, param);
        }

        void disarm(Timer& timer)
        {
            timer.disarm();
        }

        void disarm()
        {
            disarm(*timer1);
        }

        MOCK_METHOD1(callback, void(const std::string& param));
    };
}

TEST_F(TimerFDTest, IsNotCopyableAndIsNotMovable)
{
    EXPECT_FALSE(std::is_copy_assignable<TimerFD>::value);
    EXPECT_FALSE(std::is_move_assignable<TimerFD>::value);
    EXPECT_FALSE(std::is_copy_constructible<TimerFD>::value);
    EXPECT_FALSE(std::is_move_constructible<TimerFD>::value);
}

TEST_F(TimerFDTest, ArmingTheFirstTimerCallsSetTimeWithProperValues)
{
    expectSetTime(3, 4000);
    arm(3, 4000);
    Mock::VerifyAndClear(&systemMock);
}

TEST_F(TimerFDTest, ArmingAnotherTimerWithLongerTimeoutDoesntCallSetTime)
{
    expectSetTime(3, 0);
    arm(*timer1, 3, 0);
    arm(*timer2, 10, 0);
    Mock::VerifyAndClear(&systemMock);
}

TEST_F(TimerFDTest, DisarminTheOnlyTimerCallsSetTimeWithZeroValues)
{
    arm(3, 0);
    expectSetTime(0, 0);
    disarm();
}

TEST_F(TimerFDTest, DisarminTheFirstTimerCallsSetTimeWithProperValues)
{
    arm(*timer1, 3, 0);
    arm(*timer2, 4, 0);
    expectSetTime(4, 0);
    disarm(*timer1);
    Mock::VerifyAndClear(&systemMock);
}

TEST_F(TimerFDTest, AfterExecutingTheFirstTimerSetTimeIsCalledWithProperValues)
{
    InSequence dummy;
    arm(*timer1, 1, 0, "first");
    arm(*timer2, 2, 0, "second");
    expectRead(1);
    EXPECT_CALL(*this, callback("first"))
        .Times(1);
    expectSetTime(2, 0);
    savedEventHandler(Engine::EVENT_IN);
    Mock::VerifyAndClear(&systemMock);
}

TEST_F(TimerFDTest, AfterExecutingTheLastTimerSetTimeIsCalledWithZeroValues)
{
    InSequence dummy;
    arm(*timer1, 1, 0, "first");
    expectRead(1);
    EXPECT_CALL(*this, callback("first"))
        .Times(1);
    expectSetTime(0, 0);
    savedEventHandler(Engine::EVENT_IN);
    Mock::VerifyAndClear(&systemMock);
}

TEST_F(TimerFDTest, IfReadReturnsNegativeOnHandleEventsNothingIsDone)
{
    arm(10, 0);
    expectRead(boost::none);
    EXPECT_CALL(*this, callback(_))
        .Times(0);
    EXPECT_CALL(systemMock, timerfd_settime(_, _, _, _))
        .Times(0);
    savedEventHandler(Engine::EVENT_IN);
    Mock::VerifyAndClear(&systemMock);
}

TEST_F(TimerFDTest, IfReadReturnsNoEventsOnHandleEventsNothingIsDone)
{
    arm(10, 0);
    expectRead(static_cast<uint64_t>(0U));
    EXPECT_CALL(*this, callback(_))
        .Times(0);
    EXPECT_CALL(systemMock, timerfd_settime(_, _, _, _))
        .Times(0);
    savedEventHandler(Engine::EVENT_IN);
    Mock::VerifyAndClear(&systemMock);
}

TEST_F(TimerFDTest, AllTimersThatHaveExpiredDuringTheEventLoopAreExecutedWithTheSameTimerFdExpiration)
{
    InSequence dummy;

    /* The first timer is armed to expire after 10 seconds */
    expectTimeSinceEpoch(std::chrono::seconds(0));
    arm(*timer1, 10, 0, "first");

    /* Time has passed 2 seconds, the second timer is armed to expire after 8 seconds */
    expectTimeSinceEpoch(std::chrono::seconds(2));
    arm(*timer2, 8, 0, "second");

    /* Time has passed 8 more seconds, both timers expire at once */
    expectRead(1);
    expectTimeSinceEpoch(std::chrono::seconds(10));
    EXPECT_CALL(*this, callback("first"))
        .Times(1);
    EXPECT_CALL(*this, callback("second"))
        .Times(1);
    savedEventHandler(Engine::EVENT_IN);
}
