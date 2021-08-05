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

#include "config.h"
#include "private/configurationpaths.hpp"
#include <dirent.h>
#include <boost/algorithm/string/predicate.hpp>

using namespace shareddatalayer;

namespace
{
    void findConfigurationFilesFromDirectory(const std::string& path, std::vector<std::string>& paths)
    {
        DIR *dir; struct dirent *diread;
        if ((dir = opendir(path.c_str())) != nullptr)
        {
            while ((diread = readdir(dir)) != nullptr)
            {
                if ((diread->d_type == DT_REG) &&
                    (diread->d_name[0] != '.') &&
                    (boost::algorithm::ends_with(diread->d_name, ".json")))
                    paths.push_back(std::string(diread->d_name));
            }
            closedir (dir);
        }
    }
}

Directories shareddatalayer::getDefaultConfDirectories()
{
    return Directories({ SDL_CONF_DIR, "/run/" PACKAGE_NAME ".d" });
}

Files shareddatalayer::findConfigurationFiles(const Directories& directories)
{
    std::vector<std::string> paths;
    for (const auto& i : directories)
        findConfigurationFilesFromDirectory(i, paths);
    std::sort(paths.begin(), paths.end());
    return paths;
}
