/* 
 * File:   EventLoop_Test.cc
 * Author: heisenb
 * 
 * Created on July 29, 2016, 2:06 PM
 */

#include "karabo/net/EventLoop.hh"
#include "karabo/log/Logger.hh"
#include "EventLoop_Test.hh"


using namespace karabo::util;
using namespace karabo::net;

CPPUNIT_TEST_SUITE_REGISTRATION(EventLoop_Test);


EventLoop_Test::EventLoop_Test() {
}


EventLoop_Test::~EventLoop_Test() {   
}


void EventLoop_Test::handler1(boost::asio::deadline_timer& timer, int count) {

    if (count == -1) {
        CPPUNIT_ASSERT(EventLoop::getNumberOfThreads() == 0);
        return;
    }

    CPPUNIT_ASSERT(EventLoop::getNumberOfThreads() == count);

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