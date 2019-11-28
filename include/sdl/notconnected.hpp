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

#ifndef SHAREDDATALAYER_NOTCONNECTED_HPP_
#define SHAREDDATALAYER_NOTCONNECTED_HPP_

#include <sdl/exception.hpp>

namespace shareddatalayer
{
    /**
     * @brief %Exception for SDL API being not connected to the backend data storage.
     *
     * %Exception is analogous with shareddatalayer::Error::NOT_CONNECTED error code.<br>
     * See shareddatalayer::Error::NOT_CONNECTED documentation in sdl/errorqueries.hpp
     * for further infomation about the error situation and for handling advices.
     */
    class NotConnected: public Exception
    {
    public:
        explicit NotConnected(const std::string& error);
    };
}

#endif
