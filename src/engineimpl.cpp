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

#include "private/engineimpl.hpp"
#include "private/abort.hpp"
#include "private/timerfd.hpp"
#include "private/eventfd.hpp"
#include "private/system.hpp"

using namespace shareddatalayer;

EngineImpl::EngineImpl():
    EngineImpl(System::getSystem())
{
}

EngineImpl::EngineImpl(System& system):
    system(system),
    stopped(false),
    epollFD(system, system.epoll_create1(EPOLL_CLOEXEC))
{
}

EngineImpl::~EngineImpl()
{
}

void EngineImpl::handleEvents()
{
    if (!handlers.empty())
        epollWait(0);
}

void EngineImpl::epollWait(int timeout)
{
    ebuffer.resize(handlers.size());
    const int count(system.epoll_wait(epollFD, ebuffer.data(), static_cast<int>(ebuffer.size()), timeout));
    if (count <= 0)
        return;
    ebuffer.resize(count);
    for (const auto& i : ebuffer)
        if (i.data.fd != -1)
            callHandler(i);
}

void EngineImpl::callHandler(const epoll_event& e)
{
    handlers[e.data.fd](e.events);
}

void EngineImpl::addMonitoredFD(int fd, unsigned int events, const EventHandler& eh)
{
    if (handlers.find(fd) != handlers.end())
        SHAREDDATALAYER_ABORT("Monitored fd has already been added");

    epoll_event e = { };
    e.events = events;
    e.data.fd = fd;
    system.epoll_ctl(epollFD, EPOLL_CTL_ADD, fd, &e);
    handlers.insert(std::make_pair(fd, eh));
}

void EngineImpl::addMonitoredFD(FileDescriptor& fd, unsigned int events, const EventHandler& eh)
{
    int native(fd);
    addMonitoredFD(native, events, eh);

    fd.atClose([this] (int fd)
        {
             deleteMonitoredFD(fd);
        });
}

void EngineImpl::modifyMonitoredFD(int fd, unsigned int events)
{
    if (handlers.find(fd) == handlers.end())
        SHAREDDATALAYER_ABORT("Modified monitored fd does not exist");

    epoll_event e = { };
    e.events = events;
    e.data.fd = fd;
    system.epoll_ctl(epollFD, EPOLL_CTL_MOD, fd, &e);
}

void EngineImpl::deleteMonitoredFD(int fd)
{
    const auto i(handlers.find(fd));
    if (i == handlers.end())
        SHAREDDATALAYER_ABORT("Monitored (to be deleted) fd does not exist");

    handlers.erase(i);
    for (auto& i : ebuffer)
        if (i.data.fd == fd)
        {
            i.data.fd = -1;
            break;
        }
    system.epoll_ctl(epollFD, EPOLL_CTL_DEL, fd, nullptr);
}

void EngineImpl::armTimer(Timer& timer, const Timer::Duration& duration, const Timer::Callback& cb)
{
    getTimerFD().arm(timer, duration, cb);
}

void EngineImpl::disarmTimer(const Timer& timer)
{
    getTimerFD().disarm(timer);
}

void EngineImpl::postCallback(const Callback& callback)
{
    getEventFD().post(callback);
}

void EngineImpl::run()
{
    while (!stopped)
        epollWait(-1);
    stopped = false;
}

void EngineImpl::stop()
{
    postCallback([this] () { stopped = true; });
}

TimerFD& EngineImpl::getTimerFD()
{
    if (!timerFD)
        timerFD.reset(new TimerFD(system, *this));

    return *timerFD;
}

EventFD& EngineImpl::getEventFD()
{
    if (!eventFD)
        eventFD.reset(new EventFD(system, *this));

    return *eventFD;
}
