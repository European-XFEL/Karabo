/*
 * InfluxDbClient_Test.hh
 *
 * Created on January, 31, 2023.
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef INFLUXDBCLIENT_TEST_HH
#define INFLUXDBCLIENT_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <boost/thread/thread.hpp>

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
    boost::thread m_eventLoopThread;
};

#endif