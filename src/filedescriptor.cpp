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

#include "private/filedescriptor.hpp"
#include "private/system.hpp"

using namespace shareddatalayer;

namespace
{
    int moveFD(int& fd)
    {
        const int ret(fd);
        fd = -1;
        return ret;
    }
}

FileDescriptor::FileDescriptor(int fd) noexcept:
    FileDescriptor(System::getSystem(), fd)
{
}

FileDescriptor::FileDescriptor(System& system, int fd) noexcept:
    system(&system),
    fd(fd)
{
}

FileDescriptor::FileDescriptor(FileDescriptor&& fd) noexcept:
    system(fd.system),
    fd(moveFD(fd.fd)),
    atCloseCb(std::move(fd.atCloseCb))
{
}

FileDescriptor& FileDescriptor::operator = (FileDescriptor&& fd) noexcept
{
    close();
    system = fd.system;
    this->fd = moveFD(fd.fd);
    atCloseCb = std::move(fd.atCloseCb);
    return *this;
}

FileDescriptor::~FileDescriptor()
{
    close();
}

void FileDescriptor::atClose(std::function<void(int)> atCloseCb)
{
    this->atCloseCb = atCloseCb;
}

void FileDescriptor::close()
{
    if (fd < 0)
        return;
    if (atCloseCb)
        atCloseCb(fd);
    system->close(fd);
    fd = -1;
}
