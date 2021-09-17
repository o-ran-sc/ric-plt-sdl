/*
   Copyright (c) 2018-2020 Nokia.

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

#ifndef SHAREDDATALAYER_TST_MOCKABLESYNCSTORAGE_HPP_
#define SHAREDDATALAYER_TST_MOCKABLESYNCSTORAGE_HPP_

#include <iostream>
#include <cstdlib>
#include <sdl/syncstorage.hpp>

namespace shareddatalayer
{
    namespace tst
    {
        /**
         * @brief Helper class for mocking SyncStorage
         *
         * MockableSyncStorage can be used in application's unit tests to mock SyncStorage.
         * MockableSyncStorage guarantees that even if new functions are added into
         * SyncStorage, existing unit test cases do not have to be modified unless
         * those new functions are used.
         *
         * If a function is not mocked but called in unit tests, then the call
         * will be logged and the process will abort.
         */
        class MockableSyncStorage: public SyncStorage
        {
        public:
            MockableSyncStorage() { }

            virtual void waitReady(const Namespace&, const std::chrono::steady_clock::duration&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual void set(const Namespace&, const DataMap&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual bool setIf(const Namespace&, const Key&, const Data&, const Data&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual bool setIfNotExists(const Namespace&, const Key&, const Data&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual DataMap get(const Namespace&, const Keys&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual void remove(const Namespace&, const Keys&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual bool removeIf(const Namespace&, const Key&, const Data&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual Keys findKeys(const Namespace&, const std::string&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual Keys listKeys(const Namespace&, const std::string&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual void removeAll(const Namespace&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual void setOperationTimeout(const std::chrono::steady_clock::duration&) override { logAndAbort(__PRETTY_FUNCTION__); }

        private:
            static void logAndAbort(const char* function) noexcept __attribute__ ((__noreturn__))
            {
                std::cerr << "MockableSyncStorage: calling not-mocked function "
                          << function << ", aborting" << std::endl;
                abort();
            }
        };
    }
}

#endif
