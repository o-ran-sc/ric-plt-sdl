/*
   Copyright (c) 2018-2022 Nokia.

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

#include <cstdlib>
#include "config.h"
#include "private/abort.hpp"
#include "private/databaseconfiguration.hpp"
#include "private/logger.hpp"
#include "private/redis/asyncdatabasediscovery.hpp"
#if HAVE_HIREDIS
#include "private/redis/asynchiredisdatabasediscovery.hpp"
#endif
#include "private/redis/asyncsentineldatabasediscovery.hpp"

using namespace shareddatalayer::redis;

std::shared_ptr<AsyncDatabaseDiscovery> AsyncDatabaseDiscovery::create(std::shared_ptr<Engine> engine,
                                                                       const boost::optional<Namespace>& ns,
                                                                       const DatabaseConfiguration& staticDatabaseConfiguration,
                                                                       const boost::optional<std::size_t>& addressIndex,
                                                                       std::shared_ptr<Logger> logger)
{
    auto staticAddresses(staticDatabaseConfiguration.getServerAddresses(addressIndex));

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
    {
#if HAVE_HIREDIS
        if (staticDbType == DatabaseConfiguration::DbType::REDIS_SENTINEL ||
            staticDbType == DatabaseConfiguration::DbType::SDL_SENTINEL_CLUSTER)
        {
            auto sentinelAddress(staticDatabaseConfiguration.getSentinelAddress(addressIndex));
            auto sentinelMasterName(staticDatabaseConfiguration.getSentinelMasterName(addressIndex));

            if (sentinelAddress)
                return std::make_shared<AsyncSentinelDatabaseDiscovery>(engine,
                                                                        logger,
                                                                        *sentinelAddress,
                                                                        sentinelMasterName);
            else
                SHAREDDATALAYER_ABORT("Sentinel address not configured.");
        }
        else
        {
            return std::make_shared<AsyncHiredisDatabaseDiscovery>(engine,
                                                                   ns,
                                                                   DatabaseInfo::Type::SINGLE,
                                                                   staticAddresses,
                                                                   logger);
        }
#else
        static_cast<void>(logger);
        SHAREDDATALAYER_ABORT("No Hiredis");
#endif
    }
}
