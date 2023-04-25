/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Dims_Test.hh
 * Author: wrona
 *
 * Created on February 19, 2013, 1:33 PM
 */

#ifndef DIMS_TEST_HH
#define DIMS_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Dims_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Dims_Test);
    CPPUNIT_TEST(testDims);
    CPPUNIT_TEST(testArrayTools);
    CPPUNIT_TEST_SUITE_END();

   public:
    Dims_Test();
    virtual ~Dims_Test();
    void setUp();
    void tearDown();

   private:
    void testDims();
    void testArrayTools();
};

#endif /* DIMS_TEST_HH */
