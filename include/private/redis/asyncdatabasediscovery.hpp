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

#ifndef SHAREDDATALAYER_REDIS_ASYNCDATABASEDISCOVERY_HPP_
#define SHAREDDATALAYER_REDIS_ASYNCDATABASEDISCOVERY_HPP_

#include <functional>
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <boost/optional.hpp>
#include "private/logger.hpp"

namespace shareddatalayer
{
    class Engine;
    class DatabaseConfiguration;

    namespace redis
    {
        struct DatabaseInfo;

        class AsyncDatabaseDiscovery
        {
        public:
            AsyncDatabaseDiscovery(const AsyncDatabaseDiscovery&) = delete;

            AsyncDatabaseDiscovery& operator = (const AsyncDatabaseDiscovery&) = delete;

            virtual ~AsyncDatabaseDiscovery() = default;

            using StateChangedCb = std::function<void(const DatabaseInfo&)>;

            /**
             * Register a callback to be invoked whenever connection information changes. Client would tear down
             * its current connection and setup a new one using the information that was passed to the callback.
             *
             * @note connection state changes (e.g. connection down, switchover)) do not invoke callback if
             *       connection information does not change.
             *
             * @note When a callback is set, it should be scheduled right away (not called inside this function)
             *       if connection info is already available.
             *
             * @param stateChangedCb Callback to be invoked when the information of the database changes.
             */
            virtual void setStateChangedCb(const StateChangedCb& stateChangedCb) = 0;

            virtual void clearStateChangedCb() = 0;

            using Namespace = std::string;

            static std::shared_ptr<AsyncDatabaseDiscovery> create(std::shared_ptr<Engine> engine,
                                                                  const boost::optional<Namespace>& ns,
                                                                  const DatabaseConfiguration& staticDatabaseConfiguration,
                                                                  const boost::optional<std::size_t>& addressIndex,
                                                                  std::shared_ptr<Logger> logger);

        protected:
            AsyncDatabaseDiscovery() = default;
        };
    }
}

#endif
