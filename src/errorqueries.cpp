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
#include "config.h"
#include <sdl/errorqueries.hpp>
#include "private/createlogger.hpp"
#include "private/error.hpp"

using namespace shareddatalayer;

namespace
{
    class SharedDataLayerErrorCategory : public std::error_category
    {
    public:
      SharedDataLayerErrorCategory() = default;
      const char* name() const noexcept override;
      std::string message(int condition) const override;
      bool equivalent(const std::error_code& ec, int condition) const noexcept override;
    };

    const char* SharedDataLayerErrorCategory::name() const noexcept
    {
        return "shareddatalayer-errorcodes";
    }

    std::string SharedDataLayerErrorCategory::message(int condition) const
    {
        switch (static_cast<shareddatalayer::Error>(condition))
        {
            case shareddatalayer::Error::SUCCESS:
                return std::error_code().message();
            case shareddatalayer::Error::NOT_CONNECTED:
                return "shareddatalayer not connected to backend data storage";
            case shareddatalayer::Error::OPERATION_INTERRUPTED:
                return "shareddatalayer sent the request to backend data storage but did not receive reply";
            case shareddatalayer::Error::BACKEND_FAILURE:
                return "backend data storage failed to process the request";
            case shareddatalayer::Error::REJECTED_BY_BACKEND:
                return "backend data storage rejected the request";
            case shareddatalayer::Error::REJECTED_BY_SDL:
                return "SDL rejected the request";
            default:
                return "description missing for SharedDataLayerErrorCategory error: " + std::to_string(condition);
        }
    }

    const std::error_category& getSharedDataLayerErrorCategory() noexcept
    {
        static const SharedDataLayerErrorCategory theSharedDataLayerErrorCategory;
        return theSharedDataLayerErrorCategory;
    }


    bool SharedDataLayerErrorCategory::equivalent(const std::error_code& ec, int condition) const noexcept
    {
        /* Reasoning for possibly not self-evident mappings:
            - InternalError::BACKEND_NOT_READY is mapped to shareddatalayer::Error::NOT_CONNECTED
              even though we are connected to backend in that situation. Client handling for InternalError::BACKEND_NOT_READY
              situation is identical than handling for other shareddatalayer::Error::NOT_CONNECTED client error cases.
              To have minimal amount of  client error codes we map it to that.
            - InternalError::SDL_ERROR_CODE_LOGIC_ERROR is mapped to shareddatalayer::Error::BACKEND_FAILURE.
              That internal failure should never happen. If it does happen, we cannot know here which kind of error
              has really happened. Thus we map to the most severe client error code (other places will write logs about this).
         */
        switch (static_cast<shareddatalayer::Error>(condition))
        {
            case shareddatalayer::Error::SUCCESS:
                return ec == InternalError::SUCCESS ||
                       ec == std::error_code();
            case shareddatalayer::Error::NOT_CONNECTED:
                return ec == InternalError::SDL_NOT_CONNECTED_TO_BACKEND ||
                       ec == InternalError::BACKEND_NOT_READY ||
                       ec == InternalError::SDL_NOT_READY;
            case shareddatalayer::Error::OPERATION_INTERRUPTED:
                return ec == InternalError::BACKEND_CONNECTION_LOST;
            case shareddatalayer::Error::BACKEND_FAILURE:
                return ec == InternalError::BACKEND_ERROR ||
                       ec == InternalError::SDL_ERROR_CODE_LOGIC_ERROR;
            case shareddatalayer::Error::REJECTED_BY_BACKEND:
                return ec == InternalError::BACKEND_REJECTED_REQUEST;
            case shareddatalayer::Error::REJECTED_BY_SDL:
                return ec == InternalError::SDL_RECEIVED_INVALID_PARAMETER;
            default:
                /* Since clients can compare shareddatalayer::Error conditions against any std::error_code based ec, this error log
                 * can occur without fault in SDL implementation. If error log appears:
                 *  - First check is the problematic ec set in SDL implementation
                 *      - If yes, do needed updates to SDL (update error mappings for given ec or change SDL to set some error_code)
                 *      - If no, ask client to check why they are comparing non-SDL originated error_code against shareddatalayer::Error
                 */
                std::ostringstream msg;
                msg << "SharedDataLayerErrorCategory::equivalent no mapping for error: " << ec.category().name()
                    << ec.value();
                logErrorOnce(msg.str());
                return false;
        }
    }
}

namespace shareddatalayer
{
    std::error_condition make_error_condition(shareddatalayer::Error errorCode)
    {
      return {static_cast<int>(errorCode), getSharedDataLayerErrorCategory()};
    }
}
