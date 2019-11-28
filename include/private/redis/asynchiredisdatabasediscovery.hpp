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

#ifndef SHAREDDATALAYER_REDIS_ASYNCHIREDISDATABASEDISCOVERY_HPP_
#define SHAREDDATALAYER_REDIS_ASYNCHIREDISDATABASEDISCOVERY_HPP_

#include "private/redis/asyncdatabasediscovery.hpp"
#include "private/redis/databaseinfo.hpp"
#include "private/databaseconfiguration.hpp"
#include "private/logger.hpp"
#include <string>
#include <memory>

namespace shareddatalayer
{
    class Engine;

    namespace redis
    {
        class AsyncHiredisDatabaseDiscovery: public AsyncDatabaseDiscovery
        {
        public:
            AsyncHiredisDatabaseDiscovery(const AsyncHiredisDatabaseDiscovery&) = delete;

            AsyncHiredisDatabaseDiscovery& operator = (const AsyncHiredisDatabaseDiscovery&) = delete;

            AsyncHiredisDatabaseDiscovery(std::shared_ptr<Engine> engine,
                                          const boost::optional<std::string>& ns,
                                          DatabaseInfo::Type databaseType,
                                          const DatabaseConfiguration::Addresses& databaseAddresses,
                                          std::shared_ptr<Logger> logger);

            ~AsyncHiredisDatabaseDiscovery() override = default;

            // Note: Current implementation does not monitor possible cluster configuration changes.
            void setStateChangedCb(const StateChangedCb& stateChangedCb) override;

            void clearStateChangedCb() override;

            void setConnected(bool state);

        private:
            std::shared_ptr<Engine> engine;
            const boost::optional<std::string> ns;
            const DatabaseInfo::Type databaseType;
            const DatabaseConfiguration::Addresses databaseAddresses;
            std::shared_ptr<Logger> logger;
        };
    }
}

#endif
