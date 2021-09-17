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

#ifndef SHAREDDATALAYER_TST_ASYNCSTORAGEMOCK_HPP_
#define SHAREDDATALAYER_TST_ASYNCSTORAGEMOCK_HPP_

#include <gmock/gmock.h>
#include <sdl/asyncstorage.hpp>

namespace shareddatalayer
{
    namespace tst
    {
        class AsyncStorageMock: public AsyncStorage
        {
        public:
            MOCK_METHOD3(setAsync, void(const Namespace& ns, const DataMap& dataMap, const ModifyAck& modifyAck));

            MOCK_METHOD5(setIfAsync, void(const Namespace& ns, const Key& key, const Data& oldData, const Data& newData, const ModifyIfAck& modifyIfAck));

            MOCK_METHOD4(setIfNotExistsAsync, void(const Namespace& ns, const Key& key, const Data& data, const ModifyIfAck& modifyIfAck));

            MOCK_METHOD3(getAsync, void(const Namespace& ns, const Keys& keys, const GetAck& getAck));

            MOCK_METHOD3(removeAsync, void(const Namespace& ns, const Keys& keys, const ModifyAck& modifyAck));

            MOCK_METHOD4(removeIfAsync, void(const Namespace& ns, const Key& key, const Data& data, const ModifyIfAck& modifyIfAck));

            MOCK_METHOD3(findKeysAsync, void(const Namespace& ns, const std::string& keyPrefix, const FindKeysAck& findKeysAck));

            MOCK_METHOD3(listKeys, void(const Namespace& ns, const std::string& pattern, const FindKeysAck& findKeysAck));

            MOCK_METHOD2(removeAllAsync, void(const Namespace& ns, const ModifyAck& modifyAck));

            MOCK_METHOD2(waitReadyAsync, void(const Namespace& ns, const ReadyAck& readyAck));

            MOCK_CONST_METHOD0(fd, int());

            MOCK_METHOD0(handleEvents, void());
        };
    }
}

#endif
