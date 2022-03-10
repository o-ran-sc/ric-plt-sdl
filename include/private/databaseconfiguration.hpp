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

#ifndef SHAREDDATALAYER_DATABASECONFIGURATION_HPP_
#define SHAREDDATALAYER_DATABASECONFIGURATION_HPP_

#include <string>
#include <vector>
#include <boost/optional.hpp>
#include "private/hostandport.hpp"

namespace shareddatalayer
{
    class DatabaseConfiguration
    {
    public:
        class InvalidDbType;
        using Addresses = std::vector<HostAndPort>;
        using SentinelPorts = std::vector<uint16_t>;
        using SentinelMasterNames = std::vector<std::string>;
        enum class DbType
        {
            UNKNOWN = 0,
            REDIS_STANDALONE,
            REDIS_CLUSTER,
            REDIS_SENTINEL,
            SDL_STANDALONE_CLUSTER,
            SDL_SENTINEL_CLUSTER
        };

        virtual ~DatabaseConfiguration() = default;
        virtual void checkAndApplyDbType(const std::string& type) = 0;
        virtual void checkAndApplyServerAddress(const std::string& address) = 0;
        virtual void checkAndApplySentinelPorts(const std::string& sentinelPortsEnvStr) = 0;
        virtual void checkAndApplySentinelMasterNames(const std::string& sentinelMasterNamesEnvStr) = 0;
        virtual DatabaseConfiguration::DbType getDbType() const = 0;
        virtual DatabaseConfiguration::Addresses getServerAddresses() const = 0;
        virtual DatabaseConfiguration::Addresses getServerAddresses(const boost::optional<std::size_t>& addressIndex) const = 0;
        virtual DatabaseConfiguration::Addresses getDefaultServerAddresses() const = 0;
        virtual boost::optional<HostAndPort> getSentinelAddress(const boost::optional<std::size_t>& addressIndex) const = 0;
        virtual std::string getSentinelMasterName(const boost::optional<std::size_t>& addressIndex) const = 0;
        virtual bool isEmpty() const = 0;

        DatabaseConfiguration(DatabaseConfiguration&&) = delete;
        DatabaseConfiguration(const DatabaseConfiguration&) = delete;
        DatabaseConfiguration& operator = (DatabaseConfiguration&&) = delete;
        DatabaseConfiguration& operator = (const DatabaseConfiguration&) = delete;

    protected:
        DatabaseConfiguration() = default;
    };

    class DatabaseConfiguration::InvalidDbType: public Exception
    {
    public:
        InvalidDbType() = delete;

        explicit InvalidDbType(const std::string& type);
    };
}

#endif
