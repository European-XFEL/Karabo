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
 * File:   StringTools_Test.hh
 * Author: parenti
 *
 * Created on December 15, 2014, 12:16 PM
 */

#ifndef STRINGTOOLS_TEST_HH
#define STRINGTOOLS_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class StringTools_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(StringTools_Test);
    CPPUNIT_TEST(testFromString);
    CPPUNIT_TEST(testToString);
    CPPUNIT_TEST(testWiden);
    CPPUNIT_TEST(testTokenize);
    CPPUNIT_TEST_SUITE_END();

   public:
    StringTools_Test();
    virtual ~StringTools_Test();
    void setUp();
    void tearDown();

   private:
    void testFromString();
    void testToString();
    void testWiden();
    void testTokenize();
};

#endif /* STRINGTOOLS_TEST_HH */
