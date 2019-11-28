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
#include <gtest/gtest.h>
#include <async.h>
#include <sdl/errorqueries.hpp>
#include "private/redis/asyncredisstorage.hpp"
#include "private/error.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;
using namespace testing;

namespace
{
    std::string getErrorCodeMessage(std::error_code ec)
    {
        return ec.message();
    }

    class ErrorCodesTest: public testing::Test
    {
    public:
        ErrorCodesTest()
        {
        }

        virtual ~ErrorCodesTest()
        {
        }
    };
}

TEST_F(ErrorCodesTest, AllAsyncRedisCommandDispatcherErrorCodesHaveCorrectDescriptionMessage)
{
    std::error_code ec;

    for (AsyncRedisCommandDispatcherErrorCode aec = AsyncRedisCommandDispatcherErrorCode::SUCCESS; aec != AsyncRedisCommandDispatcherErrorCode::END_MARKER; ++aec)
    {
        switch (aec)
        {
            case AsyncRedisCommandDispatcherErrorCode::SUCCESS:
                ec = aec;
                EXPECT_EQ(std::error_code().message(), getErrorCodeMessage(ec));
                break;
            case AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR:
                ec = aec;
                EXPECT_EQ("redis error", getErrorCodeMessage(ec));
                break;
            case AsyncRedisCommandDispatcherErrorCode::CONNECTION_LOST:
                ec = aec;
                EXPECT_EQ("redis connection lost", getErrorCodeMessage(ec));
                break;
            case AsyncRedisCommandDispatcherErrorCode::PROTOCOL_ERROR:
                ec = aec;
                EXPECT_EQ("redis protocol error", getErrorCodeMessage(ec));
                break;
            case AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY:
                ec = aec;
                EXPECT_EQ("redis out of memory", getErrorCodeMessage(ec));
                break;
            case AsyncRedisCommandDispatcherErrorCode::DATASET_LOADING:
                ec = aec;
                EXPECT_EQ("redis dataset still being loaded into memory", getErrorCodeMessage(ec));
                break;
            case AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED:
                ec = aec;
                EXPECT_EQ("not connected to redis, SDL operation not started", getErrorCodeMessage(ec));
                break;
            case AsyncRedisCommandDispatcherErrorCode::IO_ERROR:
                ec = aec;
                EXPECT_EQ("redis I/O error", getErrorCodeMessage(ec));
                break;
            case AsyncRedisCommandDispatcherErrorCode::WRITING_TO_SLAVE:
                ec = aec;
                EXPECT_EQ("writing to slave", getErrorCodeMessage(ec));
                break;
            case AsyncRedisCommandDispatcherErrorCode::END_MARKER:
                ec = aec;
                EXPECT_EQ("unsupported error code for message()", getErrorCodeMessage(ec));
                break;
            default:
                FAIL() << "No mapping for AsyncRedisCommandDispatcherErrorCode value: " << aec;
                break;
        }
    }
}

TEST_F(ErrorCodesTest, AllAsyncRedisCommandDispatcherErrorCodesAreMappedToCorrectSDLInternalError)
{
    /* If this test case detects missing error code, remember to add new error code also to AllAsyncRedisCommandDispatcherErrorCodesAreMappedToCorrectClientErrorCode
     * test case (and add also mapping implementation from InternalError to Error if needed).
     */
    std::error_code ec;

    for (AsyncRedisCommandDispatcherErrorCode aec = AsyncRedisCommandDispatcherErrorCode::SUCCESS; aec != AsyncRedisCommandDispatcherErrorCode::END_MARKER; ++aec)
    {
        switch (aec)
        {
            case AsyncRedisCommandDispatcherErrorCode::SUCCESS:
                ec = aec;
                EXPECT_TRUE(ec == InternalError::SUCCESS);
                break;
            case AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR:
                ec = aec;
                EXPECT_TRUE(ec == InternalError::BACKEND_ERROR);
                break;
            case AsyncRedisCommandDispatcherErrorCode::CONNECTION_LOST:
                ec = aec;
                EXPECT_TRUE(ec == InternalError::BACKEND_CONNECTION_LOST);
                break;
            case AsyncRedisCommandDispatcherErrorCode::PROTOCOL_ERROR:
                ec = aec;
                EXPECT_TRUE(ec == InternalError::BACKEND_REJECTED_REQUEST);
                break;
            case AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY:
                ec = aec;
                EXPECT_TRUE(ec == InternalError::BACKEND_ERROR);
                break;
            case AsyncRedisCommandDispatcherErrorCode::DATASET_LOADING:
                ec = aec;
                EXPECT_TRUE(ec == InternalError::BACKEND_NOT_READY);
                break;
            case AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED:
                ec = aec;
                EXPECT_TRUE(ec == InternalError::SDL_NOT_CONNECTED_TO_BACKEND);
                break;
            case AsyncRedisCommandDispatcherErrorCode::IO_ERROR:
                ec = aec;
                EXPECT_TRUE(ec == InternalError::BACKEND_ERROR);
                break;
            case AsyncRedisCommandDispatcherErrorCode::WRITING_TO_SLAVE:
                ec = aec;
                EXPECT_TRUE(ec == InternalError::BACKEND_ERROR);
                break;
            case AsyncRedisCommandDispatcherErrorCode::END_MARKER:
                ec = aec;
                EXPECT_TRUE(ec == InternalError::SDL_ERROR_CODE_LOGIC_ERROR);
                break;
            default:
                FAIL() << "No mapping for AsyncRedisCommandDispatcherErrorCode value: " << aec;
                break;
        }
    }
}

