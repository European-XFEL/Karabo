/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   EventLoop.hh
 * Author: heisenb
 *
 * Created on July 27, 2016, 4:21 PM
 */

#ifndef KARABO_NET_EVENTLOOP_HH
#define KARABO_NET_EVENTLOOP_HH

#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#include "karabo/data/types/ClassInfo.hh"

namespace karabo {
    namespace net {

        /**
         * An exception that is thrown if a thread cannot be removed from the
         * EventLoop
         */
        struct RemoveThreadException : public std::exception {};

        /**
         * @class EventLoop
         * @brief Karabo's central event loop. Asynchronous events are passed throughout
         *        the distributed system by posting to the loop.
         */
        class EventLoop {
           public:
            KARABO_CLASSINFO(EventLoop, "EventLoop", "1.0")

            virtual ~EventLoop() = default;

            /**
             * Add a number of threads to the event loop, increasing the number
             * of thread available to handle events posted to the loop
             * @param nThreads
             */
            static void addThread(const int nThreads = 1);

            /**
             * Remove a number of threads from the event loop, reducing the
             * number of threads available to handle events posted to the loop
             * @param nThreads
             */
            static void removeThread(const int nThreads = 1);

            /**
             * Post a task on the underlying io event loop for later execution
             * @param func a functor not taking any argument, but with any return type
             * @param delayMs execution will be delayed by given time (in milliseconds)
             */
            template <class Function>
            static void post(Function&& func, unsigned int delayMs = 0);

            /**
             * Return the Eventloop's underlying boost::asio::io_service
             * @return
             */
            static boost::asio::io_context& getIOService();

            /** Start the event loop and block until EventLoop::stop() is called.
             *
             *  The system signals SIGINT and SIGTERM will be caught and trigger the following actions:
             *  - a signal handler set via setSignalHandler is called,
             *  - and EventLoop::stop() is called.
             *
             *  Must not be called in parallel to itself or to run().

             * If one or more tasks are in deadlock and thus their threads cannot be joined at
             * the end, a karabo::util::TimeoutException is thrown.
             */
            static void work();

            /** Start the event loop and block until all work posted to its io service is
             *  completed or until EventLoop::stop() is called.
             *
             *  Must not be called in parallel to itself or to work().
             *
             * If one or more tasks are in deadlock and thus their threads cannot be joined at
             * the end, a karabo::util::TimeoutException is thrown.
             */
            static void run();

            /**
             * Stop the event loop, canceling any remaining work, i.e. unblocking
             * run()
             */
            static void stop();

            /**
             * Return the number of threads currently available to the event loop
             * for distributing work
             * @return
             */
            static size_t getNumberOfThreads();

            typedef std::function<void(int /*signal*/)> SignalHandler;
            /** Set the handler to be called if a system signal is caught.
             *
             * See work() about which signals are caught.
             *
             * @param handler function with signature 'void (int signal)'
             */
            static void setSignalHandler(const SignalHandler& handler);

            /**
             * Public flag to change behaviour of catching exceptions in threads
             *
             * By default, flag is true and the event loop runs its threads such that any exception is caught
             * and logged as error. If flag is false, exceptions are rethrown after logging them.
             *
             * @param flag that should be valid now
             * @return previous value of flag
             */
            static bool setCatchExceptions(bool flag) {
                return instance()->m_catchExceptions.exchange(flag);
            }

           private:
            EventLoop() : m_running(false), m_catchExceptions(true){};

            // Delete copy constructor and assignment operator since EventLoop is a singleton:
            EventLoop(const EventLoop&) = delete;
            EventLoop& operator=(const EventLoop&) = delete;

            static void init();

            static std::shared_ptr<EventLoop> instance();

            void _run();

            void _addThread(const int nThreads);

            void _removeThread(const int nThreads);

            bool runProtected();

            static void asyncInjectException();

            void asyncDestroyThread(const std::jthread::id& id);

            /**
             * Clears the thread pool and joins the threads
             *
             * If joining fails repeatedly, throws karabo::util::TimeoutException.
             */
            void clearThreadPool();

            size_t _getNumberOfThreads() const;

            void _setSignalHandler(const SignalHandler& handler);

            bool is_this_thread_in();

            boost::asio::io_context m_ioService;
            std::atomic<bool> m_running;
            std::atomic<bool> m_catchExceptions;

            static std::shared_ptr<EventLoop> m_instance;
            static std::once_flag m_initInstanceFlag;

            typedef std::map<std::thread::id, std::jthread*> ThreadMap;
            ThreadMap m_threadMap;
            mutable std::mutex m_threadMapMutex;

            std::mutex m_signalHandlerMutex;
            SignalHandler m_signalHandler;
        };

        // Implementation of templated functions
        template <class Function>
        void EventLoop::post(Function&& func, unsigned int delayMs) {
            boost::asio::io_context& service = getIOService();
            if (0 == delayMs) {
                service.post(std::forward<Function>(func));
            } else {
                auto timer = std::make_shared<boost::asio::steady_timer>(service);
                timer->expires_after(std::chrono::milliseconds(delayMs));
                // Bind timer shared_ptr to lambda to keep it alive as long as needed
#if __cplusplus < 201402L
                // Pre-C++14 does not support generalized lambda capture
                timer->async_wait([func, timer]
#else
                timer->async_wait([func = std::forward<Function>(func), timer]
#endif
                                  (const boost::system::error_code& e) {
                                      if (e) return;
                                      func();
                                  });
            }
        }
    } // namespace net
} // namespace karabo


#endif /* EVENTLOOP_HH */
