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

#include "private/redis/hiredisclusterepolladapter.hpp"
#include <sys/epoll.h>
#include "private/engine.hpp"
#include "private/redis/hiredisclustersystem.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;

namespace
{
    int attachFunction(redisAsyncContext* ac, void* data)
    {
        auto instance(static_cast<HiredisClusterEpollAdapter*>(data));
        instance->attach(ac);
        return REDIS_OK;
    }

    void addReadWrap(void* data)
    {
        auto instance(static_cast<HiredisClusterEpollAdapter::Node*>(data));
        instance->addRead();
    }

    void addWriteWrap(void* data)
    {
        auto instance(static_cast<HiredisClusterEpollAdapter::Node*>(data));
        instance->addWrite();
    }

    void delReadWrap(void* data)
    {
        auto instance(static_cast<HiredisClusterEpollAdapter::Node*>(data));
        instance->delRead();
    }

    void delWriteWrap(void* data)
    {
        auto instance(static_cast<HiredisClusterEpollAdapter::Node*>(data));
        instance->delWrite();
    }

    void cleanupWrap(void* data)
    {
        auto instance(static_cast<HiredisClusterEpollAdapter::Node*>(data));
        instance->cleanup();
    }
}

HiredisClusterEpollAdapter::HiredisClusterEpollAdapter(Engine& engine):
    HiredisClusterEpollAdapter(engine, HiredisClusterSystem::getInstance())
{
}

HiredisClusterEpollAdapter::HiredisClusterEpollAdapter(Engine& engine, HiredisClusterSystem& hiredisClusterSystem):
    engine(engine),
    hiredisClusterSystem(hiredisClusterSystem)
{
}

void HiredisClusterEpollAdapter::setup(redisClusterAsyncContext* acc)
{
    acc->adapter = this;
    acc->attach_fn = attachFunction;
}

void HiredisClusterEpollAdapter::attach(redisAsyncContext* ac)
{
    detach(ac);
    nodes.insert(std::make_pair(ac->c.fd,
                                std::unique_ptr<Node>(new Node(engine,
                                                               ac,
                                                               hiredisClusterSystem))));
}

void HiredisClusterEpollAdapter::detach(const redisAsyncContext* ac)
{
    auto it = nodes.find(ac->c.fd);
    if (it != nodes.end())
        nodes.erase(it);
}

HiredisClusterEpollAdapter::Node::Node(Engine& engine,
                                       redisAsyncContext* ac,
                                       HiredisClusterSystem& hiredisClusterSystem):
    hiredisClusterSystem(hiredisClusterSystem),
    engine(engine),
    ac(ac),
    eventState(0),
    reading(false),
    writing(false)
{
    this->ac->ev.data = this;
    this->ac->ev.addRead = addReadWrap;
    this->ac->ev.addWrite = addWriteWrap;
    this->ac->ev.delRead = delReadWrap;
    this->ac->ev.delWrite = delWriteWrap;
    this->ac->ev.cleanup = cleanupWrap;
    engine.addMonitoredFD(ac->c.fd,
                          eventState,
                          std::bind(&HiredisClusterEpollAdapter::Node::eventHandler,
                                    this,
                                    std::placeholders::_1));
    isMonitoring = true;
}

HiredisClusterEpollAdapter::Node::~Node()
{
    if (isMonitoring)
        cleanup();
}

void HiredisClusterEpollAdapter::Node::eventHandler(unsigned int events)
{
    if (events & Engine::EVENT_IN)
        if (reading && isMonitoring)
            hiredisClusterSystem.redisAsyncHandleRead(ac);
    if (events & Engine::EVENT_OUT)
        if (writing && isMonitoring)
            hiredisClusterSystem.redisAsyncHandleWrite(ac);
}

void HiredisClusterEpollAdapter::Node::addRead()
{
    if (reading)
        return;
    reading = true;
    eventState |= Engine::EVENT_IN;
    engine.modifyMonitoredFD(ac->c.fd, eventState);
}

void HiredisClusterEpollAdapter::Node::addWrite()
{
    if (writing)
        return;
    writing = true;
    eventState |= Engine::EVENT_OUT;
    engine.modifyMonitoredFD(ac->c.fd, eventState);
}

void HiredisClusterEpollAdapter::Node::delRead()
{
    reading = false;
    eventState &= ~Engine::EVENT_IN;
    engine.modifyMonitoredFD(ac->c.fd, eventState);
}

void HiredisClusterEpollAdapter::Node::delWrite()
{
    writing = false;
    eventState &= ~Engine::EVENT_OUT;
    engine.modifyMonitoredFD(ac->c.fd, eventState);
}

void HiredisClusterEpollAdapter::Node::cleanup()
{
    reading = false;
    writing = false;
    eventState = 0;
    engine.deleteMonitoredFD(ac->c.fd);
    isMonitoring = false;
}
