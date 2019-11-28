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

#include <memory>
#include <sys/epoll.h>
#include <gmock/gmock.h>
#include "private/filedescriptor.hpp"
#include "private/engineimpl.hpp"
#include "private/tst/systemmock.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class EventHandlerMock
    {
    public:
        MOCK_METHOD1(handleEvents, void(unsigned int events));
    };

    class EngineImplTest: public testing::Test
    {
    public:
        const int fd1;
        const int fd2;
        const int epfd;
        NiceMock<SystemMock> systemMock;
        std::shared_ptr<EngineImpl> services;
        EventHandlerMock eventHandlerMock1;
        EventHandlerMock eventHandlerMock2;

        EngineImplTest(): fd1(100), fd2(200), epfd(300)
        {
            EXPECT_CALL(systemMock, epoll_create1(EPOLL_CLOEXEC))
                .Times(1)
                .WillOnce(Return(epfd));
            services.reset(new EngineImpl(systemMock));
            Mock::VerifyAndClear(&systemMock);
        }

        ~EngineImplTest()
        {
            stopEngineImpl();
        }

        void addMonitoredFD(int fd, unsigned int events)
        {
            services->addMonitoredFD(fd, events, [] (unsigned int) { });
        }

        void addMonitoredFD(int fd, unsigned int events, EventHandlerMock& eventHandlerMock)
        {
            services->addMonitoredFD(fd, events, std::bind(&EventHandlerMock::handleEvents, &eventHandlerMock, std::placeholders::_1));
        }

        void modifyMonitoredFD(int fd, unsigned int events)
        {
            services->modifyMonitoredFD(fd, events);
        }

        void deleteMonitoredFD(int fd)
        {
            services->deleteMonitoredFD(fd);
        }

        void expectEpollCtl(int epfd, int op, int fd, unsigned int events)
        {
            EXPECT_CALL(systemMock, epoll_ctl(epfd, op, fd, NotNull()))
                .Times(1)
                .WillOnce(Invoke([fd, events] (int, int, int, epoll_event* event)
                                 {
                                     epoll_data data = { };
                                     data.fd = fd;
                                     EXPECT_EQ(data.fd, event->data.fd);
                                     EXPECT_EQ(data.u64, event->data.u64);
                                     EXPECT_EQ(data.ptr, event->data.ptr);
                                     EXPECT_EQ(events, event->events);
                                 }));
        }

        void expectAtLeastOneEpollCtl(int epfd, int op, int fd, unsigned int events)
        {
            EXPECT_CALL(systemMock, epoll_ctl(epfd, op, fd, NotNull()))
                .Times(AtLeast(1))
                .WillOnce(Invoke([fd, events] (int, int, int, epoll_event* event)
                                 {
                                     EXPECT_EQ(fd, event->data.fd);
                                     EXPECT_EQ(events, event->events);
                                 }));
        }

        void stopEngineImpl()
        {
            if (!services)
                return;

            EXPECT_CALL(systemMock, close(epfd)).Times(1);
            services.reset();
            Mock::VerifyAndClear(&systemMock);
        }
    };

    using EngineImplDeathTest = EngineImplTest;
}

TEST_F(EngineImplTest, HandleEventsWithoutAnyAddedFDsDoesNothing)
{
    EXPECT_CALL(systemMock, epoll_wait(_, _, _, _))
        .Times(0);
    services->handleEvents();
}

TEST_F(EngineImplTest, FDReturnsTheEpollFD)
{
    EXPECT_EQ(epfd, services->fd());
}

TEST_F(EngineImplTest, AddingFDAddsTheFDToEpoll)
{
    expectEpollCtl(epfd, EPOLL_CTL_ADD, fd1, Engine::EVENT_IN);
    addMonitoredFD(fd1, Engine::EVENT_IN);
}

TEST_F(EngineImplTest, AddingFileDescriptorSetsAtCloseCallback)
{
    FileDescriptor fd(systemMock, fd1);
    expectEpollCtl(epfd, EPOLL_CTL_ADD, fd1, Engine::EVENT_IN);
    services->addMonitoredFD(fd, Engine::EVENT_IN, Engine::EventHandler());

    InSequence dummy;
    EXPECT_CALL(systemMock, epoll_ctl(epfd, EPOLL_CTL_DEL, fd1, nullptr))
        .Times(1);
    EXPECT_CALL(systemMock, close(fd1)).Times(1);
}

