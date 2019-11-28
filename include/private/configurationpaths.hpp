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

#ifndef SHAREDDATALAYER_CONFIGURATIONPATHS_HPP_
#define SHAREDDATALAYER_CONFIGURATIONPATHS_HPP_

#include <string>
#include <vector>

namespace shareddatalayer
{
    using Directories = std::vector<std::string>;
    using Files = std::vector<std::string>;

    Directories getDefaultConfDirectories();

    Files findConfigurationFiles(const Directories& directories);
}

#endif
