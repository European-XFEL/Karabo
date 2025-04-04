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
 * File:   NDArray_Test.hh
 * Author: parenti
 *
 * Created on May 22, 2015, 11:41:21 AM
 */

#ifndef NDARRAY_TEST_HH
#define NDARRAY_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/data/types/NDArray.hh>

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
