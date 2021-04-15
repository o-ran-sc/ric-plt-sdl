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

#ifndef SHAREDDATALAYER_CONFIGURATIONREADER_HPP_
#define SHAREDDATALAYER_CONFIGURATIONREADER_HPP_

#define DB_HOST_ENV_VAR_NAME "DBAAS_SERVICE_HOST"
#define DB_PORT_ENV_VAR_NAME "DBAAS_SERVICE_PORT"
#define SENTINEL_PORT_ENV_VAR_NAME "DBAAS_SERVICE_SENTINEL_PORT"
#define SENTINEL_MASTER_NAME_ENV_VAR_NAME "DBAAS_MASTER_NAME"
#define DB_CLUSTER_ADDR_LIST_ENV_VAR_NAME "DBAAS_CLUSTER_ADDR_LIST"

#include <iosfwd>
#include <string>
#include <unordered_map>
#include <boost/property_tree/ptree.hpp>
#include <sdl/exception.hpp>
#include "private/configurationpaths.hpp"
#include "private/logger.hpp"

namespace shareddatalayer
{
    class DatabaseConfiguration;
    class NamespaceConfigurations;
    class System;

    class ConfigurationReader
    {
    public:
        explicit ConfigurationReader(std::shared_ptr<Logger> logger);

        ConfigurationReader(const Directories& directories,
                            System& system,
                            std::shared_ptr<Logger> logger);

        ~ConfigurationReader();

        void readDatabaseConfiguration(DatabaseConfiguration& databaseConfiguration);

        void readNamespaceConfigurations(NamespaceConfigurations& namespaceConfigurations);

        /**
         * Overrides existing json configuration with json format input stream given as
         * parameter. Does not override existing configration if existing configration
         * originates from environment variable. Meant for UT usage.
         */
        void readConfigurationFromInputStream(const std::istream& input);

    private:
        const std::string dbHostEnvVariableName;
        std::string dbHostEnvVariableValue;
        const std::string dbPortEnvVariableName;
        std::string dbPortEnvVariableValue;
        const std::string sentinelPortEnvVariableName;
        std::string sentinelPortEnvVariableValue;
        const std::string sentinelMasterNameEnvVariableName;
        std::string sentinelMasterNameEnvVariableValue;
        const std::string dbClusterAddrListEnvVariableName;
        std::string dbClusterAddrListEnvVariableValue;
        boost::optional<boost::property_tree::ptree> jsonDatabaseConfiguration;
        std::string sourceForDatabaseConfiguration;
        std::unordered_map<std::string, std::pair<boost::property_tree::ptree, std::string>> jsonNamespaceConfigurations;
        std::shared_ptr<Logger> logger;

        void readConfigurationFromDirectories(const Directories& directories);

        template<typename T>
        void readConfiguration(T& input, const std::string& sourceName);
    };
}

#endif
