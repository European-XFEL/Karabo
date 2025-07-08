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
 * File:   SceneProvider_Test.cc
 * Author: steffen.hauf@xfel.eu

 */

#include "SceneProvider_Test.hh"

#include <cstdlib>
#include <karabo/net/EventLoop.hh>


using namespace std;

#define KRB_TEST_MAX_TIMEOUT 10

USING_KARABO_NAMESPACES


CPPUNIT_TEST_SUITE_REGISTRATION(SceneProvider_Test);


SceneProvider_Test::SceneProvider_Test() {}


SceneProvider_Test::~SceneProvider_Test() {}


void SceneProvider_Test::setUp() {
    // uncomment this if ever testing against a local broker
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);
    // Start central event-loop
    m_eventLoopThread = std::jthread([](std::stop_token stoken) { karabo::net::EventLoop::work(); });
    // Create and start server
    Hash config("serverId", "testServerSceneProvider", "log.level", "FATAL");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();
    // Create client
    m_deviceClient = std::make_shared<DeviceClient>(std::string(), false);
    m_deviceClient->initialize();
}


void SceneProvider_Test::tearDown() {
    m_deviceClient.reset();
    m_deviceServer.reset();
    EventLoop::stop();
}


void SceneProvider_Test::appTestRunner() {
    std::pair<bool, std::string> success;
    success = m_deviceClient->instantiate("testServerSceneProvider", "SceneProviderTestDevice",
                                          Hash("deviceId", "sceneProvider"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    success = m_deviceClient->instantiate("testServerSceneProvider", "NonSceneProviderTestDevice",
                                          Hash("deviceId", "noSceneProvider"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    testInstanceInfo();
}


void SceneProvider_Test::testInstanceInfo() {
    // Tests if the instance info correctly reports scene availability
    const Hash& topo = m_deviceClient->getSystemTopology();
    CPPUNIT_ASSERT(topo.has("device"));
    const Hash& device = topo.get<Hash>("device");
    CPPUNIT_ASSERT(device.hasAttribute("sceneProvider", "capabilities"));
    CPPUNIT_ASSERT(
          (device.getAttribute<unsigned int>("sceneProvider", "capabilities") & karabo::core::PROVIDES_SCENES) == 1u);
    CPPUNIT_ASSERT(device.hasAttribute("noSceneProvider", "capabilities"));
    CPPUNIT_ASSERT(
          (device.getAttribute<unsigned int>("noSceneProvider", "capabilities") & karabo::core::PROVIDES_SCENES) == 0u);
    std::clog << "Tested scene providers identified in instanceInfo.. Ok" << std::endl;
}
