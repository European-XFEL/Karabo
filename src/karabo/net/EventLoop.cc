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
 * File:   EventLoop.cc
 * Author: heisenb
 *
 * Created on July 27, 2016, 4:21 PM
 */

#include "EventLoop.hh"

#include <csignal>
#include <iostream>

#include "karabo/log/Logger.hh"

#if defined(__GNUC__) || defined(__clang__)
#include <cxxabi.h>
#endif

namespace karabo {
    namespace net {


        boost::shared_ptr<EventLoop> EventLoop::m_instance{nullptr};
        boost::once_flag EventLoop::m_initInstanceFlag = BOOST_ONCE_INIT;


        void EventLoop::init() {
            m_instance.reset(new EventLoop);
        }


        boost::shared_ptr<EventLoop> EventLoop::instance() {
            boost::call_once(&EventLoop::init, m_initInstanceFlag);
            return m_instance;
        }


        boost::asio::io_service& EventLoop::getIOService() {
            return instance()->m_ioService;
        }


        void EventLoop::work() {
            // Note: If signal set is changed, adjust documentation (also in karathon)
            boost::asio::signal_set signals(getIOService(), SIGINT, SIGTERM);
            auto loop = instance();
            // TODO: Consider to use ordinary function instead of this lengthy lambda.
            boost::function<void(boost::system::error_code ec, int signo)> signalHandler =
                  [&loop](boost::system::error_code ec, int signo) {
                      if (ec == boost::asio::error::operation_aborted) {
                          KARABO_LOG_FRAMEWORK_WARN
                                << "*** EventLoop::work() signalHandler: signal_set cancelled. signal: " << signo;
                      }
                      if (ec) return;

                      {
                          boost::mutex::scoped_lock lock(loop->m_signalHandlerMutex);
                          if (loop->m_signalHandler) {
                              loop->m_signalHandler(signo);
                              // Clear handler, so it is called exactly once as handlers passed to 'signals.async_wait'
                              loop->m_signalHandler.clear();
                          }
                      }
                      // Some time to do all actions possibly triggered by handler.
                      boost::this_thread::sleep(boost::posix_time::seconds(1));
                      // Finally go down, i.e. leave work()
                      EventLoop::stop();
                      // TODO (check!):
                      // Once we have no thread running for the DeviceServer, but only the EventLoop,
                      // we could stop() without sleep and then run().
                      // If the main in deviceServer.cc registers a handler that resets the DeviceServer::Pointer,
                      // the DeviceServer destructor will stop all re-registrations and thus let run() fade out.
                  };
            signals.async_wait(signalHandler);

            boost::asio::io_service::work work(getIOService());
            run();
        }


        void EventLoop::run() {
            auto loop = instance();
            loop->_run();
        }

        void EventLoop::_run() {
            // First restart io service if e.g. stop() was called
            // before this run() and after a previous run() had finished since out of work.
            m_ioService.restart();
            m_running = true; // _addThread(..) must not directly add a thread before m_ioService.restart();
            runProtected();
            m_running = false;
            clearThreadPool();
        }


        void EventLoop::stop() {
            instance()->m_ioService.stop();
        }


        size_t EventLoop::getNumberOfThreads() {
            return instance()->_getNumberOfThreads();
        }


        size_t EventLoop::_getNumberOfThreads() const {
            boost::mutex::scoped_lock lock(m_threadPoolMutex);
            return m_threadPool.size();
        }


        void EventLoop::setSignalHandler(const SignalHandler& handler) {
            instance()->_setSignalHandler(handler);
        }


        void EventLoop::_setSignalHandler(const SignalHandler& handler) {
            boost::mutex::scoped_lock lock(m_signalHandlerMutex);
            m_signalHandler = handler;
        }


        void EventLoop::addThread(const int nThreads) {
            auto loop = instance();
            loop->_addThread(nThreads);
        }


        void EventLoop::_addThread(const int nThreads) {
            auto loop = instance();
            auto add = [loop, nThreads]() {
                boost::mutex::scoped_lock lock(loop->m_threadPoolMutex);
                for (int i = 0; i < nThreads; ++i) {
                    boost::thread* thread =
                          loop->m_threadPool.create_thread(boost::bind(&EventLoop::runProtected, loop));
                    loop->m_threadMap[thread->get_id()] = thread;
                    KARABO_LOG_FRAMEWORK_DEBUG
                          << "A thread (id: " << thread->get_id()
                          << ") was added to the event-loop, now running: " << loop->m_threadPool.size()
                          << " threads in total";
                }
            };
            // If main thread is already running, we can directly add the thread. Otherwise we have to postpone to avoid
            // that the new thread calls m_ioService.run() before EventLoop::run() calls m_ioService.restart().
            if (m_running) {
                add();
            } else {
                // Postpone until main thread is running.
                m_ioService.post(add);
            }
        }


        void EventLoop::removeThread(const int nThreads) {
            instance()->_removeThread(nThreads);
        }


        void EventLoop::_removeThread(const int nThreads) {
            for (int i = 0; i < nThreads; ++i) {
                m_ioService.post(&asyncInjectException);
            }
        }


        void EventLoop::asyncInjectException() {
            throw RemoveThreadException();
        }


