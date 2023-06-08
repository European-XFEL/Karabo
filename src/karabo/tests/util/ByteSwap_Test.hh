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
 * File:   ByteSwap_Test.hh
 * Author: parenti
 *
 * Created on October 4, 2013, 3:51 PM
 */

#ifndef BYTESWAP_TEST_HH
#define BYTESWAP_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class ByteSwap_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(ByteSwap_Test);
    CPPUNIT_TEST(test16);
    CPPUNIT_TEST(test32);
    CPPUNIT_TEST(test64);
    CPPUNIT_TEST_SUITE_END();

   public:
    ByteSwap_Test();
    virtual ~ByteSwap_Test();
    void setUp();
    void tearDown();

   private:
    void test16();
    void test32();
    void test64();
};

#endif /* BYTESWAP_TEST_HH */
