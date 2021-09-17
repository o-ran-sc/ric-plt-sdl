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

#include "config.h"
#include <sstream>
#include "private/error.hpp"
#include <sdl/emptynamespace.hpp>
#include <sdl/invalidnamespace.hpp>
#include <sdl/publisherid.hpp>
#include "private/abort.hpp"
#include "private/createlogger.hpp"
#include "private/engine.hpp"
#include "private/logger.hpp"
#include "private/namespacevalidator.hpp"
#include "private/configurationreader.hpp"
#include "private/redis/asynccommanddispatcher.hpp"
#include "private/redis/asyncdatabasediscovery.hpp"
#include "private/redis/asyncredisstorage.hpp"
#include "private/redis/contents.hpp"
#include "private/redis/contentsbuilder.hpp"
#include "private/redis/redisgeneral.hpp"
#include "private/redis/reply.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;

/*  TODO: This implementation contains lot of duplicated code with old API (asyncRedisConnection).
 *  When this new API is fully ready and tested old API implementation could be changed to utilize this
 *  (bit like sync API utilizes async API).
 */

namespace
{
    std::shared_ptr<AsyncCommandDispatcher> asyncCommandDispatcherCreator(Engine& engine,
                                                                          const DatabaseInfo& databaseInfo,
                                                                          std::shared_ptr<ContentsBuilder> contentsBuilder,
                                                                          std::shared_ptr<Logger> logger)
    {
        return AsyncCommandDispatcher::create(engine,
                                              databaseInfo,
                                              contentsBuilder,
                                              false,
                                              logger,
                                              false);
    }

    class AsyncRedisStorageErrorCategory: public std::error_category
    {
    public:
        AsyncRedisStorageErrorCategory() = default;

        const char* name() const noexcept override;

        std::string message(int condition) const override;

        std::error_condition default_error_condition(int condition) const noexcept override;
    };

    const char* AsyncRedisStorageErrorCategory::name() const noexcept
    {
        return "asyncredisstorage";
    }

    std::string AsyncRedisStorageErrorCategory::message(int condition) const
    {
        switch (static_cast<AsyncRedisStorage::ErrorCode>(condition))
        {
            case AsyncRedisStorage::ErrorCode::SUCCESS:
                return std::error_code().message();
            case AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED:
                return "connection to the underlying data storage not yet available";
            case AsyncRedisStorage::ErrorCode::INVALID_NAMESPACE:
                return "invalid namespace identifier passed to SDL API";
            case AsyncRedisStorage::ErrorCode::END_MARKER:
                logErrorOnce("AsyncRedisStorage::ErrorCode::END_MARKER is not meant to be queried (it is only for enum loop control)");
                return "unsupported error code for message()";
            default:
                return "description missing for AsyncRedisStorageErrorCategory error: " + std::to_string(condition);
        }
    }

    std::error_condition AsyncRedisStorageErrorCategory::default_error_condition(int condition) const noexcept
    {
        switch (static_cast<AsyncRedisStorage::ErrorCode>(condition))
        {
            case AsyncRedisStorage::ErrorCode::SUCCESS:
                return InternalError::SUCCESS;
            case AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED:
                return InternalError::SDL_NOT_READY;
            case AsyncRedisStorage::ErrorCode::INVALID_NAMESPACE:
                return InternalError::SDL_RECEIVED_INVALID_PARAMETER;
            case AsyncRedisStorage::ErrorCode::END_MARKER:
                logErrorOnce("AsyncRedisStorage::ErrorCode::END_MARKER is not meant to be mapped to InternalError (it is only for enum loop control)");
                return InternalError::SDL_ERROR_CODE_LOGIC_ERROR;
            default:
                std::ostringstream msg;
                msg << "default error condition missing for AsyncRedisStorageErrorCategory error: "
                    << condition;
                logErrorOnce(msg.str());
                return InternalError::SDL_ERROR_CODE_LOGIC_ERROR;
        }
    }

