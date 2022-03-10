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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "private/configurationreader.hpp"
#include "private/createlogger.hpp"
#include "private/logger.hpp"
#include "private/tst/databaseconfigurationmock.hpp"
#include "private/tst/gettopsrcdir.hpp"
#include "private/tst/namespaceconfigurationsmock.hpp"
#include "private/tst/systemmock.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class ConfigurationReaderBaseTest: public testing::Test
    {
    public:
        const std::string someKnownInputSource;
        const Directories noDirectories;
        DatabaseConfigurationMock databaseConfigurationMock;
        NamespaceConfigurationsMock namespaceConfigurationsMock;
        std::unique_ptr<ConfigurationReader> configurationReader;
        SystemMock systemMock;
        std::shared_ptr<Logger> logger;

        ConfigurationReaderBaseTest(std::string inputSourceName):
            someKnownInputSource(inputSourceName),
            logger(createLogger(SDL_LOG_PREFIX))
        {
            EXPECT_CALL(databaseConfigurationMock, isEmpty()).WillRepeatedly(Return(true));
            EXPECT_CALL(namespaceConfigurationsMock, isEmpty()).WillRepeatedly(Return(true));
        }

        void expectDbTypeConfigurationCheckAndApply(const std::string& type)
        {
            EXPECT_CALL(databaseConfigurationMock, checkAndApplyDbType(type));
        }

        void expectGetDbTypeAndWillOnceReturn(DatabaseConfiguration::DbType type)
        {
            EXPECT_CALL(databaseConfigurationMock, getDbType())
                    .WillOnce(Return(type));
        }

        void expectDBServerAddressConfigurationCheckAndApply(const std::string& address)
        {
            EXPECT_CALL(databaseConfigurationMock, checkAndApplyServerAddress(address));
        }

        void expectCheckAndApplySentinelPorts(const std::string& portsEnvStr)
        {
            EXPECT_CALL(databaseConfigurationMock, checkAndApplySentinelPorts(portsEnvStr));
        }

        void expectSentinelMasterNameConfigurationCheckAndApply(const std::string& address)
        {
            EXPECT_CALL(databaseConfigurationMock, checkAndApplySentinelMasterNames(address));
        }

        void expectDatabaseConfigurationIsEmpty_returnFalse()
        {
            EXPECT_CALL(databaseConfigurationMock, isEmpty()).
                WillOnce(Return(false));
        }

        void tryToReadDatabaseConfigurationToNonEmptyContainer()
        {
            expectDatabaseConfigurationIsEmpty_returnFalse();
            configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
        }

        void expectNamespaceConfigurationIsEmpty_returnFalse()
        {
            EXPECT_CALL(namespaceConfigurationsMock, isEmpty()).
                WillOnce(Return(false));
        }

        void tryToReadNamespaceConfigurationToNonEmptyContainer()
        {
            expectNamespaceConfigurationIsEmpty_returnFalse();
            configurationReader->readNamespaceConfigurations(namespaceConfigurationsMock);
        }

        void expectAddNamespaceConfiguration(const std::string&  namespacePrefix, bool useDbBackend,
                                             bool enableNotifications)
        {
            NamespaceConfiguration expectedNamespaceConfiguration{namespacePrefix, useDbBackend,
                                                                  enableNotifications,
                                                                  someKnownInputSource};
            EXPECT_CALL(namespaceConfigurationsMock, addNamespaceConfiguration(expectedNamespaceConfiguration));
        }

        void expectGetEnvironmentString(const char* returnValue)
        {
            EXPECT_CALL(systemMock, getenv(_))
                .WillOnce(Return(returnValue));
        }

        void initializeReaderWithoutDirectories()
        {
            configurationReader.reset(new ConfigurationReader(noDirectories, systemMock, logger));
        }

        void initializeReaderWithSDLconfigFileDirectory()
        {
            configurationReader.reset(new ConfigurationReader({getTopSrcDir() + "/conf"}, systemMock, logger));
        }
    };

    class ConfigurationReaderSDLConfigFileTest: public ConfigurationReaderBaseTest
    {
    public:

        ConfigurationReaderSDLConfigFileTest():
            ConfigurationReaderBaseTest("ConfFileFromSDLrepo")
        {
            expectGetEnvironmentString(nullptr);
            initializeReaderWithSDLconfigFileDirectory();
        }
    };

    class ConfigurationReaderInputStreamTest: public ConfigurationReaderBaseTest
    {
    public:

        ConfigurationReaderInputStreamTest():
            ConfigurationReaderBaseTest("<istream>")
        {
            expectGetEnvironmentString(nullptr);
            initializeReaderWithoutDirectories();
        }

        void readConfigurationAndExpectJsonParserException(const std::istringstream& is)
        {
            const std::string expectedError("error in SDL configuration <istream> at line 7: expected ':'");

            EXPECT_THROW( {
                try
                {
                    configurationReader->readConfigurationFromInputStream(is);
                    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
                    configurationReader->readNamespaceConfigurations(namespaceConfigurationsMock);
                }
                catch (const std::exception& e)
                {
                    EXPECT_EQ(expectedError, e.what());
                    throw;
                }
            }, Exception);
        }

        void readConfigurationAndExpectBadValueException(const std::istringstream& is,
                                                         const std::string& param)
        {
            std::ostringstream os;
            os << "Configuration error in " << someKnownInputSource << ": "
               << "invalid \"" << param << "\": \"bad-value\"";

            EXPECT_THROW( {
                try
                {
                    configurationReader->readConfigurationFromInputStream(is);
                    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
                    configurationReader->readNamespaceConfigurations(namespaceConfigurationsMock);
                }
                catch (const std::exception& e)
                {
                    EXPECT_EQ(os.str(), e.what());
                    throw;
                }
            }, Exception);
        }

        void readConfigurationAndExpectDbTypeException(const std::istringstream& is)
        {
            std::ostringstream os;
            os << "Configuration error in " << someKnownInputSource << ": some error";

            EXPECT_CALL(databaseConfigurationMock, checkAndApplyDbType(_))
                .WillOnce(Throw(Exception("some error")));

            EXPECT_THROW( {
                try
                {
                    configurationReader->readConfigurationFromInputStream(is);
                    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
                }
                catch (const std::exception& e)
                {
                    EXPECT_EQ(os.str(), e.what());
                    throw;
                }
            }, Exception);
        }

        void readConfigurationAndExpectAddressException(const std::istringstream& is,
                                                        const std::string& addressValue)
        {
            std::ostringstream os;
            os << "Configuration error in " << someKnownInputSource << ": "
               << "invalid \"address\": \"" << addressValue << "\" some error";

            EXPECT_CALL(databaseConfigurationMock, checkAndApplyServerAddress(_))
                .WillOnce(Throw(Exception("some error")));

            EXPECT_THROW( {
                try
                {
                    configurationReader->readConfigurationFromInputStream(is);
                    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
                }
                catch (const std::exception& e)
                {
                    EXPECT_EQ(os.str(), e.what());
                    throw;
                }
            }, Exception);
        }

        void readConfigurationAndExpectMissingParameterException(const std::istringstream& is, const std::string& param)
        {
            std::ostringstream os;
            os << "Configuration error in " << someKnownInputSource << ": "
               << "missing \"" << param << "\"";

            EXPECT_THROW( {
                try
                {
                    configurationReader->readConfigurationFromInputStream(is);
                    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
                    configurationReader->readNamespaceConfigurations(namespaceConfigurationsMock);
                }
                catch (const std::exception& e)
                {
                    EXPECT_EQ(os.str(), e.what());
                    throw;
                }
            }, Exception);
        }

        void readConfigurationAndExpectNamespacePrefixValidationException(const std::istringstream& is,
                                                                          const std::string& namespacePrefix)
        {
            std::ostringstream os;
            os << "Configuration error in " << someKnownInputSource << ": "
               << "\"namespacePrefix\": \"" << namespacePrefix << "\""
               << " contains some of these disallowed characters: ,{}";

            EXPECT_THROW( {
                try
                {
                    configurationReader->readConfigurationFromInputStream(is);
                    configurationReader->readNamespaceConfigurations(namespaceConfigurationsMock);
                }
                catch (const std::exception& e)
                {
                    EXPECT_EQ(os.str(), e.what() );
                    throw;
                }
            }, Exception);
        }

        void readConfigurationAndExpectEnableNotificationsValidationException(const std::istringstream& is)
        {
            std::ostringstream os;
            os << "Configuration error in " << someKnownInputSource << ": "
               << "\"enableNotifications\" cannot be true, when \"useDbBackend\" is false";

            EXPECT_THROW( {
                try
                {
                    configurationReader->readConfigurationFromInputStream(is);
                    configurationReader->readNamespaceConfigurations(namespaceConfigurationsMock);
                }
                catch (const std::exception& e)
                {
                    EXPECT_EQ(os.str(), e.what() );
                    throw;
                }
            }, Exception);
        }

    };
}

