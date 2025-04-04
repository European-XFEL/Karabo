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
 * File:   NetworkInterface_Test.hh
 * Author: <jose.vazquez@xfel.eu>
 *
 */

#ifndef NETWORK_INTERFACE_TEST_HH
#define NETWORK_INTERFACE_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class NetworkInterface_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(NetworkInterface_Test);

    CPPUNIT_TEST(testRightInAddrT);
    CPPUNIT_TEST(testConstructor);

    CPPUNIT_TEST_SUITE_END();

   public:
    NetworkInterface_Test();
    ~NetworkInterface_Test() override;

    void setUp() override;
    void tearDown() override;

   private:
    void testRightInAddrT();
    void testConstructor();
};


#endif
