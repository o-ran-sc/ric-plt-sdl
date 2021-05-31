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
#include <type_traits>
#include "config.h"
#include "private/asyncdummystorage.hpp"
#include "private/asyncstorageimpl.hpp"
#include "private/createlogger.hpp"
#include "private/logger.hpp"
#include "private/redis/asyncredisstorage.hpp"
#include "private/tst/enginemock.hpp"
#include "private/tst/asyncdatabasediscoverymock.hpp"
#include "private/tst/databaseconfigurationmock.hpp"
#include "private/tst/namespaceconfigurationsmock.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class AsyncStorageImplTest: public testing::Test
    {
    public:
        std::shared_ptr<StrictMock<EngineMock>> engineMock;
        std::shared_ptr<DatabaseConfiguration> dummyDatabaseConfiguration;
        std::shared_ptr<StrictMock<NamespaceConfigurationsMock>> namespaceConfigurationsMock;
        std::shared_ptr<NiceMock<AsyncDatabaseDiscoveryMock>> discoveryMock;
        int fd;
        AsyncStorage::Namespace ns;
        Engine::Callback storedCallback;
        std::unique_ptr<AsyncStorageImpl> asyncStorageImpl;
        std::shared_ptr<Logger> logger;

        AsyncStorageImplTest():
            engineMock(std::make_shared<StrictMock<EngineMock>>()),
            dummyDatabaseConfiguration(std::make_shared<DatabaseConfigurationImpl>()),
            namespaceConfigurationsMock(std::make_shared<StrictMock<NamespaceConfigurationsMock>>()),
            discoveryMock(std::make_shared<NiceMock<AsyncDatabaseDiscoveryMock>>()),
            fd(10),
            ns("someKnownNamespace"),
            logger(createLogger(SDL_LOG_PREFIX))
        {
            dummyDatabaseConfiguration->checkAndApplyDbType("redis-standalone");
            dummyDatabaseConfiguration->checkAndApplyServerAddress("dummydatabaseaddress.local");
            asyncStorageImpl.reset(new AsyncStorageImpl(engineMock,
                                                        boost::none,
                                                        dummyDatabaseConfiguration,
                                                        namespaceConfigurationsMock,
                                                        logger,
                                                        std::bind(&AsyncStorageImplTest::asyncDatabaseDiscoveryCreator,
                                                                  this,
                                                                  std::placeholders::_1,
                                                                  std::placeholders::_2,
                                                                  std::placeholders::_3,
                                                                  std::placeholders::_4,
                                                                  std::placeholders::_5)));
        }

        std::shared_ptr<redis::AsyncDatabaseDiscovery> asyncDatabaseDiscoveryCreator(std::shared_ptr<Engine>,
                                                                                     const std::string&,
                                                                                     const DatabaseConfiguration&,
                                                                                     const boost::optional<std::size_t>&,
                                                                                     std::shared_ptr<Logger>)
        {
            return discoveryMock;
        }

        void expectNamespaceConfigurationIsDbBackendUseEnabled_returnFalse()
        {
            EXPECT_CALL(*namespaceConfigurationsMock, isDbBackendUseEnabled(ns)).
                WillOnce(Return(false));
        }

        void expectNamespaceConfigurationIsDbBackendUseEnabled_returnTrue()
        {
            EXPECT_CALL(*namespaceConfigurationsMock, isDbBackendUseEnabled(ns)).
                WillOnce(Return(true));
        }

        void expectPostCallback()
        {
            EXPECT_CALL(*engineMock, postCallback(_))
                .Times(1);
        }
    };
}

TEST_F(AsyncStorageImplTest, IsNotCopyableAndIsNotMovable)
{
    EXPECT_FALSE(std::is_copy_assignable<AsyncStorageImpl>::value);
    EXPECT_FALSE(std::is_move_assignable<AsyncStorageImpl>::value);
    EXPECT_FALSE(std::is_copy_constructible<AsyncStorageImpl>::value);
    EXPECT_FALSE(std::is_move_constructible<AsyncStorageImpl>::value);
}

TEST_F(AsyncStorageImplTest, ImplementsAsyncStorage)
{
    EXPECT_TRUE((std::is_base_of<AsyncStorage, AsyncStorageImpl>::value));
}

TEST_F(AsyncStorageImplTest, CanGetFd)
{
    EXPECT_CALL(*engineMock, fd())
        .Times(1)
        .WillOnce(Return(fd));
    EXPECT_EQ(fd, asyncStorageImpl->fd());
}

TEST_F(AsyncStorageImplTest, CanHandleEvents)
{
    EXPECT_CALL(*engineMock, handleEvents())
        .Times(1);
    asyncStorageImpl->handleEvents();
}

TEST_F(AsyncStorageImplTest, CorrectHandlerIsUsedBasedOnConfiguration)
{
    InSequence dummy;
    expectNamespaceConfigurationIsDbBackendUseEnabled_returnTrue();
    AsyncStorage& returnedHandler1 = asyncStorageImpl->getOperationHandler(ns);
    EXPECT_EQ(typeid(AsyncRedisStorage&), typeid(returnedHandler1));

    expectNamespaceConfigurationIsDbBackendUseEnabled_returnFalse();
    AsyncStorage& returnedHandler2 = asyncStorageImpl->getOperationHandler(ns);
    EXPECT_EQ(typeid(AsyncDummyStorage&), typeid(returnedHandler2));
}

TEST_F(AsyncStorageImplTest, CorrectSdlClusterHandlerIsUsedBasedOnConfiguration)
{
    expectNamespaceConfigurationIsDbBackendUseEnabled_returnTrue();
    dummyDatabaseConfiguration->checkAndApplyDbType("sdl-sentinel-cluster");
    AsyncStorage& returnedHandler = asyncStorageImpl->getOperationHandler(ns);
    EXPECT_EQ(typeid(AsyncRedisStorage&), typeid(returnedHandler));
}
