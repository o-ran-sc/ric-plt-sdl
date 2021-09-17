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

#ifndef SHAREDDATALAYER_ASYNCSTORAGE_HPP_
#define SHAREDDATALAYER_ASYNCSTORAGE_HPP_

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <sdl/errorqueries.hpp>
#include <sdl/publisherid.hpp>

namespace shareddatalayer
{
    /**
     * @brief Class providing asynchronous access to shared data layer storage.
     *
     * AsyncStorage class provides asynchronous access to all namespaces in
     * shared data layer storage. Data can be saved, read and removed based on keys known
     * to clients. Keys are unique within a namespace, namespace identifier is passed as
     * a parameter to all operations.
     *
     * AsyncStorage is primarily intended for event-loop based applications.
     *
     * Asynchronous functions taking const reference parameters do not assume that the
     * references are valid after the function returns. The underlying implementation will
     * make copies of the given parameter when needed.
     *
     * @see fd
     * @see handleEvents
     *
     * @note The same instance of AsyncStorage must not be shared between multiple
     *       threads without explicit application level locking.
     *
     * @see SyncStorage for synchronous interface.
     */
    class AsyncStorage
    {
    public:
        AsyncStorage(const AsyncStorage&) = delete;

        AsyncStorage& operator = (const AsyncStorage&) = delete;

        AsyncStorage(AsyncStorage&&) = delete;

        AsyncStorage& operator = (AsyncStorage&&) = delete;

        virtual ~AsyncStorage() = default;

        using Namespace = std::string;

        /**
         * Separator character is used by the API internally in namespace handling. Thus, separator
         * character cannot be part of the namespace identifier string provided by the client.
         * Also namespace identifier cannot be an empty string.
         *
         * API otherwise does not impose additional restrictions to the characters used in namespace
         * identifiers. Excessive and unnecessary usage of special characters is strongly discouraged
         * though.
         */
        static constexpr char SEPARATOR = ',';

        /**
         * Get the file descriptor to monitor in application's event loop. The file descriptor
         * can be monitored for example with <code>select(2)</code>, <code>poll(2)</code> or
         * <code>epoll_wait(2)</code>. The event to be monitored is <i>input</i> event (like
         * for example <code>POLLIN</code>).
         *
         * Whenever there is input event in the file descriptor, application must call
         * handleEvents() function. Application must keep monitoring events until all desired
         * acknowledgement functions have been invoked. Application must stop monitoring events
         * before destroying <code>this</code>.
         *
         * @return The file descriptor to monitor in application's event loop.
         *
         * @see handleEvents
         */
        virtual int fd() const = 0;

        /**
         * Function to be called whenever there is an input event in the file descriptor
         * returned by fd() function. Each call to handleEvents() may invoke zero or more
         * acknowledgement functions.
         *
         * @see fd
         */
        virtual void handleEvents() = 0;

        /**
         * Ready acknowledgement to be called when shared data layer storage is ready to serve.
         *
         * @param error Error code describing the status of the request. The <code>std::error_code::category()</code>
         *              and <code>std::error_code::value()</code> are implementation specific. If ReadyAck
         *              is passed an error, client is advised to instantiate a new AsyncStorage and
         *              waitReadyAsync again.
         */
        using ReadyAck = std::function<void(const std::error_code& error)>;

        /**
         * Wait for the service to become ready to serve. There typically is a waiting period
         * when product cluster is brought up (commissioning, VNFC restart etc.). The function
         * can be called and used to synchronize the application startup with the readiness of
         * the underlying data storage.
         *
         * In a steady state, the callback is called almost immediately, varying based on runtime
         * conditions.
         *
         * @param ns Namespace under which this operation is targeted. As it is possible to
         *           configure for each namespace whether DB backend is in use, it is neccasary
         *           to provide namespace for this call. If it is known that DB backend usage
         *           configuration is the same for all used namespaces, this call can be done only
         *           once for certain namespace. If the configuration can vary between namespaces,
         *           it is recommended to call this always when starting to use new namespace.
         * @param readyAck The acknowledgement to be called once the request has been handled.
         *                 The given function is called in the context of handleEvents() function.
         *
         * @note Each instance/namespace should be waited on only *once*.
         */
        virtual void waitReadyAsync(const Namespace& ns,
                                    const ReadyAck& readyAck) = 0;

