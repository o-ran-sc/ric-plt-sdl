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

#ifndef SHAREDDATALAYER_NAMESPACECONFIGURATIONMOCK_HPP_
#define SHAREDDATALAYER_NAMESPACECONFIGURATIONMOCK_HPP_

#include "private/namespaceconfigurations.hpp"

namespace shareddatalayer
{
    namespace tst
    {
        class NamespaceConfigurationsMock: public NamespaceConfigurations
        {
        public:
            MOCK_METHOD1(addNamespaceConfiguration, void(const NamespaceConfiguration&));
            MOCK_CONST_METHOD1(isDbBackendUseEnabled, bool(const std::string&));
            MOCK_CONST_METHOD1(areNotificationsEnabled, bool(const std::string&));
            MOCK_CONST_METHOD1(getDescription, std::string(const std::string&));
            MOCK_CONST_METHOD0(isEmpty, bool());
        };
    }
}

#endif
