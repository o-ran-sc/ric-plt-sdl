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

#include <chrono>
#include <sstream>
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

/* TODO: This synchronous API implementation could probably be refactored to be boost::asio based
 * instead of current (bit error prone) poll based implementation.
 */

SyncStorageImpl::SyncStorageImpl(std::unique_ptr<AsyncStorage> asyncStorage):
    SyncStorageImpl(std::move(asyncStorage), System::getSystem())
{
}

SyncStorageImpl::SyncStorageImpl(std::unique_ptr<AsyncStorage> pAsyncStorage,
                                 System& system):
    asyncStorage(std::move(pAsyncStorage)),
    system(system),
    localStatus(false),
    synced(false),
    isReady(false),
    events{ asyncStorage->fd(), POLLIN, 0 },
    operationTimeout(std::chrono::steady_clock::duration::zero())
{
}

void SyncStorageImpl::waitReadyAck(const std::error_code& error)
{
    isReady = true;
    localError = error;
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

void SyncStorageImpl::waitForOperationCallback()
{
    while(!synced)
        pollAndHandleEvents(NO_TIMEOUT);
}

void SyncStorageImpl::pollAndHandleEvents(int timeout_ms)
{
    if (system.poll(&events, 1, timeout_ms) > 0 && (events.revents & POLLIN))
        asyncStorage->handleEvents();
}

void SyncStorageImpl::waitForReadinessCheckCallback(const std::chrono::steady_clock::duration& timeout)
{
    if (timeout == std::chrono::steady_clock::duration::zero())
    {
        while (!isReady)
            pollAndHandleEvents(NO_TIMEOUT);
    }
    else
    {
        auto timeout_ms(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
        auto pollTimeout_ms(timeout_ms / 10);
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        while(!isReady && (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(timeout_ms)))
            pollAndHandleEvents(pollTimeout_ms);
    }
}

void SyncStorageImpl::waitSdlToBeReady(const Namespace& ns)
{
    waitSdlToBeReady(ns, operationTimeout);
}

void SyncStorageImpl::waitSdlToBeReady(const Namespace& ns, const std::chrono::steady_clock::duration& timeout)
{
    isReady = false;
    asyncStorage->waitReadyAsync(ns,
                                 std::bind(&shareddatalayer::SyncStorageImpl::waitReadyAck,
                                           this,
                                           std::placeholders::_1));
    waitForReadinessCheckCallback(timeout);
}

void SyncStorageImpl::waitReady(const Namespace& ns, const std::chrono::steady_clock::duration& timeout)
{
    waitSdlToBeReady(ns, timeout);
    if(!isReady)
        throw RejectedBySdl("Timeout, SDL service not ready for the '" + ns + "' namespace");
    verifyBackendResponse();
}

void SyncStorageImpl::set(const Namespace& ns, const DataMap& dataMap)
{
    handlePendingEvents();
    waitSdlToBeReady(ns);
    synced = false;

    asyncStorage->setAsync(ns,
                           dataMap,
                           std::bind(&shareddatalayer::SyncStorageImpl::modifyAck,
                                     this,
                                     std::placeholders::_1));
    waitForOperationCallback();
    verifyBackendResponse();
}

bool SyncStorageImpl::setIf(const Namespace& ns, const Key& key, const Data& oldData, const Data& newData)
{
    handlePendingEvents();
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
    waitForOperationCallback();
    verifyBackendResponse();
    return localStatus;
}

bool SyncStorageImpl::setIfNotExists(const Namespace& ns, const Key& key, const Data& data)
{
    handlePendingEvents();
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->setIfNotExistsAsync(ns,
                                      key,
                                      data,
                                      std::bind(&shareddatalayer::SyncStorageImpl::modifyIfAck,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2));
    waitForOperationCallback();
    verifyBackendResponse();
    return localStatus;
}

SyncStorageImpl::DataMap SyncStorageImpl::get(const Namespace& ns, const Keys& keys)
{
    handlePendingEvents();
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->getAsync(ns,
                           keys,
                           std::bind(&shareddatalayer::SyncStorageImpl::getAck,
                                     this,
                                     std::placeholders::_1,
                                     std::placeholders::_2));
    waitForOperationCallback();
    verifyBackendResponse();
    return localMap;
}

void SyncStorageImpl::remove(const Namespace& ns, const Keys& keys)
{
    handlePendingEvents();
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->removeAsync(ns,
                              keys,
                              std::bind(&shareddatalayer::SyncStorageImpl::modifyAck,
                                        this,
                                        std::placeholders::_1));
    waitForOperationCallback();
    verifyBackendResponse();
}

bool SyncStorageImpl::removeIf(const Namespace& ns, const Key& key, const Data& data)
{
    handlePendingEvents();
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->removeIfAsync(ns,
                                key,
                                data,
                                std::bind(&shareddatalayer::SyncStorageImpl::modifyIfAck,
                                          this,
                                          std::placeholders::_1,
                                          std::placeholders::_2));
    waitForOperationCallback();
    verifyBackendResponse();
    return localStatus;
}

SyncStorageImpl::Keys SyncStorageImpl::findKeys(const Namespace& ns, const std::string& keyPrefix)
{
    handlePendingEvents();
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->findKeysAsync(ns,
                                keyPrefix,
                                std::bind(&shareddatalayer::SyncStorageImpl::findKeysAck,
                                          this,
                                          std::placeholders::_1,
                                          std::placeholders::_2));
    waitForOperationCallback();
    verifyBackendResponse();
    return localKeys;
}

SyncStorageImpl::Keys SyncStorageImpl::listKeys(const Namespace& ns, const std::string& pattern)
{
    handlePendingEvents();
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->listKeys(ns,
                           pattern,
                           std::bind(&shareddatalayer::SyncStorageImpl::findKeysAck,
                                     this,
                                     std::placeholders::_1,
                                     std::placeholders::_2));
    waitForOperationCallback();
    verifyBackendResponse();
    return localKeys;
}

void SyncStorageImpl::removeAll(const Namespace& ns)
{
    handlePendingEvents();
    waitSdlToBeReady(ns);
    synced = false;
    asyncStorage->removeAllAsync(ns,
                                 std::bind(&shareddatalayer::SyncStorageImpl::modifyAck,
                                           this,
                                           std::placeholders::_1));
    waitForOperationCallback();
    verifyBackendResponse();
}

void SyncStorageImpl::handlePendingEvents()
{
    int pollRetVal = system.poll(&events, 1, 0);

    while (pollRetVal > 0 && events.revents & POLLIN)
    {
        asyncStorage->handleEvents();
        pollRetVal = system.poll(&events, 1, 0);
    }
}

void SyncStorageImpl::setOperationTimeout(const std::chrono::steady_clock::duration& timeout)
{
    operationTimeout = timeout;
}
