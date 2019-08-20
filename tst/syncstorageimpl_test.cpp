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

#include <gtest/gtest.h>
#include "private/error.hpp"
#include "private/redis/asyncredisstorage.hpp"
#include "private/syncstorageimpl.hpp"
#include "private/tst/asyncstoragemock.hpp"
#include "private/tst/systemmock.hpp"
#include <sdl/backenderror.hpp>
#include <sdl/invalidnamespace.hpp>
#include <sdl/notconnected.hpp>
#include <sdl/operationinterrupted.hpp>
#include <sdl/rejectedbybackend.hpp>

using namespace shareddatalayer;
using namespace shareddatalayer::redis;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class SyncStorageImplTest: public testing::Test
    {
    public:
        std::unique_ptr<SyncStorageImpl> syncStorage;
        /* AsyncStorageMock ownership will be passed to implementation. To be able to do verification
         * with the mock object also here after its ownership is passed we take raw pointer to
         * AsyncStorageMock before passing it to implementation. Works fine, as implementation will
         * not release injected mock object before test case execution finishes
         */
        std::unique_ptr<StrictMock<AsyncStorageMock>> asyncStorageMockPassedToImplementation;
        StrictMock<AsyncStorageMock>* asyncStorageMockRawPtr;
        StrictMock<SystemMock> systemMock;
        AsyncStorage::ModifyAck savedModifyAck;
        AsyncStorage::ModifyIfAck savedModifyIfAck;
        AsyncStorage::GetAck savedGetAck;
        AsyncStorage::FindKeysAck savedFindKeysAck;
        AsyncStorage::ReadyAck savedReadyAck;
        int pFd;
        SyncStorage::DataMap dataMap;
        SyncStorage::Keys keys;
        const SyncStorage::Namespace ns;
        SyncStorageImplTest():
            asyncStorageMockPassedToImplementation(new StrictMock<AsyncStorageMock>()),
            asyncStorageMockRawPtr(asyncStorageMockPassedToImplementation.get()),
            pFd(10),
            dataMap({{ "key1", { 0x0a, 0x0b, 0x0c } }, { "key2", { 0x0d, 0x0e, 0x0f, 0xff } }}),
            keys({ "key1", "key2" }),
            ns("someKnownNamespace")
        {
            expectConstructorCalls();
            syncStorage.reset(new SyncStorageImpl(std::move(asyncStorageMockPassedToImplementation), systemMock));
        }

        void expectConstructorCalls()
        {
            InSequence dummy;
            EXPECT_CALL(*asyncStorageMockRawPtr, fd())
                .Times(1)
                .WillOnce(Return(pFd));
        }

        void expectSdlReadinessCheck()
        {
            InSequence dummy;
            expectWaitReadyAsync();
            expectPollWait();
            expectHandleEvents();
        }

        void expectPollWait()
        {
            EXPECT_CALL(systemMock, poll( _, 1, -1))
                .Times(1)
                .WillOnce(Invoke([](struct pollfd *fds, nfds_t, int)
                                 {
                                     fds->revents = POLLIN;
                                     return 1;
                                 }));
        }

        void expectPollError()
        {
            EXPECT_CALL(systemMock, poll( _, 1, -1))
                .Times(1)
                .WillOnce(Invoke([](struct pollfd *fds, nfds_t, int)
                                 {
                                     fds->revents = POLLIN;
                                     return -1;
                                 }));
        }

        void expectPollExceptionalCondition()
        {
            EXPECT_CALL(systemMock, poll( _, 1, -1))
                .Times(1)
                .WillOnce(Invoke([](struct pollfd *fds, nfds_t, int)
                                 {
                                     fds->revents = POLLPRI;
                                     return 1;
                                 }));
        }

        void expectHandleEvents()
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, handleEvents())
                .Times(1)
                .WillOnce(Invoke([this]()
                                 {
                                    savedReadyAck(std::error_code());
                                 }));
        }

        void expectWaitReadyAsync()
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, waitReadyAsync(ns,_))
                .Times(1)
                .WillOnce(SaveArg<1>(&savedReadyAck));
        }


        void expectModifyAckWithError()
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, handleEvents())
                .Times(1)
                .WillOnce(Invoke([this]()
                                 {
                                    savedModifyAck(AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY);
                                 }));
        }

        void expectModifyIfAck(const std::error_code& error, bool status)
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, handleEvents())
                .Times(1)
                .WillOnce(Invoke([this, error, status]()
                                 {
                                    savedModifyIfAck(error, status);
                                 }));
        }

        void expectGetAckWithError()
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, handleEvents())
                .Times(1)
                .WillOnce(Invoke([this]()
                                 {
                                    savedGetAck(AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY, dataMap);
                                 }));
        }

        void expectGetAck()
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, handleEvents())
                .Times(1)
                .WillOnce(Invoke([this]()
                                 {
                                    savedGetAck(std::error_code(), dataMap);
                                 }));
        }

        void expectFindKeysAck()
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, handleEvents())
                .Times(1)
                .WillOnce(Invoke([this]()
                                 {
                                    savedFindKeysAck(std::error_code(), keys);
                                 }));
        }

        void expectFindKeysAckWithError()
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, handleEvents())
                .Times(1)
                .WillOnce(Invoke([this]()
                                 {
                                    savedFindKeysAck(AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY, keys);
                                 }));
        }

        void expectSetAsync(const SyncStorage::DataMap& dataMap)
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, setAsync(ns, dataMap, _))
                .Times(1)
                .WillOnce(SaveArg<2>(&savedModifyAck));
        }

        void expectSetIfAsync(const SyncStorage::Key& key, const SyncStorage::Data& oldData, const SyncStorage::Data& newData)
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, setIfAsync(ns, key, oldData, newData, _))
                .Times(1)
                .WillOnce(SaveArg<4>(&savedModifyIfAck));
        }

        void expectGetAsync(const SyncStorage::Keys& keys)
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, getAsync(ns, keys, _))
                .Times(1)
                .WillOnce(SaveArg<2>(&savedGetAck));
        }

        void expectFindKeysAsync()
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, findKeysAsync(ns, _, _))
                .Times(1)
                .WillOnce(SaveArg<2>(&savedFindKeysAck));
        }

        void expectRemoveAsync(const SyncStorage::Keys& keys)
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, removeAsync(ns, keys, _))
                .Times(1)
                .WillOnce(SaveArg<2>(&savedModifyAck));
        }

        void expectRemoveIfAsync(const SyncStorage::Key& key, const SyncStorage::Data& data)
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, removeIfAsync(ns, key, data, _))
                .Times(1)
                .WillOnce(SaveArg<3>(&savedModifyIfAck));
        }

        void expectRemoveAllAsync()
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, removeAllAsync(ns, _))
                .Times(1)
                .WillOnce(SaveArg<1>(&savedModifyAck));
        }

        void expectSetIfNotExistsAsync(const SyncStorage::Key& key, const SyncStorage::Data& data)
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, setIfNotExistsAsync(ns, key, data, _))
                .Times(1)
                .WillOnce(SaveArg<3>(&savedModifyIfAck));
        }
    };
}

