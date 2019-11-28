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
#include <gtest/gtest.h>
#include "private/redis/hiredisclusterepolladapter.hpp"
#include "private/tst/enginemock.hpp"
#include "private/tst/hiredisclustersystemmock.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class HiredisClusterEpollAdapterTest: public testing::Test
    {
    public:
        StrictMock<EngineMock> engineMock;
        StrictMock<HiredisClusterSystemMock> hiredisClusterSystemMock;
        redisClusterAsyncContext acc;
        std::unique_ptr<HiredisClusterEpollAdapter> adapter;
        redisAsyncContext ac;
        redisAsyncContext secondAc;
        int acFd;
        int secondAcFd;
        Engine::EventHandler savedEventHandler;
        Engine::EventHandler anotherSavedEventHandler;

        HiredisClusterEpollAdapterTest():
            acc { },
            adapter(new HiredisClusterEpollAdapter(engineMock, hiredisClusterSystemMock)),
            ac { },
            secondAc { },
            acFd(20),
            secondAcFd(30)
        {
            InSequence dummy;
            ac.c.fd = acFd;
            adapter->setup(&acc);
        }

        ~HiredisClusterEpollAdapterTest()
        {
        }

        void expectAddMonitoredFD(int fd, unsigned int events)
        {
            EXPECT_CALL(engineMock, addMonitoredFD(fd,events,_))
                .Times(1)
                .WillOnce(SaveArg<2>(&savedEventHandler));
        }

        void expectAddMonitoredFD2(int fd, unsigned int events)
        {
            EXPECT_CALL(engineMock, addMonitoredFD(fd,events,_))
                .Times(1)
                .WillOnce(SaveArg<2>(&anotherSavedEventHandler));
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

    class HiredisClusterEpollAdapterAttachedTest: public HiredisClusterEpollAdapterTest
    {
    public:
        HiredisClusterEpollAdapterAttachedTest()
        {
            expectAddMonitoredFD(ac.c.fd, 0);
            acc.attach_fn(&ac, adapter.get());
        }

        ~HiredisClusterEpollAdapterAttachedTest()
        {
            expectDeleteMonitoredFD(ac.c.fd);
        }
    };
}

TEST_F(HiredisClusterEpollAdapterTest, IsNotCopyableAndIsNotMovable)
{
    EXPECT_FALSE(std::is_copy_constructible<HiredisClusterEpollAdapter>::value);
    EXPECT_FALSE(std::is_copy_assignable<HiredisClusterEpollAdapter>::value);
    EXPECT_FALSE(std::is_move_constructible<HiredisClusterEpollAdapter>::value);
    EXPECT_FALSE(std::is_move_assignable<HiredisClusterEpollAdapter>::value);
}

TEST_F(HiredisClusterEpollAdapterTest, HasVirtualDestructor)
{
    EXPECT_TRUE(std::has_virtual_destructor<HiredisClusterEpollAdapter>::value);
}

TEST_F(HiredisClusterEpollAdapterAttachedTest, EventStateChangedIdempotently)
{
    InSequence dummy;
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN);
    ac.ev.addRead(ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN | EngineMock::EVENT_OUT);
    ac.ev.addWrite(ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_OUT);
    ac.ev.delRead(ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd, 0);
    ac.ev.delWrite(ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN);
    ac.ev.addRead(ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN | EngineMock::EVENT_OUT);
    ac.ev.addWrite(ac.ev.data);
}

TEST_F(HiredisClusterEpollAdapterAttachedTest, InputEventIsSetOnce)
{
    InSequence dummy;
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN);
    ac.ev.addRead(ac.ev.data);
    ac.ev.addRead(ac.ev.data);
}

TEST_F(HiredisClusterEpollAdapterAttachedTest, OutputEventIsSetOnce)
{
    InSequence dummy;
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_OUT);
    ac.ev.addWrite(ac.ev.data);
    ac.ev.addWrite(ac.ev.data);
}

TEST_F(HiredisClusterEpollAdapterAttachedTest, CanHandleInputEvent)
{
    InSequence dummy;
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN);
    ac.ev.addRead(ac.ev.data);
    EXPECT_CALL(hiredisClusterSystemMock, redisAsyncHandleRead(&ac))
        .Times(1);
    savedEventHandler(EngineMock::EVENT_IN);
}

TEST_F(HiredisClusterEpollAdapterAttachedTest, DoesNotHandleInputEventIfNotReading)
{
    InSequence dummy;
    EXPECT_CALL(hiredisClusterSystemMock, redisAsyncHandleRead(&ac))
        .Times(0);
    savedEventHandler(EngineMock::EVENT_IN);
}

