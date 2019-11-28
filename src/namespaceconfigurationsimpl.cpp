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

#include "private/abort.hpp"
#include "private/namespaceconfigurationsimpl.hpp"
#include <sstream>
#include <iomanip>

using namespace shareddatalayer;

namespace
{
    const NamespaceConfiguration getDefaultNamespaceConfiguration()
    {
        return {"", true, false, "<default>"};
    }
}

NamespaceConfigurationsImpl::NamespaceConfigurationsImpl():
    namespaceConfigurations({getDefaultNamespaceConfiguration()}),
    //Current implementation assumes that this index is zero. If changing the index, do needed modifications to implementation.
    defaultConfigurationIndex(0),
    namespaceConfigurationsLookupTable()
{
}

NamespaceConfigurationsImpl::~NamespaceConfigurationsImpl()
{
}

void NamespaceConfigurationsImpl::addNamespaceConfiguration(const NamespaceConfiguration& namespaceConfiguration)
{
    if (!namespaceConfigurationsLookupTable.empty())
        SHAREDDATALAYER_ABORT("Cannot add namespace configurations after lookup table is initialized");

    namespaceConfigurations.push_back(namespaceConfiguration);
}

std::string NamespaceConfigurationsImpl::getDescription(const std::string& ns) const
{
    NamespaceConfiguration namespaceConfiguration = findConfigurationForNamespace(ns);
    std::string sourceInfo = namespaceConfiguration.sourceName;

    if (!namespaceConfiguration.namespacePrefix.empty())
        sourceInfo += (" prefix: " + namespaceConfiguration.namespacePrefix);

    std::ostringstream os;
    os << std::boolalpha;
    os << sourceInfo << ", ";
    os << "useDbBackend: " << namespaceConfiguration.dbBackendIsUsed << ", ";
    os << "enableNotifications: " << namespaceConfiguration.notificationsAreEnabled;
    return os.str();
}

bool NamespaceConfigurationsImpl::isDbBackendUseEnabled(const std::string& ns) const
{
    return findConfigurationForNamespace(ns).dbBackendIsUsed;
}

bool NamespaceConfigurationsImpl::areNotificationsEnabled(const std::string& ns) const
{
    return findConfigurationForNamespace(ns).notificationsAreEnabled;
}

const NamespaceConfiguration& NamespaceConfigurationsImpl::findConfigurationForNamespace(const std::string& ns) const
{
    if (namespaceConfigurationsLookupTable.count(ns) > 0)
        return namespaceConfigurations.at(namespaceConfigurationsLookupTable.at(ns));

    size_t longestMatchingPrefixLen(0);
    std::vector<int>::size_type foundIndex(0);
    bool configurationFound(false);

    for(std::vector<int>::size_type i = (defaultConfigurationIndex + 1); i < namespaceConfigurations.size(); i++)
    {
        const auto prefixLen(namespaceConfigurations[i].namespacePrefix.length());

        if (ns.compare(0, prefixLen, namespaceConfigurations[i].namespacePrefix) == 0)
        {
            if (prefixLen >= longestMatchingPrefixLen)
            {
                configurationFound = true;
                foundIndex = i;
                longestMatchingPrefixLen = prefixLen;
            }
        }
    }

    if (!configurationFound)
        foundIndex = defaultConfigurationIndex;

    namespaceConfigurationsLookupTable[ns] = foundIndex;
    return namespaceConfigurations[foundIndex];
}

bool NamespaceConfigurationsImpl::isEmpty() const
{
    /* Empty when contains only default configuration */
    return namespaceConfigurations.size() == 1;
}

bool NamespaceConfigurationsImpl::isNamespaceInLookupTable(const std::string& ns) const
{
    return namespaceConfigurationsLookupTable.count(ns) > 0;
}
