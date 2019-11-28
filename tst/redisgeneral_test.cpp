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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "private/redis/asyncredisreply.hpp"
#include "private/redis/redisgeneral.hpp"
#include "private/tst/redisreplybuilder.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class RedisGeneralTest: public testing::Test
    {
    public:
        RedisReplyBuilder redisReplyBuilder;
        RedisGeneralTest():
            redisReplyBuilder { }
        {
        }

        virtual ~RedisGeneralTest()
        {
        }
    };
}

TEST_F(RedisGeneralTest, ParseCommandListReplySuccessfully)
{
    std::set<std::string> requiredRedisModuleCommands(getRequiredRedisModuleCommands());
    const AsyncRedisReply replyHavingAllRequiredCommands(redisReplyBuilder.buildCommandListQueryReply());
    std::set<std::string> parsedRedisModuleCommands(parseCommandListReply(replyHavingAllRequiredCommands));
    EXPECT_THAT(requiredRedisModuleCommands, ContainerEq(parsedRedisModuleCommands));
}

TEST_F(RedisGeneralTest, CheckRedisModuleCommandsAllCommandsFound)
{
    std::set<std::string> requiredRedisModuleCommands(getRequiredRedisModuleCommands());
    EXPECT_TRUE(checkRedisModuleCommands(requiredRedisModuleCommands));
}

TEST_F(RedisGeneralTest, CheckRedisModuleCommandsOneCommandMissing)
{
    std::set<std::string> requiredRedisModuleCommands(getRequiredRedisModuleCommands());
    auto commandToRemove = --requiredRedisModuleCommands.end();
    requiredRedisModuleCommands.erase(commandToRemove);
    EXPECT_FALSE(checkRedisModuleCommands(requiredRedisModuleCommands));
}

TEST_F(RedisGeneralTest, CheckRedisModuleCommandsAllCommandsMissing)
{
    std::set<std::string> requiredRedisModuleCommands({});
    EXPECT_FALSE(checkRedisModuleCommands(requiredRedisModuleCommands));
}
