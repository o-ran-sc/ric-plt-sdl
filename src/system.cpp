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

#include "private/system.hpp"
#include <system_error>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/eventfd.h>
#include "private/abort.hpp"
#include "private/createlogger.hpp"

using namespace shareddatalayer;

int System::poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    const int ret(::poll(fds, nfds, timeout));
    if (ret == -1 && errno != EINTR)
        throw std::system_error(errno, std::system_category(), "poll");
    return ret;
}

int System::epoll_create1(int flags)
{
    const int ret(::epoll_create1(flags));
    if (ret == -1)
        throw std::system_error(errno, std::system_category(), "epoll_create1");
    return ret;
}

void System::epoll_ctl(int epfd, int op, int fd, epoll_event* event)
{
    const int ret(::epoll_ctl(epfd, op, fd, event));
    if (ret == -1)
        throw std::system_error(errno, std::system_category(), "epoll_ctl");
}

int System::epoll_wait(int epfd, epoll_event* events, int maxevents, int timeout)
{
    const int ret(::epoll_wait(epfd, events, maxevents, timeout));
    if ((ret == -1) && (errno != EINTR))
        throw std::system_error(errno, std::system_category(), "epoll_wait");
    return ret;
}

int System::timerfd_create(int clockid, int flags)
{
    const int ret(::timerfd_create(clockid, flags));
    if (ret == -1)
        throw std::system_error(errno, std::system_category(), "timerfd_create");
    return ret;
}

void System::timerfd_settime(int fd, int flags, const itimerspec* new_value, itimerspec* old_value)
{
    const int ret(::timerfd_settime(fd, flags, new_value, old_value));
    if (ret == -1)
        throw std::system_error(errno, std::system_category(), "timerfd_settime");
}

ssize_t System::read(int fd, void* buf, size_t count)
{
    const ssize_t ret(::read(fd, buf, count));
    if ((ret == -1) && (errno != EINTR) && (errno != EAGAIN))
        throw std::system_error(errno, std::system_category(), "read");
    return ret;
}

int System::eventfd(unsigned int initval, int flags)
{
    const int ret(::eventfd(initval, flags));
    if (ret == -1)
        throw std::system_error(errno, std::system_category(), "eventfd");
    return ret;
}

ssize_t System::write(int fd, const void* buf, size_t count)
{
    ssize_t ret;
    do
        ret = ::write(fd, buf, count);
    while ((ret == -1) && (errno == EINTR));
    if (ret == -1)
        throw std::system_error(errno, std::system_category(), "write");
    return ret;
}

std::chrono::steady_clock::duration System::time_since_epoch()
{
    return std::chrono::steady_clock::now().time_since_epoch();
}

void System::close(int fd)
{
    if (::close(fd) == -1)
    {
        int errno_saved = errno;
        std::ostringstream msg;
        msg << "close(" << fd << ") failed: " << strerror(errno_saved);
        logErrorOnce(msg.str());
        SHAREDDATALAYER_ABORT("close failed");
    }
}

const char* System::getenv(const char* name)
{
    return ::getenv(name);
}

System& System::getSystem() noexcept
{
    static System system;
    return system;
}