TEST_F(ConfigurationReaderInputStreamTest, CanReadJSONDatabaseConfiguration)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "database":
            {
                "type": "redis-standalone",
                "servers":
                [
                    {
                        "address": "someKnownDbAddress:65535"
                    }
                ]
            }
        })JSON");
    expectDbTypeConfigurationCheckAndApply("redis-standalone");
    expectDBServerAddressConfigurationCheckAndApply("someKnownDbAddress:65535");
    configurationReader->readConfigurationFromInputStream(is);
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}

TEST_F(ConfigurationReaderInputStreamTest, CanReadJSONDatabaseConfigurationWithMultipleServerAddresses)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "database":
            {
                "type": "redis-cluster",
                "servers":
                [
                    {
                        "address": "10.20.30.40:50000"
                    },
                    {
                        "address": "10.20.30.50:50001"
                    }
                ]
            }
        })JSON");

    expectDbTypeConfigurationCheckAndApply("redis-cluster");
    expectDBServerAddressConfigurationCheckAndApply("10.20.30.40:50000");
    expectDBServerAddressConfigurationCheckAndApply("10.20.30.50:50001");
    configurationReader->readConfigurationFromInputStream(is);
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}

TEST_F(ConfigurationReaderInputStreamTest, CanReadJSONDatabaseConfigurationWithMultipleReadOperations)
{
    InSequence dummy;
    std::istringstream isOne(R"JSON(
        {
            "database":
            {
                "type": "redis-cluster",
                "servers":
                [
                    {
                        "address": "10.20.30.40:50000"
                    }
                ]
            }
        })JSON");
    std::istringstream isTwo(R"JSON(
        {
            "database":
            {
                "type": "redis-cluster",
                "servers":
                [
                    {
                        "address": "10.20.30.50:50001"
                    }
                ]
            }
        })JSON");

    expectDbTypeConfigurationCheckAndApply("redis-cluster");
    expectDBServerAddressConfigurationCheckAndApply("10.20.30.40:50000");
    expectDbTypeConfigurationCheckAndApply("redis-cluster");
    expectDBServerAddressConfigurationCheckAndApply("10.20.30.50:50001");
    configurationReader->readConfigurationFromInputStream(isOne);
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
    configurationReader->readConfigurationFromInputStream(isTwo);
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}

