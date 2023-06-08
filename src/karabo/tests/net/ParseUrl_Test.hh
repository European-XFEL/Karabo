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
 * File:   ParseUrl_Test.hh
 * Author: parenti
 *
 * Created on January  7, 2022,  4:00 PM
 */

#ifndef PARSEURL_TEST_HH
#define PARSEURL_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class ParseUrl_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(ParseUrl_Test);
    CPPUNIT_TEST(testGenericParse);
    CPPUNIT_TEST(testHttpParse);
    CPPUNIT_TEST_SUITE_END();

   public:
    ParseUrl_Test();
    virtual ~ParseUrl_Test();
    void setUp();
    void tearDown();

   private:
    void testGenericParse();

    void testHttpParse();
};

#endif /* PARSEURL_TEST_HH */
