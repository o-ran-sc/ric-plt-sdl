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

#ifndef SHAREDDATALAYER_REDIS_ASYNCRCPDATABASEDISCOVERY_HPP_
#define SHAREDDATALAYER_REDIS_ASYNCRCPDATABASEDISCOVERY_HPP_

#include "private/redis/asyncdatabasediscovery.hpp"
#include "private/redis/databaseinfo.hpp"
#include "private/logger.hpp"
#include <rcpdbproxy/databasediscovery.hpp>
#include <string>
#include <memory>
#include <unordered_map>
#include <boost/optional.hpp>

namespace shareddatalayer
{
    class Engine;

    namespace redis
    {
        class AsyncRcpDatabaseDiscovery: public AsyncDatabaseDiscovery
        {
        public:
            AsyncRcpDatabaseDiscovery(const AsyncRcpDatabaseDiscovery&) = delete;

            AsyncRcpDatabaseDiscovery& operator = (const AsyncRcpDatabaseDiscovery&) = delete;

            explicit AsyncRcpDatabaseDiscovery(std::shared_ptr<Engine> engine, const boost::optional<Namespace>& ns, std::shared_ptr<Logger> logger);

            ~AsyncRcpDatabaseDiscovery() override;

            void setStateChangedCb(const StateChangedCb& stateChangedCb) override;

            void clearStateChangedCb() override;

        private:
            std::shared_ptr<Engine> engine;
            const boost::optional<std::string> ns;
            std::shared_ptr<dbservicediscovery::DatabaseDiscovery> dbDiscovery;
            int discoveryFd;
            std::unordered_map<db_sd_session_type, DatabaseInfo::Type> typeMap { { DB_SD_SESSION_TYPE_2N, DatabaseInfo::Type::REDUNDANT },
                                                                               { DB_SD_SESSION_TYPE_CLUSTER, DatabaseInfo::Type::CLUSTER } };
            redis::DatabaseInfo currentDatabaseInfo;
            std::shared_ptr<Logger> logger;

            void stateChangedCb(const dbservicediscovery::DatabaseDiscovery::Info& info,
                                const AsyncRcpDatabaseDiscovery::StateChangedCb& stateChangedCb);

            void eventHandler();
        };
    }
}

#endif