TEST_F(ConfigurationReaderInputStreamTest, CanCatchAndThrowMisingMandatoryDatabaseTypeParameter)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "database":
            {
                "servers":
                [
                    {
                        "address": "10.20.30.50:50001"
                    }
                ]
            }
        })JSON");

    readConfigurationAndExpectMissingParameterException(is, "type");
}

TEST_F(ConfigurationReaderInputStreamTest, CanCatchAndThrowMisingMandatoryDatabaseServersArray)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "database":
            {
                "type": "redis-standalone"
            }
        })JSON");

    expectDbTypeConfigurationCheckAndApply("redis-standalone");
    readConfigurationAndExpectMissingParameterException(is, "servers");
}

TEST_F(ConfigurationReaderInputStreamTest, CanCatchAndThrowMisingMandatoryDatabaseServerAddressParameter)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "database":
            {
                "type": "redis-standalone",
                "servers":
                [
                    {
                    }
                ]
            }
        })JSON");

    expectDbTypeConfigurationCheckAndApply("redis-standalone");
    readConfigurationAndExpectMissingParameterException(is, "address");
}

TEST_F(ConfigurationReaderInputStreamTest, CanCatchAndThrowDatabaseConfigurationDbTypeError)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "database":
            {
                "type": "someBadType",
                "servers":
                [
                    {
                        "address": "10.20.30.50:50001"
                    }
                ]
            }
        })JSON");

    readConfigurationAndExpectDbTypeException(is);
}

