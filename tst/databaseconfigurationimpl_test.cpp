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
#include <arpa/inet.h>
#include "private/databaseconfigurationimpl.hpp"

using namespace shareddatalayer;
using namespace testing;

namespace
{
    class DatabaseConfigurationImplTest: public testing::Test
    {
    public:
        std::unique_ptr<DatabaseConfigurationImpl> databaseConfigurationImpl;

        DatabaseConfigurationImplTest()
        {
            InSequence dummy;
            databaseConfigurationImpl.reset(new DatabaseConfigurationImpl());
        }
    };
}

TEST_F(DatabaseConfigurationImplTest, CanReturnDefaultAddress)
{
    const auto retAddresses(databaseConfigurationImpl->getDefaultServerAddresses());
    EXPECT_EQ(1U, retAddresses.size());
    EXPECT_EQ("localhost", retAddresses.back().getHost());
    EXPECT_EQ(6379U, ntohs(retAddresses.back().getPort()));
}

TEST_F(DatabaseConfigurationImplTest, CanReturnEmptyAddressListIfNoAddressesAreApplied)
{
    const auto retAddresses(databaseConfigurationImpl->getServerAddresses());
    EXPECT_TRUE(retAddresses.empty());
}

TEST_F(DatabaseConfigurationImplTest, CanReturnUnknownTypeIfNoRedisDbTypeIsApplied)
{
    const auto retDbType(databaseConfigurationImpl->getDbType());
    EXPECT_EQ(DatabaseConfiguration::DbType::UNKNOWN, retDbType);
}

TEST_F(DatabaseConfigurationImplTest, CanApplyRedisDbTypeStringAndReturnType)
{
    databaseConfigurationImpl->checkAndApplyDbType("redis-standalone");
    const auto retDbType(databaseConfigurationImpl->getDbType());
    EXPECT_EQ(DatabaseConfiguration::DbType::REDIS_STANDALONE, retDbType);
}

TEST_F(DatabaseConfigurationImplTest, CanApplyRedisClusterDbTypeStringAndReturnType)
{
    databaseConfigurationImpl->checkAndApplyDbType("redis-cluster");
    const auto retDbType(databaseConfigurationImpl->getDbType());
    EXPECT_EQ(DatabaseConfiguration::DbType::REDIS_CLUSTER, retDbType);
}

TEST_F(DatabaseConfigurationImplTest, CanApplyRedisSentinelDbTypeStringAndReturnType)
{
    databaseConfigurationImpl->checkAndApplyDbType("redis-sentinel");
    const auto retDbType(databaseConfigurationImpl->getDbType());
    EXPECT_EQ(DatabaseConfiguration::DbType::REDIS_SENTINEL, retDbType);
}

TEST_F(DatabaseConfigurationImplTest, CanApplySdlStandaloneClusterDbTypeStringAndReturnType)
{
    databaseConfigurationImpl->checkAndApplyDbType("sdl-standalone-cluster");
    const auto retDbType(databaseConfigurationImpl->getDbType());
    EXPECT_EQ(DatabaseConfiguration::DbType::SDL_STANDALONE_CLUSTER, retDbType);
}

TEST_F(DatabaseConfigurationImplTest, CanApplySdlSentinelClusterDbTypeStringAndReturnType)
{
    databaseConfigurationImpl->checkAndApplyDbType("sdl-sentinel-cluster");
    const auto retDbType(databaseConfigurationImpl->getDbType());
    EXPECT_EQ(DatabaseConfiguration::DbType::SDL_SENTINEL_CLUSTER, retDbType);
}

TEST_F(DatabaseConfigurationImplTest, CanApplyNewAddressesOneByOneAndReturnAllAddresses)
{
    databaseConfigurationImpl->checkAndApplyServerAddress("dummydatabaseaddress.local");
    databaseConfigurationImpl->checkAndApplyServerAddress("10.20.30.40:65535");
    const auto retAddresses(databaseConfigurationImpl->getServerAddresses());
    EXPECT_EQ(2U, retAddresses.size());
    EXPECT_EQ("dummydatabaseaddress.local", retAddresses.at(0).getHost());
    EXPECT_EQ(6379U, ntohs(retAddresses.at(0).getPort()));
    EXPECT_EQ("10.20.30.40", retAddresses.at(1).getHost());
    EXPECT_EQ(65535U, ntohs(retAddresses.at(1).getPort()));
}

