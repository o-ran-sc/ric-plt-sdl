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
#include <sdl/asyncstorage.hpp>
#include <sdl/invalidnamespace.hpp>
#include "private/namespacevalidator.hpp"

using namespace shareddatalayer;

TEST(AsyncStorageTest, IsNotCopyable)
{
    EXPECT_FALSE(std::is_copy_constructible<AsyncStorage>::value);
    EXPECT_FALSE(std::is_copy_assignable<AsyncStorage>::value);
}

TEST(AsyncStorageTest, HasVirtualDestructor)
{
    EXPECT_TRUE(std::has_virtual_destructor<AsyncStorage>::value);
}

TEST(AsyncStorageTest, IsAbstract)
{
    EXPECT_TRUE(std::is_abstract<AsyncStorage>::value);
}

TEST(AsyncStorageTest, CommaIsTheSeparator)
{
    EXPECT_EQ(',', AsyncStorage::SEPARATOR);
}

TEST(AsyncStorageTest, CanThrowWhenDisallowedSeparatorCharacterIsUsedInNamespace)
{
    const std::string notValidNamespace(std::string("someNamespace") + AsyncStorage::SEPARATOR);
    EXPECT_THROW(validateNamespace(notValidNamespace), InvalidNamespace);
}

TEST(AsyncStorageTest, AsyncStorageCreateInstanceHasCorrectType)
{
    auto asyncStorageInstance(shareddatalayer::AsyncStorage::create());
    EXPECT_EQ(typeid(std::unique_ptr<AsyncStorage>), typeid(asyncStorageInstance));
}