    AsyncStorage::DataMap buildDataMap(const AsyncStorage::Keys& keys, const Reply::ReplyVector& replyVector)
    {
        AsyncStorage::DataMap dataMap;
        auto i(0U);
        for (const auto& j : keys)
        {
            if (replyVector[i]->getType() == Reply::Type::STRING)
            {
                AsyncStorage::Data data;
                auto dataStr(replyVector[i]->getString());
                for (ReplyStringLength k(0); k < dataStr->len; ++k)
                    data.push_back(static_cast<uint8_t>(dataStr->str[static_cast<size_t>(k)]));
                dataMap.insert({ j, data });
            }
            ++i;
        }
        return dataMap;
    }

    AsyncStorage::Key getKey(const Reply::DataItem& item)
    {
        std::string str(item.str.c_str(), static_cast<size_t>(item.len));
        auto res(str.find(AsyncRedisStorage::SEPARATOR));
        return str.substr(res + 1);
    }

    AsyncStorage::Keys getKeys(const Reply::ReplyVector& replyVector)
    {
        AsyncStorage::Keys keys;
        for (const auto& i : replyVector)
        {
            if (i->getType() == Reply::Type::STRING)
                keys.insert(getKey(*i->getString()));
        }
        return keys;
    }

    void escapeRedisSearchPatternCharacters(std::string& stringToProcess)
    {
        const std::string redisSearchPatternCharacters = R"(*?[]\)";

        std::size_t foundPosition = stringToProcess.find_first_of(redisSearchPatternCharacters);

        while (foundPosition != std::string::npos)
        {
            stringToProcess.insert(foundPosition, R"(\)");
            foundPosition = stringToProcess.find_first_of(redisSearchPatternCharacters, foundPosition + 2);
        }
    }
}

AsyncRedisStorage::ErrorCode& shareddatalayer::operator++ (AsyncRedisStorage::ErrorCode& ecEnum)
{
    if (ecEnum == AsyncRedisStorage::ErrorCode::END_MARKER)
        throw std::out_of_range("for AsyncRedisStorage::ErrorCode& operator ++");
    ecEnum = AsyncRedisStorage::ErrorCode(static_cast<std::underlying_type<AsyncRedisStorage::ErrorCode>::type>(ecEnum) + 1);
    return ecEnum;
}

std::error_code shareddatalayer::make_error_code(AsyncRedisStorage::ErrorCode errorCode)
{
    return std::error_code(static_cast<int>(errorCode), AsyncRedisStorage::errorCategory());
}

const std::error_category& AsyncRedisStorage::errorCategory() noexcept
{
    static const AsyncRedisStorageErrorCategory theAsyncRedisStorageErrorCategory;
    return theAsyncRedisStorageErrorCategory;
}

AsyncRedisStorage::AsyncRedisStorage(std::shared_ptr<Engine> engine,
                                     std::shared_ptr<AsyncDatabaseDiscovery> discovery,
                                     const boost::optional<PublisherId>& pId,
                                     std::shared_ptr<NamespaceConfigurations> namespaceConfigurations,
                                     std::shared_ptr<Logger> logger):
    AsyncRedisStorage(engine,
                      discovery,
                      pId,
                      namespaceConfigurations,
                      ::asyncCommandDispatcherCreator,
                      std::make_shared<redis::ContentsBuilder>(SEPARATOR),
                      logger)
{
}

