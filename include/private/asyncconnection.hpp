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

#ifndef SHAREDDATALAYER_ASYNCCONNECTION_HPP_
#define SHAREDDATALAYER_ASYNCCONNECTION_HPP_

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <sdl/errorqueries.hpp>

namespace shareddatalayer
{
    /**
     * @brief Deprecated API, some type definitions left because still used by other classes in SDL implementation.
     */
    class AsyncConnection
    {
    public:
        /**
         * Separator defines the character that API uses to logically separate the namespace
         * and the key when forming an SDL key. Thus, the separator character cannot be part
         * of the namespace string. Also namespace identifier cannot be an empty string.
         *
         * The character used as separator is <code>,</code>
         *
         * API otherwise does not impose additional restrictions to used characters, though
         * excessive and unnecessary usage of special characters is strongly discouraged.
         */
        static const char SEPARATOR;

        using Key = std::string;

        using Data = std::vector<uint8_t>;

        using DataMap = std::map<Key, Data>;

        using Keys = std::set<Key>;

        using Namespace = std::string;

        /**
         * publisherId: Identification that can be set by a publisher to identify the source of
         *              the modification. Process that is modifying data in a namespace and is
         *              also subscribed to the same namespace can utilize the publisherId to
         *              distinguish events that it published itself.
         */
        using PublisherId = std::string;
    };
}

#endif
