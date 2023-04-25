/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   NDArray_Test.hh
 * Author: parenti
 *
 * Created on May 22, 2015, 11:41:21 AM
 */

#ifndef NDARRAY_TEST_HH
#define NDARRAY_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/util.hpp>

class NDArray_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(NDArray_Test);
    CPPUNIT_TEST(testConstructor);
    CPPUNIT_TEST(testShapeException);
    CPPUNIT_TEST(testDataTypeException);
    CPPUNIT_TEST_SUITE_END();

   public:
    NDArray_Test();
    virtual ~NDArray_Test();
    void setUp();
    void tearDown();

   private:
    void testConstructor();
    void testShapeException();
    void testDataTypeException();
};

#endif /* NDARRAY_TEST_HH */
