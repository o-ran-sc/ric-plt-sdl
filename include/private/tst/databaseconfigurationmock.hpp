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

#ifndef SHAREDDATALAYER_DATABASECONFIGURATIONMOCK_HPP_
#define SHAREDDATALAYER_DATABASECONFIGURATIONMOCK_HPP_

#include "private/databaseconfiguration.hpp"

namespace shareddatalayer
{
    namespace tst
    {
        class DatabaseConfigurationMock: public DatabaseConfiguration
        {
        public:
            MOCK_METHOD1(checkAndApplyDbType, void(const std::string& type));
            MOCK_METHOD1(checkAndApplyServerAddress, void(const std::string& address));
            MOCK_METHOD1(checkAndApplySentinelAddress, void(const std::string& address));
            MOCK_METHOD1(checkAndApplySentinelMasterName, void(const std::string& name));
            MOCK_CONST_METHOD0(getDbType, DatabaseConfiguration::DbType());
            MOCK_CONST_METHOD0(getServerAddresses, DatabaseConfiguration::Addresses());
            MOCK_CONST_METHOD0(getDefaultServerAddresses, DatabaseConfiguration::Addresses());
            MOCK_CONST_METHOD0(isEmpty, bool());
            MOCK_CONST_METHOD0(getSentinelAddress, boost::optional<HostAndPort>());
            MOCK_CONST_METHOD0(getSentinelMasterName, std::string());
        };
    }
}

#endif
