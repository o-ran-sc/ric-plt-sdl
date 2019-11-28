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

#include <type_traits>
#include <memory>
#include <sys/epoll.h>
#include <async.h>
#include <gtest/gtest.h>
#include "private/redis/hiredisepolladapter.hpp"
#include "private/tst/enginemock.hpp"
#include "private/tst/hiredissystemmock.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class HiredisEpollAdapterTest: public testing::Test
    {
    public:
        StrictMock<EngineMock> engineMock;
        StrictMock<HiredisSystemMock> hiredisSystemMock;
        redisAsyncContext ac;
        int acFd;
        std::unique_ptr<HiredisEpollAdapter> adapter;
        Engine::EventHandler savedEventHandler;

        HiredisEpollAdapterTest():
            ac { },
            acFd(10)
        {
            ac.c.fd = acFd;
            adapter.reset(new HiredisEpollAdapter(engineMock, hiredisSystemMock));
        }

        void expectAddMonitoredFD(int fd, unsigned int events)
        {
            EXPECT_CALL(engineMock, addMonitoredFD(fd,events,_))
                .Times(1)
                .WillOnce(SaveArg<2>(&savedEventHandler));
        }

        void expectModifyMonitoredFD(int fd, unsigned int events)
        {
            EXPECT_CALL(engineMock, modifyMonitoredFD(fd,events))
                .Times(1);
        }

        void expectDeleteMonitoredFD(int fd)
        {
            EXPECT_CALL(engineMock, deleteMonitoredFD(fd))
                .Times(1);
        }
    };

    class HiredisEpollAdapterAttachedTest: public HiredisEpollAdapterTest
    {
    public:
        HiredisEpollAdapterAttachedTest()
        {
            expectAddMonitoredFD(ac.c.fd, 0);
            adapter->attach(&ac);
        }

        ~HiredisEpollAdapterAttachedTest()
        {
            expectDeleteMonitoredFD(ac.c.fd);
        }
    };
}

TEST_F(HiredisEpollAdapterTest, IsNotCopyable)
{
    EXPECT_FALSE(std::is_copy_constructible<HiredisEpollAdapter>::value);
    EXPECT_FALSE(std::is_copy_assignable<HiredisEpollAdapter>::value);
}

TEST_F(HiredisEpollAdapterAttachedTest, CanHandleInputEvents)
{
    InSequence dummy;
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN);
    ac.ev.addRead(ac.ev.data);
    EXPECT_CALL(hiredisSystemMock, redisAsyncHandleRead(&ac))
        .Times(1);
    savedEventHandler(EngineMock::EVENT_IN);
}

TEST_F(HiredisEpollAdapterAttachedTest, DoesNotHandleInputEventIfNotReading)
{
    InSequence dummy;
    EXPECT_CALL(hiredisSystemMock, redisAsyncHandleRead(&ac))
        .Times(0);
    savedEventHandler(EngineMock::EVENT_IN);
}

TEST_F(HiredisEpollAdapterAttachedTest, CanHandleOutputEvents)
{
    InSequence dummy;
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_OUT);
    ac.ev.addWrite(ac.ev.data);
    EXPECT_CALL(hiredisSystemMock, redisAsyncHandleWrite(&ac))
        .Times(1);
    savedEventHandler(EngineMock::EVENT_OUT);
}

TEST_F(HiredisEpollAdapterAttachedTest, DoesNotHandleOutputEventIfNotWriting)
{
    InSequence dummy;
    EXPECT_CALL(hiredisSystemMock, redisAsyncHandleWrite(&ac))
        .Times(0);
    savedEventHandler(EngineMock::EVENT_OUT);
}

TEST_F(HiredisEpollAdapterAttachedTest, CanBeAttachedAndEventStateChangedIdempotently)
{
    InSequence dummy;
    EXPECT_EQ(adapter.get(), ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN);
    ac.ev.addRead(ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN | EngineMock::EVENT_OUT);
    ac.ev.addWrite(ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_OUT);
    ac.ev.delRead(ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd, 0);
    ac.ev.delWrite(ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd,EngineMock::EVENT_IN);
    ac.ev.addRead(ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN | EngineMock::EVENT_OUT);
    ac.ev.addWrite(ac.ev.data);
    expectDeleteMonitoredFD(ac.c.fd);
    ac.ev.cleanup(ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_OUT);
    ac.ev.addWrite(ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN | EngineMock::EVENT_OUT);
    ac.ev.addRead(ac.ev.data);
    expectAddMonitoredFD(ac.c.fd, 0);
    adapter->attach(&ac);
}

TEST_F(HiredisEpollAdapterAttachedTest, FurtherAttachementsResetEventState)
{
    InSequence dummy;
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN);
    ac.ev.addRead(ac.ev.data);
    expectAddMonitoredFD(ac.c.fd, 0);
    adapter->attach(&ac);
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_OUT);
    ac.ev.addWrite(ac.ev.data);
}

TEST_F(HiredisEpollAdapterAttachedTest, InputEventIsSetOnce)
{
    InSequence dummy;
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN);
    ac.ev.addRead(ac.ev.data);
    ac.ev.addRead(ac.ev.data);
}

TEST_F(HiredisEpollAdapterAttachedTest, OutputEventIsSetOnce)
{
    InSequence dummy;
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_OUT);
    ac.ev.addWrite(ac.ev.data);
    ac.ev.addWrite(ac.ev.data);
}

TEST_F(HiredisEpollAdapterAttachedTest, MakesCleanupIfDestructedWhileAttahced)
{
    InSequence dummy;
    expectDeleteMonitoredFD(ac.c.fd);
    adapter.reset(new HiredisEpollAdapter(engineMock, hiredisSystemMock));
    expectAddMonitoredFD(ac.c.fd, 0);
    adapter->attach(&ac);
}

TEST_F(HiredisEpollAdapterTest, DoesNotMakeCleanupIfDestructedWhileNotAttahced)
{
    InSequence dummy;
    adapter.reset(new HiredisEpollAdapter(engineMock, hiredisSystemMock));
}
