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

#include "private/abort.hpp"
#include "private/configurationreader.hpp"
#include <boost/property_tree/json_parser.hpp>
#include <sdl/exception.hpp>
#include "private/createlogger.hpp"
#include "private/databaseconfiguration.hpp"
#include "private/logger.hpp"
#include "private/namespaceconfigurations.hpp"
#include "private/namespacevalidator.hpp"
#include "private/system.hpp"

using namespace shareddatalayer;

namespace
{
    template <typename T>
    T get(const boost::property_tree::ptree& ptree, const std::string& param, const std::string& sourceName)
    {
        try
        {
            return ptree.get<T>(param);
        }
        catch (const boost::property_tree::ptree_bad_path&)
        {
            std::ostringstream os;
            os << "Configuration error in " << sourceName << ": "
               << "missing \"" << param << '\"';
            throw Exception(os.str());
        }
        catch (const boost::property_tree::ptree_bad_data& e)
        {
            std::ostringstream os;
            os << "Configuration error in " << sourceName << ": "
               << "invalid \"" << param << "\": \"" << e.data<boost::property_tree::ptree::data_type>() << '\"';
            throw Exception(os.str());
        }
    }

    void validateAndSetDbType(const std::string& type, DatabaseConfiguration& databaseConfiguration,
                              const std::string& sourceName)
    {
        try
        {
            databaseConfiguration.checkAndApplyDbType(type);
        }
        catch (const std::exception& e)
        {
            std::ostringstream os;
            os << "Configuration error in " << sourceName << ": "
               << e.what();
            throw Exception(os.str());
        }
    }

    void validateAndSetDbServerAddress(const std::string& address, DatabaseConfiguration& databaseConfiguration,
                                       const std::string& sourceName)
    {
        try
        {
            databaseConfiguration.checkAndApplyServerAddress(address);
        }
        catch (const std::exception& e)
        {
            std::ostringstream os;
            os << "Configuration error in " << sourceName << ": "
               << "invalid \"address\": \"" << address << "\" " << e.what();
            throw Exception(os.str());
        }
    }

    void parseDatabaseServerConfiguration(DatabaseConfiguration& databaseConfiguration,
                                          const boost::property_tree::ptree& ptree,
                                          const std::string& sourceName)
    {
        const auto address(get<std::string>(ptree, "address", sourceName));
        validateAndSetDbServerAddress(address, databaseConfiguration, sourceName);
    }

    void parseDatabaseServersConfiguration(DatabaseConfiguration& databaseConfiguration,
                                           const boost::property_tree::ptree& ptree,
                                           const std::string& sourceName)
    {
        const auto servers(ptree.get_child_optional("servers"));
        if (servers)
            for(const auto& server : *servers)
                parseDatabaseServerConfiguration(databaseConfiguration, server.second, sourceName);
        else
        {
            std::ostringstream os;
            os << "Configuration error in " << sourceName << ": "
               << "missing \"servers\"";
            throw Exception(os.str());
        }
    }

    void parseDatabaseConfiguration(DatabaseConfiguration& databaseConfiguration,
                                    const boost::property_tree::ptree& ptree,
                                    const std::string& sourceName)
    {
        const auto type(get<std::string>(ptree, "type", sourceName));
        validateAndSetDbType(type, databaseConfiguration, sourceName);

        parseDatabaseServersConfiguration(databaseConfiguration, ptree, sourceName);
    }

    void parseDatabaseConfigurationTree(DatabaseConfiguration& databaseConfiguration,
                                        const boost::optional<boost::property_tree::ptree>& databaseConfigurationPtree,
                                        const std::string& sourceName)
    {
        if (databaseConfigurationPtree)
            parseDatabaseConfiguration(databaseConfiguration, *databaseConfigurationPtree, sourceName);
    }

    void parseDatabaseServersConfigurationFromString(DatabaseConfiguration& databaseConfiguration,
                                                     const std::string& serverConfiguration,
                                                     const std::string& sourceName)
    {
        size_t base(0);
        auto done(false);
        do
        {
            auto split = serverConfiguration.find(',', base);
            done = std::string::npos == split;
            validateAndSetDbServerAddress(serverConfiguration.substr(base, done ? std::string::npos : split-base),
                                          databaseConfiguration,
                                          sourceName);
            base = split+1;
        } while (!done);
    }

    void validateNamespacePrefix(const std::string& prefix,
                                 const std::string& sourceName)
    {
        if (!isValidNamespaceSyntax(prefix))
        {
            std::ostringstream os;
            os << "Configuration error in " << sourceName << ": "
               << "\"namespacePrefix\": \"" << prefix << "\""
               << " contains some of these disallowed characters: "
               << getDisallowedCharactersInNamespace();
            throw Exception(os.str());
        }
    }

    void validateEnableNotifications(bool enableNotifications, bool useDbBackend,
                                     const std::string& sourceName)
    {
        if (enableNotifications && !useDbBackend)
        {
            std::ostringstream os;
            os << "Configuration error in " << sourceName << ": "
               << "\"enableNotifications\" cannot be true, when \"useDbBackend\" is false";
            throw Exception(os.str());
        }
    }

    void parseNsConfiguration(NamespaceConfigurations& namespaceConfigurations,
                              const std::string& namespacePrefix,
                              const boost::property_tree::ptree& ptree,
                              const std::string& sourceName)
    {
        const auto useDbBackend(get<bool>(ptree, "useDbBackend", sourceName));
        const auto enableNotifications(get<bool>(ptree, "enableNotifications", sourceName));

        validateNamespacePrefix(namespacePrefix, sourceName);
        validateEnableNotifications(enableNotifications, useDbBackend, sourceName);

        namespaceConfigurations.addNamespaceConfiguration({namespacePrefix, useDbBackend, enableNotifications, sourceName});
    }

