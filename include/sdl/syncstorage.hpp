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

#ifndef SHAREDDATALAYER_SYNCSTORAGE_HPP_
#define SHAREDDATALAYER_SYNCSTORAGE_HPP_

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <sdl/exception.hpp>
#include <sdl/publisherid.hpp>

namespace shareddatalayer
{
    /**
     * @brief Class providing synchronous access to shared data layer storage.
     *
     * SyncStorage class provides synchronous access to all namespaces in
     * shared data layer storage. Data can be saved, read and removed based on keys known
     * to clients. Keys are unique within a namespace, namespace identifier is passed as
     * a parameter to all operations.
     *
     * SyncStorage is primarily intended for command-line interface-typed applications, or
     * non-event-loop based, such as multi-threaded applications, where shareddatalayer
     * operations are carried out in a separate thread.
     *
     * @note The same instance of SyncStorage must not be shared between multiple threads
     *       without explicit application level locking.
     *
     * @see AsyncStorage for asynchronous interface.
     * @see AsyncStorage::SEPARATOR for namespace format restrictions.
     */
    class SyncStorage
    {
    public:
        SyncStorage(const SyncStorage&) = delete;

        SyncStorage& operator = (const SyncStorage&) = delete;

        SyncStorage(SyncStorage&&) = delete;

        SyncStorage& operator = (SyncStorage&&) = delete;

        virtual ~SyncStorage() = default;

        using Namespace = std::string;

        using Key = std::string;

        using Data = std::vector<uint8_t>;

        using DataMap = std::map<Key, Data>;

        /**
         * Write data to shared data layer storage. Writing is done atomically, i.e. either
         * all succeeds or all fails.
         *
         * Exceptions thrown (excluding standard exceptions such as std::bad_alloc) are all derived from
         * shareddatalayer::Exception base class. Client can catch only that exception if separate handling
         * for different shareddatalayer error situations is not needed.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param dataMap Data to be written.
         *
         * @throw BackendError if the backend data storage fails to process the request.
         * @throw NotConnected if shareddatalayer is not connected to the backend data storage.
         * @throw OperationInterrupted if shareddatalayer does not receive a reply from the backend data storage.
         * @throw InvalidNamespace if given namespace does not meet the namespace format restrictions.
         */
        virtual void set(const Namespace& ns,
                         const DataMap& dataMap) = 0;

        /**
         * Conditionally modify the value of a key if the current value in data storage
         * matches the user's last known value.
         *
         * Exceptions thrown (excluding standard exceptions such as std::bad_alloc) are all derived from
         * shareddatalayer::Exception base class. Client can catch only that exception if separate handling
         * for different shareddatalayer error situations is not needed.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param key Key for which data modification will be executed.
         * @param oldData Last known data.
         * @param newData Data to be written.
         *
         * @return True for successful modification, false if the user's last known data did
         *         not match the current value in data storage.
         *
         * @throw BackendError if the backend data storage fails to process the request.
         * @throw NotConnected if shareddatalayer is not connected to the backend data storage.
         * @throw OperationInterrupted if shareddatalayer does not receive a reply from the backend data storage.
         * @throw InvalidNamespace if given namespace does not meet the namespace format restrictions.
         */
        virtual bool setIf(const Namespace& ns,
                           const Key& key,
                           const Data& oldData,
                           const Data& newData) = 0;

        /**
         * Conditionally set the value of a key. If key already exists, then it's value
         * is not modified. Checking the key existence and potential set operation is done
         * as a one atomic operation.
         *
         * Exceptions thrown (excluding standard exceptions such as std::bad_alloc) are all derived from
         * shareddatalayer::Exception base class. Client can catch only that exception if separate handling
         * for different shareddatalayer error situations is not needed.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param key Key.
         * @param data Data to be written.
         *
         * @return True if key didn't exist yet and set operation was executed, false if
         *         key already existed and thus its value was left untouched.
         *
         * @throw BackendError if the backend data storage fails to process the request.
         * @throw NotConnected if shareddatalayer is not connected to the backend data storage.
         * @throw OperationInterrupted if shareddatalayer does not receive a reply from the backend data storage.
         * @throw InvalidNamespace if given namespace does not meet the namespace format restrictions.
         */
        virtual bool setIfNotExists(const Namespace& ns,
                                    const Key& key,
                                    const Data& data) = 0;

        using Keys = std::set<Key>;