TEST_F(EngineImplDeathTest, AddingAlreadyAddedFDCallsSHAREDDATALAYER_ABORT)
{
    expectAtLeastOneEpollCtl(epfd, EPOLL_CTL_ADD, fd1, Engine::EVENT_IN);
    addMonitoredFD(fd1, Engine::EVENT_IN);
    EXPECT_EXIT(addMonitoredFD(fd1, Engine::EVENT_IN),
        KilledBySignal(SIGABRT), "ABORT.*engineimpl\\.cpp");
}

TEST_F(EngineImplTest, ModifyingFDModifiesTheFDInEpoll)
{
    addMonitoredFD(fd1, Engine::EVENT_IN);
    expectEpollCtl(epfd, EPOLL_CTL_MOD, fd1, Engine::EVENT_OUT);
    modifyMonitoredFD(fd1, Engine::EVENT_OUT);
}

TEST_F(EngineImplDeathTest, ModifyingNonExistingFDCallsSHAREDDATALAYER_ABORT)
{
    EXPECT_EXIT(modifyMonitoredFD(fd1, 0U),
        KilledBySignal(SIGABRT), "ABORT.*engineimpl\\.cpp");
}

TEST_F(EngineImplDeathTest, DellingFDDelsTheFDFromEpollAndFromTheMap)
{
    addMonitoredFD(fd1, Engine::EVENT_IN);
    EXPECT_CALL(systemMock, epoll_ctl(epfd, EPOLL_CTL_DEL, fd1, nullptr))
        .Times(1);
    deleteMonitoredFD(fd1);
    EXPECT_EXIT(modifyMonitoredFD(fd1, 0U),
        KilledBySignal(SIGABRT), "ABORT.*engineimpl\\.cpp");
}

TEST_F(EngineImplDeathTest, DellingNonExistingFDCallsSHAREDDATALAYER_ABORT)
{
    EXPECT_EXIT(deleteMonitoredFD(fd1),
        KilledBySignal(SIGABRT), "ABORT.*engineimpl\\.cpp");
}

TEST_F(EngineImplTest, HandleEventsCallsAddedEventHandlersAccordingToEpollReturnValue)
{
    addMonitoredFD(fd1, Engine::EVENT_IN, eventHandlerMock1);
    addMonitoredFD(fd2, Engine::EVENT_IN, eventHandlerMock2);
    InSequence dummy;
    EXPECT_CALL(systemMock, epoll_wait(epfd, NotNull(), 2, 0))
        .Times(1)
        .WillOnce(Invoke([this] (int, epoll_event* events, int, int) -> int
                         {
                             events[0].events = EPOLLIN;
                             events[0].data.fd = fd1;
                             events[1].events = EPOLLOUT;
                             events[1].data.fd = fd2;
                             return 2;
                         }));
    EXPECT_CALL(eventHandlerMock1, handleEvents(Engine::EVENT_IN))
        .Times(1);
    EXPECT_CALL(eventHandlerMock2, handleEvents(Engine::EVENT_OUT))
        .Times(1);
    services->handleEvents();
}

TEST_F(EngineImplTest, PendingEventsOfDeletedFileDescriptorAreForgotten)
{
    addMonitoredFD(fd1, Engine::EVENT_IN, eventHandlerMock1);
    addMonitoredFD(fd2, Engine::EVENT_IN, eventHandlerMock2);
    InSequence dummy;
    EXPECT_CALL(eventHandlerMock2, handleEvents(_))
        .Times(0);
    EXPECT_CALL(systemMock, epoll_wait(epfd, NotNull(), 2, 0))
        .Times(1)
        .WillOnce(Invoke([this] (int, epoll_event* events, int, int) -> int
                         {
                             events[0].events = EPOLLIN;
                             events[0].data.fd = fd1;
                             events[1].events = EPOLLIN;
                             events[1].data.fd = fd2;
                             return 2;
                         }));
    EXPECT_CALL(eventHandlerMock1, handleEvents(_))
        .Times(1)
        .WillOnce(Invoke([this](unsigned int)
                         {
                             deleteMonitoredFD(fd2);
                             addMonitoredFD(fd2, Engine::EVENT_IN, eventHandlerMock2);
                         }));
    services->handleEvents();
}
