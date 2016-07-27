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

        class EventLoop {

        public:

            KARABO_CLASSINFO(EventLoop, "EventLoop", "1.0")

            typedef boost::shared_ptr<boost::asio::io_service> IOServicePointer;

            EventLoop();

            EventLoop(const EventLoop& orig);

            virtual ~EventLoop();

            static void addThread(const int nThreads = 1);

            static void removeThread(const int nThreads = 1);

            static IOServicePointer getIOService();

            static void run();

            static void work();

            static EventLoop& getInstance();

        private:


            void _addThread(const int nThreads);

            void _removeThread(const int nThreads);
            
            void runProtected();

            static void asyncInjectException();

            void asyncDestroyThread(const boost::thread::id& id);

            bool isValidThreadId();



            IOServicePointer m_ioServicePointer;
            boost::thread_group m_threadPool;
            boost::mutex m_threadPoolMutex;

            typedef std::map<boost::thread::id, boost::thread*> ThreadMap;
            ThreadMap m_threadMap;

        };
    }
}



#endif	/* EVENTLOOP_HH */

