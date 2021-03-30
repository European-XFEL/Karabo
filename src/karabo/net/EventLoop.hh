/* 
 * File:   EventLoop.hh
 * Author: heisenb
 *
 * Created on July 27, 2016, 4:21 PM
 */

#ifndef KARABO_NET_EVENTLOOP_HH
#define	KARABO_NET_EVENTLOOP_HH

#include <map>
#include <mutex>
#include <memory>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/function/function_fwd.hpp>

#include "karabo/util/ClassInfo.hh"

namespace karabo {
    namespace net {

        /**
         * An exception that is thrown if a thread cannot be removed from the
         * EventLoop
         */
        struct RemoveThreadException : public std::exception {

        };

        /**
         * @class EventLoop
         * @brief Karabo's central event loop. Asynchronous events are passed throughout
         *        the distributed system by posting to the loop. 
         */
        class EventLoop : private boost::noncopyable {

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
             * Return the Eventloop's underlying boost::asio::io_service
             * @return 
             */
            static boost::asio::io_service& getIOService();

            /** Start the event loop and block until EventLoop::stop() is called.
             *
             *  The system signals SIGINT and SIGTERM will be caught and trigger the following actions:
             *  - a signal handler set via setSignalHandler is called,
             *  - and the event loop is stopped.
             */
            static void work();

            /** Start the event loop and block until all work posted to its io service is
             *  completed or until EventLoop::stop() is called.
             *   
             *  Frequently, this function should be called in a separate thread, which
             *  blocks upon joining until all work has been processed or stop has been 
             *  called.
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

            typedef boost::function<void (int /*signal*/) > SignalHandler;
            /** Set the handler to be called if a system signal is caught.
             *
             * See work() about which signals are caught.
             *
             * @param handler function with signature 'void (int signal)'
             */
            static void setSignalHandler(const SignalHandler& handler);

        private:

            EventLoop() = default;

            // Delete copy constructor and assignment operator since EventLoop is a singleton:
            EventLoop(const EventLoop&) = delete;
            EventLoop& operator=(const EventLoop&) = delete;

            static void init();

            static boost::shared_ptr<EventLoop> instance();

            void _addThread(const int nThreads);

            void _removeThread(const int nThreads);

            void runProtected();

            static void asyncInjectException();

            void asyncDestroyThread(const boost::thread::id& id);

            void clearThreadPool();
            
            size_t _getNumberOfThreads() const;

            void _setSignalHandler(const SignalHandler& handler);

            boost::asio::io_service m_ioService;
            boost::thread_group m_threadPool;
            mutable boost::mutex m_threadPoolMutex;

            static boost::shared_ptr<EventLoop> m_instance;
            static boost::once_flag m_initInstanceFlag;

            typedef std::map<boost::thread::id, boost::thread*> ThreadMap;
            ThreadMap m_threadMap;

            boost::mutex m_signalHandlerMutex;
            SignalHandler m_signalHandler;
        };
    }
}



#endif	/* EVENTLOOP_HH */

