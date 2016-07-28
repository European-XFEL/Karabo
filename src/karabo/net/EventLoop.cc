/* 
 * File:   EventLoop.cc
 * Author: heisenb
 * 
 * Created on July 27, 2016, 4:21 PM
 */

#include "EventLoop.hh"
#include "karabo/log/Logger.hh"

namespace karabo {
    namespace net {


        EventLoop::EventLoop() : m_ioServicePointer(new boost::asio::io_service()) {          
        }


        EventLoop::~EventLoop() {
        }


        EventLoop& EventLoop::getInstance() {
            static EventLoop loop;
            return loop;
        }


        EventLoop::IOServicePointer EventLoop::getIOService() {
            return getInstance().m_ioServicePointer;
        }


        void EventLoop::run() {
            getInstance().runProtected();          
        }


        void EventLoop::work() {
            EventLoop& loop = getInstance();
            boost::asio::io_service::work work(*(loop.m_ioServicePointer));
            loop.runProtected();
        }


        void EventLoop::reset() {
            getInstance().m_ioServicePointer->reset();
        }


        void EventLoop::addThread(const int nThreads) {
            EventLoop& loop = getInstance();
            loop.m_ioServicePointer->post(boost::bind(&karabo::net::EventLoop::_addThread, &loop, nThreads));
        }


        void EventLoop::_addThread(const int nThreads) {
            boost::mutex::scoped_lock lock(m_threadPoolMutex);
            for (int i = 0; i < nThreads; ++i) {
                boost::thread* thread = m_threadPool.create_thread(boost::bind(&karabo::net::EventLoop::runProtected, this));
                m_threadMap[thread->get_id()] = thread;
                KARABO_LOG_FRAMEWORK_DEBUG << "A thread (id: " << thread->get_id()
                        << ") was added to the event-loop, now running: "
                        << m_threadPool.size() << " threads in total";
            }
        }


        void EventLoop::removeThread(const int nThreads) {
            getInstance()._removeThread(nThreads);
        }


        void EventLoop::_removeThread(const int nThreads) {
            for (int i = 0; i < nThreads; ++i) {
                m_ioServicePointer->post(&asyncInjectException);
            }
        }


        void EventLoop::asyncInjectException() {
            throw RemoveThreadException();
        }


        void EventLoop::asyncDestroyThread(const boost::thread::id& id) {
            boost::mutex::scoped_lock lock(m_threadPoolMutex);
            ThreadMap::iterator it = m_threadMap.find(id);
            if (it != m_threadMap.end()) {
                m_threadPool.remove_thread(it->second);
                delete it->second;
            }
            m_threadMap.erase(id);
            KARABO_LOG_FRAMEWORK_DEBUG << "Removed thread (id: " << id
                    << ") from event-loop, now running: "
                    << m_threadPool.size() << " threads in total";
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

            const std::string fullMessage(" during event-loop callback (io_service), continuing in 100 ms");

            while (true) {
                try {
                    m_ioServicePointer->run();
                    return; // Regular exit
                } catch (const RemoveThreadException&) {
                    // This is a sign to remove this thread from the pool
                    // As we can not kill ourselves we will ask another thread to kindly do so
                    if (isValidThreadId()) {
                        m_ioServicePointer->post(boost::bind(&karabo::net::EventLoop::asyncDestroyThread, this, boost::this_thread::get_id()));                       
                        return; // No more while, we want to die
                    } else {
                        m_ioServicePointer->post(&asyncInjectException);
                        // This sleep is experimental, the idea is to prevent being called on the main thread again
                        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
                    }
                } catch (karabo::util::Exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Exception" << fullMessage << ": " << e;
                } catch (std::exception& e) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Standard exception" << fullMessage << ": " << e.what();
                } catch (...) {
                    KARABO_LOG_FRAMEWORK_ERROR << "Unknown exception" << fullMessage << ".";
                }
                boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            }
        }


        bool EventLoop::isValidThreadId() {
            boost::mutex::scoped_lock lock(m_threadPoolMutex);
            return m_threadMap.find(boost::this_thread::get_id()) != m_threadMap.end();
        }

    }
}



