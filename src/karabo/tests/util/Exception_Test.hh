/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Exception_Test.hh
 * Author: heisenb
 *
 * Created on September 29, 2016, 5:28 PM
 */

#ifndef EXCEPTION_TEST_HH
#define EXCEPTION_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Exception_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Exception_Test);
    CPPUNIT_TEST(testMethod);
    CPPUNIT_TEST(testDetails);
    CPPUNIT_TEST(testTraceOrder);
    // CPPUNIT_TEST(testThreadSafeTrace); needs EventLoop, so tested as part of EventLoop_Test
    CPPUNIT_TEST_SUITE_END();

   public:
    Exception_Test();
    virtual ~Exception_Test();

   private:
    void testMethod();
    void testDetails();
    void testTraceOrder();
};

#endif /* EXCEPTION_TEST_HH */
