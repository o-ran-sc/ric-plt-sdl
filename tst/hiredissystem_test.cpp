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
#include "private/redis/hiredissystem.hpp"

using namespace shareddatalayer::redis;
using namespace testing;

TEST(HiredisSystemTest, IsDefaultConstructible)
{
    EXPECT_TRUE(std::is_default_constructible<HiredisSystem>::value);
}

TEST(HiredisSystemTest, IsNotCopyable)
{
    EXPECT_FALSE(std::is_copy_constructible<HiredisSystem>::value);
    EXPECT_FALSE(std::is_copy_assignable<HiredisSystem>::value);
}

TEST(HiredisSystemTest, HasVirtualDestructor)
{
    EXPECT_TRUE(std::has_virtual_destructor<HiredisSystem>::value);
}

TEST(HiredisSystemTest, CanCreateInstance)
{
    HiredisSystem& s1(HiredisSystem::getHiredisSystem());
    HiredisSystem& s2(HiredisSystem::getHiredisSystem());
    EXPECT_EQ(&s1, &s2);
}
