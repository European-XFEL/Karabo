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
 * File:   Types_Test.hh
 * Author: heisenb
 *
 * Created on February 27, 2013, 3:09 PM
 */

#ifndef TYPES_TEST_HH
#define TYPES_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Types_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Types_Test);
    CPPUNIT_TEST(testIsVector);
    CPPUNIT_TEST(testIsSimple);
    CPPUNIT_TEST(testCategory);
    CPPUNIT_TEST(testFrom);
    CPPUNIT_TEST(testTo);
    CPPUNIT_TEST(testConvert);
    CPPUNIT_TEST_SUITE_END();

   public:
    Types_Test();
    virtual ~Types_Test();
    void setUp();
    void tearDown();

   private:
    void testCategory();
    void testFrom();
    void testTo();
    void testConvert();
    void testIsVector();
    void testIsSimple();
};

#endif /* TYPES_TEST_HH */
