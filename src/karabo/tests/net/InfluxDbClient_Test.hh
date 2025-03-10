/*
 * InfluxDbClient_Test.hh
 *
 * Created on January, 31, 2023.
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

#ifndef INFLUXDBCLIENT_TEST_HH
#define INFLUXDBCLIENT_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <thread>

#include "karabo/net/InfluxDbClient.hh"

class InfluxDbClient_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(InfluxDbClient_Test);
    CPPUNIT_TEST(testShowDatabases);
    CPPUNIT_TEST_SUITE_END();

   public:
    void setUp() override;
    void tearDown() override;

   private:
    void testShowDatabases();

    karabo::net::InfluxDbClient::Pointer m_influxClient;
    std::jthread m_eventLoopThread;
};

#endif
