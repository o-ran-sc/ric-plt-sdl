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
#include "private/namespacevalidator.hpp"
#include <sdl/invalidnamespace.hpp>
#include <sdl/emptynamespace.hpp>

using namespace shareddatalayer;

TEST(NamespaceValidatorTest, CanGetDisallowedCharactersInNamespace)
{
    EXPECT_EQ(",{}", getDisallowedCharactersInNamespace());
}

TEST(NamespaceValidatorTest, CanAcceptValidNamespaceSyntax)
{
    EXPECT_TRUE(isValidNamespaceSyntax("someValidNamespace"));
    EXPECT_TRUE(isValidNamespaceSyntax(""));
}

TEST(ValidateNamespaceTest, CanDetectDisallowedCharactersInNamespaceSyntax)
{
    EXPECT_FALSE(isValidNamespaceSyntax("some,IllegalNamespace"));
    EXPECT_FALSE(isValidNamespaceSyntax("someIllegal{Namespace"));
    EXPECT_FALSE(isValidNamespaceSyntax("someIllegalNamespace}"));
}

TEST(NamespaceValidatorTest, CanAcceptValidNamespace)
{
    EXPECT_NO_THROW(validateNamespace("someValidNamespace"));
    EXPECT_TRUE(isValidNamespace("someValidNamespace"));
}

TEST(NamespaceValidatorTest, WillNotAcceptEmptyNamespace)
{
    EXPECT_THROW(validateNamespace(""), EmptyNamespace);
    EXPECT_FALSE(isValidNamespace(""));
}

TEST(NamespaceValidatorTest, WillNotAcceptNamespaceContainingDisallowedCharacters)
{
    EXPECT_THROW(validateNamespace("some,IllegalNamespace"), InvalidNamespace);
    EXPECT_FALSE(isValidNamespace("some,IllegalNamespace"));
    EXPECT_THROW(validateNamespace("someIllegal{Namespace}"), InvalidNamespace);
    EXPECT_FALSE(isValidNamespace("someIllegal{Namespace}"));
    EXPECT_THROW(validateNamespace("some,Illegal{Namespace}"), InvalidNamespace);
    EXPECT_FALSE(isValidNamespace("some,Illegal{Namespace}"));
}
