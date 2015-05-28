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
#include <karabo/util.hpp>

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
            : m_callback()
            , m_timeout(-1)
            , m_repetition(-1)
            , m_running(false)
            , m_abort(false)
            , m_suspended(false)
            , m_request()
            , m_thread(0)
            , m_mutexRequest()
            , m_condRequest() {
            }

            /**
             * Construct worker with callback and time and repetition parameters
             * @param callback this function will be called periodically
             * @param timeout time in milliseconds auxiliary thread is waiting on the <b>request</b> queue; 0 means <i>nowait</i> mode; <0 means <i>waiting forever</i>
             * @param repetition <0 means <i>cycling forever</i>; 0 makes no sense; >0 means number of cycles.
             */
            BaseWorker(const boost::function<void()>& callback, int timeout = -1, int repetition = -1)
            : m_callback(callback)
            , m_timeout(timeout)
            , m_repetition(repetition)
            , m_running(false)
            , m_abort(false)
            , m_suspended(false)
            , m_request()
            , m_thread(0)
            , m_mutexRequest()
            , m_condRequest() {
            }

            BaseWorker(const BaseWorker& other)
            : m_callback(other.getCallback())
            , m_timeout(other.getTimeout())
            , m_repetition(other.getRepetition())
            , m_running(false)
            , m_abort(false)
            , m_suspended(false)
            , m_request()
            , m_thread(0)
            , m_mutexRequest()
            , m_condRequest() {
            }

            virtual ~BaseWorker() {
            }

            /**
             * Set parameters defining the behavior of the worker
             * @param callback    function to be called will boolean parameter signaling repetition counter expiration
             * @param timeout     timeout for receiving from queue
             * @param repetition  repetition counter
             */
            BaseWorker& set(const boost::function<void()>& callback, int timeout = -1, int repetition = -1) {
                m_callback = callback;
                m_timeout = timeout;
                m_repetition = repetition;
                return *this;
            }

            const boost::function<void()>& getCallback() const {
                return m_callback;
            }

            /**
             * Set parameters defining the behavior of the worker
             * @param timeout     timeout for receiving from queue
             */
            BaseWorker& setTimeout(int timeout = -1) {
                m_timeout = timeout;
                return *this;
            }

            const int& getTimeout() const {
                return m_timeout;
            }

            /**
             * Set parameters defining the behavior of the worker
             * @param repetition     repetition counter
             */
            BaseWorker& setRepetition(int repetition = -1) {
                m_repetition = repetition;
                return *this;
            }

            const int& getRepetition() const {
                return m_repetition;
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
            
            void setErrorHandler(const boost::function<void(const karabo::util::Exception&)>& handler) {
                m_error = handler;
            }
            
            void setExitHandler(const boost::function<void()>& handler) {
                m_exit = handler;
            }

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
                try {
                    while (!m_abort) {
                        T t;
                        if (!m_count) {   
                            if (m_exit) m_exit();
                            break;
                        }
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
                                if (stopCondition(t)) {
                                    if (m_exit) m_exit();
                                    break;
                                }
                            }
                        }
                        // decrement counter and call callback if not suspended
                        if (m_count > 0)
                            m_count--;
                        if (m_running) {
                            m_callback();
                        }
                    }
                } catch (const karabo::util::Exception& e) {
                    if (m_error) m_error(e);
                } catch (...) {
                    // Uncaught exception
                    if (m_error) m_error(karabo::util::Exception("Exception in worker callback", "Uncaught exception",
                                                                 __FILE__, BOOST_CURRENT_FUNCTION, __LINE__));
                }
                if (m_running) {
                    boost::mutex::scoped_lock lock(m_mutexRequest);
                    m_running = false;
                }
            }

        private:
            boost::function<void () > m_callback; // this callback defined once in constructor
            int m_timeout; // timeout (milliseconds), 0 = nowait, -1 = wait forever
            int m_repetition; // number of periodic cycles, <0 = no limit
            bool m_running; // "running" flag (default: true)
            bool m_abort; // "abort" flag   (default: false)
            bool m_suspended; // "suspended" flag (default: false))
            std::queue<T> m_request; // request queue
            boost::thread* m_thread; // auxiliary thread
            boost::mutex m_mutexRequest; // mutex of request queue
            boost::condition_variable m_condRequest; // condition variable of the request queue
            int m_count; // current repetition counter
            boost::function<void(const karabo::util::Exception&)> m_error;
            boost::function<void () > m_exit; // this callback defined once in constructor
        };

        struct Worker : public BaseWorker<bool> {

            KARABO_CLASSINFO(Worker, "Worker", "1.0")

            Worker() : BaseWorker<bool>() {
            }

            Worker(const boost::function<void()>& callback, int delay = -1, int repetitions = -1)
            : BaseWorker<bool>(callback, delay, repetitions) {
            }

            virtual ~Worker() {
                abort().join();
            }

            bool stopCondition(const bool& data) {
                return data;
            }
        };

        struct QueueWorker : public BaseWorker<karabo::util::Hash::Pointer> {

            KARABO_CLASSINFO(QueueWorker, "QueueWorker", "1.0")

            QueueWorker(const boost::function<void(const karabo::util::Hash::Pointer&)>& callback)
            : BaseWorker<karabo::util::Hash::Pointer>(boost::bind(&QueueWorker::onWork, this))
            , m_callback(callback) {
            }

            virtual ~QueueWorker() {
                abort().join();
            }

            void onWork() {
                m_callback(m_hash);
                m_hash->clear();
            }
            
            bool stopCondition(const karabo::util::Hash::Pointer& hash) {
                if (hash->has("stop")) return true;
                m_hash = hash;
                return false;
            }

        private:

            karabo::util::Hash::Pointer m_hash;
            boost::function<void(const karabo::util::Hash::Pointer&)> m_callback;
        };
    }
}

#endif	/* KARABO_CORE_WORKER_HH */