        using Key = std::string;

        using Data = std::vector<uint8_t>;

        using DataMap = std::map<Key, Data>;

        /**
         * Modify acknowledgement to be called when setAsync/removeAsync/removeAllAsync request has been
         * handled.
         *
         * @param error Error code describing the status of the request. The <code>std::error_code::category()</code>
         *              and <code>std::error_code::value()</code> are implementation specific. Client is advised
         *              to compare received error against <code>shareddatalayer::Error</code> constants when
         *              doing error handling. See documentation: sdl/errorqueries.hpp for further information.
         *              Received <code>std::error_code</code> and <code>std::error_code::message()</code> can be stored
         *              and provided to shareddatalayer developers if problem needs further investigation.
         */
        using ModifyAck = std::function<void(const std::error_code& error)>;

        /**
         * Write data to shared data layer storage. Writing is done atomically, i.e. either
         * all succeeds or all fails.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param dataMap Data to be written.
         * @param modifyAck The acknowledgement to be called once the request has been handled.
         *                  The given function is called in the context of handleEvents() function.
         */
        virtual void setAsync(const Namespace& ns,
                              const DataMap& dataMap,
                              const ModifyAck& modifyAck) = 0;

        /**
         * Modify acknowledgement to be called when setIfAsync/setIfNotExistsAsync/removeIfAsync request has been handled.
         *
         * @param error Error code describing the status of the request. The <code>std::error_code::category()</code>
         *              and <code>std::error_code::value()</code> are implementation specific. Client is advised
         *              to compare received error against <code>shareddatalayer::Error</code> constants when
         *              doing error handling. See documentation: sdl/errorqueries.hpp for further information.
         *              Received <code>std::error_code</code> and <code>std::error_code::message()</code> can be stored
         *              and provided to shareddatalayer developers if problem needs further investigation.
         * @param status Status of the modification. True for successful modification, false if modification
         *               was not done due to the given prerequisite.
         */
        using ModifyIfAck = std::function<void(const std::error_code& error, bool status)>;

        /**
         * Conditionally modify the value of a key if the current value in data storage
         * matches the user's last known value.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param key Key for which data modification will be executed.
         * @param oldData Last known data.
         * @param newData Data to be written.
         * @param modifyIfAck The acknowledgement to be called once the request has been handled.
         *                    The given function is called in the context of handleEvents() function.
         */
        virtual void setIfAsync(const Namespace& ns,
                                const Key& key,
                                const Data& oldData,
                                const Data& newData,
                                const ModifyIfAck& modifyIfAck) = 0;

        /**
         * Conditionally set the value of a key. If key already exists, then it's value
         * is not modified. Checking the key existence and potential set operation is done
         * as a one atomic operation.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param key Key.
         * @param data Data to be written.
         * @param modifyIfAck The acknowledgement to be called once the request has been handled.
         *                    The given function is called in the context of handleEvents() function.
         */
        virtual void setIfNotExistsAsync(const Namespace& ns,
                                         const Key& key,
                                         const Data& data,
                                         const ModifyIfAck& modifyIfAck) = 0;

        using Keys = std::set<Key>;

        /**
         * Read acknowledgement to be called when getAsync request has been handled.
         *
         * @param error Error code describing the status of the request. The <code>std::error_code::category()</code>
         *              and <code>std::error_code::value()</code> are implementation specific. Client is advised
         *              to compare received error against <code>shareddatalayer::Error</code> constants when
         *              doing error handling. See documentation: sdl/errorqueries.hpp for further information.
         *              Received <code>std::error_code</code> and <code>std::error_code::message()</code> can be stored
         *              and provided to shareddatalayer developers if problem needs further investigation.
         * @param dataMap Data from the storage. Empty container is returned in case of error.
         */
        using GetAck = std::function<void(const std::error_code& error, const DataMap& dataMap)>;

