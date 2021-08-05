/*
   Copyright (c) 2018-2021 Nokia.

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

#ifndef SHAREDDATALAYER_TST_SYNCSTORAGEIMPLMOCK_HPP_
#define SHAREDDATALAYER_TST_SYNCSTORAGEIMPLMOCK_HPP_

#include "config.h"
#include "private/syncstorageimpl.hpp"
#include <gmock/gmock.h>

namespace shareddatalayer
{
    namespace tst
    {
        class SyncStorageImplMock: public SyncStorageImpl
        {
        public:
            SyncStorageImplMock(std::unique_ptr<AsyncStorageImpl> asyncStorageImpl):
                SyncStorageImpl(std::move(asyncStorageImpl)) {}

            MOCK_METHOD2(set,
                         void(const Namespace& ns, const DataMap& dataMap));

            MOCK_METHOD4(setIf,
                         bool(const Namespace& ns, const Key& key, const Data& oldData, const Data& newData));

            MOCK_METHOD4(setIf,
                         bool(const Namespace& ns, const DataMap& dataToBeChecked, const Keys& keysWhichShouldNotExist, const DataMap& dataToBeWritten));

            MOCK_METHOD3(setIfNotExists,
                         bool(const Namespace& ns, const Key& key, const Data& data));

            MOCK_METHOD2(get,
                         DataMap(const Namespace& ns, const Keys& keys));

            MOCK_METHOD2(remove,
                         void(const Namespace& ns, const Keys& keys));

            MOCK_METHOD3(removeIf,
                         bool(const Namespace& ns, const Key& key, const Data& data));

            MOCK_METHOD2(findKeys,
                         Keys(const Namespace& ns, const std::string& keyPrefix));

            MOCK_METHOD1(removeAll,
                         void(const Namespace& ns));
        };
    }
}

#endif
