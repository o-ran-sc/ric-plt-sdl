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
#include "private/engine.hpp"

using namespace shareddatalayer;
using namespace testing;

TEST(EngineTest, HasVirtualDestructor)
{
    EXPECT_TRUE(std::has_virtual_destructor<Engine>::value);
}

TEST(EngineTest, IsAbstract)
{
    EXPECT_TRUE(std::is_abstract<Engine>::value);
}

TEST(EngineTest, IsNotCopyableAndIsNotMovable)
{
    EXPECT_FALSE(std::is_copy_assignable<Engine>::value);
    EXPECT_FALSE(std::is_move_assignable<Engine>::value);
    EXPECT_FALSE(std::is_copy_constructible<Engine>::value);
    EXPECT_FALSE(std::is_move_constructible<Engine>::value);
}