        /**
         * Read data from shared data layer storage. Only those entries that are found will
         * be returned.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param keys Data to be read.
         * @param getAck The acknowledgement to be called once the request has been handled.
         *               The given function is called in the context of handleEvents() function.
         */
        virtual void getAsync(const Namespace& ns,
                              const Keys& keys,
                              const GetAck& getAck) = 0;

        /**
         * Remove data from shared data layer storage. Existing keys are removed. Removing
         * is done atomically, i.e. either all succeeds or all fails.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param keys Data to be removed.
         * @param modifyAck The acknowledgement to be called once the request has been handled.
         *                  The given function is called in the context of handleEvents() function.
         */
        virtual void removeAsync(const Namespace& ns,
                                 const Keys& keys,
                                 const ModifyAck& modifyAck) = 0;

        /**
         * Conditionally remove data from shared data layer storage if the current data value
         * matches the user's last known value.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param key Data to be removed
         * @param data Last known value of data
         * @param modifyIfAck The acknowledgement to be called once the request has been handled.
         *                    The given function is called in the context of handleEvents() function.
         */
        virtual void removeIfAsync(const Namespace& ns,
                                   const Key& key,
                                   const Data& data,
                                   const ModifyIfAck& modifyIfAck) = 0;

        /**
         * Read acknowledgement to be called when findKeysAsync request has been handled.
         *
         * @param error Error code describing the status of the request. The <code>std::error_code::category()</code>
         *              and <code>std::error_code::value()</code> are implementation specific. Client is advised
         *              to compare received error against <code>shareddatalayer::Error</code> constants when
         *              doing error handling. See documentation: sdl/errorqueries.hpp for further information.
         *              Received <code>std::error_code</code> and <code>std::error_code::message()</code> can be stored
         *              and provided to shareddatalayer developers if problem needs further investigation.
         * @param keys Found keys.
         */
        using FindKeysAck = std::function<void(const std::error_code& error, const Keys& keys)>;

        /**
         * Find all keys matching search key prefix under the namespace. No prior knowledge about the keys in the given
         * namespace exists, thus operation is not guaranteed to be atomic or isolated.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param keyPrefix Only keys starting with given keyPrefix are returned. Passing empty string as
         *                  keyPrefix will return all keys.
         * @param findKeysAck The acknowledgement to be called once the request has been handled.
         *                    The given function is called in the context of handleEvents() function.
         */
        [[deprecated("Use listKeys() instead.")]]
        virtual void findKeysAsync(const Namespace& ns,
                                   const std::string& keyPrefix,
                                   const FindKeysAck& findKeysAck) = 0;

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
         * @param findKeysAck The acknowledgement to be called once the request has been handled.
         *                    The given function is called in the context of handleEvents() function.
         */
        virtual void listKeys(const Namespace& ns,
                              const std::string& pattern,
                              const FindKeysAck& findKeysAck) = 0;

        /**
         * Remove all keys under the namespace. Found keys are removed atomically, i.e.
         * either all succeeds or all fails.
         *
         * @param ns Namespace under which this operation is targeted.
         * @param modifyAck The acknowledgement to be called once the request has been handled.
         *                  The given function is called in the context of handleEvents() function.
         */
        virtual void removeAllAsync(const Namespace& ns,
                                    const ModifyAck& modifyAck) = 0;

        /**
         * Create a new instance of AsyncStorage.
         *
         * @return New instance of AsyncStorage.
         */
        static std::unique_ptr<AsyncStorage> create();

    protected:
        AsyncStorage() = default;
    };
}

#endif
