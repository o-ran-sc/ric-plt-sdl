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
#include <sdl/rejectedbysdl.hpp>

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
        std::chrono::steady_clock::duration TEST_READY_WAIT_TIMEOUT;
        std::chrono::steady_clock::duration TEST_OPERATION_WAIT_TIMEOUT;
        int TEST_READY_POLL_WAIT_TIMEOUT;
        int TEST_OPERATION_POLL_WAIT_TIMEOUT;
        SyncStorageImplTest():
            asyncStorageMockPassedToImplementation(new StrictMock<AsyncStorageMock>()),
            asyncStorageMockRawPtr(asyncStorageMockPassedToImplementation.get()),
            pFd(10),
            dataMap({{ "key1", { 0x0a, 0x0b, 0x0c } }, { "key2", { 0x0d, 0x0e, 0x0f, 0xff } }}),
            keys({ "key1", "key2" }),
            ns("someKnownNamespace"),
            TEST_READY_WAIT_TIMEOUT(std::chrono::minutes(1)),
            TEST_OPERATION_WAIT_TIMEOUT(std::chrono::seconds(1)),
            TEST_READY_POLL_WAIT_TIMEOUT(std::chrono::duration_cast<std::chrono::milliseconds>(TEST_READY_WAIT_TIMEOUT).count() / 10),
            TEST_OPERATION_POLL_WAIT_TIMEOUT(std::chrono::duration_cast<std::chrono::milliseconds>(TEST_OPERATION_WAIT_TIMEOUT).count() / 10)
        {
            expectConstructorCalls();
            syncStorage.reset(new SyncStorageImpl(std::move(asyncStorageMockPassedToImplementation), systemMock));
        }

        ~SyncStorageImplTest()
        {
            syncStorage->setOperationTimeout(std::chrono::steady_clock::duration::zero());
        }

        void expectConstructorCalls()
        {
            InSequence dummy;
            EXPECT_CALL(*asyncStorageMockRawPtr, fd())
                .Times(1)
                .WillOnce(Return(pFd));
        }

        void expectSdlReadinessCheck(int timeout)
        {
            InSequence dummy;
            expectPollForPendingEvents_ReturnNoEvents();
            expectWaitReadyAsync();
            expectPollWait(timeout);
            expectHandleEvents_callWaitReadyAck();
        }

        void expectPollForPendingEvents_ReturnNoEvents()
        {
            EXPECT_CALL(systemMock, poll( _, 1, 0))
                .Times(1)
                .WillOnce(Invoke([](struct pollfd *, nfds_t, int)
                                 {
                                     return 0;
                                 }));
        }

        void expectPollWait(int timeout)
        {
            EXPECT_CALL(systemMock, poll( _, 1, timeout))
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
                .Times(1);
        }

        void expectHandleEvents_callWaitReadyAck()
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, handleEvents())
                .Times(1)
                .WillOnce(Invoke([this]()
                                 {
                                    savedReadyAck(std::error_code());
                                 }));
        }

        void expectHandleEvents_callWaitReadyAckWithError()
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, handleEvents())
                .Times(1)
                .WillOnce(Invoke([this]()
                                 {
                                    savedReadyAck(AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED);
                                 }));
        }

        void expectHandleEvents_callModifyAck()
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, handleEvents())
                .Times(1)
                .WillOnce(Invoke([this]()
                                 {
                                    savedModifyAck(std::error_code());
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

        void expectListKeys()
        {
            EXPECT_CALL(*asyncStorageMockRawPtr, listKeys(ns, _, _))
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
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectSetAsync(dataMap);
    expectPollError();
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectHandleEvents_callModifyAck();
    syncStorage->set(ns, dataMap);
}

TEST_F(SyncStorageImplTest, EventsAreNotHandledWhenThereIsAnExceptionalConditionOnTheFd)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectSetAsync(dataMap);
    expectPollExceptionalCondition();
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectHandleEvents_callModifyAck();
    syncStorage->set(ns, dataMap);
}

TEST_F(SyncStorageImplTest, WaitReadySuccessfully)
{
    InSequence dummy;
    expectWaitReadyAsync();
    expectPollWait(TEST_READY_POLL_WAIT_TIMEOUT);
    expectHandleEvents_callWaitReadyAck();
    syncStorage->waitReady(ns, TEST_READY_WAIT_TIMEOUT);
}

TEST_F(SyncStorageImplTest, WaitReadyCanThrowRejectedBySdl)
{
    InSequence dummy;
    expectWaitReadyAsync();
    EXPECT_THROW(syncStorage->waitReady(ns, std::chrono::nanoseconds(1)), RejectedBySdl);
}

TEST_F(SyncStorageImplTest, WaitReadyCanThrowNotConnected)
{
    InSequence dummy;
    expectWaitReadyAsync();
    expectPollWait(TEST_READY_POLL_WAIT_TIMEOUT);
    expectHandleEvents_callWaitReadyAckWithError();
    EXPECT_THROW(syncStorage->waitReady(ns, TEST_READY_WAIT_TIMEOUT), NotConnected);
}

TEST_F(SyncStorageImplTest, SetSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectSetAsync(dataMap);
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectHandleEvents_callModifyAck();
    syncStorage->set(ns, dataMap);
}

TEST_F(SyncStorageImplTest, SetWithReadinessTimeoutSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(TEST_OPERATION_POLL_WAIT_TIMEOUT);
    expectSetAsync(dataMap);
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectHandleEvents_callModifyAck();
    syncStorage->setOperationTimeout(TEST_OPERATION_WAIT_TIMEOUT);
    syncStorage->set(ns, dataMap);
}

TEST_F(SyncStorageImplTest, SetCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectSetAsync(dataMap);
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectModifyAckWithError();
    EXPECT_THROW(syncStorage->set(ns, dataMap), BackendError);
}

TEST_F(SyncStorageImplTest, SetIfSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectSetAsync(dataMap);
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectHandleEvents_callModifyAck();
    syncStorage->set(ns, dataMap);
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectSetIfAsync("key1", { 0x0a, 0x0b, 0x0c }, { 0x0d, 0x0e, 0x0f });
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectHandleEvents_callModifyAck();
    syncStorage->setIf(ns, "key1", { 0x0a, 0x0b, 0x0c }, { 0x0d, 0x0e, 0x0f });
}

TEST_F(SyncStorageImplTest, SetIfWithReadinessTimeoutSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(TEST_OPERATION_POLL_WAIT_TIMEOUT);
    expectSetAsync(dataMap);
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectHandleEvents_callModifyAck();
    syncStorage->setOperationTimeout(TEST_OPERATION_WAIT_TIMEOUT);
    syncStorage->set(ns, dataMap);
    expectSdlReadinessCheck(TEST_OPERATION_POLL_WAIT_TIMEOUT);
    expectSetIfAsync("key1", { 0x0a, 0x0b, 0x0c }, { 0x0d, 0x0e, 0x0f });
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectHandleEvents_callModifyAck();
    syncStorage->setIf(ns, "key1", { 0x0a, 0x0b, 0x0c }, { 0x0d, 0x0e, 0x0f });
}

TEST_F(SyncStorageImplTest, SetIfCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectSetAsync(dataMap);
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectHandleEvents_callModifyAck();
    syncStorage->set(ns, dataMap);
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectSetIfAsync("key1", { 0x0a, 0x0b, 0x0c }, { 0x0d, 0x0e, 0x0f });
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectModifyIfAck(AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY, false);
    EXPECT_THROW(syncStorage->setIf(ns, "key1", { 0x0a, 0x0b, 0x0c }, { 0x0d, 0x0e, 0x0f }), BackendError);
}

TEST_F(SyncStorageImplTest, SetIfNotExistsSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectSetIfNotExistsAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectModifyIfAck(std::error_code(), true);
    EXPECT_TRUE(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }));
}