        void EventLoop::asyncDestroyThread(const boost::thread::id& id) {
            boost::mutex::scoped_lock lock(m_threadPoolMutex);
            ThreadMap::iterator it = m_threadMap.find(id);
            if (it != m_threadMap.end()) {
                boost::thread* theThread = it->second;
                m_threadMap.erase(it);
                m_threadPool.remove_thread(theThread);
                const size_t poolSize = m_threadPool.size();
                if (poolSize > 1) { // Failed to print the last thread: SIGSEGV
                    // An attempt to use Logger API here may result in SIGSEGV: we are depending on
                    // life time of this object (that's bad!!!).  We depend on the answer to the following questions:
                    // 1. How does the order of static's initialization (and the corresponding destruction order) work?
                    // 2. How does the static'c re-initialization influence on such order?
                    KARABO_LOG_FRAMEWORK_DEBUG << "Removed thread (id: " << id
                                               << ") from event-loop, now running: " << poolSize << " threads in total";
                }
                // Join _with_ lock:
                // * We get here only when m_ioService.run() has finished for 'theThread', so nothing running anymore
                //   and join() is trivial.
                // * The lock guarantees that two threads do not try to join each other concurrently which would be a
                //   deadlock (though logically that cannot happen anyway here since m_ioService.run() is not running
                //   anymore and thus the thread cannot get the task to destroy another thread).
                theThread->join();
                delete theThread;
            }
        }

        void EventLoop::clearThreadPool() {
            boost::mutex::scoped_lock lock(m_threadPoolMutex);

            int round = 1;
            auto clearThreads = [this, &round]() {
                for (ThreadMap::iterator it = m_threadMap.begin(); it != m_threadMap.end();) {
                    boost::thread* theThread = it->second;
                    // Try to join the thread and clean-up
                    if (theThread->try_join_for(boost::chrono::milliseconds(100))) {
                        m_threadPool.remove_thread(theThread);
                        delete theThread;
                        it = m_threadMap.erase(it);
                    } else {
                        // Joining can fail if 'asyncDestroyThread()' is blocked when it tries to lock
                        // m_threadPoolMutex that is locked here as well.
                        ++it;
                        KARABO_LOG_FRAMEWORK_WARN << "Thread not joined after 100 ms (round " << round << ")";
                    }
                }
            };
            clearThreads();

            // There may be threads left. Hopefully only those that ran 'asyncDestroyThread', but were stuck on the
            // mutex. For those we release the lock and try a few more times. If that does not help, there is probably
            // something misbehaving and we give up - the exception will likely finish the process.
            while (!m_threadMap.empty()) {
                // 100 rounds: overall delay of up to 10 s from this loop plus 0.1 s per loop times stuck thread
                if (++round > 100) {
                    using karabo::util::toString; // note that m_threadPool.size() needs mutex
                    throw KARABO_TIMEOUT_EXCEPTION("Repeated failure to join all threads, " +
                                                         toString(m_threadPool.size()) += " threads left");
                };
                lock.unlock();
                boost::this_thread::sleep(boost::posix_time::milliseconds(100));
                lock.lock();
                clearThreads();
            }
        }


        void EventLoop::runProtected() {
            // See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference/io_service.html:
            // "If an exception is thrown from a handler, the exception is allowed to propagate through the throwing
            //  thread's invocation of run(), run_one(), poll() or poll_one(). No other threads that are calling any of
            //  these functions are affected. It is then the responsibility of the application to catch the exception.
            //
            //  After the exception has been caught, the run(), run_one(), poll() or poll_one() call may be restarted
            //  without the need for an intervening call to reset(). This allows the thread to rejoin the io_service
            //  object's thread pool without impacting any other threads in the pool."

            const std::string fullMessage(" during event-loop callback (io_service) ");

            while (true) {
                try {
                    m_ioService.run();
                    return; // Regular exit
                } catch (const RemoveThreadException&) {
                    // This is a sign to remove this thread from the pool
                    // As we can not kill ourselves we will ask another thread to kindly do so
                    boost::mutex::scoped_lock lock(m_threadPoolMutex);
                    if (m_threadPool.is_this_thread_in()) {
                        m_ioService.post(
                              boost::bind(&EventLoop::asyncDestroyThread, this, boost::this_thread::get_id()));
                        return; // No more while, we want to die
                    } else {
                        // We are in the main blocking thread here, which we never want to kill
                        // Hence, we are injecting the exception again to be taken by another thread
                        // Only if at least one thread to kill
                        if (m_threadPool.size() > 0) {
                            m_ioService.post(&asyncInjectException);
                            // We kindly ask the scheduler to put us on the back of the threads queue, avoiding we will
                            // eat the just posted exception again
                            lock.unlock(); // Just in case yield() could block...
                            boost::this_thread::yield();
                        }
                    }
                } catch (karabo::util::Exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Exception" << fullMessage << ": " << e;
                } catch (std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception" << fullMessage << ": " << e.what();
                } catch (...) {
                    std::string extraInfo;
#if defined(__GNUC__) || defined(__clang__)
                    // See
                    // https://stackoverflow.com/questions/561997/determining-exception-type-after-the-exception-is-caught
                    int status = 42; // Better init with a non-zero value...
                    char* txt = abi::__cxa_demangle(abi::__cxa_current_exception_type()->name(), 0, 0, &status);
                    if (status == 0 && txt) {
                        extraInfo = ": ";
                        extraInfo += txt;
                        free(txt);
                    }
#endif
                    KARABO_LOG_FRAMEWORK_ERROR << "Unknown exception" << fullMessage << extraInfo << ".";
                }
            }
        }
    } // namespace net
} // namespace karabo
