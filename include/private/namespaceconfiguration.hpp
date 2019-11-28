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

#ifndef SHAREDDATALAYER_NAMESPACECONFIGURATION_HPP_
#define SHAREDDATALAYER_NAMESPACECONFIGURATION_HPP_

#include <string>

namespace shareddatalayer
{
    struct NamespaceConfiguration
    {
        const std::string namespacePrefix;
        bool dbBackendIsUsed;
        bool notificationsAreEnabled;
        const std::string sourceName;

        NamespaceConfiguration(const std::string& namespacePrefix,
                               bool useDbBackend,
                               bool enableNotifications,
                               const std::string& sourceName):
            namespacePrefix(namespacePrefix),
            dbBackendIsUsed(useDbBackend),
            notificationsAreEnabled(enableNotifications),
            sourceName(sourceName)
        {}

        bool operator==(const NamespaceConfiguration& nc) const
        {
            return namespacePrefix == nc.namespacePrefix &&
                   dbBackendIsUsed == nc.dbBackendIsUsed &&
                   notificationsAreEnabled == nc.notificationsAreEnabled &&
                   sourceName == nc.sourceName;
        }
    };
}

#endif
