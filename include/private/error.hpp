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

#ifndef SHAREDDATALAYER_ERROR_HPP_
#define SHAREDDATALAYER_ERROR_HPP_

#include <system_error>

/* Error codes defined here are for SDL internal usage only.
 * Error codes meant to be used by SDL clients are defined in sdl/errorqueries.hpp.
 *
 * Error code handling in SDL goes following way:
 *
 * - Implementation sets so called implementation specific error codes (e.g AsyncRedisCommandDispatcherErrorCode
 *   and AsyncRedisConnection::ErrorCode).
 * - All implementation specific error codes have mapping to InternalErrors. This mapping is handled by overriding
 *   std::error_category default_error_condition function in all implementation specific error code categories.
 * - All InternalErrors are mapped to shareddatalayer::Errors which SDL client can conveniently use in
 *   its error handling implemention. This mapping is handled by overriding std::error_category equivalent function in
 *   SharedDataLayerErrorCategory.
 * - Async API error codes have corresponding exceptions in sync API. When modifying other one (async API error codes or
 *   sync API exceptions) check the need for similar modification to other one also.
 *
 * When adding a new implementation specific error code, do also needed updates to mappings described above.
 * At least mapping from new implementation specific error code error code to InternalError is always needed.
 */

namespace shareddatalayer
{
    enum class InternalError
    {
        SUCCESS = 0,
        SDL_NOT_READY,
        SDL_NOT_CONNECTED_TO_BACKEND,
        SDL_ERROR_CODE_LOGIC_ERROR,
        SDL_RECEIVED_INVALID_PARAMETER,
        BACKEND_CONNECTION_LOST,
        BACKEND_NOT_READY,
        BACKEND_REJECTED_REQUEST,
        BACKEND_ERROR
    };

    std::error_condition make_error_condition(shareddatalayer::InternalError ec);

    namespace redis
    {
        enum class AsyncRedisCommandDispatcherErrorCode
        {
            SUCCESS = 0,
            UNKNOWN_ERROR,
            CONNECTION_LOST,
            PROTOCOL_ERROR,
            OUT_OF_MEMORY,
            DATASET_LOADING,
            NOT_CONNECTED,
            IO_ERROR,
            WRITING_TO_SLAVE,
            //Keep this always as last item. Used in unit tests to loop all enum values.
            END_MARKER
        };

        std::error_code make_error_code(AsyncRedisCommandDispatcherErrorCode errorCode);
        AsyncRedisCommandDispatcherErrorCode& operator++ (AsyncRedisCommandDispatcherErrorCode& ecEnum);
    }
}

namespace std
{
    template <>
    struct is_error_condition_enum<shareddatalayer::InternalError> : public true_type { };

    template <>
    struct is_error_code_enum<shareddatalayer::redis::AsyncRedisCommandDispatcherErrorCode>: public true_type { };
}

#endif
