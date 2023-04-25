/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   BufferSet_Test.hh
 * Author: flucke
 *
 * Created on April 12, 2019, 1:34 PM
 */

#ifndef BUFFERSET_TEST_HH
#define BUFFERSET_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class BufferSet_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(BufferSet_Test);
    CPPUNIT_TEST(testEmplaceAppend);
    CPPUNIT_TEST_SUITE_END();

   public:
    BufferSet_Test();
    virtual ~BufferSet_Test();
    void setUp();
    void tearDown();

   private:
    void testEmplaceAppend();
};

#endif /* BUFFERSET_TEST_HH */
