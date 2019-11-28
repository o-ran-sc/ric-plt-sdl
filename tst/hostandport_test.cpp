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

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include "private/hostandport.hpp"
#include <sstream>

using namespace shareddatalayer;
using namespace testing;

TEST(HostAndPortTest, UseDefaultPortNumber)
{
    const HostAndPort hostAndPort("host", htons(100));
    EXPECT_EQ("host", hostAndPort.getHost());
    EXPECT_EQ(100, ntohs(hostAndPort.getPort()));
    EXPECT_EQ("host:100", hostAndPort.getString());
}

TEST(HostAndPortTest, UseExplicitPortNumber)
{
    const HostAndPort hostAndPort("host:999", htons(100));
    EXPECT_EQ("host", hostAndPort.getHost());
    EXPECT_EQ(999, ntohs(hostAndPort.getPort()));
    EXPECT_EQ("host:999", hostAndPort.getString());
}

TEST(HostAndPortTest, UseExplicitPortName)
{
    const HostAndPort hostAndPort("host:ssh", htons(100));
    EXPECT_EQ("host", hostAndPort.getHost());
    EXPECT_EQ(22, ntohs(hostAndPort.getPort()));
    EXPECT_EQ("host:22", hostAndPort.getString());
}

TEST(HostAndPortTest, IPv6AddressWithExplicitPortNumber)
{
    const HostAndPort hostAndPort("[2016::dead:beef]:999", htons(100));
    EXPECT_EQ("2016::dead:beef", hostAndPort.getHost());
    EXPECT_EQ(999, ntohs(hostAndPort.getPort()));
    EXPECT_EQ("[2016::dead:beef]:999", hostAndPort.getString());
}

TEST(HostAndPortTest, IPv6AddressWithDefaultPortNumber)
{
    const HostAndPort hostAndPort("2016::dead:beef", htons(100));
    EXPECT_EQ("2016::dead:beef", hostAndPort.getHost());
    EXPECT_EQ(100, ntohs(hostAndPort.getPort()));
    EXPECT_EQ("[2016::dead:beef]:100", hostAndPort.getString());
}

TEST(HostAndPortTest, HostnameInBracketsWithExplicitPort)
{
    const HostAndPort hostAndPort("[host]:999", htons(100));
    EXPECT_EQ("host", hostAndPort.getHost());
    EXPECT_EQ(999, ntohs(hostAndPort.getPort()));
    EXPECT_EQ("host:999", hostAndPort.getString());
}

TEST(HostAndPortTest, HostnameInBracketsWithDefaultPort)
{
    const HostAndPort hostAndPort("[host]", htons(100));
    EXPECT_EQ("host", hostAndPort.getHost());
    EXPECT_EQ(100, ntohs(hostAndPort.getPort()));
    EXPECT_EQ("host:100", hostAndPort.getString());
}

TEST(HostAndPortTest, IPv6AddressInBracketsWithDefaultPort)
{
    const HostAndPort hostAndPort("[2016::dead:beef]", htons(100));
    EXPECT_EQ("2016::dead:beef", hostAndPort.getHost());
    EXPECT_EQ(100, ntohs(hostAndPort.getPort()));
    EXPECT_EQ("[2016::dead:beef]:100", hostAndPort.getString());
}

TEST(HostAndPortTest, CanThrowAndCatchInvalidPort)
{
    try
    {
        throw HostAndPort::InvalidPort("foo");
    }
    catch (const std::exception& e)
    {
        EXPECT_STREQ("invalid port: foo", e.what());
    }
}

TEST(HostAndPortTest, InvalidPortThrows)
{
    EXPECT_THROW(HostAndPort("host:definitely_invalid_port", htons(100)), HostAndPort::InvalidPort);
}

TEST(HostAndPortTest, CanThrowAndCatchEmptyPort)
{
    try
    {
        throw HostAndPort::EmptyPort();
    }
    catch (const std::exception& e)
    {
        EXPECT_STREQ("empty port", e.what());
    }
}

TEST(HostAndPortTest, EmptyPortThrows)
{
    EXPECT_THROW(HostAndPort("host:", htons(100)), HostAndPort::EmptyPort);
}

TEST(HostAndPortTest, CanThrowAndCatchEmptyHost)
{
    try
    {
        throw HostAndPort::EmptyHost();
    }
    catch (const std::exception& e)
    {
        EXPECT_STREQ("empty host", e.what());
    }
}

TEST(HostAndPortTest, EmptyHostThrows)
{
    EXPECT_THROW(HostAndPort(":1234", htons(100)), HostAndPort::EmptyHost);
}

TEST(HostAndPortTest, CanOutput)
{
    std::string expectedOutput("somehost.somesubdomain.somedomain:1234");
    std::stringstream ss;
    HostAndPort hostAndPort("somehost.somesubdomain.somedomain", 1234);
    ss << hostAndPort;
    EXPECT_EQ(expectedOutput, ss.str());
}
