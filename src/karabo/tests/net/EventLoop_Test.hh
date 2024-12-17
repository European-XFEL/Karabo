/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   EventLoop_Test.hh
 * Author: heisenb
 *
 * Created on July 29, 2016, 2:06 PM
 */

#ifndef EVENTLOOP_TEST_HH
#define EVENTLOOP_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <boost/asio.hpp>

class EventLoop_Test : public CPPUNIT_NS::TestFixture {
    boost::mutex m_mutex;
    bool m_finished;

    CPPUNIT_TEST_SUITE(EventLoop_Test);
    CPPUNIT_TEST(testMethod);
    CPPUNIT_TEST(testMethod2);
    CPPUNIT_TEST(testSignalCapture);
    CPPUNIT_TEST(testPost);
    CPPUNIT_TEST(testAddThreadDirectly);
    CPPUNIT_TEST(testExceptionTrace);
    CPPUNIT_TEST_SUITE_END();

   public:
    EventLoop_Test();
    virtual ~EventLoop_Test();

   private:
    void handler1(boost::asio::steady_timer&, int count);
    void handler2();
    void handler3();
    void testMethod();
    void testMethod2();
    void testSignalCapture();
    void testPost();
    void testAddThreadDirectly();
    void testExceptionTrace();
};

#endif /* EVENTLOOP_TEST_HH */
