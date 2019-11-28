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

#ifndef SHAREDDATALAYER_TIMER_HPP_
#define SHAREDDATALAYER_TIMER_HPP_

#include <map>
#include <functional>
#include <chrono>

namespace shareddatalayer
{
    class Engine;
    class TimerFD;

    /**
     * @brief One shot timer
     *
     * Timer is a one shot timer which expires once after being armed.
     */
    class Timer
    {
    public:
        using Duration = std::chrono::steady_clock::duration;

        using Callback = std::function<void()>;

        /**
         * Create a new timer. Each timer is associated with an Engine instance
         * and the associated Engine instance must exist as long as the timer
         * exists.
         *
         * @param engine The associated Engine instance.
         */
        explicit Timer(Engine& engine);

        ~Timer();

        /**
         * Arm this timer. If already armed, then first disarm and then arm.
         *
         * @param duration Duration until this timer expires starting from
         *                 <i>now</i>.
         * @param cb       Callback function to be called when this timer
         *                 expires. The callback will not be called if this
         *                 timer is disarmed or deleted before expiration.
         *
         * @see disarm
         */
        void arm(const Duration& duration, const Callback& cb);

        /**
         * Disarm this timer if armed. If not armed, then nothing is done.
         *
         * @see arm
         */
        void disarm();

        /**
         * Check if this timer is armed.
         *
         * @return <code>true</code>, if this timer is armed, otherwise
         *         <code>false</code>.
         */
        bool isArmed() const { return armed; }

        Timer(Timer&&) = delete;
        Timer(const Timer&) = delete;
        Timer& operator = (Timer&&) = delete;
        Timer& operator = (const Timer&) = delete;

    private:
        friend class TimerFD;

        using Queue = std::multimap<Timer::Duration, std::pair<const Timer*, Timer::Callback>>;

        Engine& engine;
        bool armed;
        /* The iterator is valid only while this timer is armed */
        Queue::iterator iterator;
    };
}

#endif
