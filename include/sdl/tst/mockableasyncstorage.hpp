#ifndef SHAREDDATALAYER_TST_MOCKABLEASYNCSTORAGE_HPP_
#define SHAREDDATALAYER_TST_MOCKABLEASYNCSTORAGE_HPP_

#include <iostream>
#include <cstdlib>
#include <sdl/asyncstorage.hpp>

namespace shareddatalayer
{
    namespace tst
    {
        /**
         * @brief Helper class for mocking AsyncStorage
         *
         * MockableAsyncStorage can be used in application's unit tests to mock AsyncStorage.
         * MockableAsyncStorage guarantees that even if new functions are added into
         * AsyncStorage, existing unit test cases do not have to be modified unless
         * those new functions are used.
         *
         * If a function is not mocked but called in unit tests, then the call
         * will be logged and the process will abort.
         */
        class MockableAsyncStorage: public AsyncStorage
        {
        public:
            MockableAsyncStorage() { }

            virtual int fd() const override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual void handleEvents() override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual void waitReadyAsync(const Namespace&, const ReadyAck&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual void setAsync(const Namespace&, const DataMap&, const ModifyAck&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual void setIfAsync(const Namespace&, const Key&, const Data&, const Data&, const ModifyIfAck&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual void setIfNotExistsAsync(const Namespace&, const Key&, const Data&, const ModifyIfAck&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual void getAsync(const Namespace&, const Keys&, const GetAck&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual void removeAsync(const Namespace&, const Keys&, const ModifyAck&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual void removeIfAsync(const Namespace&, const Key&, const Data&, const ModifyIfAck&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual void findKeysAsync(const Namespace&, const std::string&, const FindKeysAck&) override { logAndAbort(__PRETTY_FUNCTION__); }

            virtual void removeAllAsync(const Namespace&, const ModifyAck&) override { logAndAbort(__PRETTY_FUNCTION__); }

        private:
            static void logAndAbort(const char* function) noexcept __attribute__ ((__noreturn__))
            {
                std::cerr << "MockableAsyncStorage: calling not-mocked function "
                          << function << ", aborting" << std::endl;
                abort();
            }
        };
    }
}

#endif
