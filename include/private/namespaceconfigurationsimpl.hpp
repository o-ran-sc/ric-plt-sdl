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

#ifndef SHAREDDATALAYER_NAMESPACECONFIGURATIONIMPL_HPP_
#define SHAREDDATALAYER_NAMESPACECONFIGURATIONIMPL_HPP_

#include "private/namespaceconfigurations.hpp"
#include <unordered_map>
#include <vector>

namespace shareddatalayer
{
    class NamespaceConfigurationsImpl: public NamespaceConfigurations
    {
    public:
        NamespaceConfigurationsImpl();

        ~NamespaceConfigurationsImpl() override;

        void addNamespaceConfiguration(const NamespaceConfiguration& namespaceConfiguration) override;
        bool isDbBackendUseEnabled(const std::string& ns) const override;
        bool areNotificationsEnabled(const std::string& ns) const override;
        std::string getDescription(const std::string& ns) const override;
        bool isEmpty() const override;

        //Meant for UT usage
        bool isNamespaceInLookupTable(const std::string& ns) const;

    private:
        std::vector<NamespaceConfiguration> namespaceConfigurations;
        const std::vector<int>::size_type defaultConfigurationIndex;
        mutable std::unordered_map<std::string, std::vector<int>::size_type> namespaceConfigurationsLookupTable;

        const NamespaceConfiguration& findConfigurationForNamespace(const std::string& ns) const;
    };
}

#endif
