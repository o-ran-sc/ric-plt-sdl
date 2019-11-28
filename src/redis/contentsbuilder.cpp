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

#include "private/redis/contentsbuilder.hpp"
#include "private/redis/contents.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::redis;

ContentsBuilder::ContentsBuilder(const char nsKeySeparator):
    nsKeySeparator(nsKeySeparator)
{
}

ContentsBuilder::~ContentsBuilder()
{
}

Contents ContentsBuilder::build(const std::string& string) const
{
    Contents contents;
    addString(contents, string);
    return contents;
}

Contents ContentsBuilder::build(const std::string& string,
                                const std::string& string2) const
{
    Contents contents;
    addString(contents, string);
    addString(contents, string2);
    return contents;
}

Contents ContentsBuilder::build(const std::string& string,
                                const std::string& string2,
                                const std::string& string3) const
{
    Contents contents;
    addString(contents, string);
    addString(contents, string2);
    addString(contents, string3);
    return contents;
}

Contents ContentsBuilder::build(const std::string& string,
                                const AsyncConnection::Namespace& ns,
                                const AsyncConnection::DataMap& dataMap) const
{
    Contents contents;
    addString(contents, string);
    addDataMap(contents, ns, dataMap);
    return contents;
}

Contents ContentsBuilder::build(const std::string& string,
                                const AsyncConnection::Namespace& ns,
                                const AsyncConnection::DataMap& dataMap,
                                const std::string& string2,
                                const std::string& string3) const
{
    Contents contents;
    addString(contents, string);
    addDataMap(contents, ns, dataMap);
    addString(contents, string2);
    addString(contents, string3);
    return contents;
}

Contents ContentsBuilder::build(const std::string& string,
                                const AsyncConnection::Namespace& ns,
                                const AsyncConnection::Key& key,
                                const AsyncConnection::Data& data) const
{
    Contents contents;
    addString(contents, string);
    addKey(contents, ns, key);
    addData(contents, data);
    return contents;
}

Contents ContentsBuilder::build(const std::string& string,
                                const AsyncConnection::Namespace& ns,
                                const AsyncConnection::Key& key,
                                const AsyncConnection::Data& data,
                                const std::string& string2,
                                const std::string& string3) const
{
    Contents contents;
    addString(contents, string);
    addKey(contents, ns, key);
    addData(contents, data);
    addString(contents, string2);
    addString(contents, string3);
    return contents;
}

Contents ContentsBuilder::build(const std::string& string,
                                const AsyncConnection::Namespace& ns,
                                const AsyncConnection::Key& key,
                                const AsyncConnection::Data& data,
                                const AsyncConnection::Data& data2) const
{
    Contents contents;
    addString(contents, string);
    addKey(contents, ns, key);
    addData(contents, data);
    addData(contents, data2);
    return contents;
}

Contents ContentsBuilder::build(const std::string& string,
                                const AsyncConnection::Namespace& ns,
                                const AsyncConnection::Key& key,
                                const AsyncConnection::Data& data,
                                const AsyncConnection::Data& data2,
                                const std::string& string2,
                                const std::string& string3) const
{
    Contents contents;
    addString(contents, string);
    addKey(contents, ns, key);
    addData(contents, data);
    addData(contents, data2);
    addString(contents, string2);
    addString(contents, string3);
    return contents;
}

Contents ContentsBuilder::build(const std::string& string,
                                const AsyncConnection::Namespace& ns,
                                const AsyncConnection::Keys& keys) const
{
    Contents contents;
    addString(contents, string);
    addKeys(contents, ns, keys);
    return contents;
}

Contents ContentsBuilder::build(const std::string& string,
                                const AsyncConnection::Namespace& ns,
                                const AsyncConnection::Keys& keys,
                                const std::string& string2,
                                const std::string& string3) const
{
    Contents contents;
    addString(contents, string);
    addKeys(contents, ns, keys);
    addString(contents, string2);
    addString(contents, string3);
    return contents;
}

void ContentsBuilder::addString(Contents& contents,
                                const std::string& string) const
{
    contents.stack.push_back(string);
    contents.sizes.push_back(string.size());
}

void ContentsBuilder::addDataMap(Contents& contents,
                                 const AsyncConnection::Namespace& ns,
                                 const AsyncConnection::DataMap& dataMap) const
{
    for (const auto& i : dataMap)
    {
        addKey(contents, ns, i.first);
        addData(contents, i.second);
    }
}

void ContentsBuilder::addKey(Contents& contents,
                             const AsyncConnection::Namespace& ns,
                             const AsyncConnection::Key& key) const
{
    auto content('{' + ns + '}' + nsKeySeparator + key);
    contents.stack.push_back(content);
    contents.sizes.push_back((content).size());
}

void ContentsBuilder::addData(Contents& contents,
                              const AsyncConnection::Data& data) const
{
    contents.stack.push_back(std::string(reinterpret_cast<const char*>(data.data()),
                                         static_cast<size_t>(data.size())));
    contents.sizes.push_back(data.size());
}

void ContentsBuilder::addKeys(Contents& contents,
                              const AsyncConnection::Namespace& ns,
                              const AsyncConnection::Keys& keys) const
{
    for (const auto& i : keys)
        addKey(contents, ns, i);
}