TEST_F(SyncStorageImplTest, SetIfNotExistsIfWithReadinessTimeoutSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(TEST_OPERATION_POLL_WAIT_TIMEOUT);
    expectSetIfNotExistsAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectModifyIfAck(std::error_code(), true);
    syncStorage->setOperationTimeout(TEST_OPERATION_WAIT_TIMEOUT);
    EXPECT_TRUE(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }));
}

TEST_F(SyncStorageImplTest, SetIfNotExistsReturnsFalseIfKeyAlreadyExists)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectSetIfNotExistsAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectModifyIfAck(std::error_code(), false);
    EXPECT_FALSE(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }));
}

TEST_F(SyncStorageImplTest, SetIfNotExistsCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectSetIfNotExistsAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectModifyIfAck(AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY, false);
    EXPECT_THROW(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }), BackendError);
}

TEST_F(SyncStorageImplTest, GetSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectGetAsync(keys);
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectGetAck();
    auto map(syncStorage->get(ns, keys));
    EXPECT_EQ(map, dataMap);
}

TEST_F(SyncStorageImplTest, GetWithReadinessTimeoutSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(TEST_OPERATION_POLL_WAIT_TIMEOUT);
    expectGetAsync(keys);
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectGetAck();
    syncStorage->setOperationTimeout(TEST_OPERATION_WAIT_TIMEOUT);
    auto map(syncStorage->get(ns, keys));
    EXPECT_EQ(map, dataMap);
}

TEST_F(SyncStorageImplTest, GetCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectGetAsync(keys);
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectGetAckWithError();
    EXPECT_THROW(syncStorage->get(ns, keys), BackendError);
}

TEST_F(SyncStorageImplTest, RemoveSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectRemoveAsync(keys);
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectHandleEvents_callModifyAck();
    syncStorage->remove(ns, keys);
}

