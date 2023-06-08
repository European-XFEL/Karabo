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
