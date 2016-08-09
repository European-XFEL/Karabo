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


        EventLoop::EventLoop() {
        }


        EventLoop::EventLoop(const EventLoop&) {
        }


        EventLoop::~EventLoop() {
        }


        EventLoop& EventLoop::instance() {
            static EventLoop loop;
            return loop;
        }


        boost::asio::io_service& EventLoop::getIOService() {
            return instance().m_ioService;
        }


        void EventLoop::run() {
            instance().runProtected();
            instance().m_threadPool.join_all();
            instance().clearThreadPool();
            instance().m_ioService.reset();
        }


        void EventLoop::stop() {
            instance().m_ioService.stop();
        }


        size_t EventLoop::getNumberOfThreads() {
            return instance()._getNumberOfThreads();
        }


        size_t EventLoop::_getNumberOfThreads() const {
            boost::mutex::scoped_lock lock(m_threadPoolMutex);
            return m_threadPool.size();
        }


        void EventLoop::addThread(const int nThreads) {
            EventLoop& loop = instance();
            loop.m_ioService.post(boost::bind(&karabo::net::EventLoop::_addThread, &loop, nThreads));
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
            instance()._removeThread(nThreads);
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
                it->second->join();
                m_threadPool.remove_thread(it->second);
                delete it->second;
            }
            m_threadMap.erase(id);
            KARABO_LOG_FRAMEWORK_DEBUG << "Removed thread (id: " << id
                    << ") from event-loop, now running: "
                    << m_threadPool.size() << " threads in total";
        }


        void EventLoop::clearThreadPool() {
            for (ThreadMap::iterator it = m_threadMap.begin(); it != m_threadMap.end(); ++it) {
                m_threadPool.remove_thread(it->second);
                delete it->second;
            }
            m_threadMap.clear();
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
                    if (m_threadPool.is_this_thread_in()) {
                        m_ioService.post(boost::bind(&karabo::net::EventLoop::asyncDestroyThread, this, boost::this_thread::get_id()));
                        return; // No more while, we want to die
                    } else {
                        // We are in the main blocking thread here, which we never want to kill
                        // Hence, we are injecting the exception again to be taken by another thread
                        // Only if at least one thread to kill
                        if (m_threadPool.size() > 0) {
                            //std::cout << "Illegal trial to remove thread on empty thread-pool" << std::endl;
                            m_ioService.post(&asyncInjectException);
                            // We kindly ask the scheduler to put us on the back of the threads queue, avoiding we will
                            // eat the just posted exception again
                            boost::this_thread::yield();
                        }
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
    }
}



