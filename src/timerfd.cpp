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

#include "private/timerfd.hpp"
#include "private/engine.hpp"
#include "private/system.hpp"

using namespace shareddatalayer;

TimerFD::TimerFD(Engine& engine):
    TimerFD(System::getSystem(), engine)
{
}

TimerFD::TimerFD(System& system, Engine& engine):
    system(system),
    fd(system, system.timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC))
{
    engine.addMonitoredFD(fd, Engine::EVENT_IN, std::bind(&TimerFD::handleEvents, this));
}

TimerFD::~TimerFD()
{
}

void TimerFD::arm(Timer& timer, const Timer::Duration& duration, const Timer::Callback& cb)
{
    const auto absolute(toAbsolute(duration));
    const auto i(queue.insert(std::make_pair(absolute, std::make_pair(&timer, cb))));
    timer.iterator = i;
    if (isFirst(i))
        armTimerFD(absolute);
}

bool TimerFD::isFirst(Queue::iterator it) const
{
    return queue.begin() == it;
}

void TimerFD::disarm(const Timer& timer)
{
    const bool wasFirst(isFirst(timer.iterator));
    queue.erase(timer.iterator);

    if (queue.empty())
        disarmTimerFD();
    else if (wasFirst)
        armTimerFD(nextTrigger());
}

Timer::Duration TimerFD::toAbsolute(const Timer::Duration& duration)
{
    return std::chrono::duration_cast<Timer::Duration>(system.time_since_epoch()) + duration;
}

void TimerFD::handleEvents()
{
    if (timerExpired())
        handleExpiredTimers();
}

bool TimerFD::timerExpired() const
{
    uint64_t count;
    return (system.read(fd, &count, sizeof(count)) == sizeof(count)) && (count > 0U);
}

void TimerFD::handleExpiredTimers()
{
    const auto now(system.time_since_epoch());
    do
    {
        popAndExecuteFirstTimer();
        if (queue.empty())
        {
            disarmTimerFD();
            return;
        }
    }
    while (queue.begin()->first <= now);
    armTimerFD(nextTrigger());
}

void TimerFD::popAndExecuteFirstTimer()
{
    const auto i(queue.begin());
    const auto cb(i->second.second);
    queue.erase(i);
    cb();
}

Timer::Duration TimerFD::nextTrigger() const
{
    return queue.begin()->first;
}

void TimerFD::disarmTimerFD()
{
    setTimeForTimerFD(0, 0);
}

void TimerFD::armTimerFD(const Timer::Duration& duration)
{
    static const long int NANOSECONDS_IN_ONE_SECOND(1E9);
    setTimeForTimerFD(std::chrono::duration_cast<std::chrono::seconds>(duration).count(),
                      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() % NANOSECONDS_IN_ONE_SECOND);
}

void TimerFD::setTimeForTimerFD(time_t seconds, long int nanoseconds)
{
    const itimerspec ts{ { 0, 0 }, { seconds, nanoseconds } };
    system.timerfd_settime(fd, TFD_TIMER_ABSTIME, &ts, nullptr);
}
