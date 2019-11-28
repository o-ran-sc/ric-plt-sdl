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

#ifndef SHAREDDATALAYER_HOSTANDPORT_HPP_
#define SHAREDDATALAYER_HOSTANDPORT_HPP_

#include <string>
#include <stdint.h>
#include <sdl/exception.hpp>
#include <iosfwd>

namespace shareddatalayer
{
    class HostAndPort
    {
    public:
        class InvalidPort;

        class EmptyPort;

        class EmptyHost;

        HostAndPort() = delete;

        /* defaultPort in network byte order */
        HostAndPort(const std::string& addressAndOptionalPort, uint16_t defaultPort);

        const std::string& getHost() const;

        /* Returned port in network byte order */
        uint16_t getPort() const;

        std::string getString() const;

        bool operator==(const HostAndPort& hp) const;

        bool operator!=(const HostAndPort& hp) const;

        bool operator<(const HostAndPort& hp) const;

    private:
        void parse(const std::string& addressAndOptionalPort, uint16_t defaultPort);

        std::string host;
        uint16_t port;
        bool literalIPv6;
    };

    class HostAndPort::InvalidPort: public Exception
    {
    public:
        InvalidPort() = delete;

        explicit InvalidPort(const std::string& port);
    };

    class HostAndPort::EmptyPort: public Exception
    {
    public:
        EmptyPort();
    };

    class HostAndPort::EmptyHost: public Exception
    {
    public:
        EmptyHost();
    };

    std::ostream& operator << (std::ostream& os, const HostAndPort& hostAndPort);
}

#endif
