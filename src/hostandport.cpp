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

#include "private/hostandport.hpp"
#include <sstream>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/lexical_cast.hpp>
#include <ostream>

using namespace shareddatalayer;

namespace
{
    uint16_t stringToPort(const std::string& port)
    {
        if (port.empty())
            throw HostAndPort::EmptyPort();
        try
        {
            return htons(boost::lexical_cast<uint16_t>(port));
        }
        catch (const boost::bad_lexical_cast&)
        {
            char buf[1024];
            servent resultbuf = { };
            servent *result(nullptr);
            getservbyname_r(port.c_str(), nullptr, &resultbuf, buf, sizeof(buf), &result);
            if (result == nullptr)
                throw HostAndPort::InvalidPort(port);
            return static_cast<uint16_t>(result->s_port);
        }
    }

    bool isInBrackets(const std::string& str)
    {
        return ((!str.empty()) && (str.front() == '[') && (str.back() == ']'));
    }

    std::string removeBrackets(const std::string& str)
    {
        return str.substr(1U, str.size() - 2U);
    }

    bool isLiteralIPv6(const std::string& str)
    {
        in6_addr tmp;
        return inet_pton(AF_INET6, str.c_str(), &tmp) == 1;
    }

    std::string buildInvalidPortError(const std::string& port)
    {
        std::ostringstream os;
        os << "invalid port: " << port;
        return os.str();
    }
}

HostAndPort::InvalidPort::InvalidPort(const std::string& port):
    Exception(buildInvalidPortError(port))
{
}

HostAndPort::EmptyPort::EmptyPort():
    Exception("empty port")
{
}

HostAndPort::EmptyHost::EmptyHost():
    Exception("empty host")
{
}

HostAndPort::HostAndPort(const std::string& addressAndOptionalPort, uint16_t defaultPort)
{
    if (isInBrackets(addressAndOptionalPort))
        parse(removeBrackets(addressAndOptionalPort), defaultPort);
    else
        parse(addressAndOptionalPort, defaultPort);
}

void HostAndPort::parse(const std::string& addressAndOptionalPort, uint16_t defaultPort)
{
    const auto pos(addressAndOptionalPort.rfind(':'));
    if (pos == std::string::npos)
    {
        host = addressAndOptionalPort;
        port = defaultPort;
        literalIPv6 = false;
    }
    else if (isLiteralIPv6(addressAndOptionalPort))
    {
        host = addressAndOptionalPort;
        port = defaultPort;
        literalIPv6 = true;
    }
    else
    {
        host = addressAndOptionalPort.substr(0, pos);
        if (isInBrackets(host))
            host = removeBrackets(host);
        literalIPv6 = isLiteralIPv6(host);
        port = stringToPort(addressAndOptionalPort.substr(pos + 1U, addressAndOptionalPort.size() - pos - 1U));
    }
    if (host.empty())
        throw EmptyHost();
}

const std::string& HostAndPort::getHost() const
{
    return host;
}

uint16_t HostAndPort::getPort() const
{
    return port;
}

std::string HostAndPort::getString() const
{
    std::ostringstream os;
    if (literalIPv6)
        os << '[' << host << "]:" << ntohs(port);
    else
        os << host << ':' << ntohs(port);
    return os.str();
}

bool HostAndPort::operator==(const HostAndPort& hp) const
{
    return this->getPort() == hp.getPort() && this->getHost() == hp.getHost();
}

bool HostAndPort::operator!=(const HostAndPort& hp) const
{
    return !(hp == *this);
}

bool HostAndPort::operator<(const HostAndPort& hp) const
{
    if (this->getHost() == hp.getHost())
        return this->getPort() < hp.getPort();
    else
        return this->getHost() < hp.getHost();
}

std::ostream& shareddatalayer::operator << (std::ostream& os, const HostAndPort& hostAndPort)
{
    os << hostAndPort.getHost() << ":" << hostAndPort.getPort();
    return os;
}
