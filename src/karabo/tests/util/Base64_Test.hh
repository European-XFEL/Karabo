/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Base64_Test.hh
 * Author: parenti
 *
 * Created on October 2, 2013, 4:18 PM
 */

#ifndef BASE64_TEST_HH
#define BASE64_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Base64_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Base64_Test);
    CPPUNIT_TEST(testEncode);
    CPPUNIT_TEST(testDecode);
    CPPUNIT_TEST_SUITE_END();

   public:
    Base64_Test();
    virtual ~Base64_Test();
    void setUp();
    void tearDown();

   private:
    void testEncode();
    void testDecode();
};

#endif /* BASE64_TEST_HH */
