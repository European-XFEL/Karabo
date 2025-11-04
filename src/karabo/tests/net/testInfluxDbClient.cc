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

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <thread>

#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/StringTools.hh"
#include "karabo/log/Logger.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/HttpResponse.hh"
#include "karabo/net/InfluxDbClient.hh"
#include "karabo/net/InfluxDbClientUtils.hh"


class TestInfluxDbClient : public ::testing::Test {
   protected:
    TestInfluxDbClient() : m_eventLoopThread() {}
    ~TestInfluxDbClient() override {}

    void SetUp() override;
    void TearDown() override;

    std::jthread m_eventLoopThread;
};


using karabo::data::Configurator;
using karabo::data::Hash;
using karabo::data::toString;
using karabo::log::Logger;
using karabo::net::EventLoop;
using karabo::net::HttpResponse;
using karabo::net::InfluxDbClient;

using std::string;


void TestInfluxDbClient::SetUp() {
    // Output messages logged during the test to the test output.
    Hash config("level", "INFO");
    Logger::configure(config);
    Logger::useConsole();

    m_eventLoopThread = std::jthread([](std::stop_token stoken) { EventLoop::work(); });
}


void TestInfluxDbClient::TearDown() {
    EventLoop::stop();
    if (m_eventLoopThread.joinable()) {
        m_eventLoopThread.join();
        std::clog << "Thread(s) in the event loop: " << EventLoop::getNumberOfThreads() << std::endl;
    }
}


TEST_F(TestInfluxDbClient, testShowDatabases) {
    auto influxClient = karabo::net::buildInfluxReadClient();
    std::clog << "Testing InfluxDbClient execution of SHOW DATABASES ..." << std::endl;
    auto prom = std::make_shared<std::promise<HttpResponse>>();
    std::future<HttpResponse> fut = prom->get_future();
    influxClient->queryDb("SHOW DATABASES", [prom](const HttpResponse& resp) { prom->set_value(resp); });
    auto status = fut.wait_for(std::chrono::milliseconds(3500));
    if (status != std::future_status::ready) {
        ASSERT_TRUE(false) << "SHOW MESSAGE returned an empty response.";
    } else {
        std::clog << " ... command submitted to Influx '" << influxClient->influxVersion() << "' at '"
                  << influxClient->serverUrl() << "'." << std::endl;
    }
    auto resp = fut.get();
    ASSERT_EQ(200, resp.code) << "SHOW DATABASES failed";
    ASSERT_TRUE(resp.payload.length() > 0) << "SHOW MESSAGE returned an empty response.";
    // std::clog << "OK" << std::endl;
}