AsyncRedisStorage::AsyncRedisStorage(std::shared_ptr<Engine> engine,
                                     std::shared_ptr<redis::AsyncDatabaseDiscovery> discovery,
                                     const boost::optional<PublisherId>& pId,
                                     std::shared_ptr<NamespaceConfigurations> namespaceConfigurations,
                                     const AsyncCommandDispatcherCreator& asyncCommandDispatcherCreator,
                                     std::shared_ptr<redis::ContentsBuilder> contentsBuilder,
                                     std::shared_ptr<Logger> logger):
    engine(engine),
    dispatcher(nullptr),
    discovery(discovery),
    publisherId(pId),
    asyncCommandDispatcherCreator(asyncCommandDispatcherCreator),
    contentsBuilder(contentsBuilder),
    namespaceConfigurations(namespaceConfigurations),
    logger(logger)
{
    if(publisherId && (*publisherId).empty())
    {
        throw std::invalid_argument("AsyncRedisStorage: empty publisher ID string given");
    }

    discovery->setStateChangedCb([this](const redis::DatabaseInfo& databaseInfo)
                                 {
                                     serviceStateChanged(databaseInfo);
                                 });
}

AsyncRedisStorage::~AsyncRedisStorage()
{
    if (discovery)
        discovery->clearStateChangedCb();
    if (dispatcher)
        dispatcher->disableCommandCallbacks();
}

redis::DatabaseInfo& AsyncRedisStorage::getDatabaseInfo()
{
    return dbInfo;
}

void AsyncRedisStorage::serviceStateChanged(const redis::DatabaseInfo& newDatabaseInfo)
{
    dispatcher = asyncCommandDispatcherCreator(*engine,
                                               newDatabaseInfo,
                                               contentsBuilder,
                                               logger);
    if (readyAck)
        dispatcher->waitConnectedAsync([this]()
                                       {
                                           readyAck(std::error_code());
                                           readyAck = ReadyAck();
                                       });
    dbInfo = newDatabaseInfo;
}

int AsyncRedisStorage::fd() const
{
    return engine->fd();
}

void AsyncRedisStorage::handleEvents()
{
    engine->handleEvents();
}

bool AsyncRedisStorage::canOperationBePerformed(const Namespace& ns,
                                                boost::optional<bool> noKeysGiven,
                                                std::error_code& ecToReturn)
{
    if (!::isValidNamespace(ns))
    {
        logErrorOnce("Invalid namespace identifier: " + ns + " passed to SDL");
        ecToReturn = std::error_code(ErrorCode::INVALID_NAMESPACE);
        return false;
    }
    if (noKeysGiven && *noKeysGiven)
    {
        ecToReturn = std::error_code();
        return false;
    }
    if (!dispatcher)
    {
        ecToReturn = std::error_code(ErrorCode::REDIS_NOT_YET_DISCOVERED);
        return false;
    }

    ecToReturn = std::error_code();
    return true;
}

void AsyncRedisStorage::waitReadyAsync(const Namespace&,
                                       const ReadyAck& readyAck)
{
    if (dispatcher)
        dispatcher->waitConnectedAsync([readyAck]()
                                       {
                                           readyAck(std::error_code());
                                       });
    else
        this->readyAck = readyAck;
}

void AsyncRedisStorage::setAsync(const Namespace& ns,
                                 const DataMap& dataMap,
                                 const ModifyAck& modifyAck)
{
    std::error_code ec;

    if (!canOperationBePerformed(ns, dataMap.empty(), ec))
    {
        engine->postCallback(std::bind(modifyAck, ec));
        return;
    }

    if (namespaceConfigurations->areNotificationsEnabled(ns))
        dispatcher->dispatchAsync(std::bind(&AsyncRedisStorage::modificationCommandCallback,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            modifyAck),
                                  ns,
                                  contentsBuilder->build("MSETPUB", ns, dataMap, ns, getPublishMessage()));
    else
        dispatcher->dispatchAsync(std::bind(&AsyncRedisStorage::modificationCommandCallback,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            modifyAck),
                                  ns,
                                  contentsBuilder->build("MSET", ns, dataMap));
}

void AsyncRedisStorage::modificationCommandCallback(const std::error_code& error,
                                                    const Reply&,
                                                    const ModifyAck& modifyAck )
{
    modifyAck(error);
}

