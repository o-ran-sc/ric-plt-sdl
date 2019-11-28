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

#include <sstream>
#include <sys/poll.h>
#include <sdl/asyncstorage.hpp>
#include <sdl/backenderror.hpp>
#include <sdl/errorqueries.hpp>
#include <sdl/invalidnamespace.hpp>
#include <sdl/notconnected.hpp>
#include <sdl/operationinterrupted.hpp>
#include <sdl/rejectedbybackend.hpp>
#include <sdl/rejectedbysdl.hpp>
#include "private/redis/asyncredisstorage.hpp"
#include "private/syncstorageimpl.hpp"
#include "private/system.hpp"

using namespace shareddatalayer;

namespace
{
    void throwExceptionForErrorCode[[ noreturn ]](const std::error_code& ec)
    {
        if (ec == shareddatalayer::Error::BACKEND_FAILURE)
            throw BackendError(ec.message());
        else if (ec == shareddatalayer::Error::NOT_CONNECTED)
            throw NotConnected(ec.message());
        else if (ec == shareddatalayer::Error::OPERATION_INTERRUPTED)
            throw OperationInterrupted(ec.message());
        else if (ec == shareddatalayer::Error::REJECTED_BY_BACKEND)
            throw RejectedByBackend(ec.message());
        else if (ec == AsyncRedisStorage::ErrorCode::INVALID_NAMESPACE)
            throw InvalidNamespace(ec.message());
        else if (ec == shareddatalayer::Error::REJECTED_BY_SDL)
            throw RejectedBySdl(ec.message());

        std::ostringstream os;
        os << "No corresponding SDL exception found for error code: " << ec.category().name() << " " << ec.value();
        throw std::range_error(os.str());
    }
}

SyncStorageImpl::SyncStorageImpl(std::unique_ptr<AsyncStorage> asyncStorage):
    SyncStorageImpl(std::move(asyncStorage), System::getSystem())
{
}

SyncStorageImpl::SyncStorageImpl(std::unique_ptr<AsyncStorage> pAsyncStorage,
                                 System& system):
    asyncStorage(std::move(pAsyncStorage)),
    system(system),
    pFd(asyncStorage->fd()),
    localStatus(false),
    synced(false)
{
}

void SyncStorageImpl::modifyAck(const std::error_code& error)
{
    synced = true;
    localError = error;
}

void SyncStorageImpl::modifyIfAck(const std::error_code& error, bool status)
{
    synced = true;
    localError = error;
    localStatus = status;
}

void SyncStorageImpl::getAck(const std::error_code& error, const DataMap& dataMap)
{
    synced = true;
    localError = error;
    localMap = dataMap;
}

void SyncStorageImpl::findKeysAck(const std::error_code& error, const Keys& keys)
{
    synced = true;
    localError = error;
    localKeys = keys;
}

void SyncStorageImpl::verifyBackendResponse()
{
    if(localError)
        throwExceptionForErrorCode(localError);
}

void SyncStorageImpl::waitForCallback()
{
    struct pollfd events { pFd, POLLIN, 0 };
    while(!synced)
        if (system.poll(&events, 1, -1) > 0 && (events.revents & POLLIN))
            asyncStorage->handleEvents();
}

void SyncStorageImpl::waitSdlToBeReady(const Namespace& ns)
{
    synced = false;
    asyncStorage->waitReadyAsync(ns,
                                 std::bind(&shareddatalayer::SyncStorageImpl::modifyAck,
                                           this,
                                           std::error_code()));
    waitForCallback();
    verifyBackendResponse();
}

void SyncStorageImpl::set(const Namespace& ns, const DataMap& dataMap)
{
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->setAsync(ns,
                           dataMap,
                           std::bind(&shareddatalayer::SyncStorageImpl::modifyAck,
                                     this,
                                     std::placeholders::_1));
    waitForCallback();
    verifyBackendResponse();
}

bool SyncStorageImpl::setIf(const Namespace& ns, const Key& key, const Data& oldData, const Data& newData)
{
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->setIfAsync(ns,
                             key,
                             oldData,
                             newData,
                             std::bind(&shareddatalayer::SyncStorageImpl::modifyIfAck,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2));
    waitForCallback();
    verifyBackendResponse();
    return localStatus;
}

bool SyncStorageImpl::setIfNotExists(const Namespace& ns, const Key& key, const Data& data)
{
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->setIfNotExistsAsync(ns,
                                      key,
                                      data,
                                      std::bind(&shareddatalayer::SyncStorageImpl::modifyIfAck,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2));
    waitForCallback();
    verifyBackendResponse();
    return localStatus;
}

SyncStorageImpl::DataMap SyncStorageImpl::get(const Namespace& ns, const Keys& keys)
{
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->getAsync(ns,
                           keys,
                           std::bind(&shareddatalayer::SyncStorageImpl::getAck,
                                     this,
                                     std::placeholders::_1,
                                     std::placeholders::_2));
    waitForCallback();
    verifyBackendResponse();
    return localMap;
}

void SyncStorageImpl::remove(const Namespace& ns, const Keys& keys)
{
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->removeAsync(ns,
                              keys,
                              std::bind(&shareddatalayer::SyncStorageImpl::modifyAck,
                                        this,
                                        std::placeholders::_1));
    waitForCallback();
    verifyBackendResponse();
}

bool SyncStorageImpl::removeIf(const Namespace& ns, const Key& key, const Data& data)
{
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->removeIfAsync(ns,
                                key,
                                data,
                                std::bind(&shareddatalayer::SyncStorageImpl::modifyIfAck,
                                          this,
                                          std::placeholders::_1,
                                          std::placeholders::_2));
    waitForCallback();
    verifyBackendResponse();
    return localStatus;
}

SyncStorageImpl::Keys SyncStorageImpl::findKeys(const Namespace& ns, const std::string& keyPrefix)
{
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->findKeysAsync(ns,
                                keyPrefix,
                                std::bind(&shareddatalayer::SyncStorageImpl::findKeysAck,
                                          this,
                                          std::placeholders::_1,
                                          std::placeholders::_2));
    waitForCallback();
    verifyBackendResponse();
    return localKeys;
}

void SyncStorageImpl::removeAll(const Namespace& ns)
{
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->removeAllAsync(ns,
                                 std::bind(&shareddatalayer::SyncStorageImpl::modifyAck,
                                           this,
                                           std::placeholders::_1));
    waitForCallback();
    verifyBackendResponse();
}
