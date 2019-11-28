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

#ifndef SHAREDDATALAYER_ENGINE_HPP_
#define SHAREDDATALAYER_ENGINE_HPP_

#include <functional>
#include "private/timer.hpp"

namespace shareddatalayer
{
    class FileDescriptor;

    /**
     * Engine provides the basic services needed by asynchronous library:
     * - File descriptor monitoring
     * - Timer monitoring
     * - Callback queue
     */
    class Engine
    {
        friend class Timer;

    public:
        Engine() = default;

        virtual ~Engine() = default;

        virtual int fd() const = 0;

        virtual void handleEvents() = 0;

        /** There is data to read. */
        static const unsigned int EVENT_IN;

        /** Writing is now possible. */
        static const unsigned int EVENT_OUT;

        /** Error condition. */
        static const unsigned int EVENT_ERR;

        /** Hang up. */
        static const unsigned int EVENT_HUP;

        /**
         * Event handler function type.
         *
         * @param events Pending events.
         */
        using EventHandler = std::function<void(unsigned int events)>;

        /**
         * Add a new file descriptor to be monitored. The added file descriptor
         * must be deleted with deleteMonitoredFD() function when no longer
         * monitored.
         *
         * @param fd     The file descriptor to monitor.
         * @param events The events to monitor as a bit mask of EVENT_IN and/or
         *               EVENT_OUT. There is no need to explicitly monitor
         *               EVENT_ERR or EVENT_HUP as they are automatically
         *               monitored.
         * @param eh     The event handler to call when any of the requested
         *               events occur.
         *
         * @see modifyMonitoredFD
         * @see deleteMonitoredFD
         */
        virtual void addMonitoredFD(int fd, unsigned int events, const EventHandler& eh) = 0;

        /**
         * Add a new file descriptor to be monitored. The added file descriptor
         * is automatically deleted with deleteMonitorFD() upon its destructor.
         * This is achieved by using the FileDescriptor::atClose() function.
         *
         * @param fd     The file descriptor to monitor.
         * @param events The events to monitor as a bit mask of EVENT_IN and/or
         *               EVENT_OUT. There is no need to explicitly monitor
         *               EVENT_ERR or EVENT_HUP as they are automatically
         *               monitored.
         * @param eh     The event handler to call when any of the requested
         *               events occur.
         *
         * @see modifyMonitoredFD
         */
        virtual void addMonitoredFD(FileDescriptor& fd, unsigned int events, const EventHandler& eh) = 0;

        /**
         * Modify monitored events related to an earlier added file descriptor.
         *
         * @param fd     The file descriptor whose monitored events to modify.
         * @param events The new events to monitor, bitmask of EVENTS_IN and/or
         *               EVENT_OUT. There is no need to explicitly monitor
         *               EVENT_ERR or EVENT_HUP as they are automatically
         *               monitored.
         *
         * @see addMonitoredFD
         * @see deleteMonitoredFD
         */
        virtual void modifyMonitoredFD(int fd, unsigned int events) = 0;

        /**
         * Stop monitoring the given file descriptor.
         *
         * @param fd The file descriptor to stop monitoring.
         *
         * @see addMonitoredFD
         * @see modifyMonitoredFD
         */
        virtual void deleteMonitoredFD(int fd) = 0;

        using Callback = std::function<void()>;

        /**
         * Post a callback function. The function will be called by the
         * application's thread which is running the event loop and monitoring
         * the library epoll file descriptor.
         *
         * The main use cases for this function are
         * 1. To push work for the application's thread
         * 2. Fake asynchronous API with synchronous implementation
         *
         * @note This function <b>is</b> thread-safe.
         */
        virtual void postCallback(const Callback& callback) = 0;

        virtual void run() = 0;

        virtual void stop() = 0;

        Engine(const Engine&) = delete;
        Engine(Engine&&) = delete;
        Engine& operator = (const Engine&) = delete;
        Engine& operator = (Engine&&) = delete;

    protected:
        virtual void armTimer(Timer& timer, const Timer::Duration& duration, const Timer::Callback& cb) = 0;

        virtual void disarmTimer(const Timer& timer) = 0;
    };
}

#endif