TEST_F(ConfigurationReaderInputStreamTest, CanCatchAndThrowDatabaseConfigurationAddressError)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "database":
            {
                "type": "redis-standalone",
                "servers":
                [
                    {
                        "address": "someBadAddress"
                    }
                ]
            }
        })JSON");

    expectDbTypeConfigurationCheckAndApply("redis-standalone");
    readConfigurationAndExpectAddressException(is, "someBadAddress");
}

TEST_F(ConfigurationReaderInputStreamTest, CanHandleJSONWithoutAnyConfiguration)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
        })JSON");

    EXPECT_CALL(databaseConfigurationMock, checkAndApplyServerAddress(_))
        .Times(0);
    EXPECT_CALL(namespaceConfigurationsMock, addNamespaceConfiguration(_))
        .Times(0);
    configurationReader->readConfigurationFromInputStream(is);
}

TEST_F(ConfigurationReaderInputStreamTest, CanReadJSONSharedDataLayerConfiguration)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "sharedDataLayer":
            [
                {
                    "namespacePrefix": "someKnownNamespacePrefix",
                    "useDbBackend": true,
                    "enableNotifications": true
                },
                {
                    "namespacePrefix": "anotherKnownNamespace",
                    "useDbBackend": false,
                    "enableNotifications": false
                }
            ]
        })JSON");

    expectAddNamespaceConfiguration("anotherKnownNamespace", false, false);
    expectAddNamespaceConfiguration("someKnownNamespacePrefix", true, true);
    configurationReader->readConfigurationFromInputStream(is);
    configurationReader->readNamespaceConfigurations(namespaceConfigurationsMock);
}

TEST_F(ConfigurationReaderInputStreamTest, CanReadJSONSharedDataLayerConfigurationWithMultipleReadOperations)
{
    InSequence dummy;
    std::istringstream isOne(R"JSON(
        {
            "sharedDataLayer":
            [
                {
                    "namespacePrefix": "someKnownNamespacePrefix",
                    "useDbBackend": true,
                    "enableNotifications": true
                }
            ]
        })JSON");

    std::istringstream isTwo(R"JSON(
        {
            "sharedDataLayer":
            [
                {
                    "namespacePrefix": "anotherKnownNamespace",
                    "useDbBackend": false,
                    "enableNotifications": false
                }
            ]
        })JSON");

    expectAddNamespaceConfiguration("someKnownNamespacePrefix", true, true);
    expectAddNamespaceConfiguration("anotherKnownNamespace", false, false);
    configurationReader->readConfigurationFromInputStream(isOne);
    configurationReader->readNamespaceConfigurations(namespaceConfigurationsMock);
    configurationReader->readConfigurationFromInputStream(isTwo);
    configurationReader->readNamespaceConfigurations(namespaceConfigurationsMock);
}

