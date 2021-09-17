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

#ifndef SHAREDDATALAYER_ASYNCDUMMYSTORAGE_HPP_
#define SHAREDDATALAYER_ASYNCDUMMYSTORAGE_HPP_

#include <sdl/asyncstorage.hpp>

namespace shareddatalayer
{
    class Engine;

    class AsyncDummyStorage: public AsyncStorage
    {
    public:
        explicit AsyncDummyStorage(std::shared_ptr<Engine> engine);

        AsyncDummyStorage(const AsyncDummyStorage&) = delete;

        AsyncDummyStorage& operator = (const AsyncDummyStorage&) = delete;

        AsyncDummyStorage(AsyncDummyStorage&&) = delete;

        AsyncDummyStorage& operator = (AsyncDummyStorage&&) = delete;

        int fd() const override;

        void handleEvents() override;

        void waitReadyAsync(const Namespace& ns, const ReadyAck& readyAck) override;

        void setAsync(const Namespace& ns, const DataMap& dataMap, const ModifyAck& modifyAck) override;

        void setIfAsync(const Namespace& ns, const Key& key, const Data& oldData, const Data& newData, const ModifyIfAck& modifyIfAck) override;

        void setIfNotExistsAsync(const Namespace& ns, const Key& key, const Data& data, const ModifyIfAck& modifyIfAck) override;

        void getAsync(const Namespace& ns, const Keys& keys, const GetAck& getAck) override;

        void removeAsync(const Namespace& ns, const Keys& keys, const ModifyAck& modifyAck) override;

        void removeIfAsync(const Namespace& ns, const Key& key, const Data& data, const ModifyIfAck& modifyIfAck) override;

        void findKeysAsync(const Namespace& ns, const std::string& keyPrefix, const FindKeysAck& findKeysAck) override;

        void listKeys(const Namespace& ns, const std::string& pattern, const FindKeysAck& findKeysAck) override;

        void removeAllAsync(const Namespace& ns, const ModifyAck& modifyAck) override;

    private:
        using Callback = std::function<void()>;

        std::shared_ptr<Engine> engine;

        void postCallback(const Callback& callback);
    };
}

#endif
