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
#include <chrono>
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
         * Wait for the service to become ready to serve. There is typically a waiting period
         * when product cluster is brought up (deploying container orchestrated service,
         * container restart etc.). The function can be called and used to synchronize the
         * application startup with the readiness of the underlying data storage.
         *
         * This function can also be used if SDL returns an error indicating connection cut
         * to backend data storage. In that situation client can use this function to get an
         * indication when SDL is again ready to serve. SDL error code documentation indicates
         * the error codes for which the usage of this function is applicable.
         *
         * Exceptions thrown (excluding standard exceptions such as std::bad_alloc) are all
         * derived from shareddatalayer::Exception base class. Client can catch only that
         * exception if separate handling for different shareddatalayer error situations is not
         * needed.
         *
         * @param ns Namespace under which this operation is targeted. As it is possible to
         *           use different DB backend instances for namespaces, it is necessary to
         *           provide namespace for this call to test the DB backend readiness of that
         *           particular namespace. it is recommended to call this always when starting
         *           to use new namespace.
         *
         * @param timeout Timeout value after which readiness waiting will be expired in case
         *                the SDL service won't come up. It is recommended to use rather long
         *                timeout value to have enough time for a system to recover for example
         *                from restart scenarios. Suitable timeout value depends greatly from
         *                the environment itself. As an example an application could use 10
         *                seconds timeout value and have a loop what does a re-try every time
         *                when previous waitReady call has failed. On a side note, timeout
         *                value 0 means that readiness is waited interminable.
         *
         * @throw NotConnected if shareddatalayer is not connected to the backend data storage.
         * @throw RejectedBySdl if SDL rejects the request.
         * @throw BackendError if the backend data storage fails to process the request.
         */
        virtual void waitReady(const Namespace& ns, const std::chrono::steady_clock::duration& timeout) = 0;

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
         * Find all keys matching search key prefix under the namespace. No prior knowledge about the keys in the given
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
        [[deprecated("Use listKeys() instead.")]]
        virtual Keys findKeys(const Namespace& ns,
                              const std::string& keyPrefix) = 0;

        /**
         * List all keys matching search glob-style pattern under the namespace.
         *
         * Supported glob-style patterns:
         *   h?llo matches hello, hallo and hxllo
         *   h*llo matches hllo and heeeello
         *   h[ae]llo matches hello and hallo, but not hillo
         *   h[^e]llo matches hallo, hbllo, ... but not hello
         *   h[a-b]llo matches hallo and hbllo
         *
         * The \ escapes character(s) in key search pattern and those will be treated as a normal
         * character(s):
         *   h\[?llo\* matches h[ello* and h[allo*
         *
         * @param ns Namespace under which this operation is targeted.
         * @param pattern Find keys matching a given glob-style pattern.
         *
         * @return Found keys.
         *
         * @throw BackendError if the backend data storage fails to process the request.
         * @throw NotConnected if shareddatalayer is not connected to the backend data storage.
         * @throw OperationInterrupted if shareddatalayer does not receive a reply from the backend data storage.
         * @throw InvalidNamespace if given namespace does not meet the namespace format restrictions.
         */
        virtual Keys listKeys(const Namespace& ns,
                              const std::string& pattern) = 0;

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
         * Set a timeout value for the synchronous SDL read, write and remove operations.
         * By default synchronous read, write and remove operations do not have any timeout
         * for the backend data storage readiness, operations are pending interminable to
         * finish until backend is ready. With this API function default behaviour can be
         * changed and when a timeout happens, an error exception is risen for the SDL
         * operation in question. To avoid unnecessary timeout failure due to a temporal
         * connection issue, it is recommended not to set too short timeout value.
         * Reasonable timeout value is 5 seconds or bigger value. On a side note, timeout
         * value 0 means interminable pending time.
         *
         * @param timeout Timeout value to set.
         */
         virtual void setOperationTimeout(const std::chrono::steady_clock::duration& timeout) = 0;

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
