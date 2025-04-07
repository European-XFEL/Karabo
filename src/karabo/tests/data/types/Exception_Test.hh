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
 * File:   Exception_Test.hh
 * Author: heisenb
 *
 * Created on September 29, 2016, 5:28 PM
 */

#ifndef EXCEPTION_TEST_HH
#define EXCEPTION_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Exception_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Exception_Test);
    CPPUNIT_TEST(testMethod);
    CPPUNIT_TEST(testDetails);
    CPPUNIT_TEST(testTraceOrder);
    // CPPUNIT_TEST(testThreadSafeTrace); needs EventLoop, so tested as part of EventLoop_Test
    CPPUNIT_TEST_SUITE_END();

   public:
    Exception_Test();
    virtual ~Exception_Test();

   private:
    void testMethod();
    void testDetails();
    void testTraceOrder();
};

#endif /* EXCEPTION_TEST_HH */
