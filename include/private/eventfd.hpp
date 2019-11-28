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

#ifndef SHAREDDATALAYER_EVENTFD_HPP_
#define SHAREDDATALAYER_EVENTFD_HPP_

#include <functional>
#include <list>
#include <mutex>
#include "private/filedescriptor.hpp"

namespace shareddatalayer
{
    class FDMonitor;
    class Engine;
    class System;

    class EventFD
    {
    public:
        explicit EventFD(Engine& engine);

        EventFD(System& system, Engine& engine);

        ~EventFD();

        using Callback = std::function<void()>;

        void post(const Callback& callback);

        EventFD(EventFD&&) = delete;
        EventFD(const EventFD&) = delete;
        EventFD& operator = (EventFD&&) = delete;
        EventFD& operator = (const EventFD&) = delete;

    private:
        using Callbacks = std::list<Callback>;

        void atomicPushBack(const Callback& callback);

        Callbacks atomicPopAll();

        void handleEvents();

        void executeCallbacks();

        static void popAndExecuteFirstCallback(Callbacks& callbacks);

        System& system;
        FileDescriptor fd;
        Callbacks callbacks;
        std::mutex callbacksMutex;
    };
}

#endif
