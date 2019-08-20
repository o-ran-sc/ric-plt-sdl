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

#include "private/timer.hpp"
#include "private/engine.hpp"
#include "private/abort.hpp"

using namespace shareddatalayer;

Timer::Timer(Engine& engine):
    engine(engine),
    armed(false)
{
}

Timer::~Timer()
{
    disarm();
}

void Timer::arm(const Duration& duration, const Callback& cb)
{
    if (!cb)
        SHAREDDATALAYER_ABORT("Timer::arm: a null callback");

    disarm();
    engine.armTimer(*this, duration, [this, cb] () { armed = false; cb(); });
    armed = true;
}

void Timer::disarm()
{
    if (!armed)
        return;

    engine.disarmTimer(*this);
    armed = false;
}
