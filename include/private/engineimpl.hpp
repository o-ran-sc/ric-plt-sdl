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

#ifndef SHAREDDATALAYER_ENGINEIMPL_HPP_
#define SHAREDDATALAYER_ENGINEIMPL_HPP_

#include <map>
#include <memory>
#include <vector>
#include <sys/epoll.h>
#include "private/engine.hpp"
#include "private/filedescriptor.hpp"

namespace shareddatalayer
{
    class TimerFD;
    class EventFD;
    class System;

    class EngineImpl: public Engine
    {
    public:
        EngineImpl();

        explicit EngineImpl(System& system);

        ~EngineImpl();

        int fd() const override { return epollFD; }

        void handleEvents() override;

        void addMonitoredFD(int fd, unsigned int events, const EventHandler& eh) override;

        void addMonitoredFD(FileDescriptor& fd, unsigned int events, const EventHandler& eh) override;

        void modifyMonitoredFD(int fd, unsigned int events) override;

        void deleteMonitoredFD(int fd) override;

        void postCallback(const Callback& callback) override;

        void run() override;

        void stop() override;

    protected:
        void armTimer(Timer& timer, const Timer::Duration& duration, const Timer::Callback& cb) override;

        void disarmTimer(const Timer& timer) override;

    private:
        TimerFD& getTimerFD();

        EventFD& getEventFD();

        void epollWait(int timeout);

        void callHandler(const epoll_event& e);

        System& system;
        bool stopped;
        FileDescriptor epollFD;
        std::map<int, EventHandler> handlers;
        std::vector<epoll_event> ebuffer;
        std::unique_ptr<TimerFD> timerFD;
        std::unique_ptr<EventFD> eventFD;
    };
}

#endif
