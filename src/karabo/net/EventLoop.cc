/* 
 * File:   EventLoop.cc
 * Author: heisenb
 * 
 * Created on July 27, 2016, 4:21 PM
 */

#include <csignal>
#include <iostream>

#include "EventLoop.hh"
#include "karabo/log/Logger.hh"

namespace karabo {
    namespace net {


        std::shared_ptr<EventLoop> EventLoop::m_instance {nullptr};
        std::once_flag EventLoop::m_initInstanceFlag;


        void EventLoop::init() {
            m_instance.reset(new EventLoop);
        } 


        std::shared_ptr<EventLoop> EventLoop::instance() {
            std::call_once(m_initInstanceFlag, &EventLoop::init);
            return m_instance;
        }


        boost::asio::io_service& EventLoop::getIOService() {
            return instance()->m_ioService;
        }


      void EventLoop::work() {

            boost::asio::signal_set signals(getIOService(), SIGINT, SIGTERM);
            auto loop = instance();
            // TODO: Consider to use ordinary function instead of this lengthy lambda.
            boost::function<void(boost::system::error_code ec, int signo) > signalHandler
                    = [&loop](boost::system::error_code ec, int signo) {
                        if (ec) return;

                        {
                            boost::mutex::scoped_lock(loop->m_signalHandlerMutex);
                            if (loop->m_signalHandler) {
                                loop->m_signalHandler(signo);
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
            // First reset io service if e.g. stop() was called before this run()
            // and after a previous run() had finished since out of work.
            auto loop = instance();
            loop->m_ioService.reset();
            loop->runProtected();
            loop->m_threadPool.join_all();
            loop->clearThreadPool();
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
            boost::mutex::scoped_lock(m_signalHandlerMutex);
            m_signalHandler = handler;
        }


        void EventLoop::addThread(const int nThreads) {
            auto loop = instance();
            loop->m_ioService.post(boost::bind(&karabo::net::EventLoop::_addThread, loop, nThreads));
        }


        void EventLoop::_addThread(const int nThreads) {
            boost::mutex::scoped_lock lock(m_threadPoolMutex);
            auto loop = instance();
            for (int i = 0; i < nThreads; ++i) {
                boost::thread* thread = m_threadPool.create_thread(boost::bind(&EventLoop::runProtected, loop));
                m_threadMap[thread->get_id()] = thread;
                KARABO_LOG_FRAMEWORK_DEBUG << "A thread (id: " << thread->get_id()
                        << ") was added to the event-loop, now running: "
                        << m_threadPool.size() << " threads in total";
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
                it->second->join();
                m_threadPool.remove_thread(it->second);
                delete it->second;
                m_threadMap.erase(it);
                if (m_threadPool.size() > 1) { // Failed to print the last thread: SIGSEGV
                    KARABO_LOG_FRAMEWORK_DEBUG << "Removed thread (id: " << id
                            << ") from event-loop, now running: "
                            << m_threadPool.size() << " threads in total";
                }
            }
        }


        void EventLoop::clearThreadPool() {
            boost::mutex::scoped_lock lock(m_threadPoolMutex);
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
                    boost::mutex::scoped_lock lock(m_threadPoolMutex);
                    if (m_threadPool.is_this_thread_in()) {
                        m_ioService.post(boost::bind(&EventLoop::asyncDestroyThread, this, boost::this_thread::get_id()));
                        return; // No more while, we want to die
                    } else {
                        // We are in the main blocking thread here, which we never want to kill
                        // Hence, we are injecting the exception again to be taken by another thread
                        // Only if at least one thread to kill
                        if (m_threadPool.size() > 0) {
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



