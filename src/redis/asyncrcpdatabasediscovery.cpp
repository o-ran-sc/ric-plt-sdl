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

#include "private/redis/asyncrcpdatabasediscovery.hpp"
#include <cstdlib>
#include <sstream>
#include <arpa/inet.h>
#include "private/abort.hpp"
#include "private/createlogger.hpp"
#include "private/engine.hpp"
#include "private/logger.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;

AsyncRcpDatabaseDiscovery::AsyncRcpDatabaseDiscovery(std::shared_ptr<Engine> engine,
                                                     const boost::optional<Namespace>& ns,
                                                     std::shared_ptr<Logger> logger):
    engine(engine),
    ns(ns),
    /** @todo: Figure out how to configure dbservicediscovery::DatabaseDiscovery (dbname/dbtype/other). */
    dbDiscovery(dbservicediscovery::DatabaseDiscovery::create(dbservicediscovery::DatabaseDiscovery::Parameters())),
    discoveryFd(dbDiscovery->fd()),
    currentDatabaseInfo({}),
    logger(logger)
{
    engine->addMonitoredFD(discoveryFd,
                           Engine::EVENT_IN,
                           std::bind(&AsyncRcpDatabaseDiscovery::eventHandler,
                                     this));
}

AsyncRcpDatabaseDiscovery::~AsyncRcpDatabaseDiscovery()
{
    engine->deleteMonitoredFD(discoveryFd);
}

void AsyncRcpDatabaseDiscovery::eventHandler()
{
    dbDiscovery->handleEvents();
}

void AsyncRcpDatabaseDiscovery::setStateChangedCb(const StateChangedCb& stateChangedCb)
{
    dbDiscovery->setStateChangedCb(std::bind(&AsyncRcpDatabaseDiscovery::stateChangedCb,
                                             this,
                                             std::placeholders::_1,
                                             stateChangedCb));
}

void AsyncRcpDatabaseDiscovery::clearStateChangedCb()
{
    dbDiscovery->clearStateChangedCb();
}

void AsyncRcpDatabaseDiscovery::stateChangedCb(const dbservicediscovery::DatabaseDiscovery::Info& info,
                                               const AsyncRcpDatabaseDiscovery::StateChangedCb& stateChangedCb)
{
    DatabaseInfo newDatabaseInfo;
    for (const auto& i : info.hosts)
        newDatabaseInfo.hosts.push_back({ i.address, htons(i.port) });
    try
    {
        newDatabaseInfo.type = typeMap.at(info.dbSessionType);
    }
    catch (const std::out_of_range&)
    {
        logger->error() << "unknown database type received: " << info.dbSessionType
                        << ", aborting";
        SHAREDDATALAYER_ABORT("Unknown database type.");
    }
    newDatabaseInfo.ns = ns;
    newDatabaseInfo.discovery = DatabaseInfo::Discovery::RCP;

    /* calling stateChangedCb triggers dispacther re-creation. In standalone deployment there is no need
     * to recreate dispatcher if database configuration does not change since dispacther will start
     * reconnecting when connection is disconnected. Cluster dispacther needs to be recreated even
     * if configuration does not change since cluster dispacther does no reconnecting from disconnect
     * callback (as it is node specific). hiredis_vip requires that context is recreated and re-connected
     * if whole cluster goes down and gets back up.
     */
    if (newDatabaseInfo.type == DatabaseInfo::Type::CLUSTER || newDatabaseInfo != currentDatabaseInfo)
        stateChangedCb(newDatabaseInfo);
    else
        logger->debug() << "AsyncRcpDatabaseDiscovery: stateChanged callback received but database info did not change";

    currentDatabaseInfo = newDatabaseInfo;
}
