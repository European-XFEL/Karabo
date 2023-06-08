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
 * File:   ReadAsyncStringUntil_Test.hh
 * Author: giovanet
 *
 * Created on May 16, 2018, 1:46 PM
 */

#ifndef READASYNCSTRINGUNTIL_TEST_HH
#define READASYNCSTRINGUNTIL_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class ReadAsyncStringUntil_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(ReadAsyncStringUntil_Test);
    CPPUNIT_TEST(runTest);
    CPPUNIT_TEST_SUITE_END();

   public:
    ReadAsyncStringUntil_Test();
    virtual ~ReadAsyncStringUntil_Test();
    void setUp();
    void tearDown();

   private:
    void runTest();
};

#endif /* READASYNCSTRINGUNTIL_TEST_HH */