TEST_F(ConfigurationReaderInputStreamTest, CanReadJSONSharedDataLayerConfigurationWithEmptyNamespacePrefixValue)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "sharedDataLayer":
            [
                {
                    "namespacePrefix": "",
                    "useDbBackend": false,
                    "enableNotifications": false
                }
            ]
        })JSON");

    expectAddNamespaceConfiguration("", false, false);
    configurationReader->readConfigurationFromInputStream(is);
    configurationReader->readNamespaceConfigurations(namespaceConfigurationsMock);
}

TEST_F(ConfigurationReaderInputStreamTest, CanCatchAndThrowJSONSyntaxError)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "sharedDataLayer":
            [
                {
                    "abc"
                }
            ]
        })JSON");

    readConfigurationAndExpectJsonParserException(is);
}

TEST_F(ConfigurationReaderInputStreamTest, CanCatchAndThrowParameterUseDbBackendBadValue)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "sharedDataLayer":
            [
                {
                    "namespacePrefix": "someKnownNamespacePrefix",
                    "useDbBackend": "bad-value",
                    "enableNotifications": false
                }
            ]
        })JSON");

    readConfigurationAndExpectBadValueException(is, "useDbBackend");
}

TEST_F(ConfigurationReaderInputStreamTest, CanCatchAndThrowParameterEnableNotificationsBadValue)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "sharedDataLayer":
            [
                {
                    "namespacePrefix": "someKnownNamespacePrefix",
                    "useDbBackend": true,
                    "enableNotifications": "bad-value"
                }
            ]
        })JSON");

    readConfigurationAndExpectBadValueException(is, "enableNotifications");
}

TEST_F(ConfigurationReaderInputStreamTest, CanCatchAndThrowMisingMandatoryNamespacePrefixParameter)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "sharedDataLayer":
            [
                {
                    "useDbBackend": true,
                    "enableNotifications": true
                }
            ]
        })JSON");

    readConfigurationAndExpectMissingParameterException(is, "namespacePrefix");
}

TEST_F(ConfigurationReaderInputStreamTest, CanCatchAndThrowMisingMandatoryUseDbBackendParameter)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "sharedDataLayer":
            [
                {
                    "namespacePrefix": "someKnownNamespacePrefix",
                    "enableNotifications": true
                }
            ]
        })JSON");

    readConfigurationAndExpectMissingParameterException(is, "useDbBackend");
}

TEST_F(ConfigurationReaderInputStreamTest, CanCatchAndThrowMisingMandatoryEnableNotificationsParameter)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "sharedDataLayer":
            [
                {
                    "namespacePrefix": "someKnownNamespacePrefix",
                    "useDbBackend": true
                }
            ]
        })JSON");

    readConfigurationAndExpectMissingParameterException(is, "enableNotifications");
}

TEST_F(ConfigurationReaderInputStreamTest, CanThrowValidationErrorForNamespacePrefixWithDisallowedCharacters)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "sharedDataLayer":
            [
                {
                    "namespacePrefix": "a,b{c}",
                    "useDbBackend": true,
                    "enableNotifications": true
                }
            ]
        })JSON");

    readConfigurationAndExpectNamespacePrefixValidationException(is, "a,b{c}");
}

TEST_F(ConfigurationReaderInputStreamTest, CanThrowValidationErrorForEnableNotificationsWithNoDbBackend)
{
    InSequence dummy;
    std::istringstream is(R"JSON(
        {
            "sharedDataLayer":
            [
                {
                    "namespacePrefix": "someKnownNamespacePrefix",
                    "useDbBackend": false,
                    "enableNotifications": true
                }
            ]
        })JSON");

    readConfigurationAndExpectEnableNotificationsValidationException(is);
}

TEST_F(ConfigurationReaderInputStreamTest, WillNotReadDatabaseConfigurationToNonEmptyContainer)
{
    EXPECT_EXIT(tryToReadDatabaseConfigurationToNonEmptyContainer(),
        KilledBySignal(SIGABRT), "ABORT.*configurationreader\\.cpp");
}

