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
 * File:   Worker.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on June 24, 2014, 10:48 AM
 */

#ifndef KARABO_CORE_WORKER_HH
#define KARABO_CORE_WORKER_HH

#include <chrono>
#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>

#include "karabo/data/types/Hash.hh"

namespace karabo {
    namespace core {

        /**
         * WorkerBase class contains one queue: <b>request</b> and can start auxiliary thread
         * that will run on opposite end of the queue:
         *                 Main thread   Queue        Auxiliary thread
         *  Methods        push(...) --> request  --> receive(...)
         */
        template <typename T>
        class BaseWorker {
           public:
            KARABO_CLASSINFO(BaseWorker<T>, "BaseWorker", "1.0")

            BaseWorker()
                : m_callback(),
                  m_timeout(-1),
                  m_repetition(-1),
                  m_running(false),
                  m_abort(false),
                  m_suspended(false),
                  m_request(),
                  m_thread(0),
                  m_mutexRequest(),
                  m_condRequest() {}

            /**
             * Construct worker with callback and time and repetition parameters
             * @param callback this function will be called periodically
             * @param timeout time in milliseconds auxiliary thread is waiting on the <b>request</b> queue; 0 means
             * <i>nowait</i> mode; ,-1 means <i>waiting forever</i>
             * @param repetition -1 means <i>cycling forever</i>; >0 means number of cycles.
             */
            BaseWorker(const std::function<void()>& callback, int timeout = -1, int repetition = -1)
                : m_callback(callback),
                  m_timeout(timeout),
                  m_repetition(repetition),
                  m_running(false),
                  m_abort(false),
                  m_suspended(false),
                  m_request(),
                  m_thread(0),
                  m_mutexRequest(),
                  m_condRequest() {}

            BaseWorker(const BaseWorker& other)
                : m_callback(other.m_callback),
                  m_timeout(other.m_timeout),
                  m_repetition(other.m_repetition),
                  m_running(false),
                  m_abort(false),
                  m_suspended(false),
                  m_request(),
                  m_thread(0),
                  m_mutexRequest(),
                  m_condRequest(),
                  m_error(other.m_error),
                  m_exit(other.m_exit) {}

            BaseWorker& operator=(const BaseWorker& other) {
                if (this != &other) {
                    m_callback = other.m_callback;
                    m_timeout = other.m_timeout;
                    m_repetition = other.m_repetition;
                    m_error = other.m_error;
                    m_exit = other.m_exit;
                }
                return *this;
            }

            virtual ~BaseWorker() {}

            /**
             * Set parameters defining the behavior of the worker
             * @param callback    function to be called will boolean parameter signaling repetition counter expiration
             * @param timeout     timeout for receiving from queue
             * @param repetition  repetition counter
             */
            BaseWorker& set(const std::function<void()>& callback, int timeout = -1, int repetition = -1) {
                m_callback = callback;
                m_timeout = timeout;
                m_repetition = repetition;
                return *this;
            }

            /**
             * Set parameters defining the behavior of the worker
             * @param timeout     timeout for receiving from queue
             */
            BaseWorker& setTimeout(int timeout = -1) {
                m_timeout = timeout;
                return *this;
            }

            /**
             * Set parameters defining the behavior of the worker
             * @param repetition     repetition counter
             */
            BaseWorker& setRepetition(int repetition = -1) {
                m_repetition = repetition;
                return *this;
            }

            /**
             * Starts auxiliary thread that works on far ends of the queues
             * Default settings are "waiting forever" and "repeat forever"
             */
            BaseWorker& start() {
                if (m_thread == 0) {
                    m_running = true;
                    m_abort = false;
                    m_suspended = false;
                    m_thread = new std::jthread(&BaseWorker::run, this);
                }
                if (m_suspended) {
                    m_suspended = false;
                    m_condRequest.notify_all();
                }
                return *this;
            }

            /**
             * Stop thread activity.  If "request" queue still has some entries they will be received before thread
             * exits. After requesting a stop the new entries can not be put (just ignored) into <b>request</b> queue.
             */
            BaseWorker& stop() {
                if (m_running) {
                    std::lock_guard<std::mutex> lock(m_mutexRequest);
                    m_running = false;
                    m_suspended = false;
                }
                m_condRequest.notify_all();
                return *this;
            }

            /**
             * This function stops thread immediately despite the fact like nonempty queue.
             */
            BaseWorker& abort() {
                if (!m_abort) {
                    std::lock_guard<std::mutex> lock(m_mutexRequest);
                    m_abort = true;
                    m_suspended = false;
                    m_condRequest.notify_all();
                }
                return *this;
            }

            BaseWorker& pause() {
                if (!m_suspended) {
                    std::lock_guard<std::mutex> lock(m_mutexRequest);
                    m_suspended = true;
                    m_condRequest.notify_all();
                }
                return *this;
            }

            bool is_running() {
                return m_running;
            }

            /**
             * It will block until the auxiliary thread is joined.
             */
            void join() {
                if (m_thread) {
                    if (m_thread->joinable()) m_thread->join();
                    delete m_thread;
                    m_thread = 0;
                }
            }

            /**
             * Call this function from the main thread to put new data block to the <b>request</b> queue
             * @param t data of type T to put into the request queue
             */
            void push(const T& t) {
                if (m_running) {
                    std::lock_guard<std::mutex> lock(m_mutexRequest);
                    m_request.push(t);
                    m_condRequest.notify_all();
                }
            }

            virtual bool stopCondition(const T& t) = 0;

            void setErrorHandler(const std::function<void(const karabo::data::Exception&)>& handler) {
                m_error = handler;
            }

