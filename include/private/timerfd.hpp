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

#ifndef SHAREDDATALAYER_TIMERFD_HPP_
#define SHAREDDATALAYER_TIMERFD_HPP_

#include <sys/timerfd.h>
#include "private/timer.hpp"
#include "private/filedescriptor.hpp"

namespace shareddatalayer
{
    class Engine;
    class System;

    class TimerFD
    {
    public:
        explicit TimerFD(Engine& engine);

        TimerFD(System& system, Engine& engine);

        ~TimerFD();

        void arm(Timer& timer, const Timer::Duration& duration, const Timer::Callback& cb);

        void disarm(const Timer& timer);

        TimerFD(TimerFD&&) = delete;
        TimerFD(const TimerFD&) = delete;
        TimerFD& operator = (TimerFD&&) = delete;
        TimerFD& operator = (const TimerFD&) = delete;

    private:
        using Queue = Timer::Queue;

        bool isFirst(Queue::iterator it) const;

        Timer::Duration toAbsolute(const Timer::Duration& duration);

        void handleEvents();

        bool timerExpired() const;

        void handleExpiredTimers();

        void popAndExecuteFirstTimer();

        Timer::Duration nextTrigger() const;

        void disarmTimerFD();

        void armTimerFD(const Timer::Duration& duration);

        void setTimeForTimerFD(time_t seconds, long int nanoseconds);

        System& system;
        FileDescriptor fd;
        Queue queue;
    };
}

#endif