TEST_F(ConfigurationReaderInputStreamTest, WillNotReadNamespaceConfigurationToNonEmptyContainer)
{
    EXPECT_EXIT(tryToReadNamespaceConfigurationToNonEmptyContainer(),
        KilledBySignal(SIGABRT), "ABORT.*configurationreader\\.cpp");
}

class ConfigurationReaderEnvironmentVariableTest: public ConfigurationReaderBaseTest
{
public:
    std::string dbHostEnvVariableValue;
    std::string dbPortEnvVariableValue;
    std::string sentinelPortEnvVariableValue;
    std::string sentinelMasterNameEnvVariableValue;
    std::string dbClusterAddrListEnvVariableValue;
    std::istringstream is{R"JSON(
        {
            "database":
            {
                "type": "redis-cluster",
                "servers":
                [
                    {
                        "address": "10.20.30.40:50000"
                    },
                    {
                        "address": "10.20.30.50:50001"
                    }
                ]
            }
        })JSON"};

    ConfigurationReaderEnvironmentVariableTest():
        ConfigurationReaderBaseTest(DB_HOST_ENV_VAR_NAME)
    {
    }

    void readEnvironmentConfigurationAndExpectConfigurationErrorException(const std::string&  msg,
                                                                          bool expectCall)
    {
        std::ostringstream os;
        os << "Configuration error in " << someKnownInputSource << ": " << msg;

        if (expectCall)
            EXPECT_CALL(databaseConfigurationMock, checkAndApplyDbType(_))
                .WillOnce(Throw(Exception("some error")));

        EXPECT_THROW( {
            try
            {
                EXPECT_CALL(systemMock, getenv(_))
                    .Times(5)
                    .WillOnce(Return(dbHostEnvVariableValue.c_str()))
                    .WillOnce(Return(nullptr))
                    .WillOnce(Return(nullptr))
                    .WillOnce(Return(nullptr))
                    .WillOnce(Return(nullptr));
                initializeReaderWithoutDirectories();
                configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
            }
            catch (const std::exception& e)
            {
                EXPECT_EQ(os.str(), e.what());
                throw;
            }
        }, Exception);
    }
};

TEST_F(ConfigurationReaderEnvironmentVariableTest, EnvironmentConfigurationCanOverrideJSONDatabaseConfiguration)
{
    InSequence dummy;
    dbHostEnvVariableValue = "unknownAddress.local";
    expectGetEnvironmentString(dbHostEnvVariableValue.c_str());
    dbPortEnvVariableValue = "12345";
    expectGetEnvironmentString(dbPortEnvVariableValue.c_str());
    expectGetEnvironmentString(nullptr); //SENTINEL_PORT_ENV_VAR_NAME
    expectGetEnvironmentString(nullptr); //SENTINEL_MASTER_NAME_ENV_VAR_NAME
    expectGetEnvironmentString(nullptr); //DB_CLUSTER_ENV_VAR_NAME

    expectDbTypeConfigurationCheckAndApply("redis-standalone");
    expectDBServerAddressConfigurationCheckAndApply("unknownAddress.local:12345");
    expectGetDbTypeAndWillOnceReturn(DatabaseConfiguration::DbType::REDIS_STANDALONE);
    initializeReaderWithSDLconfigFileDirectory();
    configurationReader->readConfigurationFromInputStream(is);
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}

