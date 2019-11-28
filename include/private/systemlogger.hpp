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

#ifndef SHAREDDATALAYER_SYSTEMLOGGER_HPP_
#define SHAREDDATALAYER_SYSTEMLOGGER_HPP_

#include <memory>
#include <string>
#include "private/logger.hpp"

namespace shareddatalayer
{
    class SystemLogger: public Logger
    {
    public:
        explicit SystemLogger(const std::string& prefix);

        ~SystemLogger();

        std::ostream& emerg() override;

        std::ostream& alert() override;

        std::ostream& crit() override;

        std::ostream& error() override;

        std::ostream& warning() override;

        std::ostream& notice() override;

        std::ostream& info() override;

        std::ostream& debug() override;

    private:
        const std::string prefix;
        std::unique_ptr<std::ostream> osEmerg;
        std::unique_ptr<std::ostream> osAlert;
        std::unique_ptr<std::ostream> osCrit;
        std::unique_ptr<std::ostream> osError;
        std::unique_ptr<std::ostream> osWarning;
        std::unique_ptr<std::ostream> osNotice;
        std::unique_ptr<std::ostream> osInfo;
        std::unique_ptr<std::ostream> osDebug;
    };
}

#endif
