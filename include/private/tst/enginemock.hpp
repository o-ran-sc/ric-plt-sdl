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

#ifndef SHAREDDATALAYER_TST_ENGINEMOCK_HPP_
#define SHAREDDATALAYER_TST_ENGINEMOCK_HPP_

#include <gmock/gmock.h>
#include "private/engine.hpp"
#include "private/filedescriptor.hpp"

namespace shareddatalayer
{
    namespace tst
    {
        class EngineMock: public Engine
        {
        public:
            MOCK_CONST_METHOD0(fd, int());

            MOCK_METHOD0(handleEvents, void());

            MOCK_METHOD3(addMonitoredFD, void(int fd, unsigned int events, const EventHandler& eh));

            MOCK_METHOD3(addMonitoredFD, void(FileDescriptor& fd, unsigned int events, const EventHandler& eh));

            MOCK_METHOD2(modifyMonitoredFD, void(int fd, unsigned int events));

            MOCK_CONST_METHOD1(isMonitoredFD, bool(int fd));

            MOCK_METHOD1(deleteMonitoredFD, void(int fd));

            MOCK_METHOD1(postCallback, void(const Callback& callback));

            MOCK_METHOD3(armTimer, void(Timer& timer, const Timer::Duration& duration, const Timer::Callback& cb));

            MOCK_METHOD1(disarmTimer, void(const Timer& timer));

            MOCK_METHOD0(run, void());

            MOCK_METHOD0(stop, void());
        };
    }
}

#endif