TEST_F(ConfigurationReaderEnvironmentVariableTest, EnvironmentConfigurationWithoutPortIsAccepted)
{
    InSequence dummy;
    dbHostEnvVariableValue = "server.local";
    expectGetEnvironmentString(dbHostEnvVariableValue.c_str());
    expectGetEnvironmentString(nullptr); //DB_PORT_ENV_VAR_NAME
    expectGetEnvironmentString(nullptr); //SENTINEL_PORT_ENV_VAR_NAME
    expectGetEnvironmentString(nullptr); //SENTINEL_MASTER_NAME_ENV_VAR_NAME
    expectGetEnvironmentString(nullptr); //DB_CLUSTER_ENV_VAR_NAME

    expectDbTypeConfigurationCheckAndApply("redis-standalone");
    expectDBServerAddressConfigurationCheckAndApply("server.local");
    expectGetDbTypeAndWillOnceReturn(DatabaseConfiguration::DbType::REDIS_STANDALONE);
    initializeReaderWithoutDirectories();
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}

TEST_F(ConfigurationReaderEnvironmentVariableTest, EmptyEnvironmentVariableThrows)
{
    dbHostEnvVariableValue = "";
    readEnvironmentConfigurationAndExpectConfigurationErrorException("Missing environment variable configuration!",
                                                                      false);
}

TEST_F(ConfigurationReaderEnvironmentVariableTest, IllegalCharacterInEnvironmentVariableThrows)
{
    dbHostEnvVariableValue = "@";
    readEnvironmentConfigurationAndExpectConfigurationErrorException("some error", true);
}

TEST_F(ConfigurationReaderEnvironmentVariableTest, EnvironmentConfigurationAcceptIPv6Address)
{
    InSequence dummy;
    dbHostEnvVariableValue = "[2001::123]:12345";
    expectGetEnvironmentString(dbHostEnvVariableValue.c_str());
    expectGetEnvironmentString(nullptr); //DB_PORT_ENV_VAR_NAME
    expectGetEnvironmentString(nullptr); //SENTINEL_PORT_ENV_VAR_NAME
    expectGetEnvironmentString(nullptr); //SENTINEL_MASTER_NAME_ENV_VAR_NAME
    expectGetEnvironmentString(nullptr); //DB_CLUSTER_ENV_VAR_NAME

    expectDbTypeConfigurationCheckAndApply("redis-standalone");
    expectDBServerAddressConfigurationCheckAndApply("[2001::123]:12345");
    expectGetDbTypeAndWillOnceReturn(DatabaseConfiguration::DbType::REDIS_STANDALONE);
    initializeReaderWithoutDirectories();
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}

TEST_F(ConfigurationReaderEnvironmentVariableTest, EnvironmentConfigurationWithSentinel)
{
    InSequence dummy;
    dbHostEnvVariableValue = "sentinelAddress.local";
    expectGetEnvironmentString(dbHostEnvVariableValue.c_str());
    dbPortEnvVariableValue = "1111";
    expectGetEnvironmentString(dbPortEnvVariableValue.c_str());
    sentinelPortEnvVariableValue = "2222";
    expectGetEnvironmentString(sentinelPortEnvVariableValue.c_str());
    sentinelMasterNameEnvVariableValue = "mymaster";
    expectGetEnvironmentString(sentinelMasterNameEnvVariableValue.c_str());
    expectGetEnvironmentString(nullptr); //DB_CLUSTER_ENV_VAR_NAME

    expectDbTypeConfigurationCheckAndApply("redis-sentinel");
    expectDBServerAddressConfigurationCheckAndApply("sentinelAddress.local:1111");
    expectGetDbTypeAndWillOnceReturn(DatabaseConfiguration::DbType::REDIS_SENTINEL);
    expectCheckAndApplySentinelPorts(sentinelPortEnvVariableValue);
    expectSentinelMasterNameConfigurationCheckAndApply(sentinelMasterNameEnvVariableValue);
    initializeReaderWithoutDirectories();
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}

