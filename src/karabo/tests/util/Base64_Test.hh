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
