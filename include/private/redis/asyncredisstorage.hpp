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

#ifndef SHAREDDATALAYER_REDIS_ASYNCREDISSTORAGE_HPP_
#define SHAREDDATALAYER_REDIS_ASYNCREDISSTORAGE_HPP_

#include <functional>
#include <memory>
#include <queue>
#include <boost/optional.hpp>
#include <sdl/asyncstorage.hpp>
#include "private/logger.hpp"
#include "private/namespaceconfigurationsimpl.hpp"
#include "private/timer.hpp"
#include "private/redis/databaseinfo.hpp"
#include "private/redis/reply.hpp"

namespace shareddatalayer
{
    namespace redis
    {
        class AsyncCommandDispatcher;
        class AsyncDatabaseDiscovery;
        class Reply;
        struct Contents;
        class ContentsBuilder;
    }

    class Engine;

    class AsyncRedisStorage: public AsyncStorage
    {
    public:
        enum class ErrorCode
        {
            SUCCESS = 0,
            REDIS_NOT_YET_DISCOVERED,
            INVALID_NAMESPACE,
            //Keep this always as last item. Used in unit tests to loop all enum values.
            END_MARKER
        };

        using AsyncCommandDispatcherCreator = std::function<std::shared_ptr<redis::AsyncCommandDispatcher>(Engine& engine,
                                                                                                           const redis::DatabaseInfo& databaseInfo,
                                                                                                           std::shared_ptr<redis::ContentsBuilder> contentsBuilder,
                                                                                                           std::shared_ptr<Logger> logger)>;

        static const std::error_category& errorCategory() noexcept;

        AsyncRedisStorage(const AsyncRedisStorage&) = delete;

        AsyncRedisStorage& operator = (const AsyncRedisStorage&) = delete;

        AsyncRedisStorage(std::shared_ptr<Engine> engine,
                          std::shared_ptr<redis::AsyncDatabaseDiscovery> discovery,
                          const boost::optional<PublisherId>& pId,
                          std::shared_ptr<NamespaceConfigurations> namespaceConfigurations,
                          std::shared_ptr<Logger> logger);

        AsyncRedisStorage(std::shared_ptr<Engine> engine,
                          std::shared_ptr<redis::AsyncDatabaseDiscovery> discovery,
                          const boost::optional<PublisherId>& pId,
                          std::shared_ptr<NamespaceConfigurations> namespaceConfigurations,
                          const AsyncCommandDispatcherCreator& asyncCommandDispatcherCreator,
                          std::shared_ptr<redis::ContentsBuilder> contentsBuilder,
                          std::shared_ptr<Logger> logger);

        ~AsyncRedisStorage() override;

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

        redis::DatabaseInfo& getDatabaseInfo();

        std::string buildKeyPrefixSearchPattern(const Namespace& ns, const std::string& keyPrefix) const;

        std::string buildNamespaceKeySearchPattern(const Namespace& ns, const std::string& pattern) const;

    private:
        std::shared_ptr<Engine> engine;
        std::shared_ptr<redis::AsyncCommandDispatcher> dispatcher;
        std::shared_ptr<redis::AsyncDatabaseDiscovery> discovery;
        const boost::optional<PublisherId> publisherId;
        ReadyAck readyAck;
        AsyncCommandDispatcherCreator asyncCommandDispatcherCreator;
        std::shared_ptr<redis::ContentsBuilder> contentsBuilder;
        redis::DatabaseInfo dbInfo;
        std::shared_ptr<NamespaceConfigurations> namespaceConfigurations;
        std::shared_ptr<Logger> logger;

        bool canOperationBePerformed(const Namespace& ns, boost::optional<bool> inputDataIsEmpty, std::error_code& ecToReturn);

        void serviceStateChanged(const redis::DatabaseInfo& databaseInfo);

        std::string getPublishMessage() const;

        void modificationCommandCallback(const std::error_code& error, const redis::Reply&, const ModifyAck&);

        void conditionalCommandCallback(const std::error_code& error, const redis::Reply&, const ModifyIfAck&);

        void findKeys(const std::string& ns, const std::string& keyPattern, const FindKeysAck& findKeysAck);
    };

    AsyncRedisStorage::ErrorCode& operator++ (AsyncRedisStorage::ErrorCode& ecEnum);
    std::error_code make_error_code(AsyncRedisStorage::ErrorCode errorCode);
}

namespace std
{
    template <>
    struct is_error_code_enum<shareddatalayer::AsyncRedisStorage::ErrorCode>: public true_type { };
}

#endif
