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

#include <type_traits>
#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "private/filedescriptor.hpp"
#include "private/tst/systemmock.hpp"

using namespace shareddatalayer;
using namespace shareddatalayer::tst;
using namespace testing;

namespace
{
    class FileDescriptorTest: public Test
    {
    public:
        SystemMock systemMock;
        std::function<void(int)> atCloseCb = [&](int fd) { delFD(fd); };

        MOCK_METHOD1(delFD, void(int));
    };
}

TEST_F(FileDescriptorTest, IsNotCopyable)
{
    EXPECT_FALSE(std::is_copy_assignable<FileDescriptor>::value);
    EXPECT_FALSE(std::is_copy_constructible<FileDescriptor>::value);
}

TEST_F(FileDescriptorTest, IsNotDefaultConstructible)
{
    EXPECT_FALSE(std::is_default_constructible<FileDescriptor>::value);
}

TEST_F(FileDescriptorTest, IsConvertibleToInt)
{
    EXPECT_CALL(systemMock, close(1)).Times(1);
    EXPECT_EQ(-1, FileDescriptor(-1));
    EXPECT_EQ(1, FileDescriptor(systemMock, 1));
}

TEST_F(FileDescriptorTest, WhenDestructedFileDescriptorIsClosed)
{
    EXPECT_CALL(systemMock, close(1)).Times(1);
    FileDescriptor(systemMock, 1);
}

TEST_F(FileDescriptorTest, InvalidFileDescriptorIsNotClosed)
{
    EXPECT_CALL(systemMock, close(_)).Times(0);
    FileDescriptor(systemMock, -1);
}

TEST_F(FileDescriptorTest, DetachWhenDestructed)
{
    EXPECT_CALL(*this, delFD(1)).Times(1);
    EXPECT_CALL(systemMock, close(1)).Times(1);
    FileDescriptor(systemMock, 1).atClose(atCloseCb);
}

TEST_F(FileDescriptorTest, InvalidFileDescriptorIsNotDetached)
{
    EXPECT_CALL(*this, delFD(_)).Times(0);
    FileDescriptor(-1).atClose(atCloseCb);
}

TEST_F(FileDescriptorTest, OwnershipIsTransferredInMoveConstructor)
{
    const int rawfd(100);
    InSequence dummy;
    std::unique_ptr<FileDescriptor> fd1(new FileDescriptor(systemMock, rawfd));
    fd1->atClose(atCloseCb);
    std::unique_ptr<FileDescriptor> fd2(new FileDescriptor(std::move(*fd1)));
    EXPECT_CALL(systemMock, close(_)).Times(0);
    EXPECT_CALL(*this, delFD(_)).Times(0);
    fd1.reset();
    EXPECT_CALL(*this, delFD(rawfd)).Times(1);
    EXPECT_CALL(systemMock, close(rawfd)).Times(1);
    fd2.reset();
}

TEST_F(FileDescriptorTest, OldFdIsClosedAndOwnershipIsTransferredInMoveAssignment)
{
    const int rawfd1(100);
    const int rawfd2(200);
    InSequence dummy;
    std::unique_ptr<FileDescriptor> fd1(new FileDescriptor(systemMock, rawfd1));
    fd1->atClose(atCloseCb);
    std::unique_ptr<FileDescriptor> fd2(new FileDescriptor(systemMock, rawfd2));
    fd2->atClose(atCloseCb);
    EXPECT_CALL(*this, delFD(rawfd2)).Times(1);
    EXPECT_CALL(systemMock, close(rawfd2)).Times(1);
    *fd2 = std::move(*fd1);
    fd1.reset();
    EXPECT_CALL(*this, delFD(rawfd1)).Times(1);
    EXPECT_CALL(systemMock, close(rawfd1)).Times(1);
    fd2.reset();
}
