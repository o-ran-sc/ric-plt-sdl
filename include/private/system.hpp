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

#ifndef SHAREDDATALAYER_SYSTEM_HPP_
#define SHAREDDATALAYER_SYSTEM_HPP_

#include <unistd.h>
#include <sys/poll.h>
#include <chrono>

extern "C"
{
    struct epoll_event;
    struct itimerspec;
}

namespace shareddatalayer
{
    class System
    {
    public:
        System() = default;

        System(const System&) = delete;

        System& operator = (const System&) = delete;

        virtual ~System() = default;

        virtual int poll(struct pollfd *fds, nfds_t nfds, int timeout);

        virtual int epoll_create1(int flags);

        virtual void epoll_ctl(int epfd, int op, int fd, epoll_event* event);

        virtual int epoll_wait(int epfd, epoll_event* events, int maxevents, int timeout);

        virtual int timerfd_create(int clockid, int flags);

        virtual void timerfd_settime(int fd, int flags, const itimerspec* new_value, itimerspec* old_value);

        virtual ssize_t read(int fd, void* buf, size_t count);

        virtual int eventfd(unsigned int initval, int flags);

        virtual ssize_t write(int fd, const void* buf, size_t count);

        virtual std::chrono::steady_clock::duration time_since_epoch();

        virtual void close(int fd);

        virtual const char* getenv(const char* name);

        static System& getSystem() noexcept;
    };
}

#endif
