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

#include <type_traits>
#include <memory>
#include <cstring>
#include <arpa/inet.h>
#include <gtest/gtest.h>
#include "private/createlogger.hpp"
#include "private/databaseconfigurationimpl.hpp"
#include "private/logger.hpp"
#include "private/redis/asynchiredisdatabasediscovery.hpp"
#include "private/redis/databaseinfo.hpp"
#include "private/tst/enginemock.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class AsyncHiredisDatabaseDiscoveryBaseTest: public testing::Test
    {
    public:
        shareddatalayer::DatabaseConfiguration::Addresses someKnownStandaloneDatabaseAddress;
        shareddatalayer::DatabaseConfiguration::Addresses someKnownClusterDatabaseAddresses;
        std::shared_ptr<StrictMock<EngineMock>> engineMock;
        const std::string defaultNamespaceName;
        std::unique_ptr<AsyncHiredisDatabaseDiscovery> standaloneDiscovery;
        std::unique_ptr<AsyncHiredisDatabaseDiscovery> clusterDiscovery;
        Engine::Callback postedCallback;
        std::shared_ptr<Logger> logger;

        MOCK_METHOD1(stateChangedCb, void(const DatabaseInfo&));

        AsyncHiredisDatabaseDiscoveryBaseTest():
            someKnownStandaloneDatabaseAddress({ HostAndPort("addr1", htons(28416)) }),
            someKnownClusterDatabaseAddresses({ HostAndPort("addr1", htons(28416)),
                                                HostAndPort("addr2", htons(56832)) }),
            engineMock(std::make_shared<StrictMock<EngineMock>>()),
            defaultNamespaceName("namespace"),
            logger(createLogger(SDL_LOG_PREFIX))
        {
        }

        virtual ~AsyncHiredisDatabaseDiscoveryBaseTest()
        {
        }

        void expectStateChangedCbForStandalone()
        {
            EXPECT_CALL(*this, stateChangedCb(_))
                .Times(1)
                .WillOnce(Invoke([this](const DatabaseInfo& databaseInfo)
                                 {
                                     EXPECT_THAT(someKnownStandaloneDatabaseAddress, ContainerEq(databaseInfo.hosts));
                                     EXPECT_EQ(DatabaseInfo::Type::SINGLE, databaseInfo.type);
                                     EXPECT_STREQ(defaultNamespaceName.c_str(), databaseInfo.ns->c_str());
                                 }));
        }

        void expectStateChangedCbForCluster()
        {
            EXPECT_CALL(*this, stateChangedCb(_))
                .Times(1)
                .WillOnce(Invoke([this](const DatabaseInfo& databaseInfo)
                                 {
                                     EXPECT_THAT(someKnownClusterDatabaseAddresses, ContainerEq(databaseInfo.hosts));
                                     EXPECT_EQ(DatabaseInfo::Type::CLUSTER, databaseInfo.type);
                                     EXPECT_EQ(DatabaseInfo::Discovery::HIREDIS, databaseInfo.discovery);
                                     EXPECT_STREQ(defaultNamespaceName.c_str(), databaseInfo.ns->c_str());
                                 }));
        }

        void expectPostStateChangedCb()
        {
            EXPECT_CALL(*engineMock, postCallback(_))
                .Times(1)
                .WillOnce(SaveArg<0>(&postedCallback));
        }

        void callPostedCallback()
        {
            ASSERT_NE(postedCallback, nullptr);
            postedCallback();
        }
    };

    class AsyncHiredisDatabaseDiscoveryTest: public AsyncHiredisDatabaseDiscoveryBaseTest
    {
    public:
        AsyncHiredisDatabaseDiscoveryTest()
        {
            InSequence dummy;
            standaloneDiscovery.reset(new AsyncHiredisDatabaseDiscovery(engineMock,
                                                                        defaultNamespaceName,
                                                                        DatabaseInfo::Type::SINGLE,
                                                                        someKnownStandaloneDatabaseAddress,
                                                                        logger));
            clusterDiscovery.reset(new AsyncHiredisDatabaseDiscovery(engineMock,
                                                                     defaultNamespaceName,
                                                                     DatabaseInfo::Type::CLUSTER,
                                                                     someKnownClusterDatabaseAddresses,
                                                                     logger));
        }

        ~AsyncHiredisDatabaseDiscoveryTest()
        {
        }
    };
}

TEST_F(AsyncHiredisDatabaseDiscoveryBaseTest, IsNotCopyable)
{
    InSequence dummy;
    EXPECT_FALSE(std::is_copy_constructible<AsyncHiredisDatabaseDiscovery>::value);
    EXPECT_FALSE(std::is_copy_assignable<AsyncHiredisDatabaseDiscovery>::value);
}

TEST_F(AsyncHiredisDatabaseDiscoveryBaseTest, ImplementsAsyncDatabaseDiscovery)
{
    InSequence dummy;
    EXPECT_TRUE((std::is_base_of<AsyncDatabaseDiscovery, AsyncHiredisDatabaseDiscovery>::value));
}

TEST_F(AsyncHiredisDatabaseDiscoveryTest, StateChangedCallbackIsPostedImmediately)
{
    InSequence dummy;
    expectPostStateChangedCb();
    clusterDiscovery->setStateChangedCb(std::bind(&AsyncHiredisDatabaseDiscoveryTest::stateChangedCb,
                                                  this,
                                                  std::placeholders::_1));
    expectStateChangedCbForCluster();
    callPostedCallback();

    expectPostStateChangedCb();
    standaloneDiscovery->setStateChangedCb(std::bind(&AsyncHiredisDatabaseDiscoveryTest::stateChangedCb,
                                                     this,
                                                     std::placeholders::_1));
    expectStateChangedCbForStandalone();
    callPostedCallback();
}
