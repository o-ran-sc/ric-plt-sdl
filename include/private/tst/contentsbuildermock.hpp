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

#ifndef SHAREDDATALAYER_TST_CONTENTSBUILDERMOCK_HPP_
#define SHAREDDATALAYER_TST_CONTENTSBUILDERMOCK_HPP_

#include <gmock/gmock.h>
#include "private/redis/contentsbuilder.hpp"

namespace shareddatalayer
{
    namespace tst
    {
        class ContentsBuilderMock: public redis::ContentsBuilder
        {
        public:
            ContentsBuilderMock(const char nsKeySeparator) :
                ContentsBuilder(nsKeySeparator){}

            MOCK_CONST_METHOD2(build, redis::Contents(const std::string& string,
                                                      const std::string& string2));

            MOCK_CONST_METHOD3(build, redis::Contents(const std::string& string,
                                                      const std::string& string2,
                                                      const std::string& string3));

            MOCK_CONST_METHOD3(build, redis::Contents(const std::string& string,
                                                      const AsyncConnection::Namespace& ns,
                                                      const AsyncConnection::DataMap& dataMap));

            MOCK_CONST_METHOD5(build, redis::Contents(const std::string& string,
                                                      const AsyncConnection::Namespace& ns,
                                                      const AsyncConnection::DataMap& dataMap,
                                                      const std::string& string2,
                                                      const std::string& string3));

            MOCK_CONST_METHOD4(build, redis::Contents(const std::string& string,
                                                      const AsyncConnection::Namespace& ns,
                                                      const AsyncConnection::Key& key,
                                                      const AsyncConnection::Data& data));

            MOCK_CONST_METHOD6(build, redis::Contents(const std::string& string,
                                                      const AsyncConnection::Namespace& ns,
                                                      const AsyncConnection::Key& key,
                                                      const AsyncConnection::Data& data,
                                                      const std::string& string2,
                                                      const std::string& string3));

            MOCK_CONST_METHOD5(build, redis::Contents(const std::string& string,
                                                      const AsyncConnection::Namespace& ns,
                                                      const AsyncConnection::Key& key,
                                                      const AsyncConnection::Data& data,
                                                      const AsyncConnection::Data& data2));

            MOCK_CONST_METHOD7(build, redis::Contents(const std::string& string,
                                                      const AsyncConnection::Namespace& ns,
                                                      const AsyncConnection::Key& key,
                                                      const AsyncConnection::Data& data,
                                                      const AsyncConnection::Data& data2,
                                                      const std::string& string2,
                                                      const std::string& string3));

            MOCK_CONST_METHOD3(build, redis::Contents(const std::string& string,
                                                      const AsyncConnection::Namespace& ns,
                                                      const AsyncConnection::Keys& keys));

            MOCK_CONST_METHOD5(build, redis::Contents(const std::string& string,
                                                      const AsyncConnection::Namespace& ns,
                                                      const AsyncConnection::Keys& keys,
                                                      const std::string& string2,
                                                      const std::string& string3));
        };
    }
}

#endif
