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

#ifndef SHAREDDATALAYER_REDIS_DATABASEINFO_HPP_
#define SHAREDDATALAYER_REDIS_DATABASEINFO_HPP_

#include <algorithm>
#include <string>
#include <vector>
#include "private/databaseconfiguration.hpp"
#include <boost/optional.hpp>

namespace shareddatalayer
{
    namespace redis
    {
        struct DatabaseInfo
        {
            enum class Type
            {
                SINGLE,
                REDUNDANT,
                CLUSTER
            };
            enum class Discovery
            {
                HIREDIS,
                SENTINEL
            };

            DatabaseConfiguration::Addresses hosts;
            Type type;
            boost::optional<std::string> ns;
            Discovery discovery;

            bool operator==(const DatabaseInfo& di) const
            {
                DatabaseConfiguration::Addresses hosts1 = hosts;
                DatabaseConfiguration::Addresses hosts2 = di.hosts;

                std::sort(hosts1.begin(), hosts1.end());
                std::sort(hosts2.begin(), hosts2.end());

                return (hosts1 == hosts2 &&
                        type == di.type &&
                        ns == di.ns);
            }

            bool operator!=(const DatabaseInfo& di) const
            {
                return !(di == *this);
            }
        };
    }
}

#endif
