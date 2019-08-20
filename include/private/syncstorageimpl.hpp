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

#ifndef SHAREDDATALAYER_SYNCSTORAGEIMPL_HPP_
#define SHAREDDATALAYER_SYNCSTORAGEIMPL_HPP_

#include <sdl/syncstorage.hpp>
#include <system_error>

namespace shareddatalayer
{
    class AsyncStorage;

    class System;

    class SyncStorageImpl: public SyncStorage
    {
    public:
        explicit SyncStorageImpl(std::unique_ptr<AsyncStorage> asyncStorage);

        SyncStorageImpl(std::unique_ptr<AsyncStorage> asyncStorage,
                        System& system);

        void set(const Namespace& ns, const DataMap& dataMap) override;

        bool setIf(const Namespace& ns, const Key& key, const Data& oldData, const Data& newData) override;

        bool setIfNotExists(const Namespace& ns, const Key& key, const Data& data) override;

        DataMap get(const Namespace& ns, const Keys& keys) override;

        void remove(const Namespace& ns, const Keys& keys) override;

        bool removeIf(const Namespace& ns, const Key& key, const Data& data) override;

        Keys findKeys(const Namespace& ns, const std::string& keyPrefix) override;

        void removeAll(const Namespace& ns) override;

    private:
        std::unique_ptr<AsyncStorage> asyncStorage;
        System& system;
        int pFd;
        DataMap localMap;
        Keys localKeys;
        bool localStatus;
        std::error_code localError;
        bool synced;

        void verifyBackendResponse();

        void waitForCallback();

        void waitSdlToBeReady(const Namespace& ns);

        void modifyAck(const std::error_code& error);

        void modifyIfAck(const std::error_code& error, bool status);

        void getAck(const std::error_code& error, const DataMap& dataMap);

        void findKeysAck(const std::error_code& error, const Keys& keys);
    };
}

#endif
