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

#include "private/asyncconnection.hpp"
#include "private/redis/asynchiredisdatabasediscovery.hpp"
#include "private/engine.hpp"
#include "private/logger.hpp"
#include "private/redis/asynccommanddispatcher.hpp"
#include "private/redis/contentsbuilder.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;

AsyncHiredisDatabaseDiscovery::AsyncHiredisDatabaseDiscovery(std::shared_ptr<Engine> engine,
                                                             const boost::optional<std::string>& ns,
                                                             DatabaseInfo::Type databaseType,
                                                             const DatabaseConfiguration::Addresses& databaseAddresses,
                                                             std::shared_ptr<Logger> logger):
    engine(engine),
    ns(ns),
    databaseType(databaseType),
    databaseAddresses(databaseAddresses),
    logger(logger)
{
}

void AsyncHiredisDatabaseDiscovery::setStateChangedCb(const StateChangedCb& stateChangedCb)
{
    /* Note: Current implementation does not monitor possible database configuration changes.
     * If that is needed, we would probably need to monitor json configuration file changes.
     */
    const DatabaseInfo databaseInfo = { databaseAddresses, databaseType, ns, DatabaseInfo::Discovery::HIREDIS };
    engine->postCallback(std::bind(stateChangedCb,
                                   databaseInfo));
}

void AsyncHiredisDatabaseDiscovery::clearStateChangedCb()
{
}