TEST_F(ConfigurationReaderEnvironmentVariableTest, EnvironmentConfigurationWithSentinelAndClusterConfiguration)
{
    InSequence dummy;
    dbHostEnvVariableValue = "address-0.local";
    expectGetEnvironmentString(dbHostEnvVariableValue.c_str());
    expectGetEnvironmentString(nullptr); //DB_PORT_ENV_VAR_NAME
    sentinelPortEnvVariableValue = "2222,2223,2224";
    expectGetEnvironmentString(sentinelPortEnvVariableValue.c_str());
    sentinelMasterNameEnvVariableValue = "mymaster-0,mymaster-1,mymaster-2";
    expectGetEnvironmentString(sentinelMasterNameEnvVariableValue.c_str());
    dbClusterAddrListEnvVariableValue = "address-0.local,address-1.local,address-2.local";
    expectGetEnvironmentString(dbClusterAddrListEnvVariableValue.c_str());

    expectDbTypeConfigurationCheckAndApply("sdl-sentinel-cluster");
    expectDBServerAddressConfigurationCheckAndApply("address-0.local");
    expectDBServerAddressConfigurationCheckAndApply("address-1.local");
    expectDBServerAddressConfigurationCheckAndApply("address-2.local");
    expectGetDbTypeAndWillOnceReturn(DatabaseConfiguration::DbType::SDL_SENTINEL_CLUSTER);
    expectCheckAndApplySentinelPorts(sentinelPortEnvVariableValue);
    expectSentinelMasterNameConfigurationCheckAndApply(sentinelMasterNameEnvVariableValue);
    initializeReaderWithoutDirectories();
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}

TEST_F(ConfigurationReaderEnvironmentVariableTest, EnvironmentConfigurationWithoutSentinelAndWithClusterConfiguration)
{
    InSequence dummy;
    dbHostEnvVariableValue = "address-0.local";
    expectGetEnvironmentString(dbHostEnvVariableValue.c_str());
    expectGetEnvironmentString(nullptr); //DB_PORT_ENV_VAR_NAME
    expectGetEnvironmentString(nullptr); //SENTINEL_PORT_ENV_VAR_NAME
    expectGetEnvironmentString(nullptr); //SENTINEL_MASTER_NAME_ENV_VAR_NAME
    dbClusterAddrListEnvVariableValue = "address-0.local,address-1.local,address-2.local";
    expectGetEnvironmentString(dbClusterAddrListEnvVariableValue.c_str());

    expectDbTypeConfigurationCheckAndApply("sdl-standalone-cluster");
    expectDBServerAddressConfigurationCheckAndApply("address-0.local");
    expectDBServerAddressConfigurationCheckAndApply("address-1.local");
    expectDBServerAddressConfigurationCheckAndApply("address-2.local");
    expectGetDbTypeAndWillOnceReturn(DatabaseConfiguration::DbType::SDL_STANDALONE_CLUSTER);
    initializeReaderWithoutDirectories();
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}

TEST_F(ConfigurationReaderEnvironmentVariableTest, EnvironmentConfigurationWithoutSentinelAndWithClusterConfigurationAndDbPort)
{
    InSequence dummy;
    dbHostEnvVariableValue = "address-0.local";
    expectGetEnvironmentString(dbHostEnvVariableValue.c_str());
    dbPortEnvVariableValue = "1111";
    expectGetEnvironmentString(dbPortEnvVariableValue.c_str());
    expectGetEnvironmentString(nullptr); //SENTINEL_PORT_ENV_VAR_NAME
    expectGetEnvironmentString(nullptr); //SENTINEL_MASTER_NAME_ENV_VAR_NAME
    dbClusterAddrListEnvVariableValue = "address-0.local,address-1.local,address-2.local";
    expectGetEnvironmentString(dbClusterAddrListEnvVariableValue.c_str());

    expectDbTypeConfigurationCheckAndApply("sdl-standalone-cluster");
    expectDBServerAddressConfigurationCheckAndApply("address-0.local:1111");
    expectDBServerAddressConfigurationCheckAndApply("address-1.local:1111");
    expectDBServerAddressConfigurationCheckAndApply("address-2.local:1111");
    expectGetDbTypeAndWillOnceReturn(DatabaseConfiguration::DbType::SDL_STANDALONE_CLUSTER);
    initializeReaderWithoutDirectories();
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}
