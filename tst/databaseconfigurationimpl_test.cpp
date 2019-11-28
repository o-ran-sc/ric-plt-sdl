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
    EXPECT_EQ(boost::none, databaseConfigurationImpl->getSentinelAddress());
}

TEST_F(DatabaseConfigurationImplTest, CanApplyAndReturnSentinelAddress)
{
    databaseConfigurationImpl->checkAndApplySentinelAddress("dummydatabaseaddress.local:1234");
    auto address = databaseConfigurationImpl->getSentinelAddress();
    EXPECT_NE(boost::none, databaseConfigurationImpl->getSentinelAddress());
    EXPECT_EQ("dummydatabaseaddress.local", address->getHost());
    EXPECT_EQ(1234, ntohs(address->getPort()));
}

TEST_F(DatabaseConfigurationImplTest, DefaultSentinelMasterNameIsEmpty)
{
    EXPECT_EQ("", databaseConfigurationImpl->getSentinelMasterName());
}

TEST_F(DatabaseConfigurationImplTest, CanApplyAndReturnSentinelMasterName)
{
    databaseConfigurationImpl->checkAndApplySentinelMasterName("mymaster");
    EXPECT_EQ("mymaster", databaseConfigurationImpl->getSentinelMasterName());
}
