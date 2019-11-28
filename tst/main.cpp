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
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <cstdlib>
#include <cstring>
#include <syslog.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <gtest/gtest.h>

namespace
{
    void preventCoreDumps()
    {
        static const struct rlimit rlim { 0, 0 };
        setrlimit(RLIMIT_CORE, &rlim);
    }

    void preventCoreDumpsIfNeeded(int argc, char** argv)
    {
        static const std::string option("--gtest_internal_run_death_test");
        for (int i = 1; i < argc; ++i)
            if (!strncmp(argv[i], option.c_str(), option.size()))
            {
                preventCoreDumps();
                break;
            }
    }

    int privateMain(int argc, char** argv)
    {
        openlog(PACKAGE_NAME "-testrunner", LOG_PERROR | LOG_PID, LOG_USER);
        preventCoreDumpsIfNeeded(argc, argv);
        testing::InitGoogleTest(&argc, argv);
        const int ret(RUN_ALL_TESTS());
        closelog();
        return ret;
    }
}

int main(int argc, char** argv)
{
    const char* valgrind(getenv("VALGRIND"));
    if (!valgrind)
        return privateMain(argc, argv);

    unsetenv("VALGRIND");

    std::vector<const char*> newArgv { valgrind, "--error-exitcode=255", "--leak-check=full", "--show-reachable=yes" };

    const char* suppressions(getenv("VALGRIND_SUPPRESSIONS"));
    std::string suppressionsArg("--suppressions=");
    if (suppressions)
    {
        suppressionsArg += suppressions;
        newArgv.push_back(suppressionsArg.c_str());
    }

    const char* extraArgs(getenv("VALGRIND_EXTRA_ARGS"));
    std::vector<std::string> valgrindExtraArgs;
    if (extraArgs)
    {
        std::istringstream is(extraArgs);
        std::copy(std::istream_iterator<std::string>(is),
                  std::istream_iterator<std::string>(),
                  std::back_inserter(valgrindExtraArgs));
        for (const auto& i : valgrindExtraArgs)
            newArgv.push_back(i.c_str());
    }

    for (int i = 0; i < argc; ++i)
        newArgv.push_back(argv[i]);

    newArgv.push_back(nullptr);

    return execvp(valgrind, const_cast<char**>(newArgv.data()));
}
