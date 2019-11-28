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

#include "private/eventfd.hpp"
#include <sys/eventfd.h>
#include "private/abort.hpp"
#include "private/engine.hpp"
#include "private/system.hpp"

using namespace shareddatalayer;

namespace
{
    /*
     * Simple wrapper for executing the given callback without expecting
     * exceptions. If callback throws, we'll crash.
     */
    void execute(const EventFD::Callback& callback) noexcept
    {
        callback();
    }
}

EventFD::EventFD(Engine& engine):
    EventFD(System::getSystem(), engine)
{
}

EventFD::EventFD(System& system, Engine& engine):
    system(system),
    fd(system, system.eventfd(0U, EFD_CLOEXEC | EFD_NONBLOCK))
{
    engine.addMonitoredFD(fd, Engine::EVENT_IN, std::bind(&EventFD::handleEvents, this));
}

EventFD::~EventFD()
{
}

void EventFD::post(const Callback& callback)
{
    if (!callback)
        SHAREDDATALAYER_ABORT("A null callback was provided");

    atomicPushBack(callback);
    static const uint64_t value(1U);
    system.write(fd, &value, sizeof(value));
}

void EventFD::atomicPushBack(const Callback& callback)
{
    std::lock_guard<std::mutex> guard(callbacksMutex);
    callbacks.push_back(callback);
}

EventFD::Callbacks EventFD::atomicPopAll()
{
    std::lock_guard<std::mutex> guard(callbacksMutex);
    Callbacks extractedCallbacks;
    std::swap(callbacks, extractedCallbacks);
    return extractedCallbacks;
}

void EventFD::handleEvents()
{
    uint64_t value;
    system.read(fd, &value, sizeof(value));
    executeCallbacks();
}

void EventFD::executeCallbacks()
{
    Callbacks callbacks(atomicPopAll());
    while (!callbacks.empty())
        popAndExecuteFirstCallback(callbacks);
}

void EventFD::popAndExecuteFirstCallback(Callbacks& callbacks)
{
    const auto callback(callbacks.front());
    callbacks.pop_front();
    execute(callback);
}
