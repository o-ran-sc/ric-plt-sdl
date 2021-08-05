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

#include "private/redis/hiredisepolladapter.hpp"
#include <sys/epoll.h>
#include "private/engine.hpp"
#include "private/redis/hiredissystem.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;


namespace
{
    void addReadWrap(void* data)
    {
        auto instance(static_cast<HiredisEpollAdapter*>(data));
        instance->addRead();
    }

    void delReadWrap(void* data)
    {
        auto instance(static_cast<HiredisEpollAdapter*>(data));
        instance->delRead();
    }

    void addWriteWrap(void* data)
    {
        auto instance(static_cast<HiredisEpollAdapter*>(data));
        instance->addWrite();
    }

    void delWriteWrap(void* data)
    {
        auto instance(static_cast<HiredisEpollAdapter*>(data));
        instance->delWrite();
    }

    void cleanUpWrap(void* data)
    {
        auto instance(static_cast<HiredisEpollAdapter*>(data));
        instance->cleanUp();
    }
}

HiredisEpollAdapter::HiredisEpollAdapter(Engine& engine):
    HiredisEpollAdapter(engine, HiredisSystem::getHiredisSystem())
{
}

HiredisEpollAdapter::HiredisEpollAdapter(Engine& engine, HiredisSystem& hiredisSystem):
    hiredisSystem(hiredisSystem),
    engine(engine),
    ac(nullptr),
    eventState(0),
    reading(false),
    writing(false),
    isMonitoring(false)
{
}

HiredisEpollAdapter::~HiredisEpollAdapter()
{
    if (ac && isMonitoring)
        cleanUp();
}

void HiredisEpollAdapter::attach(redisAsyncContext* ac)
{
    eventState = 0;
    reading = false;
    writing = false;
    this->ac = ac;
    this->ac->ev.data = this;
    this->ac->ev.addRead = addReadWrap;
    this->ac->ev.delRead = delReadWrap;
    this->ac->ev.addWrite = addWriteWrap;
    this->ac->ev.delWrite = delWriteWrap;
    this->ac->ev.cleanup = cleanUpWrap;
    engine.addMonitoredFD(ac->c.fd,
                          eventState,
                          std::bind(&HiredisEpollAdapter::eventHandler,
                                    this,
                                    std::placeholders::_1));
    isMonitoring = true;
}

void HiredisEpollAdapter::eventHandler(unsigned int events)
{
    if (events & Engine::EVENT_IN)
        if (reading)
            hiredisSystem.redisAsyncHandleRead(ac);
    if (events & Engine::EVENT_OUT)
        if (writing)
            hiredisSystem.redisAsyncHandleWrite(ac);
}

void HiredisEpollAdapter::addRead()
{
    if (reading)
        return;
    reading = true;
    eventState |= Engine::EVENT_IN;
    engine.modifyMonitoredFD(ac->c.fd, eventState);
}

void HiredisEpollAdapter::delRead()
{
    reading = false;
    eventState &= ~Engine::EVENT_IN;
    engine.modifyMonitoredFD(ac->c.fd, eventState);
}

void HiredisEpollAdapter::addWrite()
{
    if (writing)
        return;
    writing = true;
    eventState |= Engine::EVENT_OUT;
    engine.modifyMonitoredFD(ac->c.fd, eventState);
}

void HiredisEpollAdapter::delWrite()
{
    writing = false;
    eventState &= ~Engine::EVENT_OUT;
    engine.modifyMonitoredFD(ac->c.fd, eventState);
}

void HiredisEpollAdapter::cleanUp()
{
    reading = false;
    writing = false;
    eventState = 0;
    engine.deleteMonitoredFD(ac->c.fd);
    isMonitoring = false;
}
