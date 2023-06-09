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
 * File:   Version_Test.hh
 * Author: cas
 *
 * Created on February 11, 2016, 2:23 PM
 */

#ifndef VERSION_TEST_HH
#define VERSION_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Version_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Version_Test);
    CPPUNIT_TEST(testVersion);
    CPPUNIT_TEST(testVersionFromString);
    CPPUNIT_TEST(testVersionComparison);
    CPPUNIT_TEST_SUITE_END();

   public:
    Version_Test();
    virtual ~Version_Test();

   private:
    void testVersion();
    void testVersionFromString();
    void testVersionComparison();
};

#endif /* VERSION_TEST_HH */
