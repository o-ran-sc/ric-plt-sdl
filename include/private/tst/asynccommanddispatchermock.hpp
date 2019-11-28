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

#ifndef SHAREDDATALAYER_TST_ASYNCCOMMANDDISPATCHERMOCK_HPP_
#define SHAREDDATALAYER_TST_ASYNCCOMMANDDISPATCHERMOCK_HPP_

#include <gmock/gmock.h>
#include "private/redis/asynccommanddispatcher.hpp"
#include "private/redis/contents.hpp"

namespace shareddatalayer
{
    namespace tst
    {
        class AsyncCommandDispatcherMock: public redis::AsyncCommandDispatcher
        {
        public:
            MOCK_METHOD1(waitConnectedAsync, void(const ConnectAck& connectAck));

            MOCK_METHOD1(registerDisconnectCb, void(const DisconnectCb& disconnectCb));

            MOCK_METHOD3(dispatchAsync, void(const CommandCb& commandCb, const AsyncConnection::Namespace& ns, const redis::Contents& contents));

            MOCK_METHOD0(disableCommandCallbacks, void());
        };
    }
}

#endif
