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

#ifndef SHAREDDATALAYER_FILEDESCRIPTOR_HPP_
#define SHAREDDATALAYER_FILEDESCRIPTOR_HPP_

#include <functional>

namespace shareddatalayer
{
    class System;

    /**
     * @brief Wrapper for native file descriptor
     *
     * FileDescriptor provides a clever wrapper for native file descriptor and
     * takes care of closing it in destructor.
     */
    class FileDescriptor
    {
    public:
        /**
         * Take ownership of the given native file descriptor.
         *
         * @param fd The native file descriptor to wrap.
         */
        explicit FileDescriptor(int fd) noexcept;

        /**
         * Take ownership of the given native file descriptor.
         *
         * @param fd The native file descriptor to wrap.
         * @param system System instance to use.
         */
        FileDescriptor(System& system, int fd) noexcept;

        /**
         * Move ownership of the given fd.
         *
         * @param fd The file descriptor to move.
         */
        FileDescriptor(FileDescriptor&& fd) noexcept;

        /**
         * Move ownership of the given fd.
         *
         * @param fd The file descriptor to move.
         */
        FileDescriptor& operator = (FileDescriptor&& fd) noexcept;

        /**
         * Close the wrapped native file descriptor. If <i>at close</i> function
         * is set, then it is called just before closing the file descriptor.
         *
         * @see atClose
         */
        ~FileDescriptor();

        /**
         * Get the wrapped native file descriptor.
         *
         * @return The wrapped native file descriptor.
         */
        operator int() const noexcept { return fd; }

        /**
         * Set function to be called just before closing the native file
         * descriptor.
         */
        void atClose(std::function<void(int)>);

        /**
         * Close the native file descriptor.
         */
        void close();

        FileDescriptor(const FileDescriptor&) = delete;
        FileDescriptor& operator = (const FileDescriptor&) = delete;

    private:
        System* system;
        int fd;
        std::function<void(int)> atCloseCb;
    };
}

#endif
