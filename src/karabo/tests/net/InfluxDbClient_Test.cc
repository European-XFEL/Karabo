/*
 * InfluxDbClient_Test.cc
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

#include "InfluxDbClient_Test.hh"

#include <memory>
#include <string>

#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/StringTools.hh"
#include "karabo/log/Logger.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/HttpResponse.hh"
#include "karabo/net/InfluxDbClientUtils.hh"

using karabo::data::Configurator;
using karabo::data::Hash;
using karabo::data::toString;
using karabo::log::Logger;
using karabo::net::EventLoop;
using karabo::net::HttpResponse;
using karabo::net::InfluxDbClient;

using std::string;

CPPUNIT_TEST_SUITE_REGISTRATION(InfluxDbClient_Test);

void InfluxDbClient_Test::setUp() {
    // Output messages logged during the test to the test output.
    Hash config("priority", "INFO");
    Logger::configure(config);
    Logger::useOstream();

    m_eventLoopThread = std::jthread([](std::stop_token stoken) { EventLoop::work(); });

    m_influxClient = karabo::net::buildInfluxReadClient();
}


void InfluxDbClient_Test::tearDown() {
    m_influxClient.reset();

    EventLoop::stop();
    if (m_eventLoopThread.joinable()) {
        m_eventLoopThread.join();
        std::clog << "Thread(s) in the event loop: " << EventLoop::getNumberOfThreads() << std::endl;
    }
}

void InfluxDbClient_Test::testShowDatabases() {
    std::clog << "Testing InfluxDbClient execution of SHOW DATABASES ..." << std::endl;
    auto prom = std::make_shared<std::promise<HttpResponse>>();
    std::future<HttpResponse> fut = prom->get_future();
    m_influxClient->queryDb("SHOW DATABASES", [prom](const HttpResponse& resp) { prom->set_value(resp); });
    auto status = fut.wait_for(std::chrono::milliseconds(3500));
    if (status != std::future_status::ready) {
        CPPUNIT_ASSERT_MESSAGE("Timed out waiting for reply of SHOW DATABASES query", false);
    } else {
        std::clog << " ... command submitted to Influx '" << m_influxClient->influxVersion() << "' at '"
                  << m_influxClient->serverUrl() << "'." << std::endl;
    }
    auto resp = fut.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE("SHOW DATABASES failed", 200, resp.code);
    CPPUNIT_ASSERT_MESSAGE("SHOW MESSAGE returned an empty response.", resp.payload.length() > 0);
    std::clog << "OK" << std::endl;
}
