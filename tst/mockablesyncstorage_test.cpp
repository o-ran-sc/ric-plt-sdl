#include <gtest/gtest.h>
#include <sdl/tst/mockablesyncstorage.hpp>

TEST(MockableSyncStorageTest, CanCreateInstance)
{
    shareddatalayer::tst::MockableSyncStorage mockableSyncStorage;
    static_cast<void>(mockableSyncStorage);
}