TEST_F(DatabaseConfigurationImplTest, CanGetAddressesOneByOneWithAddressIndex)
{
    databaseConfigurationImpl->checkAndApplyServerAddress("server0.local");
    databaseConfigurationImpl->checkAndApplyServerAddress("10.20.30.40:65535");
    const auto addresses(databaseConfigurationImpl->getServerAddresses(boost::none));
    const auto addresses0(databaseConfigurationImpl->getServerAddresses(0));
    const auto addresses1(databaseConfigurationImpl->getServerAddresses(1));
    EXPECT_EQ(2U, addresses.size());
    EXPECT_EQ(1U, addresses0.size());
    EXPECT_EQ(1U, addresses1.size());
    EXPECT_EQ("server0.local", addresses0.at(0).getHost());
    EXPECT_EQ(6379U, ntohs(addresses0.at(0).getPort()));
    EXPECT_EQ("10.20.30.40", addresses1.at(0).getHost());
    EXPECT_EQ(65535U, ntohs(addresses1.at(0).getPort()));
}

TEST_F(DatabaseConfigurationImplTest, CanThrowIfIllegalDbTypeIsApplied)
{
    EXPECT_THROW(databaseConfigurationImpl->checkAndApplyDbType("bad_db_type"), DatabaseConfiguration::InvalidDbType);
}

TEST_F(DatabaseConfigurationImplTest, CanApplyIPv6AddressAndReturnIt)
{
    databaseConfigurationImpl->checkAndApplyServerAddress("[2001::123]:12345");
    const auto retAddresses(databaseConfigurationImpl->getServerAddresses());
    EXPECT_EQ(1U, retAddresses.size());
    EXPECT_EQ("2001::123", retAddresses.at(0).getHost());
    EXPECT_EQ(12345U, ntohs(retAddresses.at(0).getPort()));
}

TEST_F(DatabaseConfigurationImplTest, IsEmptyReturnsCorrectInformation)
{
    EXPECT_TRUE(databaseConfigurationImpl->isEmpty());
    databaseConfigurationImpl->checkAndApplyServerAddress("[2001::123]:12345");
    EXPECT_FALSE(databaseConfigurationImpl->isEmpty());
}

TEST_F(DatabaseConfigurationImplTest, DefaultSentinelAddressIsNone)
{
    EXPECT_EQ(boost::none, databaseConfigurationImpl->getSentinelAddress(boost::none));
}

TEST_F(DatabaseConfigurationImplTest, CanApplyAndReturnSentinelAddress)
{
    databaseConfigurationImpl->checkAndApplyServerAddress("dummydatabaseaddress.local:1234");
    databaseConfigurationImpl->checkAndApplySentinelPorts("51234");
    const auto serverAddresses(databaseConfigurationImpl->getServerAddresses());
    const auto address = databaseConfigurationImpl->getSentinelAddress(boost::none);
    EXPECT_NE(boost::none, databaseConfigurationImpl->getSentinelAddress(boost::none));
    EXPECT_EQ("dummydatabaseaddress.local", address->getHost());
    EXPECT_EQ(51234, ntohs(address->getPort()));
    EXPECT_EQ(1U, serverAddresses.size());
    EXPECT_EQ("dummydatabaseaddress.local", serverAddresses.at(0).getHost());
    EXPECT_EQ(1234U, ntohs(serverAddresses.at(0).getPort()));
}

TEST_F(DatabaseConfigurationImplTest, CanApplyAndReturnSentinelAddressDefaultSentinelPort)
{
    databaseConfigurationImpl->checkAndApplyServerAddress("dummydatabaseaddress.local:1234");
    databaseConfigurationImpl->checkAndApplySentinelPorts("");
    const auto serverAddresses(databaseConfigurationImpl->getServerAddresses());
    const auto address = databaseConfigurationImpl->getSentinelAddress(boost::none);
    EXPECT_NE(boost::none, databaseConfigurationImpl->getSentinelAddress(boost::none));
    EXPECT_EQ("dummydatabaseaddress.local", address->getHost());
    EXPECT_EQ(26379, ntohs(address->getPort()));
    EXPECT_EQ(1U, serverAddresses.size());
    EXPECT_EQ("dummydatabaseaddress.local", serverAddresses.at(0).getHost());
    EXPECT_EQ(1234U, ntohs(serverAddresses.at(0).getPort()));
}

TEST_F(DatabaseConfigurationImplTest, DefaultSentinelMasterNameIsEmpty)
{
    EXPECT_EQ("dbaasmaster", databaseConfigurationImpl->getSentinelMasterName(boost::none));
}

TEST_F(DatabaseConfigurationImplTest, CanApplyAndReturnSentinelMasterName)
{
    databaseConfigurationImpl->checkAndApplySentinelMasterNames("mymaster");
    EXPECT_EQ("mymaster", databaseConfigurationImpl->getSentinelMasterName(boost::none));
}

