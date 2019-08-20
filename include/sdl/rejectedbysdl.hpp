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

#ifndef SHAREDDATALAYER_REJECTEDBYSDL_HPP_
#define SHAREDDATALAYER_REJECTEDBYSDL_HPP_

#include <sdl/exception.hpp>

namespace shareddatalayer
{
    /**
     * @brief %Exception for shareddatalayer rejecting the request.
     *
     * %Exception is analogous with shareddatalayer::Error::REJECTED_BY_SDL error code.<br>
     * See shareddatalayer::Error::REJECTED_BY_SDL documentation in sdl/errorqueries.hpp
     * for further infomation about the error situation and for handling advices.
     */
    class RejectedBySdl: public Exception
    {
    public:
        explicit RejectedBySdl(const std::string& error);
    };
}

#endif