void AsyncRedisStorage::conditionalCommandCallback(const std::error_code& error,
                                                   const Reply& reply,
                                                   const ModifyIfAck& modifyIfAck)
{
    auto type(reply.getType());
    if (error ||
        (type == Reply::Type::NIL) || // SETIE(PUB)
        ((type == Reply::Type::INTEGER) && (reply.getInteger() != 1))) // SETNX(PUB) and DELIE(PUB)
        modifyIfAck(error, false);
    else
        modifyIfAck(error, true);
}

void AsyncRedisStorage::setIfAsync(const Namespace& ns,
                                   const Key& key,
                                   const Data& oldData,
                                   const Data& newData,
                                   const ModifyIfAck& modifyIfAck)
{
    std::error_code ec;

    if (!canOperationBePerformed(ns, boost::none, ec))
    {
        engine->postCallback(std::bind(modifyIfAck, ec, false));
        return;
    }

    if (namespaceConfigurations->areNotificationsEnabled(ns))
        dispatcher->dispatchAsync(std::bind(&AsyncRedisStorage::conditionalCommandCallback,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            modifyIfAck),
                                  ns,
                                  contentsBuilder->build("SETIEPUB", ns, key, newData, oldData, ns, getPublishMessage()));
    else
        dispatcher->dispatchAsync(std::bind(&AsyncRedisStorage::conditionalCommandCallback,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            modifyIfAck),
                                  ns,
                                  contentsBuilder->build("SETIE", ns, key, newData, oldData));
}

void AsyncRedisStorage::removeIfAsync(const Namespace& ns,
                                      const Key& key,
                                      const Data& data,
                                      const ModifyIfAck& modifyIfAck)
{
    std::error_code ec;

    if (!canOperationBePerformed(ns, boost::none, ec))
    {
        engine->postCallback(std::bind(modifyIfAck, ec, false));
        return;
    }

    if (namespaceConfigurations->areNotificationsEnabled(ns))
        dispatcher->dispatchAsync(std::bind(&AsyncRedisStorage::conditionalCommandCallback,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            modifyIfAck),
                                  ns,
                                  contentsBuilder->build("DELIEPUB", ns, key, data, ns, getPublishMessage()));
    else
        dispatcher->dispatchAsync(std::bind(&AsyncRedisStorage::conditionalCommandCallback,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            modifyIfAck),
                                  ns,
                                  contentsBuilder->build("DELIE", ns, key, data));
}

std::string AsyncRedisStorage::getPublishMessage() const
{
    if(publisherId)
        return *publisherId;
    else
        return NO_PUBLISHER;
}

void AsyncRedisStorage::setIfNotExistsAsync(const Namespace& ns,
                                            const Key& key,
                                            const Data& data,
                                            const ModifyIfAck& modifyIfAck)
{
    std::error_code ec;

    if (!canOperationBePerformed(ns, boost::none, ec))
    {
        engine->postCallback(std::bind(modifyIfAck, ec, false));
        return;
    }

    if (namespaceConfigurations->areNotificationsEnabled(ns))
        dispatcher->dispatchAsync(std::bind(&AsyncRedisStorage::conditionalCommandCallback,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            modifyIfAck),
                                  ns,
                                  contentsBuilder->build("SETNXPUB", ns, key, data, ns ,getPublishMessage()));
    else
        dispatcher->dispatchAsync(std::bind(&AsyncRedisStorage::conditionalCommandCallback,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            modifyIfAck),
                                  ns,
                                  contentsBuilder->build("SETNX", ns, key, data));
}

void AsyncRedisStorage::getAsync(const Namespace& ns,
                                 const Keys& keys,
                                 const GetAck& getAck)
{
    std::error_code ec;

    if (!canOperationBePerformed(ns, keys.empty(), ec))
    {
        engine->postCallback(std::bind(getAck, ec, DataMap()));
        return;
    }

    dispatcher->dispatchAsync([getAck, keys](const std::error_code& error,
                                                 const Reply& reply)
                              {
                                  if (error)
                                      getAck(error, DataMap());
                                  else
                                      getAck(std::error_code(), buildDataMap(keys, *reply.getArray()));
                              },
                              ns,
                              contentsBuilder->build("MGET", ns, keys));
}