TEST_F(DatabaseConfigurationImplTest, CanApplyAndReturnSentinelMasterNames)
{
    databaseConfigurationImpl->checkAndApplySentinelMasterNames("mymaster-0,mymaster-1,mymaster-2");
    EXPECT_EQ("mymaster-0", databaseConfigurationImpl->getSentinelMasterName(0));
    EXPECT_EQ("mymaster-1", databaseConfigurationImpl->getSentinelMasterName(1));
    EXPECT_EQ("mymaster-2", databaseConfigurationImpl->getSentinelMasterName(2));
}

TEST_F(DatabaseConfigurationImplTest, CanApplyAndReturnDefaultSentinelMasterNamesWhenNotAllMasterNamesAreSet)
{
    databaseConfigurationImpl->checkAndApplySentinelMasterNames("mymaster-0");
    EXPECT_EQ("mymaster-0", databaseConfigurationImpl->getSentinelMasterName(0));
    EXPECT_EQ("dbaasmaster", databaseConfigurationImpl->getSentinelMasterName(1));
    EXPECT_EQ("dbaasmaster", databaseConfigurationImpl->getSentinelMasterName(2));
}

TEST_F(DatabaseConfigurationImplTest, CanReturnSDLSentinelClusterAddresses)
{
    databaseConfigurationImpl->checkAndApplyDbType("sdl-sentinel-cluster");
    databaseConfigurationImpl->checkAndApplyServerAddress("cluster-0.local");
    databaseConfigurationImpl->checkAndApplyServerAddress("cluster-1.local");
    databaseConfigurationImpl->checkAndApplyServerAddress("cluster-2.local");
    databaseConfigurationImpl->checkAndApplySentinelPorts("54321,54322,54323");
    auto address0 = databaseConfigurationImpl->getSentinelAddress(0);
    auto address1 = databaseConfigurationImpl->getSentinelAddress(1);
    auto address2 = databaseConfigurationImpl->getSentinelAddress(2);
    EXPECT_NE(boost::none, databaseConfigurationImpl->getSentinelAddress(0));
    EXPECT_NE(boost::none, databaseConfigurationImpl->getSentinelAddress(1));
    EXPECT_NE(boost::none, databaseConfigurationImpl->getSentinelAddress(2));
    EXPECT_EQ("cluster-0.local", address0->getHost());
    EXPECT_EQ("cluster-1.local", address1->getHost());
    EXPECT_EQ("cluster-2.local", address2->getHost());
    EXPECT_EQ(54321, ntohs(address0->getPort()));
    EXPECT_EQ(54322, ntohs(address1->getPort()));
    EXPECT_EQ(54323, ntohs(address2->getPort()));
}

TEST_F(DatabaseConfigurationImplTest, CanReturnSDLSentinelPorts)
{
    databaseConfigurationImpl->checkAndApplyDbType("sdl-sentinel-cluster");
    databaseConfigurationImpl->checkAndApplyServerAddress("cluster-0.local");
    databaseConfigurationImpl->checkAndApplyServerAddress("cluster-1.local");
    databaseConfigurationImpl->checkAndApplyServerAddress("cluster-2.local");
    databaseConfigurationImpl->checkAndApplySentinelPorts("54321");
    auto address0 = databaseConfigurationImpl->getSentinelAddress(0);
    auto address1 = databaseConfigurationImpl->getSentinelAddress(1);
    auto address2 = databaseConfigurationImpl->getSentinelAddress(2);
    EXPECT_NE(boost::none, databaseConfigurationImpl->getSentinelAddress(0));
    EXPECT_NE(boost::none, databaseConfigurationImpl->getSentinelAddress(1));
    EXPECT_NE(boost::none, databaseConfigurationImpl->getSentinelAddress(2));
    EXPECT_EQ("cluster-0.local", address0->getHost());
    EXPECT_EQ("cluster-1.local", address1->getHost());
    EXPECT_EQ("cluster-2.local", address2->getHost());
    EXPECT_EQ(54321, ntohs(address0->getPort()));
    EXPECT_EQ(26379, ntohs(address1->getPort()));
    EXPECT_EQ(26379, ntohs(address2->getPort()));
}

TEST_F(DatabaseConfigurationImplTest, CanReturnDefaultSentinelPortForSDLClusterAddress)
{
    databaseConfigurationImpl->checkAndApplyServerAddress("cluster-0.local");
    auto address = databaseConfigurationImpl->getSentinelAddress(boost::none);
    EXPECT_NE(boost::none, databaseConfigurationImpl->getSentinelAddress(boost::none));
    EXPECT_EQ("cluster-0.local", address->getHost());
    EXPECT_EQ(26379, ntohs(address->getPort()));
}
