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
#include <memory>
#include <ostream>
#include <string>
#include "private/createlogger.hpp"
#include "private/stdstreamlogger.hpp"
#if HAVE_SYSTEMLOGGER
#include "private/systemlogger.hpp"
#endif

using namespace shareddatalayer;

std::shared_ptr<Logger> shareddatalayer::createLogger(const std::string& prefix)
{
#if HAVE_SYSTEMLOGGER
    return std::shared_ptr<Logger>(new SystemLogger(prefix));
#else
    return std::shared_ptr<Logger>(new StdStreamLogger(prefix));
#endif
}

void shareddatalayer::logDebugOnce(const std::string& msg) noexcept
{
    auto logger(createLogger(SDL_LOG_PREFIX));
    logger->debug() << msg << std::endl;
}

void shareddatalayer::logErrorOnce(const std::string& msg) noexcept
{
    auto logger(createLogger(SDL_LOG_PREFIX));
    logger->error() << msg << std::endl;
}

void shareddatalayer::logInfoOnce(const std::string& msg) noexcept
{
    auto logger(createLogger(SDL_LOG_PREFIX));
    logger->info() << msg << std::endl;
}
