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
#include <gmock/gmock.h>
#include "private/namespaceconfigurationsimpl.hpp"

using namespace shareddatalayer;
using namespace testing;

namespace
{
    class NamespaceConfigurationsImplTest: public testing::Test
    {
    public:
        const std::string someKnownNamespace;
        const std::string someKnownInputSource;
        std::unique_ptr<NamespaceConfigurationsImpl> namespaceConfigurationsImpl;

        NamespaceConfigurationsImplTest():
            someKnownNamespace("someKnownPrefixValue123"),
            someKnownInputSource("someKnownInputSource")
        {
            InSequence dummy;
            namespaceConfigurationsImpl.reset(new NamespaceConfigurationsImpl());
        }
    };
}

TEST_F(NamespaceConfigurationsImplTest, CanMatchToLongestNamespacePrefixAndReturnItsValues)
{
    namespaceConfigurationsImpl->addNamespaceConfiguration({"some", true, false, someKnownInputSource});
    namespaceConfigurationsImpl->addNamespaceConfiguration({"someKnownPrefix", true, true, someKnownInputSource});
    namespaceConfigurationsImpl->addNamespaceConfiguration({"someKnownPrefixs", false, false, someKnownInputSource});
    EXPECT_TRUE(namespaceConfigurationsImpl->isDbBackendUseEnabled(someKnownNamespace));
    EXPECT_TRUE(namespaceConfigurationsImpl->areNotificationsEnabled(someKnownNamespace));
}

TEST_F(NamespaceConfigurationsImplTest, CanMatchToEmptyNamespacePrefixAndReturnItsValues)
{
    namespaceConfigurationsImpl->addNamespaceConfiguration({"anotherKnownPrefix", true, true, someKnownInputSource});
    namespaceConfigurationsImpl->addNamespaceConfiguration({"", false, false, someKnownInputSource});
    EXPECT_FALSE(namespaceConfigurationsImpl->isDbBackendUseEnabled(someKnownNamespace));
    EXPECT_FALSE(namespaceConfigurationsImpl->areNotificationsEnabled(someKnownNamespace));
}

TEST_F(NamespaceConfigurationsImplTest, CanReturnDefaultValues)
{
    EXPECT_TRUE(namespaceConfigurationsImpl->isDbBackendUseEnabled(someKnownNamespace));
    EXPECT_FALSE(namespaceConfigurationsImpl->areNotificationsEnabled(someKnownNamespace));
}

TEST_F(NamespaceConfigurationsImplTest, CanShowReadConfigurationDescription)
{
    namespaceConfigurationsImpl->addNamespaceConfiguration({"someKnownPrefix", true, true, someKnownInputSource});
    EXPECT_EQ("someKnownInputSource prefix: someKnownPrefix, useDbBackend: true, enableNotifications: true",
              namespaceConfigurationsImpl->getDescription(someKnownNamespace));

    namespaceConfigurationsImpl.reset(new NamespaceConfigurationsImpl());
    namespaceConfigurationsImpl->addNamespaceConfiguration({"someKnownPrefix", true, false, someKnownInputSource});
    EXPECT_EQ("someKnownInputSource prefix: someKnownPrefix, useDbBackend: true, enableNotifications: false",
              namespaceConfigurationsImpl->getDescription(someKnownNamespace));

    namespaceConfigurationsImpl.reset(new NamespaceConfigurationsImpl());
    namespaceConfigurationsImpl->addNamespaceConfiguration({"someKnownPrefix", false, false, someKnownInputSource});
    EXPECT_EQ("someKnownInputSource prefix: someKnownPrefix, useDbBackend: false, enableNotifications: false",
              namespaceConfigurationsImpl->getDescription(someKnownNamespace));
}

TEST_F(NamespaceConfigurationsImplTest, CanShowDefaultValuesDescription)
{
    EXPECT_EQ("<default>, useDbBackend: true, enableNotifications: false",
              namespaceConfigurationsImpl->getDescription(someKnownNamespace));
}

TEST_F(NamespaceConfigurationsImplTest, NamespaceIsAddedToLookupTableAfterFirstSearch)
{
    EXPECT_FALSE(namespaceConfigurationsImpl->isNamespaceInLookupTable(someKnownNamespace));
    namespaceConfigurationsImpl->addNamespaceConfiguration({"someKnownPrefix", true, true, someKnownInputSource});
    EXPECT_EQ("someKnownInputSource prefix: someKnownPrefix, useDbBackend: true, enableNotifications: true",
              namespaceConfigurationsImpl->getDescription(someKnownNamespace));
    EXPECT_TRUE(namespaceConfigurationsImpl->isNamespaceInLookupTable(someKnownNamespace));

    //Check that data can be correctly read also when configuration is read through lookup table
    EXPECT_EQ("someKnownInputSource prefix: someKnownPrefix, useDbBackend: true, enableNotifications: true",
              namespaceConfigurationsImpl->getDescription(someKnownNamespace));
}

TEST_F(NamespaceConfigurationsImplTest, DoesNotAllowConfigurationAdditionsAfterLookupTableInitialization)
{
    // All configurations must be added before configurations are read.
    namespaceConfigurationsImpl->addNamespaceConfiguration({"someKnownPrefix", true, true, someKnownInputSource});
    EXPECT_TRUE(namespaceConfigurationsImpl->isDbBackendUseEnabled(someKnownNamespace));
    EXPECT_EXIT(namespaceConfigurationsImpl->addNamespaceConfiguration({"someKnownPrefix2", false, false, someKnownInputSource}),
        KilledBySignal(SIGABRT), "ABORT.*namespaceconfigurationsimpl\\.cpp");
}

TEST_F(NamespaceConfigurationsImplTest, IsEmptyReturnsCorrectInformation)
{
    EXPECT_TRUE(namespaceConfigurationsImpl->isEmpty());
    namespaceConfigurationsImpl->addNamespaceConfiguration({"someKnownPrefix", true, true, someKnownInputSource});
    EXPECT_FALSE(namespaceConfigurationsImpl->isEmpty());
}
