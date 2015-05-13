/* 
 * File:   Worker.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on June 24, 2014, 10:48 AM
 */

#ifndef KARABO_CORE_WORKER_HH
#define	KARABO_CORE_WORKER_HH

#include <queue>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

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
            
            KARABO_CLASSINFO(BaseWorker, "BaseWorker", "1.0")

            BaseWorker()
            : m_callback()
            , m_timeout(-1)
            , m_repetition(-1)
            , m_running(false)
            , m_abort(false)
            , m_suspended(false)
            , m_request()
            , m_thread(0)
            , m_mutexRequest()
            , m_condRequest()
            , m_mutexPrint() {
            }

            /**
             * Construct worker with callback and time and repetition parameters
             * @param callback this function will be called periodically
             * @param timeout time in milliseconds auxiliary thread is waiting on the <b>request</b> queue; 0 means <i>nowait</i> mode; <0 means <i>waiting forever</i>
             * @param repetition <0 means <i>cycling forever</i>; 0 makes no sense; >0 means number of cycles.
             */
            BaseWorker(const boost::function<void(bool)>& callback, int timeout = -1, int repetition = -1)
            : m_callback(callback)
            , m_timeout(timeout)
            , m_repetition(repetition)
            , m_running(false)
            , m_abort(false)
            , m_suspended(false)
            , m_request()
            , m_thread(0)
            , m_mutexRequest()
            , m_condRequest()
            , m_mutexPrint() {
            }

            virtual ~BaseWorker() {
            }

            /**
             * Set parameters defining the behavior of the worker
             * @param callback    function to be called will boolean parameter signaling repetition counter expiration
             * @param timeout     timeout for receiving from queue
             * @param repetition  repetition counter
             */
            BaseWorker& set(const boost::function<void(bool)>& callback, int timeout = -1, int repetition = -1) {
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
                    m_thread = new boost::thread(&BaseWorker::run, this);
                }
                if (m_suspended) {
                    m_suspended = false;
                    m_condRequest.notify_all();
                }
                return *this;
            }

            /**
             * Stop thread activity.  If "request" queue still has some entries they will be received before thread exits.
             * After requesting a stop the new entries can not be put (just ignored) into <b>request</b> queue.
             */
            BaseWorker& stop() {
                if (m_running) {
                    boost::mutex::scoped_lock lock(m_mutexRequest);
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
                    boost::mutex::scoped_lock lock(m_mutexRequest);
                    m_abort = true;
                    m_suspended = false;
                    m_condRequest.notify_all();
                }
                return *this;
            }

            BaseWorker& pause() {
                if (!m_suspended) {
                    boost::mutex::scoped_lock lock(m_mutexRequest);
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
                    boost::mutex::scoped_lock lock(m_mutexRequest);
                    m_request.push(t);
                    m_condRequest.notify_all();
                }
            }

            virtual bool stopCondition(const T& t) = 0;

            bool isRepetitionCounterExpired() const {
                return m_count == 0;
            }

        private:

            /**
             * Receive data block of type T from <b>request</b> queue.  The behavior is defined by two parameters:
             * <b>m_timeout</b> and <b>m_repetition</b>. In case of successful receiving the <b>m_callback</b> function will be called.
             */
            void run() {
                m_count = m_repetition;
                while (!m_abort) {
                    T t;
                    if (!m_count)
                        break;
                    {
                        boost::mutex::scoped_lock lock(m_mutexRequest);
                        while (m_suspended) {
                            m_condRequest.wait(lock); // use the same condition variable for "suspended" case
                        }
                        if (!m_running && m_request.empty())
                            break;
                        if (m_timeout < 0) {
                            while (m_request.empty() && !m_abort && m_running && !m_suspended) {
                                m_condRequest.wait(lock);
                            }
                        } else if (m_timeout > 0) {
                            boost::system_time const expired = boost::get_system_time() + boost::posix_time::milliseconds(m_timeout);
                            while (m_request.empty() && !m_abort && m_running && !m_suspended) {
                                if (!m_condRequest.timed_wait(lock, expired))
                                    break;
                            }
                        }
                        if (m_suspended)
                            continue;
                        // get next request from queue if not suspended and queue is not empty
                        if (!m_request.empty()) {
                            t = m_request.front();
                            m_request.pop();
                            if (stopCondition(t))
                                break;
                        }
                    }
                    // decrement counter and call callback if not suspended
                    if (m_count > 0)
                        m_count--;
                    if (m_running) {
                        m_callback(m_count == 0);
                    }
                }
                if (m_running) {
                    boost::mutex::scoped_lock lock(m_mutexRequest);
                    m_running = false;
                }
            }

        private:
            boost::function<void (bool) > m_callback; // this callback defined once in constructor
            int m_timeout; // timeout (milliseconds), 0 = nowait, -1 = wait forever
            int m_repetition; // number of periodic cycles, <0 = no limit
            bool m_running; // "running" flag (default: true)
            bool m_abort; // "abort" flag   (default: false)
            bool m_suspended; // "suspended" flag (default: false))
            std::queue<T> m_request; // request queue
            boost::thread* m_thread; // auxiliary thread
            boost::mutex m_mutexRequest; // mutex of request queue
            boost::condition_variable m_condRequest; // condition variable of the request queue
            boost::mutex m_mutexPrint; // mutex guarding the cout stream
            int m_count; // current repetition counter
        };

        struct Worker : public BaseWorker<bool> {
            
            KARABO_CLASSINFO(Worker, "Worker", "1.0")

            Worker() : BaseWorker<bool>() {
            }

            Worker(const boost::function<void(bool)>& callback, int delay = -1, int repetitions = -1)
            : BaseWorker<bool>(callback, delay, repetitions) {
            }

            virtual ~Worker() {
                abort().join();
            }

            bool stopCondition(const bool& data) {
                return false;
            }
        };
    }
}

#endif	/* KARABO_CORE_WORKER_HH */

