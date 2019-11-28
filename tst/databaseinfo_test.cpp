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

#include <arpa/inet.h>
#include <type_traits>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "private/redis/databaseinfo.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;
using namespace testing;

namespace
{
    static const std::string defaultAddress = "address";
    static const uint16_t defaultPort = 3333;

    DatabaseInfo getDatabaseInfo(DatabaseInfo::Type type = DatabaseInfo::Type::SINGLE,
                                 std::string address = defaultAddress,
                                 uint16_t port = defaultPort)
    {
        DatabaseInfo databaseInfo;
        databaseInfo.type = type;
        databaseInfo.hosts.push_back({address, htons(port)});
        databaseInfo.ns = "namespace";
        return databaseInfo;
    }

    DatabaseInfo& addHost(DatabaseInfo& databaseInfo,
                          std::string address,
                          uint16_t port)
    {
        databaseInfo.hosts.push_back({address, htons(port)});
        return databaseInfo;
    }

    class DatabaseInfoTest: public testing::Test
    {
    public:
        DatabaseInfoTest()
        {
        }

        virtual ~DatabaseInfoTest()
        {
        }
    };
}

TEST_F(DatabaseInfoTest, EqualAndUnequalOperatorsIdenticalInfos)
{
    InSequence dummy;
    DatabaseInfo databaseInfo1;
    DatabaseInfo databaseInfo2;

    databaseInfo1 = getDatabaseInfo();
    databaseInfo2 = getDatabaseInfo();

    EXPECT_TRUE(databaseInfo1 == databaseInfo2);
    EXPECT_FALSE(databaseInfo1 != databaseInfo2);
}

TEST_F(DatabaseInfoTest, EqualOperatorDifferentAmountOfHosts)
{
    InSequence dummy;
    DatabaseInfo databaseInfo1;
    DatabaseInfo databaseInfo2;

    databaseInfo1 = getDatabaseInfo();
    databaseInfo2 = getDatabaseInfo();
    databaseInfo2 = addHost(databaseInfo2,
                            "address2",
                            123);

    EXPECT_FALSE(databaseInfo1 == databaseInfo2);
}

TEST_F(DatabaseInfoTest, EqualOperatorHostsInDifferentOrder)
{
    InSequence dummy;
    DatabaseInfo databaseInfo1;
    DatabaseInfo databaseInfo2;

    databaseInfo1 = getDatabaseInfo(DatabaseInfo::Type::SINGLE,
                                    "address2",
                                    123);
    databaseInfo1 = addHost(databaseInfo1,
                            defaultAddress,
                            defaultPort);
    databaseInfo2 = getDatabaseInfo();
    databaseInfo2 = addHost(databaseInfo2,
                            "address2",
                            123);

    EXPECT_TRUE(databaseInfo1 == databaseInfo2);
}

TEST_F(DatabaseInfoTest, EqualOperatorDiffrentAddressInHosts)
{
    InSequence dummy;
    DatabaseInfo databaseInfo1;
    DatabaseInfo databaseInfo2;

    databaseInfo1 = getDatabaseInfo(DatabaseInfo::Type::SINGLE,
                                    "address1",
                                    123);
    databaseInfo2 = getDatabaseInfo(DatabaseInfo::Type::SINGLE,
                                    "address2",
                                    123);

    EXPECT_FALSE(databaseInfo1 == databaseInfo2);
}

 TEST_F(DatabaseInfoTest, EqualOperatorDiffrentPortInHosts)
{
    InSequence dummy;
    DatabaseInfo databaseInfo1;
    DatabaseInfo databaseInfo2;

    databaseInfo1 = getDatabaseInfo(DatabaseInfo::Type::SINGLE,
                                    "address1",
                                    123);
    databaseInfo2 = getDatabaseInfo(DatabaseInfo::Type::SINGLE,
                                    "address1",
                                    1234);

    EXPECT_FALSE(databaseInfo1 == databaseInfo2);
}
