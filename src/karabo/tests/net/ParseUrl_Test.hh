/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   ParseUrl_Test.hh
 * Author: parenti
 *
 * Created on January  7, 2022,  4:00 PM
 */

#ifndef PARSEURL_TEST_HH
#define PARSEURL_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class ParseUrl_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(ParseUrl_Test);
    CPPUNIT_TEST(testGenericParse);
    CPPUNIT_TEST(testHttpParse);
    CPPUNIT_TEST_SUITE_END();

   public:
    ParseUrl_Test();
    virtual ~ParseUrl_Test();
    void setUp();
    void tearDown();

   private:
    void testGenericParse();

    void testHttpParse();
};

#endif /* PARSEURL_TEST_HH */
