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

#ifndef SHAREDDATALAYER_REDIS_CONTENTS_HPP_
#define SHAREDDATALAYER_REDIS_CONTENTS_HPP_

#include <vector>
#include <string>

namespace shareddatalayer
{
    namespace redis
    {
        struct Contents
        {
            std::vector<std::string> stack;
            std::vector<size_t> sizes;

            bool operator == (const Contents& contents) const
            {
                return (stack == contents.stack) && (sizes == contents.sizes);
            }

            bool operator != (const Contents& contents) const
            {
                return !(*this == contents);
            }
        };
    }
}

#endif
