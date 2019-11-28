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

#include <gtest/gtest.h>
#include "private/redis/contentsbuilder.hpp"
#include "private/redis/contents.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;
using namespace testing;

namespace
{
    class ContentsBuilderTest: public testing::Test
    {
    public:
        std::unique_ptr<ContentsBuilder> contentsBuilder;
        AsyncConnection::Namespace ns;
        char nsKeySeparator;
        std::string string;
        std::string string2;
        std::string string3;
        AsyncConnection::Key key;
        AsyncConnection::Key key2;
        AsyncConnection::Data data;
        AsyncConnection::Data data2;
        AsyncConnection::Keys keys;
        AsyncConnection::DataMap dataMap;

        ContentsBuilderTest():
            ns("ns"),
            nsKeySeparator('#'),
            string("string"),
            string2("string2"),
            string3("string3"),
            key("key"),
            key2("key2"),
            data({11,12}),
            data2({21,22}),
            keys({key,key2}),
            dataMap({{key,data},{key2,data2}})
        {
            contentsBuilder.reset(new ContentsBuilder(nsKeySeparator));
        }

        void expectStringInContents(Contents& contents, std::string& string, int pos)
        {
            EXPECT_EQ(string, contents.stack[pos]);
            EXPECT_EQ(string.size(), contents.sizes[pos]);
        }

        void expectDataMapInContents(Contents& contents, int pos)
        {
            expectKeyInContents(contents, key, pos);
            expectDataInContents(contents, data, pos+1);
            expectKeyInContents(contents, key2, pos+2);
            expectDataInContents(contents, data2, pos+3);
        }

        void expectKeyInContents(Contents& contents, AsyncConnection::Key& key, int pos)
        {
            AsyncConnection::Key keyWithPrefix('{' + ns + '}' + nsKeySeparator + key);
            EXPECT_EQ(keyWithPrefix, contents.stack[pos]);
            EXPECT_EQ(keyWithPrefix.size(), contents.sizes[pos]);
        }

        void expectDataInContents(Contents& contents, AsyncConnection::Data& data, int pos)
        {
            EXPECT_EQ(data.size(), contents.sizes[pos]);
            std::string dataAsString(data.begin(), data.end());
            EXPECT_EQ(dataAsString, contents.stack[pos]);
        }

        void expectMessageInContents(Contents& contents, int pos)
        {
            EXPECT_EQ(string2, contents.stack[pos]);
            EXPECT_EQ(string2.size(), contents.sizes[pos]);
        }

        void expectKeysInContents(Contents& contents, int pos)
        {
            expectKeyInContents(contents, key, pos);
            expectKeyInContents(contents, key2, pos+1);
        }
    };
}

TEST_F(ContentsBuilderTest, BuildWithString)
{
    auto contents(contentsBuilder->build(string));
    EXPECT_EQ(size_t(1), contents.stack.size());
    EXPECT_EQ(size_t(1), contents.sizes.size());
    expectStringInContents(contents, string, 0);
}

TEST_F(ContentsBuilderTest, BuildWithStringAndString2)
{
    auto contents(contentsBuilder->build(string, string2));
    EXPECT_EQ(size_t(2), contents.stack.size());
    EXPECT_EQ(size_t(2), contents.sizes.size());
    expectStringInContents(contents, string, 0);
    expectStringInContents(contents, string2, 1);
}

TEST_F(ContentsBuilderTest, BuildWithStringString2AndString3)
{
    auto contents(contentsBuilder->build(string, string2, string3));
    EXPECT_EQ(size_t(3), contents.stack.size());
    EXPECT_EQ(size_t(3), contents.sizes.size());
    expectStringInContents(contents, string, 0);
    expectStringInContents(contents, string2, 1);
    expectStringInContents(contents, string3, 2);
}

TEST_F(ContentsBuilderTest, BuildWithStringAndDataMap)
{
    auto contents(contentsBuilder->build(string, ns, dataMap));
    EXPECT_EQ(size_t(5), contents.stack.size());
    EXPECT_EQ(size_t(5), contents.sizes.size());
    expectStringInContents(contents, string, 0);
    expectDataMapInContents(contents, 1);
}

TEST_F(ContentsBuilderTest, BuildWithStringDataMapAndTwoStrings)
{
    auto contents(contentsBuilder->build(string, ns, dataMap, string2, string3));
    EXPECT_EQ(size_t(7), contents.stack.size());
    EXPECT_EQ(size_t(7), contents.sizes.size());
    expectStringInContents(contents, string, 0);
    expectDataMapInContents(contents, 1);
    expectStringInContents(contents, string2, 5);
    expectStringInContents(contents, string3, 6);
}

TEST_F(ContentsBuilderTest, BuildWithStringKeyAndData)
{
    auto contents(contentsBuilder->build(string, ns, key, data));
    EXPECT_EQ(size_t(3), contents.stack.size());
    EXPECT_EQ(size_t(3), contents.sizes.size());
    expectStringInContents(contents, string, 0);
    expectKeyInContents(contents, key, 1);
    expectDataInContents(contents, data, 2);
}

TEST_F(ContentsBuilderTest, BuildWithStringKeyDataAndTwoStrings)
{
    auto contents(contentsBuilder->build(string, ns, key, data, string2, string3));
    EXPECT_EQ(size_t(5), contents.stack.size());
    EXPECT_EQ(size_t(5), contents.sizes.size());
    expectStringInContents(contents, string, 0);
    expectKeyInContents(contents, key, 1);
    expectDataInContents(contents, data, 2);
    expectStringInContents(contents, string2, 3);
    expectStringInContents(contents, string3, 4);
}

TEST_F(ContentsBuilderTest, BuildWithStringKeyAndTwoDatas)
{
    auto contents(contentsBuilder->build(string, ns, key, data, data2));
    EXPECT_EQ(size_t(4), contents.stack.size());
    EXPECT_EQ(size_t(4), contents.sizes.size());
    expectStringInContents(contents, string, 0);
    expectKeyInContents(contents, key, 1);
    expectDataInContents(contents, data, 2);
    expectDataInContents(contents, data2, 3);
}

TEST_F(ContentsBuilderTest, BuildWithStringKeyTwoDatasAndTwoStrings)
{
    auto contents(contentsBuilder->build(string, ns, key, data, data2, string2, string3));
    EXPECT_EQ(size_t(6), contents.stack.size());
    EXPECT_EQ(size_t(6), contents.sizes.size());
    expectStringInContents(contents, string, 0);
    expectKeyInContents(contents, key, 1);
    expectDataInContents(contents, data, 2);
    expectDataInContents(contents, data2, 3);
    expectStringInContents(contents, string2, 4);
    expectStringInContents(contents, string3, 5);
}

TEST_F(ContentsBuilderTest, BuildWithStringAndKeys)
{
    auto contents(contentsBuilder->build(string, ns, keys));
    EXPECT_EQ(size_t(3), contents.stack.size());
    EXPECT_EQ(size_t(3), contents.sizes.size());
    expectStringInContents(contents, string, 0);
    expectKeysInContents(contents, 1);
}

TEST_F(ContentsBuilderTest, BuildWithStringKeysAndTwoStrings)
{
    auto contents(contentsBuilder->build(string, ns, keys, string2, string3));
    EXPECT_EQ(size_t(5), contents.stack.size());
    EXPECT_EQ(size_t(5), contents.sizes.size());
    expectStringInContents(contents, string, 0);
    expectKeysInContents(contents, 1);
    expectStringInContents(contents, string2, 3);
    expectStringInContents(contents, string3, 4);
}