        /**
         * Read data from shared data layer storage. Only those entries that are found will
         * be returned.
         *
         * Exceptions thrown (excluding standard exceptions such as std::bad_alloc) are all derived from
         * shareddatalayer::Exception base class. Client can catch only that exception if separate handling
         * for different shareddatalayer error situations is not needed.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param keys Data to be read.
         *
         * @return Data from the storage.
         *
         * @throw BackendError if the backend data storage fails to process the request.
         * @throw NotConnected if shareddatalayer is not connected to the backend data storage.
         * @throw OperationInterrupted if shareddatalayer does not receive a reply from the backend data storage.
         * @throw InvalidNamespace if given namespace does not meet the namespace format restrictions.
         */
        virtual DataMap get(const Namespace& ns,
                            const Keys& keys) = 0;

        /**
         * Remove data from shared data layer storage. Existing keys are removed. Removing
         * is done atomically, i.e. either all succeeds or all fails.
         *
         * Exceptions thrown (excluding standard exceptions such as std::bad_alloc) are all derived from
         * shareddatalayer::Exception base class. Client can catch only that exception if separate handling
         * for different shareddatalayer error situations is not needed.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param keys Data to be removed.
         *
         * @throw BackendError if the backend data storage fails to process the request.
         * @throw NotConnected if shareddatalayer is not connected to the backend data storage.
         * @throw OperationInterrupted if shareddatalayer does not receive a reply from the backend data storage.
         * @throw InvalidNamespace if given namespace does not meet the namespace format restrictions.
         */
        virtual void remove(const Namespace& ns,
                            const Keys& keys) = 0;

        /**
         * Conditionally remove data from shared data layer storage if the current data value
         * matches the user's last known value.
         *
         * Exceptions thrown (excluding standard exceptions such as std::bad_alloc) are all derived from
         * shareddatalayer::Exception base class. Client can catch only that exception if separate handling
         * for different shareddatalayer error situations is not needed.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param key Data to be removed.
         * @param data Last known value of data
         *
         * @return True if successful removal, false if the user's last known data did
         *         not match the current value in data storage.
         *
         * @throw BackendError if the backend data storage fails to process the request.
         * @throw NotConnected if shareddatalayer is not connected to the backend data storage.
         * @throw OperationInterrupted if shareddatalayer does not receive a reply from the backend data storage.
         * @throw InvalidNamespace if given namespace does not meet the namespace format restrictions.
         */
        virtual bool removeIf(const Namespace& ns,
                              const Key& key,
                              const Data& data) = 0;

        /**
         * Find all keys matching search pattern under the namespace. No prior knowledge about the keys in the given
         * namespace exists, thus operation is not guaranteed to be atomic or isolated.
         *
         * Exceptions thrown (excluding standard exceptions such as std::bad_alloc) are all derived from
         * shareddatalayer::Exception base class. Client can catch only that exception if separate handling
         * for different shareddatalayer error situations is not needed.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param keyPrefix Only keys starting with given keyPrefix are returned. Passing empty string as
         *                  keyPrefix will return all keys.
         *
         * @return Found keys.
         *
         * @throw BackendError if the backend data storage fails to process the request.
         * @throw NotConnected if shareddatalayer is not connected to the backend data storage.
         * @throw OperationInterrupted if shareddatalayer does not receive a reply from the backend data storage.
         * @throw InvalidNamespace if given namespace does not meet the namespace format restrictions.
         */
        virtual Keys findKeys(const Namespace& ns,
                              const std::string& keyPrefix) = 0;

        /**
         * Remove all keys under the namespace. Found keys are removed atomically, i.e.
         * either all succeeds or all fails.
         *
         * Exceptions thrown (excluding standard exceptions such as std::bad_alloc) are all derived from
         * shareddatalayer::Exception base class. Client can catch only that exception if separate handling
         * for different shareddatalayer error situations is not needed.
         *
         * @param ns Namespace under which this operation is targeted.
         *
         * @throw BackendError if the backend data storage fails to process the request.
         * @throw NotConnected if shareddatalayer is not connected to the backend data storage.
         * @throw OperationInterrupted if shareddatalayer does not receive a reply from the backend data storage.
         * @throw InvalidNamespace if given namespace does not meet the namespace format restrictions.
         */
        virtual void removeAll(const Namespace& ns) = 0;

        /**
         * Create a new instance of SyncStorage.
         *
         * @return New instance of SyncStorage.
         *
         * @throw EmptyNamespace if namespace string is an empty string.
         * @throw InvalidNamespace if namespace contains illegal characters.
         *
         */
        static std::unique_ptr<SyncStorage> create();

    protected:
        SyncStorage() = default;
    };
}

#endif
