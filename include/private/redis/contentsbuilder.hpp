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

#ifndef SHAREDDATALAYER_REDIS_CONTENTSBUILDER_HPP_
#define SHAREDDATALAYER_REDIS_CONTENTSBUILDER_HPP_

#include "private/asyncconnection.hpp"

namespace shareddatalayer
{
    namespace redis
    {
        struct Contents;

        class ContentsBuilder
        {
        public:
            ContentsBuilder(const char nsKeySeparator);

            ContentsBuilder(const ContentsBuilder&) = delete;

            ContentsBuilder& operator = (const ContentsBuilder&) = delete;

            virtual ~ContentsBuilder();

            virtual Contents build(const std::string& string) const;

            virtual Contents build(const std::string& string,
                                   const std::string& string2) const;

            virtual Contents build(const std::string& string,
                                   const std::string& string2,
                                   const std::string& string3) const;

            virtual Contents build(const std::string& string,
                                   const AsyncConnection::Namespace& ns,
                                   const AsyncConnection::DataMap& dataMap) const;

            virtual Contents build(const std::string& string,
                                   const AsyncConnection::Namespace& ns,
                                   const AsyncConnection::DataMap& dataMap,
                                   const std::string& string2,
                                   const std::string& string3) const;

            virtual Contents build(const std::string& string,
                                   const AsyncConnection::Namespace& ns,
                                   const AsyncConnection::Key& key,
                                   const AsyncConnection::Data& data) const;

            virtual Contents build(const std::string& string,
                                   const AsyncConnection::Namespace& ns,
                                   const AsyncConnection::Key& key,
                                   const AsyncConnection::Data& data,
                                   const std::string& string2,
                                   const std::string& string3) const;

            virtual Contents build(const std::string& string,
                                   const AsyncConnection::Namespace& ns,
                                   const AsyncConnection::Key& key,
                                   const AsyncConnection::Data& data,
                                   const AsyncConnection::Data& data2) const;

            virtual Contents build(const std::string& string,
                                   const AsyncConnection::Namespace& ns,
                                   const AsyncConnection::Key& key,
                                   const AsyncConnection::Data& data,
                                   const AsyncConnection::Data& data2,
                                   const std::string& string2,
                                   const std::string& string3) const;

            virtual Contents build(const std::string& string,
                                   const AsyncConnection::Namespace& ns,
                                   const AsyncConnection::Keys& keys) const;

            virtual Contents build(const std::string& string,
                                   const AsyncConnection::Namespace& ns,
                                   const AsyncConnection::Keys& keys,
                                   const std::string& string2,
                                   const std::string& string3) const;

        private:
            const char nsKeySeparator;

            void addString(Contents& contents,
                           const std::string& string) const;

            void addDataMap(Contents& contents,
                            const AsyncConnection::Namespace& ns,
                            const AsyncConnection::DataMap& dataMap) const;

            void addKey(Contents& contents,
                        const AsyncConnection::Namespace& ns,
                        const AsyncConnection::Key& key) const;

            void addData(Contents& contents,
                         const AsyncConnection::Data& data) const;

            void addKeys(Contents& contents,
                         const AsyncConnection::Namespace& ns,
                         const AsyncConnection::Keys& keys) const;
        };
    }
}

#endif