    void parseNsConfigurationMap(NamespaceConfigurations& namespaceConfigurations,
                                 std::unordered_map<std::string, std::pair<boost::property_tree::ptree, std::string>>& namespaceConfigurationMap)
    {
        for (const auto &namespaceConfigurationMapItem : namespaceConfigurationMap )
            parseNsConfiguration(namespaceConfigurations, namespaceConfigurationMapItem.first, namespaceConfigurationMapItem.second.first, namespaceConfigurationMapItem.second.second);
    }
}

ConfigurationReader::ConfigurationReader(std::shared_ptr<Logger> logger):
    ConfigurationReader(getDefaultConfDirectories(), System::getSystem(), logger)
{
}

ConfigurationReader::ConfigurationReader(const Directories& directories,
                                         System& system,
                                         std::shared_ptr<Logger> logger):
	dbHostEnvVariableName(DB_HOST_ENV_VAR_NAME),
	dbHostEnvVariableValue({}),
	dbPortEnvVariableName(DB_PORT_ENV_VAR_NAME),
	dbPortEnvVariableValue({}),
    jsonDatabaseConfiguration(boost::none),
    logger(logger)
{
    auto envStr = system.getenv(dbHostEnvVariableName.c_str());
    if (envStr)
    {
        dbHostEnvVariableValue = envStr;
        sourceForDatabaseConfiguration = dbHostEnvVariableName;
        auto envStr = system.getenv(dbPortEnvVariableName.c_str());
        if (envStr)
            dbPortEnvVariableValue = envStr;
    }

    readConfigurationFromDirectories(directories);
}

ConfigurationReader::~ConfigurationReader()
{
}

void ConfigurationReader::readConfigurationFromDirectories(const Directories& directories)
{
    for (const auto& i : findConfigurationFiles(directories))
        readConfiguration(i, i);
}

void ConfigurationReader::readConfigurationFromInputStream(const std::istream& input)
{
    jsonNamespaceConfigurations.clear();
    readConfiguration(const_cast<std::istream&>(input), "<istream>");
}

template<typename T>
void ConfigurationReader::readConfiguration(T& input, const std::string& currentSourceName)
{
    boost::property_tree::ptree propertyTree;

    try
    {
        boost::property_tree::read_json(input, propertyTree);
    }
    catch (const boost::property_tree::json_parser::json_parser_error& e)
    {
        std::ostringstream os;
        os << "error in SDL configuration " << currentSourceName << " at line " << e.line() << ": ";
        os << e.message();
        logger->error() << os.str();
        throw Exception(os.str());
    }

    // Environment variable configuration overrides json configuration
    if (sourceForDatabaseConfiguration != dbHostEnvVariableName)
    {
        const auto databaseConfiguration(propertyTree.get_child_optional("database"));
        if (databaseConfiguration)
        {
            jsonDatabaseConfiguration = databaseConfiguration;
            sourceForDatabaseConfiguration = currentSourceName;
        }
    }

    const auto namespaceConfigurations(propertyTree.get_child_optional("sharedDataLayer"));
    if (namespaceConfigurations)
    {
        for(const auto& namespaceConfiguration : *namespaceConfigurations)
        {
            const auto namespacePrefix = get<std::string>(namespaceConfiguration.second, "namespacePrefix", currentSourceName);
            jsonNamespaceConfigurations[namespacePrefix] = std::make_pair(namespaceConfiguration.second, currentSourceName);
        }
    }
}

void ConfigurationReader::readDatabaseConfiguration(DatabaseConfiguration& databaseConfiguration)
{
    if (!databaseConfiguration.isEmpty())
        SHAREDDATALAYER_ABORT("Database configuration can be read only to empty container");

    try
    {
        if (sourceForDatabaseConfiguration == dbHostEnvVariableName)
        {
            // Currently hard coded to redis-standalone, because RIC dbaas does not support Redis cluster configuration.
        	validateAndSetDbType("redis-standalone", databaseConfiguration, sourceForDatabaseConfiguration);
        	if (dbPortEnvVariableValue.empty())
        		parseDatabaseServersConfigurationFromString(databaseConfiguration, dbHostEnvVariableValue, sourceForDatabaseConfiguration);
        	else
        		parseDatabaseServersConfigurationFromString(databaseConfiguration, dbHostEnvVariableValue + ":" + dbPortEnvVariableValue, sourceForDatabaseConfiguration);
        }
        else
            parseDatabaseConfigurationTree(databaseConfiguration, jsonDatabaseConfiguration, sourceForDatabaseConfiguration);
    }
    catch (const std::exception& e)
    {
        logger->error() << e.what();
        throw;
    }
}

void ConfigurationReader::readNamespaceConfigurations(NamespaceConfigurations& namespaceConfigurations)
{
    if (!namespaceConfigurations.isEmpty())
        SHAREDDATALAYER_ABORT("Namespace configurations can be read only to empty container");

    try
    {
        parseNsConfigurationMap(namespaceConfigurations, jsonNamespaceConfigurations);
    }
    catch(const std::exception& e)
    {
        logger->error() << e.what();
        throw;
    }
}


template void ConfigurationReader::readConfiguration(const std::string&, const std::string&);
template void ConfigurationReader::readConfiguration(std::istream&, const std::string&);
