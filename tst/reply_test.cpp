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
#include "private/redis/reply.hpp"

using namespace shareddatalayer::redis;

TEST(ReplyTest, IsNotCopyable)
{
    EXPECT_FALSE(std::is_copy_constructible<Reply>::value);
    EXPECT_FALSE(std::is_copy_assignable<Reply>::value);
}

TEST(ReplyTest, HasVirtualDestructor)
{
    EXPECT_TRUE(std::has_virtual_destructor<Reply>::value);
}

TEST(ReplyTest, IsAbstract)
{
    EXPECT_TRUE(std::is_abstract<Reply>::value);
}
