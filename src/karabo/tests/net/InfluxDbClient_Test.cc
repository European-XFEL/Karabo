/*
 * InfluxDbClient_Test.cc
 *
 * Created on January, 31, 2023.
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "InfluxDbClient_Test.hh"

#include <memory>
#include <string>

#include "karabo/net/EventLoop.hh"
#include "karabo/net/HttpResponse.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/StringTools.hh"

using karabo::net::EventLoop;
using karabo::net::HttpResponse;
using karabo::net::InfluxDbClient;
using karabo::util::Configurator;
using karabo::util::Hash;
using karabo::util::toString;

using std::string;

CPPUNIT_TEST_SUITE_REGISTRATION(InfluxDbClient_Test);

void InfluxDbClient_Test::setUp() {
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));

    // Instantiates an InfluxDbClient
    string influxUrlRead;
    if (getenv("KARABO_INFLUXDB_QUERY_URL")) {
        influxUrlRead = getenv("KARABO_INFLUXDB_QUERY_URL");
    } else {
        influxUrlRead = "tcp://localhost:8086";
    }

    const char* envDbName = getenv("KARABO_INFLUXDB_DBNAME");
    string dbName(envDbName ? envDbName : ""); // without environment variable, use empty string

    string dbUser;
    if (getenv("KARABO_INFLUXDB_QUERY_USER")) {
        dbUser = getenv("KARABO_INFLUXDB_QUERY_USER");
    } else {
        dbUser = "infadm";
    }
    string dbPassword;
    if (getenv("KARABO_INFLUXDB_QUERY_PASSWORD")) {
        dbPassword = getenv("KARABO_INFLUXDB_QUERY_PASSWORD");
    } else {
        dbPassword = "admpasswd";
    }

    Hash dbClientCfg;
    dbClientCfg.set("dbname", dbName);
    dbClientCfg.set("url", influxUrlRead);
    dbClientCfg.set("durationUnit", "u");
    dbClientCfg.set("dbUser", dbUser);
    dbClientCfg.set("dbPassword", dbPassword);

    m_influxClient = Configurator<InfluxDbClient>::create("InfluxDbClient", dbClientCfg);
}


void InfluxDbClient_Test::tearDown() {
    m_influxClient.reset();

    EventLoop::stop();
    if (m_eventLoopThread.joinable()) {
        bool joined = m_eventLoopThread.try_join_for(boost::chrono::seconds(10));
        if (!joined) {
            std::clog << "\nWARNING: Event loop thread join timed out.\n" << std::endl;
            std::clog << "Thread(s) in the event loop: " << EventLoop::getNumberOfThreads() << std::endl;
        }
    }
}

void InfluxDbClient_Test::testShowDatabases() {
    auto prom = std::make_shared<std::promise<HttpResponse>>();
    std::future<HttpResponse> fut = prom->get_future();
    m_influxClient->queryDb("SHOW DATABASES", [prom](const HttpResponse& resp) { prom->set_value(resp); });
    auto status = fut.wait_for(std::chrono::milliseconds(3500));
    if (status != std::future_status::ready) {
        CPPUNIT_ASSERT_MESSAGE("Timed out waiting for reply of SHOW DATABASES query", false);
    }
    auto resp = fut.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("SHOW DATABASES failed", 200, resp.code);
    CPPUNIT_ASSERT_MESSAGE("SHOW MESSAGE returned an empty response.", resp.payload.length() > 0);
}