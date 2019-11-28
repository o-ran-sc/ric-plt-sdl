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

#ifndef SHAREDDATALAYER_NAMESPACECONFIGURATIONS_HPP_
#define SHAREDDATALAYER_NAMESPACECONFIGURATIONS_HPP_

#include <string>
#include "private/namespaceconfiguration.hpp"

namespace shareddatalayer
{
    class NamespaceConfigurations
    {
    public:
        virtual ~NamespaceConfigurations() = default;

        virtual void addNamespaceConfiguration(const NamespaceConfiguration& namespaceConfiguration) = 0;
        virtual bool isDbBackendUseEnabled(const std::string& ns) const = 0;
        virtual bool areNotificationsEnabled(const std::string& ns) const = 0;
        virtual std::string getDescription(const std::string& ns) const = 0;
        virtual bool isEmpty() const = 0;

        NamespaceConfigurations(const NamespaceConfigurations&) = delete;
        NamespaceConfigurations(NamespaceConfigurations&&) = delete;
        NamespaceConfigurations& operator = (const NamespaceConfigurations&) = delete;
        NamespaceConfigurations& operator = (NamespaceConfigurations&&) = delete;

    protected:
        NamespaceConfigurations() = default;
    };
}

#endif
