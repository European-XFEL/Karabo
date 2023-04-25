/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   ReadAsyncStringUntil_Test.hh
 * Author: giovanet
 *
 * Created on May 16, 2018, 1:46 PM
 */

#ifndef READASYNCSTRINGUNTIL_TEST_HH
#define READASYNCSTRINGUNTIL_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class ReadAsyncStringUntil_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(ReadAsyncStringUntil_Test);
    CPPUNIT_TEST(runTest);
    CPPUNIT_TEST_SUITE_END();

   public:
    ReadAsyncStringUntil_Test();
    virtual ~ReadAsyncStringUntil_Test();
    void setUp();
    void tearDown();

   private:
    void runTest();
};

#endif /* READASYNCSTRINGUNTIL_TEST_HH */