TEST_F(ErrorCodesTest, AllErrorCodeEnumsAreMappedToCorrectClientErrorCode)
{
    std::error_code ec;

    ec = std::error_code();
    EXPECT_TRUE(ec == shareddatalayer::Error::SUCCESS);

    ec = AsyncRedisStorage::ErrorCode::SUCCESS;
    EXPECT_TRUE(ec == shareddatalayer::Error::SUCCESS);
    ec = AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED;
    EXPECT_TRUE(ec == shareddatalayer::Error::NOT_CONNECTED);
    ec = AsyncRedisStorage::ErrorCode::INVALID_NAMESPACE;
    EXPECT_TRUE(ec == shareddatalayer::Error::REJECTED_BY_SDL);
    ec = AsyncRedisStorage::ErrorCode::END_MARKER;
    EXPECT_TRUE(ec == shareddatalayer::Error::BACKEND_FAILURE);

    ec = AsyncRedisCommandDispatcherErrorCode::SUCCESS;
    EXPECT_TRUE(ec == shareddatalayer::Error::SUCCESS);
    ec = AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR;
    EXPECT_TRUE(ec == shareddatalayer::Error::BACKEND_FAILURE);
    ec = AsyncRedisCommandDispatcherErrorCode::CONNECTION_LOST;
    EXPECT_TRUE(ec == shareddatalayer::Error::OPERATION_INTERRUPTED);
    ec = AsyncRedisCommandDispatcherErrorCode::PROTOCOL_ERROR;
    EXPECT_TRUE(ec == shareddatalayer::Error::REJECTED_BY_BACKEND);
    ec = AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY;
    EXPECT_TRUE(ec == shareddatalayer::Error::BACKEND_FAILURE);
    ec = AsyncRedisCommandDispatcherErrorCode::DATASET_LOADING;
    EXPECT_TRUE(ec == shareddatalayer::Error::NOT_CONNECTED);
    ec = AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED;
    EXPECT_TRUE(ec == shareddatalayer::Error::NOT_CONNECTED);
    ec = AsyncRedisCommandDispatcherErrorCode::IO_ERROR;
    EXPECT_TRUE(ec == shareddatalayer::Error::BACKEND_FAILURE);
    ec = AsyncRedisCommandDispatcherErrorCode::END_MARKER;
    EXPECT_TRUE(ec == shareddatalayer::Error::BACKEND_FAILURE);
}

TEST_F(ErrorCodesTest, ErrorCodeEnumsDoNotMapToIncorrectClientErrorCode)
{
    std::error_code ec;

    ec = AsyncRedisStorage::ErrorCode::SUCCESS;
    EXPECT_TRUE(ec != shareddatalayer::Error::BACKEND_FAILURE);
    ec = AsyncRedisStorage::ErrorCode::REDIS_NOT_YET_DISCOVERED;
    EXPECT_TRUE(ec != shareddatalayer::Error::BACKEND_FAILURE);
    ec = AsyncRedisStorage::ErrorCode::INVALID_NAMESPACE;
    EXPECT_TRUE(ec != shareddatalayer::Error::BACKEND_FAILURE);
    ec = AsyncRedisStorage::ErrorCode::END_MARKER;
    EXPECT_TRUE(ec != shareddatalayer::Error::SUCCESS);

    ec = AsyncRedisCommandDispatcherErrorCode::SUCCESS;
    EXPECT_TRUE(ec != shareddatalayer::Error::NOT_CONNECTED);
    ec = AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR;
    EXPECT_TRUE(ec != shareddatalayer::Error::SUCCESS);
    ec = AsyncRedisCommandDispatcherErrorCode::CONNECTION_LOST;
    EXPECT_TRUE(ec != shareddatalayer::Error::BACKEND_FAILURE);
    ec = AsyncRedisCommandDispatcherErrorCode::PROTOCOL_ERROR;
    EXPECT_TRUE(ec != shareddatalayer::Error::NOT_CONNECTED);
    ec = AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY;
    EXPECT_TRUE(ec != shareddatalayer::Error::NOT_CONNECTED);
    ec = AsyncRedisCommandDispatcherErrorCode::DATASET_LOADING;
    EXPECT_TRUE(ec != shareddatalayer::Error::SUCCESS);
    ec = AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED;
    EXPECT_TRUE(ec != shareddatalayer::Error::SUCCESS);
    ec = AsyncRedisCommandDispatcherErrorCode::IO_ERROR;
    EXPECT_TRUE(ec != shareddatalayer::Error::OPERATION_INTERRUPTED);
    ec = AsyncRedisCommandDispatcherErrorCode::END_MARKER;
    EXPECT_TRUE(ec != shareddatalayer::Error::OPERATION_INTERRUPTED);
}

TEST_F(ErrorCodesTest, AllErrorCodeEnumClassesHaveCategory)
{
    EXPECT_STREQ("asyncrediscommanddispatcher",
                 std::error_code(AsyncRedisCommandDispatcherErrorCode::SUCCESS).category().name());

    EXPECT_STREQ("asyncredisstorage",
                 std::error_code(AsyncRedisStorage::ErrorCode::SUCCESS).category().name());
}