            void setExitHandler(const std::function<void()>& handler) {
                m_exit = handler;
            }

            bool isRepetitionCounterExpired() const {
                return m_count == 0;
            }

           private:
            /**
             * Receive data block of type T from <b>request</b> queue.  The behavior is defined by two parameters:
             * <b>m_timeout</b> and <b>m_repetition</b>. In case of successful receiving the <b>m_callback</b> function
             * will be called.
             */
            void run() {
                m_count = m_repetition;
                try {
                    while (!m_abort) {
                        T t;
                        if (!m_count) {
                            if (m_exit) m_exit();
                            break;
                        }
                        {
                            std::unique_lock<std::mutex> lock(m_mutexRequest);
                            while (m_suspended) {
                                m_condRequest.wait(lock); // use the same condition variable for "suspended" case
                            }
                            if (!m_running && m_request.empty()) break;
                            if (m_timeout < 0) {
                                while (m_request.empty() && !m_abort && m_running && !m_suspended) {
                                    m_condRequest.wait(lock);
                                }
                            } else if (m_timeout > 0) {
                                auto const expiredAt =
                                      std::chrono::system_clock::now() + std::chrono::milliseconds(m_timeout);
                                while (m_request.empty() && !m_abort && m_running && !m_suspended) {
                                    if (m_condRequest.wait_until(lock, expiredAt) == std::cv_status::timeout) break;
                                }
                            }
                            if (m_suspended) continue;
                            // get next request from queue if not suspended and queue is not empty
                            if (!m_request.empty()) {
                                t = m_request.front();
                                m_request.pop();
                                if (stopCondition(t)) {
                                    if (m_exit) m_exit();
                                    break;
                                }
                            }
                        }
                        // decrement counter and call callback if not suspended
                        if (m_count > 0) m_count--;
                        if (m_running) {
                            m_callback();
                        }
                    }
                } catch (const karabo::data::Exception& e) {
                    if (m_error) m_error(e);
                } catch (...) {
                    // Uncaught exception
                    if (m_error)
                        m_error(karabo::data::Exception("Exception in worker callback", "Uncaught exception", __FILE__,
                                                        BOOST_CURRENT_FUNCTION, __LINE__));
                }
                if (m_running) {
                    std::lock_guard<std::mutex> lock(m_mutexRequest);
                    m_running = false;
                }
            }

           private:
            std::function<void()> m_callback;      // this callback defined once in constructor
            int m_timeout;                         // timeout (milliseconds), 0 = nowait, -1 = wait forever
            int m_repetition;                      // number of periodic cycles, <0 = no limit
            bool m_running;                        // "running" flag (default: true)
            bool m_abort;                          // "abort" flag   (default: false)
            bool m_suspended;                      // "suspended" flag (default: false))
            std::queue<T> m_request;               // request queue
            std::jthread* m_thread;                // auxiliary thread
            std::mutex m_mutexRequest;             // mutex of request queue
            std::condition_variable m_condRequest; // condition variable of the request queue
            int m_count;                           // current repetition counter
            std::function<void(const karabo::data::Exception&)> m_error;
            std::function<void()> m_exit; // this callback defined once in constructor
        };

        /**
         * A worker that passes any data received in its queue to a callback function
         * working asynchronously working in a separate thread.
         */
        struct Worker : public BaseWorker<bool> {
            KARABO_CLASSINFO(Worker, "Worker", "1.0")

            Worker() : BaseWorker<bool>() {}

            /**
             * Instantiate a worker with a callback function to work on data.
             * See Worker::WorkerBase for options
             * @param callback
             * @param delay
             * @param repetitions
             */
            Worker(const std::function<void()>& callback, int delay = -1, int repetitions = -1)
                : BaseWorker<bool>(callback, delay, repetitions) {}

            virtual ~Worker() {
                abort().join();
            }

            bool stopCondition(const bool& data) {
                return data;
            }
        };

        struct QueueWorker : public BaseWorker<karabo::data::Hash::Pointer> {
            KARABO_CLASSINFO(QueueWorker, "QueueWorker", "1.0")

            QueueWorker()
                : BaseWorker<karabo::data::Hash::Pointer>(std::bind(&QueueWorker::onWork, this)),
                  m_hash(new karabo::data::Hash),
                  m_callback() {}

            QueueWorker(const std::function<void(const karabo::data::Hash::Pointer&)>& callback)
                : BaseWorker<karabo::data::Hash::Pointer>(std::bind(&QueueWorker::onWork, this)),
                  m_hash(new karabo::data::Hash),
                  m_callback(callback) {}

            QueueWorker(const QueueWorker& other)
                : BaseWorker<karabo::data::Hash::Pointer>(std::bind(&QueueWorker::onWork, this)),
                  m_hash(new karabo::data::Hash),
                  m_callback(other.m_callback) {}

            QueueWorker& operator=(const QueueWorker& rhs) {
                if (this != &rhs) {
                    *m_hash = *rhs.m_hash;
                    m_callback = rhs.m_callback;
                }
                return *this;
            }

            virtual ~QueueWorker() {
                abort().join();
            }

            void onWork() {
                m_callback(m_hash);
                m_hash->clear();
            }

            bool stopCondition(const karabo::data::Hash::Pointer& hash) {
                if (hash->has("stop")) return true;
                m_hash = hash;
                return false;
            }

           private:
            karabo::data::Hash::Pointer m_hash;
            std::function<void(const karabo::data::Hash::Pointer&)> m_callback;
        };
    } // namespace core
} // namespace karabo

#endif /* KARABO_CORE_WORKER_HH */