TEST_F(SyncStorageImplTest, IsNotCopyable)
{
    InSequence dummy;
    EXPECT_FALSE(std::is_copy_constructible<SyncStorageImpl>::value);
    EXPECT_FALSE(std::is_copy_assignable<SyncStorageImpl>::value);
}

TEST_F(SyncStorageImplTest, ImplementssyncStorage)
{
    InSequence dummy;
    EXPECT_TRUE((std::is_base_of<SyncStorage, SyncStorageImpl>::value));
}

TEST_F(SyncStorageImplTest, EventsAreNotHandledWhenPollReturnsError)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectSetAsync(dataMap);
    expectPollError();
    expectPollWait();
    expectHandleEvents();
    syncStorage->set(ns, dataMap);
}

TEST_F(SyncStorageImplTest, EventsAreNotHandledWhenThereIsAnExceptionalConditionOnTheFd)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectSetAsync(dataMap);
    expectPollExceptionalCondition();
    expectPollWait();
    expectHandleEvents();
    syncStorage->set(ns, dataMap);
}

TEST_F(SyncStorageImplTest, SetSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectSetAsync(dataMap);
    expectPollWait();
    expectHandleEvents();
    syncStorage->set(ns, dataMap);
}

TEST_F(SyncStorageImplTest, SetCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectSetAsync(dataMap);
    expectPollWait();
    expectModifyAckWithError();
    EXPECT_THROW(syncStorage->set(ns, dataMap), BackendError);
}

