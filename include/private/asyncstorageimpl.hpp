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

#ifndef SHAREDDATALAYER_REDIS_ASYNCSTORAGEIMPL_HPP_
#define SHAREDDATALAYER_REDIS_ASYNCSTORAGEIMPL_HPP_

#include <sdl/asyncstorage.hpp>
#include <sdl/publisherid.hpp>
#include "private/configurationreader.hpp"
#include "private/databaseconfigurationimpl.hpp"
#include "private/logger.hpp"
#include "private/namespaceconfigurationsimpl.hpp"

namespace shareddatalayer
{
    class Engine;

    class AsyncStorageImpl: public AsyncStorage
    {
    public:
        AsyncStorageImpl(const AsyncStorageImpl&) = delete;

        AsyncStorageImpl& operator = (const AsyncStorageImpl&) = delete;

        AsyncStorageImpl(AsyncStorageImpl&&) = delete;

        AsyncStorageImpl& operator = (AsyncStorageImpl&&) = delete;

        AsyncStorageImpl(std::shared_ptr<Engine> engine,
                         const boost::optional<PublisherId>& pId,
                         std::shared_ptr<Logger> logger);

        // Meant for UT
        AsyncStorageImpl(std::shared_ptr<Engine> engine,
                         const boost::optional<PublisherId>& pId,
                         std::shared_ptr<DatabaseConfiguration> databaseConfiguration,
                         std::shared_ptr<NamespaceConfigurations> namespaceConfigurations,
                         std::shared_ptr<Logger> logger);

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

        void removeAllAsync(const Namespace& ns, const ModifyAck& modifyAck) override;

        //public for UT
        AsyncStorage& getOperationHandler(const std::string& ns);
    private:
        std::shared_ptr<Engine> engine;
        std::shared_ptr<DatabaseConfiguration> databaseConfiguration;
        std::shared_ptr<NamespaceConfigurations> namespaceConfigurations;
        const boost::optional<PublisherId> publisherId;
        std::shared_ptr<Logger> logger;

        AsyncStorage& getRedisHandler();
        AsyncStorage& getDummyHandler();
    };
}

#endif
