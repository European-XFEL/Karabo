/*
 * HttpClient_Test.cc
 *
 * Unit tests for the HttpClient class.
 * Complements the tests in the integration test GuiServerDevice_Test, which
 * issues a POST request for an HTTP server over a plain-text connection.
 *
 * Created on August, 21, 2023.
 *
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

#ifndef HTTPCLIENT_TEST_HH
#define HTTPCLIENT_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class HttpClient_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(HttpClient_Test);
    CPPUNIT_TEST(testHttpsGet);
    CPPUNIT_TEST(testHttpGet);
    CPPUNIT_TEST_SUITE_END();

   private:
    void testHttpsGet();
    void testHttpGet();
};


#endif /* HTTPCLIENT_TEST_HH */