void AsyncRedisStorage::removeAsync(const Namespace& ns,
                                    const Keys& keys,
                                    const ModifyAck& modifyAck)
{
    std::error_code ec;

    if (!canOperationBePerformed(ns, keys.empty(), ec))
    {
        engine->postCallback(std::bind(modifyAck, ec));
        return;
    }

    if (namespaceConfigurations->areNotificationsEnabled(ns))
        dispatcher->dispatchAsync(std::bind(&AsyncRedisStorage::modificationCommandCallback,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            modifyAck),
                                  ns,
                                  contentsBuilder->build("DELPUB", ns, keys, ns, getPublishMessage()));
    else
        dispatcher->dispatchAsync(std::bind(&AsyncRedisStorage::modificationCommandCallback,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            modifyAck),
                                  ns,
                                  contentsBuilder->build("DEL", ns, keys));
}

void AsyncRedisStorage::findKeys(const Namespace& ns,
                                 const std::string& keyPattern,
                                 const FindKeysAck& findKeysAck)
{
    //TODO: update to more optimal solution than current KEYS-based one.
    std::error_code ec;

    if (!canOperationBePerformed(ns, boost::none, ec))
    {
        engine->postCallback(std::bind(findKeysAck, ec, Keys()));
        return;
    }

    dispatcher->dispatchAsync([findKeysAck](const std::error_code& error, const Reply& reply)
                              {
                                  if (error)
                                      findKeysAck(error, Keys());
                                  else
                                      findKeysAck(std::error_code(), getKeys(*reply.getArray()));
                              },
                              ns,
                              contentsBuilder->build("KEYS", keyPattern));
}

void AsyncRedisStorage::findKeysAsync(const Namespace& ns,
                                      const std::string& keyPrefix,
                                      const FindKeysAck& findKeysAck)
{
    auto keyPattern(buildKeyPrefixSearchPattern(ns, keyPrefix));
    findKeys(ns, keyPattern, findKeysAck);
}

void AsyncRedisStorage::listKeys(const Namespace& ns,
                                 const std::string& pattern,
                                 const FindKeysAck& findKeysAck)
{
    auto keyPattern(buildNamespaceKeySearchPattern(ns, pattern));
    findKeys(ns, keyPattern, findKeysAck);
}

void AsyncRedisStorage::removeAllAsync(const Namespace& ns,
                                       const ModifyAck& modifyAck)
{
    std::error_code ec;

    if (!canOperationBePerformed(ns, boost::none, ec))
    {
        engine->postCallback(std::bind(modifyAck, ec));
        return;
    }

    dispatcher->dispatchAsync([this, modifyAck, ns](const std::error_code& error, const Reply& reply)
                              {
                                  if (error)
                                  {
                                      modifyAck(error);
                                      return;
                                  }
                                  const auto& array(*reply.getArray());
                                  if (array.empty())
                                      modifyAck(std::error_code());
                                  else
                                  {
                                      removeAsync(ns, getKeys(array), modifyAck);
                                  }
                              },
                              ns,
                              contentsBuilder->build("KEYS", buildKeyPrefixSearchPattern(ns, "")));
}

std::string AsyncRedisStorage::buildKeyPrefixSearchPattern(const Namespace& ns, const std::string& keyPrefix) const
{
    std::string escapedKeyPrefix = keyPrefix;
    escapeRedisSearchPatternCharacters(escapedKeyPrefix);
    std::ostringstream oss;
    oss << '{' << ns << '}' << SEPARATOR << escapedKeyPrefix << "*";
    return oss.str();
}

std::string AsyncRedisStorage::buildNamespaceKeySearchPattern(const Namespace& ns,
                                                              const std::string& pattern) const
{
    std::ostringstream oss;
    oss << '{' << ns << '}' << SEPARATOR << pattern;
    return oss.str();
}