TEST_F(SyncStorageImplTest, SetIfSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectSetAsync(dataMap);
    expectPollWait();
    expectHandleEvents();
    syncStorage->set(ns, dataMap);
    expectSdlReadinessCheck();
    expectSetIfAsync("key1", { 0x0a, 0x0b, 0x0c }, { 0x0d, 0x0e, 0x0f });
    expectPollWait();
    expectHandleEvents();
    syncStorage->setIf(ns, "key1", { 0x0a, 0x0b, 0x0c }, { 0x0d, 0x0e, 0x0f });
}

TEST_F(SyncStorageImplTest, SetIfCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectSetAsync(dataMap);
    expectPollWait();
    expectHandleEvents();
    syncStorage->set(ns, dataMap);
    expectSdlReadinessCheck();
    expectSetIfAsync("key1", { 0x0a, 0x0b, 0x0c }, { 0x0d, 0x0e, 0x0f });
    expectPollWait();
    expectModifyIfAck(AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY, false);
    EXPECT_THROW(syncStorage->setIf(ns, "key1", { 0x0a, 0x0b, 0x0c }, { 0x0d, 0x0e, 0x0f }), BackendError);
}

TEST_F(SyncStorageImplTest, SetIfNotExistsSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectSetIfNotExistsAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait();
    expectModifyIfAck(std::error_code(), true);
    EXPECT_EQ(true, syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }));
}

TEST_F(SyncStorageImplTest, SetIfNotExistsReturnsFalseIfKeyAlreadyExists)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectSetIfNotExistsAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait();
    expectModifyIfAck(std::error_code(), false);
    EXPECT_EQ(false, syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }));
}

TEST_F(SyncStorageImplTest, SetIfNotExistsCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectSetIfNotExistsAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait();
    expectModifyIfAck(AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY, false);
    EXPECT_THROW(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }), BackendError);
}

TEST_F(SyncStorageImplTest, GetSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectGetAsync(keys);
    expectPollWait();
    expectGetAck();
    auto map(syncStorage->get(ns, keys));
    EXPECT_EQ(map, dataMap);
}

TEST_F(SyncStorageImplTest, GetCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectGetAsync(keys);
    expectPollWait();
    expectGetAckWithError();
    EXPECT_THROW(syncStorage->get(ns, keys), BackendError);
}

TEST_F(SyncStorageImplTest, RemoveSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectRemoveAsync(keys);
    expectPollWait();
    expectHandleEvents();
    syncStorage->remove(ns, keys);
}

TEST_F(SyncStorageImplTest, RemoveCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectRemoveAsync(keys);
    expectPollWait();
    expectModifyAckWithError();
    EXPECT_THROW(syncStorage->remove(ns, keys), BackendError);
}

TEST_F(SyncStorageImplTest, RemoveIfSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectRemoveIfAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait();
    expectModifyIfAck(std::error_code(), true);
    EXPECT_EQ(true, syncStorage->removeIf(ns, "key1", { 0x0a, 0x0b, 0x0c }));
}

TEST_F(SyncStorageImplTest, RemoveIfReturnsFalseIfKeyDoesnotMatch)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectRemoveIfAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait();
    expectModifyIfAck(std::error_code(), false);
    EXPECT_EQ(false, syncStorage->removeIf(ns, "key1", { 0x0a, 0x0b, 0x0c }));
}

TEST_F(SyncStorageImplTest, RemoveIfCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectRemoveIfAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait();
    expectModifyIfAck(AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY, false);
    EXPECT_THROW(syncStorage->removeIf(ns, "key1", { 0x0a, 0x0b, 0x0c }), BackendError);
}

TEST_F(SyncStorageImplTest, FindKeysSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectFindKeysAsync();
    expectPollWait();
    expectFindKeysAck();
    auto ids(syncStorage->findKeys(ns, "*"));
    EXPECT_EQ(ids, keys);
}