TEST_F(HiredisClusterEpollAdapterAttachedTest, CanHandleOutputEvent)
{
    InSequence dummy;
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_OUT);
    ac.ev.addWrite(ac.ev.data);
    EXPECT_CALL(hiredisClusterSystemMock, redisAsyncHandleWrite(&ac))
        .Times(1);
    savedEventHandler(EngineMock::EVENT_OUT);
}

TEST_F(HiredisClusterEpollAdapterAttachedTest, DoesNotHandleOutputEventIfNotWriting)
{
    InSequence dummy;
    EXPECT_CALL(hiredisClusterSystemMock, redisAsyncHandleWrite(&ac))
        .Times(0);
    savedEventHandler(EngineMock::EVENT_OUT);
}

TEST_F(HiredisClusterEpollAdapterAttachedTest, FurtherAttachementsResetEventStateAndWritingAndReading)
{
    InSequence dummy;
    expectDeleteMonitoredFD(ac.c.fd);
    expectAddMonitoredFD(ac.c.fd, 0);
    acc.attach_fn(&ac, adapter.get());
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN);
    ac.ev.addRead(ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN | EngineMock::EVENT_OUT);
    ac.ev.addWrite(ac.ev.data);
    expectDeleteMonitoredFD(ac.c.fd);
    expectAddMonitoredFD(ac.c.fd, 0);
    acc.attach_fn(&ac, adapter.get());
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN);
    ac.ev.addRead(ac.ev.data);
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN | EngineMock::EVENT_OUT);
    ac.ev.addWrite(ac.ev.data);
}

TEST_F(HiredisClusterEpollAdapterAttachedTest, CanHandleTwoConnections)
{
    InSequence dummy;
    expectAddMonitoredFD2(secondAc.c.fd, 0);
    acc.attach_fn(&secondAc, adapter.get());
    // Read event in first fd:
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN);
    ac.ev.addRead(ac.ev.data);
    EXPECT_CALL(hiredisClusterSystemMock, redisAsyncHandleRead(&ac))
        .Times(1);
    savedEventHandler(EngineMock::EVENT_IN);

    // Read event in second fd:
    expectModifyMonitoredFD(secondAc.c.fd, EngineMock::EVENT_IN);
    ac.ev.addRead(secondAc.ev.data);
    EXPECT_CALL(hiredisClusterSystemMock, redisAsyncHandleRead(&secondAc))
        .Times(1);
    anotherSavedEventHandler(EngineMock::EVENT_IN);

    // Cleanup for extra ac
    expectDeleteMonitoredFD(secondAc.c.fd);
    adapter->detach(&secondAc);
}

TEST_F(HiredisClusterEpollAdapterAttachedTest, CleanupWillRemoveOngoingEventsAndDeregisterFdFromEpoll)
{
    InSequence dummy;
    expectDeleteMonitoredFD(acFd);
    ac.ev.cleanup(ac.ev.data);
    expectAddMonitoredFD(ac.c.fd, 0);
    acc.attach_fn(&ac, adapter.get());
}

TEST_F(HiredisClusterEpollAdapterAttachedTest, DetachMakesCleanupIfNotYetDone)
{
    InSequence dummy;
    expectDeleteMonitoredFD(ac.c.fd);
    adapter->detach(&ac);
    expectAddMonitoredFD(ac.c.fd, 0);
    acc.attach_fn(&ac, adapter.get());
}

TEST_F(HiredisClusterEpollAdapterAttachedTest, InputEventRemovedByCleanupIsNotHandled)
{
    InSequence dummy;
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_IN);
    ac.ev.addRead(ac.ev.data);
    expectDeleteMonitoredFD(ac.c.fd);
    ac.ev.cleanup(ac.ev.data);
    EXPECT_CALL(hiredisClusterSystemMock, redisAsyncHandleRead(&ac))
        .Times(0);
    savedEventHandler(EngineMock::EVENT_IN);
    expectAddMonitoredFD(ac.c.fd, 0);
    acc.attach_fn(&ac, adapter.get());
}

TEST_F(HiredisClusterEpollAdapterAttachedTest, OutputEventRemovedByCleanupIsNotHandled)
{
    InSequence dummy;
    expectModifyMonitoredFD(ac.c.fd, EngineMock::EVENT_OUT);
    ac.ev.addWrite(ac.ev.data);
    // Read is deleted first during cleanup handling so write (EPOLLOUT) still exists for the first EPOLL_CTL_MOD
    expectDeleteMonitoredFD(ac.c.fd);
    ac.ev.cleanup(ac.ev.data);
    EXPECT_CALL(hiredisClusterSystemMock, redisAsyncHandleWrite(&ac))
        .Times(0);
    savedEventHandler(EngineMock::EVENT_OUT);
    expectAddMonitoredFD(ac.c.fd, 0);
    acc.attach_fn(&ac, adapter.get());
}
