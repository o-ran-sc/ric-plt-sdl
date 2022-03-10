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

#ifndef SHAREDDATALAYER_DATABASECONFIGURATIONIMPL_HPP_
#define SHAREDDATALAYER_DATABASECONFIGURATIONIMPL_HPP_

#include "private/databaseconfiguration.hpp"

namespace shareddatalayer
{
    class DatabaseConfigurationImpl: public DatabaseConfiguration
    {
    public:
        DatabaseConfigurationImpl();

        ~DatabaseConfigurationImpl() override;

        void checkAndApplyDbType(const std::string& type) override;

        void checkAndApplyServerAddress(const std::string& address) override;

        void checkAndApplySentinelPorts(const std::string& sentinelPortsEnvStr) override;

        void checkAndApplySentinelMasterNames(const std::string& sentinelMasterNamesEnvStr) override;

        DatabaseConfiguration::DbType getDbType() const override;

        DatabaseConfigurationImpl::Addresses getServerAddresses() const override;

        DatabaseConfigurationImpl::Addresses getServerAddresses(const boost::optional<std::size_t>& addressIndex) const override;

        DatabaseConfiguration::Addresses getDefaultServerAddresses() const override;

        boost::optional<HostAndPort> getSentinelAddress(const boost::optional<std::size_t>& addressIndex) const override;

        std::string getSentinelMasterName(const boost::optional<std::size_t>& addressIndex) const override;

        bool isEmpty() const override;

    private:
        DbType dbType;
        Addresses serverAddresses;
        SentinelPorts sentinelPorts;
        SentinelMasterNames sentinelMasterNames;
    };
}

#endif
