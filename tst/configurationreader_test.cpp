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

        void expectDBServerAddressConfigurationCheckAndApply(const std::string& address)
        {
            EXPECT_CALL(databaseConfigurationMock, checkAndApplyServerAddress(address));
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

TEST_F(ConfigurationReaderSDLConfigFileTest, CanReadJSONSharedDataLayerConfigurationFiles)
{
    InSequence dummy;
    expectDbTypeConfigurationCheckAndApply("redis-standalone");
    expectDBServerAddressConfigurationCheckAndApply("dbaas");
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
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
    std::string environmentValue;
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
        ConfigurationReaderBaseTest(DBCONF_VAR_NAME)
    {
    }

    void readEnvironmentConfigurationAndExpectAddressException(const std::string& addressValue)
    {
        std::ostringstream os;
        os << "Configuration error in " << someKnownInputSource << ": "
            << "invalid \"address\": \"" << addressValue << "\" some error";

        EXPECT_CALL(databaseConfigurationMock, checkAndApplyServerAddress(_))
            .WillOnce(Throw(Exception("some error")));

        EXPECT_THROW( {
            try
            {
                expectGetEnvironmentString(environmentValue.c_str());
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

    void readEnvironmentConfigurationAndExpectDbTypeException()
    {
        std::ostringstream os;
        os << "Configuration error in " << someKnownInputSource << ": some error";

        EXPECT_CALL(databaseConfigurationMock, checkAndApplyDbType(_))
            .WillOnce(Throw(Exception("some error")));

        EXPECT_THROW( {
            try
            {
                expectGetEnvironmentString(environmentValue.c_str());
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
    environmentValue = "redis-standalone:unknownAddress.local:12345";
    expectGetEnvironmentString(environmentValue.c_str());

    expectDbTypeConfigurationCheckAndApply("redis-standalone");
    expectDBServerAddressConfigurationCheckAndApply("unknownAddress.local:12345");
    initializeReaderWithSDLconfigFileDirectory();
    configurationReader->readConfigurationFromInputStream(is);
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}

TEST_F(ConfigurationReaderEnvironmentVariableTest, EnvironmentConfigurationWithoutPortIsAccepted)
{
    environmentValue = "redis-standalone:server.local";
    expectGetEnvironmentString(environmentValue.c_str());

    expectDbTypeConfigurationCheckAndApply("redis-standalone");
    expectDBServerAddressConfigurationCheckAndApply("server.local");
    initializeReaderWithoutDirectories();
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}

TEST_F(ConfigurationReaderEnvironmentVariableTest, EnvironmentConfigurationWithBadServerTypeThrows)
{
    environmentValue = "bad:unknownAddress.local:12345";
    readEnvironmentConfigurationAndExpectDbTypeException();
}

TEST_F(ConfigurationReaderEnvironmentVariableTest, EnvironmentConfigurationWithoutAddressThrows)
{
    environmentValue = "redis-standalone:";

    expectDbTypeConfigurationCheckAndApply("redis-standalone");
    readEnvironmentConfigurationAndExpectAddressException("");
}


TEST_F(ConfigurationReaderEnvironmentVariableTest, EmptyEnvironmentVariableThrows)
{
    environmentValue = "";
    readEnvironmentConfigurationAndExpectDbTypeException();
}

TEST_F(ConfigurationReaderEnvironmentVariableTest, EnvironmentConfigurationAcceptClusterServerType)
{
    environmentValue = "redis-cluster:server1.local:54321";

    expectGetEnvironmentString(environmentValue.c_str());
    expectDbTypeConfigurationCheckAndApply("redis-cluster");
    expectDBServerAddressConfigurationCheckAndApply("server1.local:54321");
    initializeReaderWithoutDirectories();
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}

TEST_F(ConfigurationReaderEnvironmentVariableTest, EnvironmentConfigurationAcceptMultipleServerAddresses)
{
    environmentValue = "redis-cluster:server1.local:54321,s2.local,1.2.3.4:12345,[1544::f412]:51743";

    expectGetEnvironmentString(environmentValue.c_str());
    expectDbTypeConfigurationCheckAndApply("redis-cluster");
    expectDBServerAddressConfigurationCheckAndApply("server1.local:54321");
    expectDBServerAddressConfigurationCheckAndApply("s2.local");
    expectDBServerAddressConfigurationCheckAndApply("1.2.3.4:12345");
    expectDBServerAddressConfigurationCheckAndApply("[1544::f412]:51743");
    initializeReaderWithoutDirectories();
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}


TEST_F(ConfigurationReaderEnvironmentVariableTest, EnvironmentConfigurationAcceptIPv6Address)
{
    InSequence dummy;
    environmentValue = "redis-standalone:[2001::123]:12345";
    expectGetEnvironmentString(environmentValue.c_str());

    expectDbTypeConfigurationCheckAndApply("redis-standalone");
    expectDBServerAddressConfigurationCheckAndApply("[2001::123]:12345");
    initializeReaderWithoutDirectories();
    configurationReader->readDatabaseConfiguration(databaseConfigurationMock);
}
