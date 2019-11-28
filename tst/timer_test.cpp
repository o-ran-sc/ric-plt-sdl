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
#include <gmock/gmock.h>
#include "private/timer.hpp"
#include "private/tst/enginemock.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class TimerTest: public testing::Test
    {
    public:
        NiceMock<EngineMock> engineMock;
        const Timer::Duration duration;
        std::unique_ptr<Timer> timer;

        TimerTest():
            duration(std::chrono::duration_cast<Timer::Duration>(std::chrono::seconds(12))),
            timer(new Timer(engineMock))
        {
        }
    };

    void nop() { }
}

TEST_F(TimerTest, IsNotCopyableAndIsNotMovable)
{
    EXPECT_FALSE(std::is_copy_assignable<Timer>::value);
    EXPECT_FALSE(std::is_move_assignable<Timer>::value);
    EXPECT_FALSE(std::is_copy_constructible<Timer>::value);
    EXPECT_FALSE(std::is_move_constructible<Timer>::value);
}

TEST_F(TimerTest, ArmCallsArmOfAssociatedTimerService)
{
    EXPECT_CALL(engineMock, armTimer(_, duration, _))
        .Times(1);
    timer->arm(duration, nop);
}

TEST_F(TimerTest, DisarmingCallsDisarmOfAssociatedTimerService)
{
    timer->arm(duration, nop);
    EXPECT_CALL(engineMock, disarmTimer(_))
        .Times(1);
    timer->disarm();
}

TEST_F(TimerTest, DisarmingUnArmedTimerDoesNothing)
{
    EXPECT_CALL(engineMock, disarmTimer(_))
        .Times(0);
    timer->disarm();
}

TEST_F(TimerTest, DoubleDisarmingDoesNothing)
{
    timer->arm(duration, nop);
    EXPECT_CALL(engineMock, disarmTimer(_))
        .Times(1);
    timer->disarm();
    timer->disarm();
}

TEST_F(TimerTest, DoubleArmingDisarmsFirst)
{
    InSequence dummy;
    EXPECT_CALL(engineMock, armTimer(_, duration, _))
        .Times(1);
    timer->arm(duration, nop);
    EXPECT_CALL(engineMock, disarmTimer(_))
        .Times(1);
    EXPECT_CALL(engineMock, armTimer(_, duration, _))
        .Times(1);
    timer->arm(duration, nop);
    Mock::VerifyAndClear(&engineMock);
}

TEST_F(TimerTest, DestructorDisarms)
{
    timer->arm(duration, nop);
    EXPECT_CALL(engineMock, disarmTimer(_))
        .Times(1);
    timer.reset();
}

TEST_F(TimerTest, AfterTimerHasExpiredItIsNotArmedAnymore)
{
    Timer::Callback savedCb;
    ON_CALL(engineMock, armTimer(_, _, _))
        .WillByDefault(SaveArg<2>(&savedCb));
    timer->arm(duration, nop);
    savedCb();
    EXPECT_FALSE(timer->isArmed());
}

TEST_F(TimerTest, ArmingNullCallbackCallsSHAREDDATALAYER_ABORT)
{
    EXPECT_EXIT(timer->arm(duration, Timer::Callback()),
        KilledBySignal(SIGABRT), "timer\\.cpp");
}