TEST_F(SyncStorageImplTest, RemoveWithReadinessTimeoutSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(TEST_OPERATION_POLL_WAIT_TIMEOUT);
    expectRemoveAsync(keys);
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectHandleEvents_callModifyAck();
    syncStorage->setOperationTimeout(TEST_OPERATION_WAIT_TIMEOUT);
    syncStorage->remove(ns, keys);
}

TEST_F(SyncStorageImplTest, RemoveCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectRemoveAsync(keys);
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectModifyAckWithError();
    EXPECT_THROW(syncStorage->remove(ns, keys), BackendError);
}

TEST_F(SyncStorageImplTest, RemoveIfSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectRemoveIfAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectModifyIfAck(std::error_code(), true);
    EXPECT_TRUE(syncStorage->removeIf(ns, "key1", { 0x0a, 0x0b, 0x0c }));
}

TEST_F(SyncStorageImplTest, RemoveIfWithReadinessTimeoutSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(TEST_OPERATION_POLL_WAIT_TIMEOUT);
    expectRemoveIfAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectModifyIfAck(std::error_code(), true);
    syncStorage->setOperationTimeout(TEST_OPERATION_WAIT_TIMEOUT);
    EXPECT_TRUE(syncStorage->removeIf(ns, "key1", { 0x0a, 0x0b, 0x0c }));
}

TEST_F(SyncStorageImplTest, RemoveIfReturnsFalseIfKeyDoesnotMatch)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectRemoveIfAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectModifyIfAck(std::error_code(), false);
    EXPECT_FALSE(syncStorage->removeIf(ns, "key1", { 0x0a, 0x0b, 0x0c }));
}

TEST_F(SyncStorageImplTest, RemoveIfCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectRemoveIfAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectModifyIfAck(AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY, false);
    EXPECT_THROW(syncStorage->removeIf(ns, "key1", { 0x0a, 0x0b, 0x0c }), BackendError);
}

TEST_F(SyncStorageImplTest, FindKeysSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectFindKeysAsync();
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectFindKeysAck();
    auto ids(syncStorage->findKeys(ns, "*"));
    EXPECT_EQ(ids, keys);
}

TEST_F(SyncStorageImplTest, ListKeysSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectListKeys();
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectFindKeysAck();
    auto ids(syncStorage->listKeys(ns, "*"));
    EXPECT_EQ(ids, keys);
}

TEST_F(SyncStorageImplTest, FindKeysWithReadinessTimeoutSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(TEST_OPERATION_POLL_WAIT_TIMEOUT);
    expectFindKeysAsync();
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectFindKeysAck();
    syncStorage->setOperationTimeout(TEST_OPERATION_WAIT_TIMEOUT);
    auto ids(syncStorage->findKeys(ns, "*"));
    EXPECT_EQ(ids, keys);
}

TEST_F(SyncStorageImplTest, FindKeysAckCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectFindKeysAsync();
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectFindKeysAckWithError();
    EXPECT_THROW(syncStorage->findKeys(ns, "*"), BackendError);
}

TEST_F(SyncStorageImplTest, RemoveAllSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectRemoveAllAsync();
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectHandleEvents_callModifyAck();
    syncStorage->removeAll(ns);
}

TEST_F(SyncStorageImplTest, RemoveAllWithReadinessTimeoutSuccessfully)
{
    InSequence dummy;
    expectSdlReadinessCheck(TEST_OPERATION_POLL_WAIT_TIMEOUT);
    expectRemoveAllAsync();
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectHandleEvents_callModifyAck();
    syncStorage->setOperationTimeout(TEST_OPERATION_WAIT_TIMEOUT);
    syncStorage->removeAll(ns);
}

TEST_F(SyncStorageImplTest, RemoveAllCanThrowBackendError)
{
    InSequence dummy;
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectRemoveAllAsync();
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
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
            expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
            expectSetIfNotExistsAsync("key1", { 0x0a, 0x0b, 0x0c });
            expectPollWait(SyncStorageImpl::NO_TIMEOUT);
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
            expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
            expectSetIfNotExistsAsync("key1", { 0x0a, 0x0b, 0x0c });
            expectPollWait(SyncStorageImpl::NO_TIMEOUT);
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
            case AsyncRedisCommandDispatcherErrorCode::WRITING_TO_SLAVE:
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
    expectSdlReadinessCheck(SyncStorageImpl::NO_TIMEOUT);
    expectSetIfNotExistsAsync("key1", { 0x0a, 0x0b, 0x0c });
    expectPollWait(SyncStorageImpl::NO_TIMEOUT);
    expectModifyIfAck(std::error_code(1, std::system_category()), false);
    EXPECT_THROW(syncStorage->setIfNotExists(ns, "key1", { 0x0a, 0x0b, 0x0c }), std::range_error);
}
