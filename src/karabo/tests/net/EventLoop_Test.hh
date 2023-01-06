/*
 * File:   EventLoop_Test.hh
 * Author: heisenb
 *
 * Created on July 29, 2016, 2:06 PM
 */

#ifndef EVENTLOOP_TEST_HH
#define EVENTLOOP_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class EventLoop_Test : public CPPUNIT_NS::TestFixture {
    boost::mutex m_mutex;
    bool m_finished;

    CPPUNIT_TEST_SUITE(EventLoop_Test);
    CPPUNIT_TEST(testMethod);
    CPPUNIT_TEST(testMethod2);
    CPPUNIT_TEST(testSignalCapture);
    CPPUNIT_TEST(testAddThreadDirectly);
    CPPUNIT_TEST_SUITE_END();

   public:
    EventLoop_Test();
    virtual ~EventLoop_Test();

   private:
    void handler1(boost::asio::deadline_timer&, int count);
    void handler2();
    void handler3();
    void testMethod();
    void testMethod2();
    void testSignalCapture();
    void testAddThreadDirectly();
};

#endif /* EVENTLOOP_TEST_HH */
