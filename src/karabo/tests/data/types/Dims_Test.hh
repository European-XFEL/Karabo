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
