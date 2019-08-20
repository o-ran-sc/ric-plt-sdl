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

#include "config.h"
#include "private/error.hpp"
#include "private/abort.hpp"
#include "private/asyncstorageimpl.hpp"
#include "private/configurationreader.hpp"
#include "private/asyncdummystorage.hpp"
#include "private/engine.hpp"
#include "private/logger.hpp"
#if HAVE_REDIS
#include "private/redis/asyncdatabasediscovery.hpp"
#include "private/redis/asyncredisstorage.hpp"
#endif

using namespace shareddatalayer;

AsyncStorageImpl::AsyncStorageImpl(std::shared_ptr<Engine> engine,
                                   const boost::optional<PublisherId>& pId,
                                   std::shared_ptr<Logger> logger):
    engine(engine),
    databaseConfiguration(std::make_shared<DatabaseConfigurationImpl>()),
    namespaceConfigurations(std::make_shared<NamespaceConfigurationsImpl>()),
    publisherId(pId),
    logger(logger)
{
    ConfigurationReader configurationReader(logger);
    configurationReader.readDatabaseConfiguration(std::ref(*databaseConfiguration));
    configurationReader.readNamespaceConfigurations(std::ref(*namespaceConfigurations));
}

// Meant for UT usage
AsyncStorageImpl::AsyncStorageImpl(std::shared_ptr<Engine> engine,
                                   const boost::optional<PublisherId>& pId,
                                   std::shared_ptr<DatabaseConfiguration> databaseConfiguration,
                                   std::shared_ptr<NamespaceConfigurations> namespaceConfigurations,
                                   std::shared_ptr<Logger> logger):
    engine(engine),
    databaseConfiguration(databaseConfiguration),
    namespaceConfigurations(namespaceConfigurations),
    publisherId(pId),
    logger(logger)
{
}

AsyncStorage& AsyncStorageImpl::getRedisHandler()
{
#if HAVE_REDIS
    static AsyncRedisStorage redisHandler{engine,
                                          redis::AsyncDatabaseDiscovery::create(
                                              engine,
                                              boost::none,
                                              std::ref(*databaseConfiguration),
                                              logger),
                                          publisherId,
                                          namespaceConfigurations,
                                          logger};

    return redisHandler;
#else
    logger->error() << "Redis operations cannot be performed, Redis not enabled";
    SHAREDDATALAYER_ABORT("Invalid configuration.");
#endif
}

AsyncStorage& AsyncStorageImpl::getDummyHandler()
{
    static AsyncDummyStorage dummyHandler{engine};
    return dummyHandler;
}

AsyncStorage& AsyncStorageImpl::getOperationHandler(const std::string& ns)
{
    if (namespaceConfigurations->isDbBackendUseEnabled(ns))
        return getRedisHandler();

    return getDummyHandler();
}

int AsyncStorageImpl::fd() const
{
    return engine->fd();
}

void AsyncStorageImpl::handleEvents()
{
    engine->handleEvents();
}

void AsyncStorageImpl::waitReadyAsync(const Namespace& ns,
                                      const ReadyAck& readyAck)
{
    getOperationHandler(ns).waitReadyAsync(ns, readyAck);
}

void AsyncStorageImpl::setAsync(const Namespace& ns,
                                const DataMap& dataMap,
                                const ModifyAck& modifyAck)
{
    getOperationHandler(ns).setAsync(ns, dataMap, modifyAck);
}

void AsyncStorageImpl::setIfAsync(const Namespace& ns,
                                  const Key& key,
                                  const Data& oldData,
                                  const Data& newData,
                                  const ModifyIfAck& modifyIfAck)
{
    getOperationHandler(ns).setIfAsync(ns, key, oldData, newData, modifyIfAck);
}

void AsyncStorageImpl::removeIfAsync(const Namespace& ns,
                                     const Key& key,
                                     const Data& data,
                                     const ModifyIfAck& modifyIfAck)
{
    getOperationHandler(ns).removeIfAsync(ns, key, data, modifyIfAck);
}

void AsyncStorageImpl::setIfNotExistsAsync(const Namespace& ns,
                                           const Key& key,
                                           const Data& data,
                                           const ModifyIfAck& modifyIfAck)
{
    getOperationHandler(ns).setIfNotExistsAsync(ns, key, data, modifyIfAck);
}

void AsyncStorageImpl::getAsync(const Namespace& ns,
                                const Keys& keys,
                                const GetAck& getAck)
{
    getOperationHandler(ns).getAsync(ns, keys, getAck);
}

void AsyncStorageImpl::removeAsync(const Namespace& ns,
                                   const Keys& keys,
                                   const ModifyAck& modifyAck)
{
    getOperationHandler(ns).removeAsync(ns, keys, modifyAck);
}

void AsyncStorageImpl::findKeysAsync(const Namespace& ns,
                                     const std::string& keyPrefix,
                                     const FindKeysAck& findKeysAck)
{
    getOperationHandler(ns).findKeysAsync(ns, keyPrefix, findKeysAck);
}

void AsyncStorageImpl::removeAllAsync(const Namespace& ns,
                                       const ModifyAck& modifyAck)
{
    getOperationHandler(ns).removeAllAsync(ns, modifyAck);
}
