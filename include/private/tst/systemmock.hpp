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

#ifndef SHAREDDATALAYER_TST_SYSTEMMOCK_HPP_
#define SHAREDDATALAYER_TST_SYSTEMMOCK_HPP_

#include <gmock/gmock.h>
#include "private/system.hpp"

namespace shareddatalayer
{
    namespace tst
    {
        class SystemMock: public System
        {
        public:
            MOCK_METHOD3(poll, int(struct pollfd *fds, nfds_t nfds, int timeout));

            MOCK_METHOD1(epoll_create1, int(int flags));

            MOCK_METHOD4(epoll_ctl, void(int epfd, int op, int fd, epoll_event* event));

            MOCK_METHOD4(epoll_wait, int(int epfd, epoll_event* events, int maxevents, int timeout));

            MOCK_METHOD2(timerfd_create, int(int clockid, int flags));

            MOCK_METHOD4(timerfd_settime, void(int fd, int flags, const itimerspec* new_value, itimerspec* old_value));

            MOCK_METHOD3(read, ssize_t(int fd, void* buf, size_t count));

            MOCK_METHOD2(eventfd, int(unsigned int initval, int flags));

            MOCK_METHOD3(write, ssize_t(int fd, const void* buf, size_t count));

            MOCK_METHOD0(time_since_epoch, std::chrono::steady_clock::duration());

            MOCK_METHOD1(close, void(int fd));

            MOCK_METHOD1(getenv, const char*(const char*));
        };
    }
}

#endif
