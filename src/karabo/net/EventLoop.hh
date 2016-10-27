/* 
 * File:   EventLoop.hh
 * Author: heisenb
 *
 * Created on July 27, 2016, 4:21 PM
 */

#ifndef KARABO_NET_EVENTLOOP_HH
#define	KARABO_NET_EVENTLOOP_HH

#include <map>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/function/function_fwd.hpp>

#include "karabo/util.hpp"

namespace karabo {
    namespace net {

        struct RemoveThreadException : public std::exception {

        };

        class EventLoop : private boost::noncopyable {

        public:

            KARABO_CLASSINFO(EventLoop, "EventLoop", "1.0")

            virtual ~EventLoop();

            static void addThread(const int nThreads = 1);

            static void removeThread(const int nThreads = 1);

            static boost::asio::io_service& getIOService();

            /// Start the event loop and block until EventLoop::stop() is called.
            ///
            /// The system signals SIGINT, SIGTERM and SIGSEGV will be caught and trigger the following actions:
            /// - for SIGSEGV, a stack trace is put out to std::cerr,
            /// - a signal handler set via setSignalHandler is called,
            /// - and the event loop is stopped.
            static void work();

            /// Start the event loop and block until all work posted to its io service is
            /// completed or until EventLoop::stop() is called.
            static void run();

            static void stop();

            static size_t getNumberOfThreads();

            typedef boost::function<void (int /*signal*/) > SignalHandler;
            /// Set the handler to be called if a system signal (SIGINT, SIGTERM, SIGSEGV) is caught.
            static void setSignalHandler(const SignalHandler& handler);

        private:

            EventLoop();

            EventLoop(const EventLoop&) = delete;

            static EventLoop& instance();

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
            static boost::mutex m_initMutex;

            typedef std::map<boost::thread::id, boost::thread*> ThreadMap;
            ThreadMap m_threadMap;

            boost::mutex m_signalHandlerMutex;
            SignalHandler m_signalHandler;
        };
    }
}



#endif	/* EVENTLOOP_HH */

