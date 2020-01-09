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

/**
 * @mainpage
 *
 * @section introduction_sec Introduction
 *
 * Shared Data Layer provides a lightweight, high-speed interface for accessing shared
 * data storage. The purpose is to enable utilizing clients to become stateless,
 * conforming with, e.g., the requirements of the fifth generation mobile networks.
 *
 * @section concepts_sec Concepts
 *
 * @subsection namespace_sec Namespace
 *
 * A shared data layer connection is instantiated to a namespace and data is always
 * read and modified within the namespace. Namespace provides data isolation across
 * clients.
 *
 * Namespace management is planned to be moved under a managing entity, a shared data
 * layer manager of sort, which handles the namespaces within a cluster, enforcing
 * some control over how the namespaces are created, utilized and load balanced as well
 * as dynamically scaling the underlying data storage resources. For now, however,
 * namespace naming needs to be manually coordinated between clients.
 *
 * @subsection data_sec Keys and data
 *
 * Clients save key-data pairs. Data is passed as byte vectors, any structure that
 * this data may have (e.g. a serialized JSON) is meaningful only to the client itself.
 * Clients are responsible for managing the keys within a namespace.
 *
 * @section compiling_clients_sec Compiling clients using shareddatalayer APIs
 *
 * The necessary compilation flags and libraries needed for compiling using shareddatalayer
 * APIs can be acquired with the <code>pkg-config</code> tool:
 *
 * \code
 * pkg-config --cflags libsdl
 * pkg-config --libs libsdl
 * \endcode
 *
 * @section usage_sec Usage
 *
 * @subsection async_api_sec Asynchronous API
 *
 * shareddatalayer::AsyncStorage API is used to access SDL data asynchronously.
 *
 * Instance is created with shareddatalayer::AsyncStorage::create() function.
 *
 * Functions shareddatalayer::AsyncStorage::fd() and shareddatalayer::AsyncStorage::handleEvents()
 * are used to integrate SDL to client's event loop.
 *
 * Optionally client may verify/wait that SDL (the database service in practice) is ready
 * with shareddatalayer::AsyncStorage::waitReadyAsync() function.
 *
 * @subsection sync_api_sec Synchronous API
 *
 * shareddatalayer::SyncStorage API is used to access SDL data synchronously.
 *
 * Functionally it is similar to AsyncStorage, with an exception that functions are blocking while
 * waiting response(s) from database.
 *
 * @subsection unit_testing_sec Unit Testing
 *
 * SDL provides helper classes for clients to mock SDL APIs.
 *
 * shareddatalayer::tst::MockableAsyncStorage to mock shareddatalayer::AsyncStorage API.
 *
 * shareddatalayer::tst::MockableSyncStorage to mock shareddatalayer::SyncStorage API.
 *
 */
