/*
 * File:   EventLoop_Test.cc
 * Author: heisenb
 *
 * Created on July 29, 2016, 2:06 PM
 */

#include "karabo/net/EventLoop.hh"

#include <csignal>

#include "EventLoop_Test.hh"
#include "karabo/log/Logger.hh"


using namespace karabo::util;
using namespace karabo::net;

CPPUNIT_TEST_SUITE_REGISTRATION(EventLoop_Test);


EventLoop_Test::EventLoop_Test() {}


EventLoop_Test::~EventLoop_Test() {}


void EventLoop_Test::handler1(boost::asio::deadline_timer& timer, int count) {
    if (count == -1) {
        CPPUNIT_ASSERT(EventLoop::getNumberOfThreads() == 0);
        return;
    }

    CPPUNIT_ASSERT(EventLoop::getNumberOfThreads() == size_t(count));

    if (count == 5) {
        EventLoop::removeThread(5);
        count = -1;
        timer.expires_at(timer.expires_at() + boost::posix_time::millisec(500));
        timer.async_wait(boost::bind(&EventLoop_Test::handler1, this, boost::ref(timer), count));
        return;
    }

    EventLoop::addThread();
    count++;

    timer.expires_at(timer.expires_at() + boost::posix_time::millisec(500));
    timer.async_wait(boost::bind(&EventLoop_Test::handler1, this, boost::ref(timer), count));
}


void EventLoop_Test::testMethod() {
    boost::asio::deadline_timer timer(EventLoop::getIOService(), boost::posix_time::millisec(500));
    timer.async_wait(boost::bind(&EventLoop_Test::handler1, this, boost::ref(timer), 0));

    EventLoop::run();
}


void EventLoop_Test::handler2() {
    if (m_finished) return;

    boost::asio::deadline_timer timer(EventLoop::getIOService(), boost::posix_time::millisec(5));
    EventLoop::getIOService().post(boost::bind(&EventLoop_Test::handler2, this));
}


void EventLoop_Test::handler3() {
    EventLoop::stop();
}


void EventLoop_Test::testMethod2() {
    boost::asio::io_service::work work(EventLoop::getIOService());
    boost::thread t(boost::bind(&EventLoop::run));

    m_finished = false;
    boost::asio::deadline_timer timer(EventLoop::getIOService(), boost::posix_time::millisec(500));
    EventLoop::addThread(10);
    EventLoop::getIOService().post(boost::bind(&EventLoop_Test::handler2, this));
    timer.async_wait(boost::bind(&EventLoop_Test::handler3, this));

    t.join();

    m_finished = true;

    CPPUNIT_ASSERT(true);
}


void EventLoop_Test::testSignalCapture() {
    boost::thread t(boost::bind(&EventLoop::work));

    bool terminateCaught = false;
    EventLoop::setSignalHandler([&terminateCaught](int signal) {
        if (signal == SIGTERM) {
            terminateCaught = true;
        }
    });

    // Allow signal handling to be activated (1 ms sleep seems OK, but the test fails without sleep).
    boost::this_thread::sleep(boost::posix_time::milliseconds(10));

    std::raise(SIGTERM);

    t.join();

    CPPUNIT_ASSERT(terminateCaught);
}

void EventLoop_Test::testAddThreadDirectly() {
    // Tests that, even in a so far single threaded event loop, adding a thread before blocking on something that
    // requires another task on the event loop to unblock, actually helps.

    auto promise = std::make_shared<std::promise<bool>>();
    auto future = promise->get_future();

    std::future_status status = std::future_status::deferred;
    auto question = [&future, &status]() {
        EventLoop::addThread();                                    // thread is added and allows 'answer' to run
        status = future.wait_for(std::chrono::milliseconds(1000)); // wait for 'answer' to unblock here
        EventLoop::removeThread();
    };

    auto answer = [promise]() { promise->set_value(true); };

    // Post question and answer and run them.
    EventLoop::getIOService().post(question);
    EventLoop::getIOService().post(answer);
    EventLoop::run();

    // Check that indeed 'answer' unblocked 'question'
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, status);
    CPPUNIT_ASSERT(future.get());
}