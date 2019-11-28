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

#include "config.h"
#include <type_traits>
#include <memory>
#include <cstring>
#include <gtest/gtest.h>
#include <async.h>
#include <sdl/errorqueries.hpp>

using namespace shareddatalayer;
using namespace testing;

namespace
{
    std::string getErrorConditionMessage(std::error_condition ec)
    {
        return ec.message();
    }

    class ErrorCodeQueriesTest: public testing::Test
    {
    public:
        ErrorCodeQueriesTest()
        {
        }

        virtual ~ErrorCodeQueriesTest()
        {
        }
    };
}

TEST_F(ErrorCodeQueriesTest, AllClientErrorCodeEnumsHaveCorrectDescriptionMessage)
{
    EXPECT_EQ(std::error_code().message(), getErrorConditionMessage(shareddatalayer::Error::SUCCESS));
    EXPECT_EQ("shareddatalayer not connected to backend data storage", getErrorConditionMessage(shareddatalayer::Error::NOT_CONNECTED));
    EXPECT_EQ("shareddatalayer sent the request to backend data storage but did not receive reply",getErrorConditionMessage(shareddatalayer::Error::OPERATION_INTERRUPTED));
    EXPECT_EQ("backend data storage failed to process the request", getErrorConditionMessage(shareddatalayer::Error::BACKEND_FAILURE));
}
