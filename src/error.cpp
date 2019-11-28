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

#include <sstream>
#include "private/createlogger.hpp"
#include "private/error.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;

namespace
{
    /* Error codes under this category are not set directly. All SDL implementation specific error codes can be
     * mapped to these error codes. These error codes are further on mapped to error codes which are available to
     * SDL clients (shareddatalayer::Error category).
     * We could directly map implementation specific error codes to client error codes but having this intermediate
     * mapping gives some benefits:
     *  - We can easily provide more error categories for clients (e.g. severity, etc.) than just client error code
     *    classification if needed.
     *  - We can implement SDL internal error handling logic based on these internal error codes if needed.
     *  - If implementation specific error would be directly mapped to client error codes, mapping implementation would
     *    easily be quite complicated (especially if the amount of implementation specific errors/error categories increases).
     */
    class InternalErrorCategory : public std::error_category
    {
    public:
      InternalErrorCategory() = default;
      const char* name() const noexcept override;
      std::string message(int condition) const override;
    };

    const char* InternalErrorCategory::name() const noexcept
    {
        return "SDL-internal-errorcodes";
    }

    std::string InternalErrorCategory::message(int) const
    {
        return "Only for SDL internal usage.";
    }

    const std::error_category& getInternalErrorCategory() noexcept
    {
        static const InternalErrorCategory theInternalErrorCategory;
        return theInternalErrorCategory;
    }

    /* This error category is used by both AsyncHiredisCommandDispatcher and AsyncHiredisClusterCommandDispatcher,
     * thus it is defined here. Error categories related to single class are defined in the files of the corresponding class.
     * AsyncHiredisCommandDispatcher and AsyncHiredisClusterCommandDispatcher can use common error category as error
     * handling is identical in those two classes. Also, only one of those classes is always used at a time (depending
     * on deployment).
     */
    class AsyncRedisCommandDispatcherErrorCategory: public std::error_category
    {
    public:
        AsyncRedisCommandDispatcherErrorCategory() = default;

        const char* name() const noexcept override;

        std::string message(int condition) const override;

        std::error_condition default_error_condition(int condition) const noexcept override;
    };

    const char* AsyncRedisCommandDispatcherErrorCategory::name() const noexcept
    {
        /* As the correct dispacther is selected during runtime, we do not known here (without additional implementation)
         * which dispatcher (redis/rediscluster) is currently in use. At least for now, we do not indicate in error
         * category name the exact dispacther type but return the same name for all dispatchers.
         * Main reason for this decision was that in error investigation situations we anyway need to have some other efficient
         * way to figure out what kind of deployment was used as there can be error situations which do not generate any error
         * code. Thus it was not seen worth the errort to implement correct dispatcher name display to error category name.
         * Detailed dispacther name display can be added later if a need for that arises.
         */
        return "asyncrediscommanddispatcher";
    }

    std::string AsyncRedisCommandDispatcherErrorCategory::message(int condition) const
    {
        switch (static_cast<AsyncRedisCommandDispatcherErrorCode>(condition))
        {
            case AsyncRedisCommandDispatcherErrorCode::SUCCESS:
                return std::error_code().message();
            case AsyncRedisCommandDispatcherErrorCode::CONNECTION_LOST:
                return "redis connection lost";
            case AsyncRedisCommandDispatcherErrorCode::PROTOCOL_ERROR:
                return "redis protocol error";
            case AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY:
                return "redis out of memory";
            case AsyncRedisCommandDispatcherErrorCode::DATASET_LOADING:
                return "redis dataset still being loaded into memory";
            case AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED:
                return "not connected to redis, SDL operation not started";
            case AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR:
                return "redis error";
            case AsyncRedisCommandDispatcherErrorCode::IO_ERROR:
                return "redis I/O error";
            case AsyncRedisCommandDispatcherErrorCode::WRITING_TO_SLAVE:
                return "writing to slave";
            case AsyncRedisCommandDispatcherErrorCode::END_MARKER:
                logErrorOnce("AsyncRedisCommandDispatcherErrorCode::END_MARKER is not meant to be queried (it is only for enum loop control)");
                return "unsupported error code for message()";
            default:
                return "description missing for AsyncRedisCommandDispatcherErrorCategory error: " + std::to_string(condition);
        }
    }

    std::error_condition AsyncRedisCommandDispatcherErrorCategory::default_error_condition(int condition) const noexcept
    {
        switch (static_cast<AsyncRedisCommandDispatcherErrorCode>(condition))
        {
            case AsyncRedisCommandDispatcherErrorCode::SUCCESS:
                return InternalError::SUCCESS;
            case AsyncRedisCommandDispatcherErrorCode::CONNECTION_LOST:
                return InternalError::BACKEND_CONNECTION_LOST;
            case AsyncRedisCommandDispatcherErrorCode::PROTOCOL_ERROR:
                return InternalError::BACKEND_REJECTED_REQUEST;
            case AsyncRedisCommandDispatcherErrorCode::OUT_OF_MEMORY:
                return InternalError::BACKEND_ERROR;
            case AsyncRedisCommandDispatcherErrorCode::DATASET_LOADING:
                return InternalError::BACKEND_NOT_READY;
            case AsyncRedisCommandDispatcherErrorCode::NOT_CONNECTED:
                return InternalError::SDL_NOT_CONNECTED_TO_BACKEND;
            case AsyncRedisCommandDispatcherErrorCode::UNKNOWN_ERROR:
                return InternalError::BACKEND_ERROR;
            case AsyncRedisCommandDispatcherErrorCode::IO_ERROR:
                return InternalError::BACKEND_ERROR;
            case AsyncRedisCommandDispatcherErrorCode::WRITING_TO_SLAVE:
                return InternalError::BACKEND_ERROR;
            case AsyncRedisCommandDispatcherErrorCode::END_MARKER:
                logErrorOnce("AsyncRedisCommandDispatcherErrorCode::END_MARKER is not meant to be mapped to InternalError (it is only for enum loop control)");
                return InternalError::SDL_ERROR_CODE_LOGIC_ERROR;
            default:
                std::ostringstream msg;
                msg << "default error condition missing for AsyncRedisCommandDispatcherErrorCategory error:  "
                    << condition;
                logErrorOnce(msg.str());
                return InternalError::SDL_ERROR_CODE_LOGIC_ERROR;
        }
    }

    const std::error_category& getAsyncRedisCommandDispatcherErrorCategory() noexcept
    {
        static const AsyncRedisCommandDispatcherErrorCategory theAsyncRedisCommandDispatcherErrorCategory;
        return theAsyncRedisCommandDispatcherErrorCategory;
    }
}

namespace shareddatalayer
{
    std::error_condition make_error_condition(InternalError errorCode)
    {
      return {static_cast<int>(errorCode), getInternalErrorCategory()};
    }

    namespace redis
    {
        std::error_code make_error_code(AsyncRedisCommandDispatcherErrorCode errorCode)
        {
            return std::error_code(static_cast<int>(errorCode), getAsyncRedisCommandDispatcherErrorCategory());
        }

        AsyncRedisCommandDispatcherErrorCode& operator++ (AsyncRedisCommandDispatcherErrorCode& ecEnum)
        {
            if (ecEnum == AsyncRedisCommandDispatcherErrorCode::END_MARKER)
                throw std::out_of_range("for AsyncRedisCommandDispatcherErrorCode& operator ++");

            ecEnum = AsyncRedisCommandDispatcherErrorCode(static_cast<std::underlying_type<AsyncRedisCommandDispatcherErrorCode>::type>(ecEnum) + 1);
            return ecEnum;
        }
    }
}
