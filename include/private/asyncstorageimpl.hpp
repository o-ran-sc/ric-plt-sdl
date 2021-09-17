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

#ifndef SHAREDDATALAYER_REDIS_ASYNCSTORAGEIMPL_HPP_
#define SHAREDDATALAYER_REDIS_ASYNCSTORAGEIMPL_HPP_

#include <functional>
#include <sdl/asyncstorage.hpp>
#include <sdl/publisherid.hpp>
#include "private/configurationreader.hpp"
#include "private/databaseconfigurationimpl.hpp"
#include "private/logger.hpp"
#include "private/namespaceconfigurationsimpl.hpp"
#include "private/redis/asyncdatabasediscovery.hpp"
#include "private/redis/asyncredisstorage.hpp"

namespace shareddatalayer
{
    class Engine;

    class AsyncStorageImpl: public AsyncStorage
    {
    public:
        using AsyncDatabaseDiscoveryCreator = std::function<std::shared_ptr<redis::AsyncDatabaseDiscovery>(std::shared_ptr<Engine> engine,
                                                                                                           const std::string& ns,
                                                                                                           const DatabaseConfiguration& databaseConfiguration,
                                                                                                           const boost::optional<std::size_t>& addressIndex,
                                                                                                           std::shared_ptr<Logger> logger)>;

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
                         std::shared_ptr<Logger> logger,
                         const AsyncDatabaseDiscoveryCreator& asyncDatabaseDiscoveryCreator);

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

        //public for UT
        AsyncStorage& getOperationHandler(const std::string& ns);
    private:
        std::shared_ptr<Engine> engine;
        std::shared_ptr<DatabaseConfiguration> databaseConfiguration;
        std::shared_ptr<NamespaceConfigurations> namespaceConfigurations;
        const boost::optional<PublisherId> publisherId;
        std::shared_ptr<Logger> logger;
        AsyncDatabaseDiscoveryCreator asyncDatabaseDiscoveryCreator;

        std::vector<std::shared_ptr<AsyncRedisStorage>> asyncStorages;

        AsyncStorage& getRedisHandler(const std::string& ns);
        AsyncStorage& getDummyHandler();

        void setAsyncRedisStorageHandlers(const std::string& ns);
        void setAsyncRedisStorageHandlersForCluster(const std::string& ns);
        AsyncStorage& getAsyncRedisStorageHandler(const std::string& ns);
    };
}

#endif