TEST_F(SyncStorageImplTest, FindKeysAckCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectFindKeysAsync();
    expectPollWait();
    expectFindKeysAckWithError();
    EXPECT_THROW(syncStorage->findKeys(ns, "*"), BackendError);
}

TEST_F(SyncStorageImplTest, RemoveAllSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectRemoveAllAsync();
    expectPollWait();
    expectHandleEvents();
    syncStorage->removeAll(ns);
}

TEST_F(SyncStorageImplTest, RemoveAllCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectRemoveAllAsync();
    expectPollWait();
    expectModifyAckWithError();
    EXPECT_THROW(syncStorage->removeAll(ns), BackendError);
}

TEST_F(SyncStorageImplTest, AllAsyncRedisStorageErrorCodesThrowCorrectException)
{
    InSequence dummy;
    std::error_code ec;

    for (AsyncRedisStorage::ErrorCode arsec = AsyncRedisStorage::ErrorCode::SUCCESS; arsec < AsyncRedisStorage::ErrorCode::END_MARKER; ++arsec)
    {
        if (arsec != AsyncRedisStorage::ErrorCode::SUCCESS)
        {
            expectSdlReadinessCheck();
            expectSetIfNotExistsAsync("key1", { 0x0a, 0x0b, 0x0c });
            expectPollWait();
        }

        switch (arsec)
        {
            case AsyncRedisStorage::ErrorCode::SUCCESS:
                break;
            case AsyncRedisStorage::ErrorCode::INVALID_NAMESPACE:
                expectModifyIfAck(arsec, false);
                EXPECT_THROW(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }), InvalidNamespace);
                break;
            case AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED:
                expectModifyIfAck(arsec, false);
                EXPECT_THROW(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }), NotConnected);
                break;
            default:
                FAIL() << "No mapping for AsyncRedisStorage::ErrorCode value: " << arsec;
                break;
        }
    }
}

TEST_F(SyncStorageImplTest, AllDispatcherErrorCodesThrowCorrectException)
{
    InSequence dummy;
    std::error_code ec;

    for (AsyncRedisCommandDispatcherErrorCode aec = AsyncRedisCommandDispatcherErrorCode::SUCCESS; aec < AsyncRedisCommandDispatcherErrorCode::END_MARKER; ++aec)
    {
        if (aec != AsyncRedisCommandDispatcherErrorCode::SUCCESS)
        {
            expectSdlReadinessCheck();
            expectSetIfNotExistsAsync("key1", { 0x0a, 0x0b, 0x0c });
            expectPollWait();
        }

        switch (aec)
        {
            case AsyncRedisCommandDispatcherErrorCode::SUCCESS:
                break;
            case AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR:
                expectModifyIfAck(aec, false);
                EXPECT_THROW(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }), BackendError);
                break;
            case AsyncRedisCommandDispatcherErrorCode::CONNECTION_LOST:
                expectModifyIfAck(aec, false);
                EXPECT_THROW(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }), OperationInterrupted);
                break;
            case AsyncRedisCommandDispatcherErrorCode::PROTOCOL_ERROR:
                expectModifyIfAck(aec, false);
                EXPECT_THROW(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }), RejectedByBackend);
                break;
            case AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY:
                expectModifyIfAck(aec, false);
                EXPECT_THROW(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }), BackendError);
                break;
            case AsyncRedisCommandDispatcherErrorCode::DATASET_LOADING:
                expectModifyIfAck(aec, false);
                EXPECT_THROW(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }), NotConnected);
                break;
            case AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED:
                expectModifyIfAck(aec, false);
                EXPECT_THROW(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }), NotConnected);
                break;
            case AsyncRedisCommandDispatcherErrorCode::IO_ERROR:
                expectModifyIfAck(aec, false);
                EXPECT_THROW(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }), BackendError);
                break;
            default:
                FAIL() << "No mapping for AsyncRedisCommandDispatcherErrorCode value: " << aec;
                break;
        }
    }
}

TEST_F(SyncStorageImplTest, CanThrowStdExceptionIfDispatcherErrorCodeCannotBeMappedToSdlException)
{
    InSequence dummy;
    expectSdlReadinessCheck();
    expectSetIfNotExistsAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait();
    expectModifyIfAck(std::error_code(1, std::system_category()), false);
    EXPECT_THROW(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }), std::range_error);
}
