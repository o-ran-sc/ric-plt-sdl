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

#include "private/databaseconfigurationimpl.hpp"
#include <arpa/inet.h>
#include <boost/crc.hpp>

using namespace shareddatalayer;

namespace
{
    const std::string& getDefaultHost()
    {
        static const std::string defaultHost("localhost");
        return defaultHost;
    }

    const uint16_t DEFAULT_PORT(6379U);
    const uint16_t DEFAULT_SENTINEL_PORT(26379U);
}

DatabaseConfigurationImpl::DatabaseConfigurationImpl():
    dbType(DbType::UNKNOWN)
{
}

DatabaseConfigurationImpl::~DatabaseConfigurationImpl()
{
}

void DatabaseConfigurationImpl::checkAndApplyDbType(const std::string& type)
{
    if (type == "redis-standalone")
        dbType = DatabaseConfiguration::DbType::REDIS_STANDALONE;
    else if (type == "redis-cluster")
        dbType = DatabaseConfiguration::DbType::REDIS_CLUSTER;
    else if (type == "redis-sentinel")
        dbType = DatabaseConfiguration::DbType::REDIS_SENTINEL;
    else if (type == "sdl-standalone-cluster")
        dbType = DatabaseConfiguration::DbType::SDL_STANDALONE_CLUSTER;
    else if (type == "sdl-sentinel-cluster")
        dbType = DatabaseConfiguration::DbType::SDL_SENTINEL_CLUSTER;
    else
        throw DatabaseConfiguration::InvalidDbType(type);
}

DatabaseConfiguration::DbType DatabaseConfigurationImpl::getDbType() const
{
   return dbType;
}

void DatabaseConfigurationImpl::checkAndApplyServerAddress(const std::string& address)
{
    serverAddresses.push_back(HostAndPort(address, htons(DEFAULT_PORT)));
}

bool DatabaseConfigurationImpl::isEmpty() const
{
    return serverAddresses.empty();
}

DatabaseConfiguration::Addresses DatabaseConfigurationImpl::getServerAddresses() const
{
    return serverAddresses;
}

DatabaseConfiguration::Addresses DatabaseConfigurationImpl::getServerAddresses(const boost::optional<std::size_t>& addressIndex) const
{
    if (addressIndex)
        return { HostAndPort(serverAddresses.at(*addressIndex)) };

    return serverAddresses;
}

DatabaseConfiguration::Addresses DatabaseConfigurationImpl::getDefaultServerAddresses() const
{
    return { HostAndPort(getDefaultHost(), htons(DEFAULT_PORT)) };
}

void DatabaseConfigurationImpl::checkAndApplySentinelAddress(const std::string& address)
{
    sentinelAddress = HostAndPort(address, htons(DEFAULT_SENTINEL_PORT));
}

boost::optional<HostAndPort> DatabaseConfigurationImpl::getSentinelAddress() const
{
    return sentinelAddress;
}

boost::optional<HostAndPort> DatabaseConfigurationImpl::getSentinelAddress(const boost::optional<std::size_t>& addressIndex) const
{
    if (addressIndex)
        return { HostAndPort(serverAddresses.at(*addressIndex).getHost(), sentinelAddress->getPort()) };

    return getSentinelAddress();
}

void DatabaseConfigurationImpl::checkAndApplySentinelMasterName(const std::string& name)
{
    sentinelMasterName = name;
}

std::string DatabaseConfigurationImpl::getSentinelMasterName() const
{
    return sentinelMasterName;
}
