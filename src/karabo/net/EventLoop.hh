/* 
 * File:   EventLoop.hh
 * Author: heisenb
 *
 * Created on July 27, 2016, 4:21 PM
 */

#ifndef KARABO_NET_EVENTLOOP_HH
#define	KARABO_NET_EVENTLOOP_HH


#include <boost/thread.hpp>
#include <boost/asio.hpp>

#include <karabo/util.hpp>

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

            static void run();        

            static void stop();         

            static size_t getNumberOfThreads();

        private:

            EventLoop();

            EventLoop(const EventLoop&);

            static EventLoop& instance();

            void _addThread(const int nThreads);

            void _removeThread(const int nThreads);

            void runProtected();

            static void asyncInjectException();

            void asyncDestroyThread(const boost::thread::id& id);

            void clearThreadPool();

            size_t _getNumberOfThreads() const;

            boost::asio::io_service m_ioService;
            boost::thread_group m_threadPool;
            mutable boost::mutex m_threadPoolMutex;

            typedef std::map<boost::thread::id, boost::thread*> ThreadMap;
            ThreadMap m_threadMap;

        };
    }
}



#endif	/* EVENTLOOP_HH */

