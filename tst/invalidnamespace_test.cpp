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

#include <sdl/invalidnamespace.hpp>
#include <sstream>
#include <gtest/gtest.h>
#include "private/namespacevalidator.hpp"

using namespace shareddatalayer;

TEST(InvalidNamespaceTest, CanThrowAndCatchInvalidNamespace)
{
    try
    {
        throw InvalidNamespace("invalid");
    }
    catch (const std::exception& e)
    {
        std::ostringstream expectedString;
        expectedString << "namespace string: \"invalid\" is invalid. Disallowed characters: " << getDisallowedCharactersInNamespace();
        EXPECT_STREQ(expectedString.str().c_str(), e.what());
    }
}
