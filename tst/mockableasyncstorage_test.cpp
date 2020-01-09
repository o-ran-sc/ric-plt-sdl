#include <gtest/gtest.h>
#include <sdl/tst/mockableasyncstorage.hpp>

TEST(MockableAsyncStorageTest, CanCreateInstance)
{
    shareddatalayer::tst::MockableAsyncStorage mockableAsyncStorage;
    static_cast<void>(mockableAsyncStorage);
}
