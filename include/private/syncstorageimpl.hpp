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

#ifndef SHAREDDATALAYER_SYNCSTORAGEIMPL_HPP_
#define SHAREDDATALAYER_SYNCSTORAGEIMPL_HPP_

#include <sdl/asyncstorage.hpp>
#include <sdl/syncstorage.hpp>
#include <sys/poll.h>
#include <system_error>

namespace shareddatalayer
{
    class System;

    class SyncStorageImpl: public SyncStorage
    {
    public:
        explicit SyncStorageImpl(std::unique_ptr<AsyncStorage> asyncStorage);

        SyncStorageImpl(std::unique_ptr<AsyncStorage> asyncStorage,
                        System& system);

        virtual void waitReady(const Namespace& ns, const std::chrono::steady_clock::duration& timeout) override;

        virtual void set(const Namespace& ns, const DataMap& dataMap) override;

        virtual bool setIf(const Namespace& ns, const Key& key, const Data& oldData, const Data& newData) override;

        virtual bool setIfNotExists(const Namespace& ns, const Key& key, const Data& data) override;

        virtual DataMap get(const Namespace& ns, const Keys& keys) override;

        virtual void remove(const Namespace& ns, const Keys& keys) override;

        virtual bool removeIf(const Namespace& ns, const Key& key, const Data& data) override;

        virtual Keys findKeys(const Namespace& ns, const std::string& keyPrefix) override;

        virtual Keys listKeys(const Namespace& ns, const std::string& pattern) override;

        virtual void removeAll(const Namespace& ns) override;

        virtual void setOperationTimeout(const std::chrono::steady_clock::duration& timeout) override;

        static constexpr int NO_TIMEOUT = -1;

    private:
        std::unique_ptr<AsyncStorage> asyncStorage;
        System& system;
        DataMap localMap;
        Keys localKeys;
        bool localStatus;
        std::error_code localError;
        bool synced;
        bool isReady;
        struct pollfd events;
        std::chrono::steady_clock::duration operationTimeout;

        void verifyBackendResponse();

        void pollAndHandleEvents(int timeout_ms);

        void waitForReadinessCheckCallback(const std::chrono::steady_clock::duration& timeout);

        void waitForOperationCallback();

        void waitSdlToBeReady(const Namespace& ns);

        void waitSdlToBeReady(const Namespace& ns, const std::chrono::steady_clock::duration& timeout);

        void waitReadyAck(const std::error_code& error);

        void modifyAck(const std::error_code& error);

        void modifyIfAck(const std::error_code& error, bool status);

        void getAck(const std::error_code& error, const DataMap& dataMap);

        void findKeysAck(const std::error_code& error, const Keys& keys);

        void handlePendingEvents();
    };
}

#endif
