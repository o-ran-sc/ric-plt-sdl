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

#include "private/systemlogger.hpp"
#include <ostream>
#include <sstream>
#include <syslog.h>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/concepts.hpp>

using namespace shareddatalayer;

namespace
{
    class Sink: public boost::iostreams::sink
    {
    public:
        Sink(const std::string& prefix, int level): prefix(prefix), level(level) { }

        ~Sink() { }

        std::streamsize write(const char* s, std::streamsize n);

    private:
        const std::string prefix;
        const int level;
    };
}

std::streamsize Sink::write(const char* s, std::streamsize n)
{
    std::ostringstream os;
    os << "%s: %." << n << 's';
    syslog(level, os.str().c_str(), prefix.c_str(), s);
    return n;
}

SystemLogger::SystemLogger(const std::string& prefix):
    prefix(prefix)
{
}

SystemLogger::~SystemLogger()
{
}

std::ostream& SystemLogger::emerg()
{
    if (osEmerg == nullptr)
        osEmerg.reset(new boost::iostreams::stream<Sink>(Sink(prefix, LOG_EMERG)));
    return *osEmerg;
}

std::ostream& SystemLogger::alert()
{
    if (osAlert == nullptr)
        osAlert.reset(new boost::iostreams::stream<Sink>(Sink(prefix, LOG_ALERT)));
    return *osAlert;
}

std::ostream& SystemLogger::crit()
{
    if (osCrit == nullptr)
        osCrit.reset(new boost::iostreams::stream<Sink>(Sink(prefix, LOG_CRIT)));
    return *osCrit;
}

std::ostream& SystemLogger::error()
{
    if (osError == nullptr)
        osError.reset(new boost::iostreams::stream<Sink>(Sink(prefix, LOG_ERR)));
    return *osError;
}

std::ostream& SystemLogger::warning()
{
    if (osWarning == nullptr)
        osWarning.reset(new boost::iostreams::stream<Sink>(Sink(prefix, LOG_WARNING)));
    return *osWarning;
}

std::ostream& SystemLogger::notice()
{
    if (osNotice == nullptr)
        osNotice.reset(new boost::iostreams::stream<Sink>(Sink(prefix, LOG_NOTICE)));
    return *osNotice;
}

std::ostream& SystemLogger::info()
{
    if (osInfo == nullptr)
        osInfo.reset(new boost::iostreams::stream<Sink>(Sink(prefix, LOG_INFO)));
    return *osInfo;
}

std::ostream& SystemLogger::debug()
{
    if (osDebug == nullptr)
        osDebug.reset(new boost::iostreams::stream<Sink>(Sink(prefix, LOG_DEBUG)));
    return *osDebug;
}
