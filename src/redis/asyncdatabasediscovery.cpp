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

#include "private/redis/asyncdatabasediscovery.hpp"
#include "private/databaseconfiguration.hpp"
#include "private/logger.hpp"
#include <cstdlib>
#include "config.h"
#if HAVE_HIREDIS
#include "private/redis/asynchiredisdatabasediscovery.hpp"
#endif
#include "private/abort.hpp"

using namespace shareddatalayer::redis;

std::shared_ptr<AsyncDatabaseDiscovery> AsyncDatabaseDiscovery::create(std::shared_ptr<Engine> engine,
                                                                       const boost::optional<Namespace>& ns,
                                                                       const DatabaseConfiguration& staticDatabaseConfiguration,
                                                                       std::shared_ptr<Logger> logger)
{
    auto staticAddresses(staticDatabaseConfiguration.getServerAddresses());

    if (staticAddresses.empty())
        staticAddresses = staticDatabaseConfiguration.getDefaultServerAddresses();

    auto staticDbType(staticDatabaseConfiguration.getDbType());

    if (staticDbType == DatabaseConfiguration::DbType::REDIS_CLUSTER)
#if HAVE_HIREDIS_VIP
        return std::make_shared<AsyncHiredisDatabaseDiscovery>(engine,
                                                               ns,
                                                               DatabaseInfo::Type::CLUSTER,
                                                               staticAddresses,
                                                               logger);
#else
        SHAREDDATALAYER_ABORT("No Hiredis vip for Redis cluster configuration");
#endif
    else
#if HAVE_HIREDIS
        return std::make_shared<AsyncHiredisDatabaseDiscovery>(engine,
                                                               ns,
                                                               DatabaseInfo::Type::SINGLE,
                                                               staticAddresses,
                                                               logger);
#else
        static_cast<void>(logger);
        SHAREDDATALAYER_ABORT("No Hiredis");
#endif
}

