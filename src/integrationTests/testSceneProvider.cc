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
 * File:   testSceneProvider.cc
 * Author: steffen.hauf@xfel.eu

 */

#include <gtest/gtest.h>

#include <cstdlib>
#include <karabo/net/EventLoop.hh>
#include <memory>

#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/karabo.hpp"


class TestSceneProvider : public ::testing::Test {
   protected:
    TestSceneProvider();
    ~TestSceneProvider() override;
    void SetUp() override;
    void TearDown() override;

    void testInstanceInfo();

    karabo::core::DeviceServer::Pointer m_deviceServer;
    karabo::core::DeviceClient::Pointer m_deviceClient;
    std::jthread m_eventLoopThread;
};


#define KRB_TEST_MAX_TIMEOUT 10

USING_KARABO_NAMESPACES
using namespace std;


TestSceneProvider::TestSceneProvider() {}


TestSceneProvider::~TestSceneProvider() {}


void TestSceneProvider::SetUp() {
    // uncomment this if ever testing against a local broker
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);
    // Start central event-loop
    m_eventLoopThread = std::jthread([](std::stop_token stoken) { EventLoop::work(); });
    // Create and start server
    Hash config("serverId", "testServerSceneProvider", "log.level", "FATAL");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();
    // Create client
    m_deviceClient = std::make_shared<DeviceClient>(std::string(), false);
    m_deviceClient->initialize();
}


void TestSceneProvider::TearDown() {
    m_deviceClient.reset();
    m_deviceServer.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EventLoop::stop();
    if (m_eventLoopThread.joinable()) m_eventLoopThread.join();
}


TEST_F(TestSceneProvider, appTestRunner) {
    std::pair<bool, std::string> success;
    success = m_deviceClient->instantiate("testServerSceneProvider", "SceneProviderTestDevice",
                                          Hash("deviceId", "sceneProvider"), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    success = m_deviceClient->instantiate("testServerSceneProvider", "NonSceneProviderTestDevice",
                                          Hash("deviceId", "noSceneProvider"), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    testInstanceInfo();
}


void TestSceneProvider::testInstanceInfo() {
    // Tests if the instance info correctly reports scene availability
    const Hash& topo = m_deviceClient->getSystemTopology();
    ASSERT_TRUE(topo.has("device"));
    const Hash& device = topo.get<Hash>("device");
    ASSERT_TRUE(device.hasAttribute("sceneProvider", "capabilities"));
    ASSERT_TRUE((device.getAttribute<unsigned int>("sceneProvider", "capabilities") & PROVIDES_SCENES) == 1u);
    ASSERT_TRUE(device.hasAttribute("noSceneProvider", "capabilities"));
    ASSERT_TRUE((device.getAttribute<unsigned int>("noSceneProvider", "capabilities") & PROVIDES_SCENES) == 0u);
    std::clog << "Tested scene providers identified in instanceInfo.. Ok" << std::endl;
}
