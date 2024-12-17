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
 * File:   RunTimeSchemaAttributes.cc
 * Author: steffen
 *
 * Created on September 13, 2016, 7:10 PM
 */

#include "RunTimeSchemaAttributes_Test.hh"

using namespace karabo::core;
using namespace karabo::net;
using namespace karabo::util;

using std::placeholders::_1;
using std::placeholders::_2;

CPPUNIT_TEST_SUITE_REGISTRATION(RunTimeSchemaAttributes_Test);

#define KRB_TEST_MAX_TIMEOUT 10


RunTimeSchemaAttributes_Test::RunTimeSchemaAttributes_Test() {}


RunTimeSchemaAttributes_Test::~RunTimeSchemaAttributes_Test() {}


void RunTimeSchemaAttributes_Test::setUp() {
    // Start central event-loop
    m_eventLoopThread = boost::thread(std::bind(&EventLoop::work));
    // Create and start server
    Hash config("serverId", "testServerSchema", "scanPlugins", false, "Logger.priority", "ERROR");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();
    // Create client
    m_deviceClient = std::make_shared<DeviceClient>(std::string(), false);
    m_deviceClient->initialize();
}


void RunTimeSchemaAttributes_Test::tearDown() {
    m_deviceClient.reset();
    m_deviceServer.reset();
    EventLoop::stop();
    m_eventLoopThread.join();
}


void RunTimeSchemaAttributes_Test::appTestRunner() {
    // in order to avoid recurring setup and tear down call all tests are run in a single runner
    std::pair<bool, std::string> success =
          m_deviceClient->instantiate("testServerSchema", "GuiServerDevice",
                                      Hash("deviceId", "testGuiServerSchema", "port", 44447), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(3000));
    m_tcpAdapter =
          std::shared_ptr<karabo::TcpAdapter>(new karabo::TcpAdapter(Hash("port", 44447u /*, "debug", true*/)));
    boost::this_thread::sleep_for(boost::chrono::milliseconds(5000));
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    m_tcpAdapter->login();


    success = m_deviceClient->instantiate("testServerSchema", "AlarmTester", Hash("deviceId", "alarmTesterSchema"),
                                          KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    boost::this_thread::sleep_for(boost::chrono::milliseconds(5000));

    testRuntimeApplication();
    testGuiServerApplication();
    testGuiServerApplicationFailure();

    if (m_tcpAdapter->connected()) {
        m_tcpAdapter->disconnect();
    }
}


void RunTimeSchemaAttributes_Test::testRuntimeApplication() {
    // register a dummy monitor to assure that signals from the device are tracked
    m_deviceClient->registerDeviceMonitor("alarmTesterSchema",
                                          std::bind(&RunTimeSchemaAttributes_Test::dummyMonitor, this, _1, _2));
    boost::this_thread::sleep_for(boost::chrono::milliseconds(5000));
    m_deviceClient->setAttribute("alarmTesterSchema", "intPropNeedsAck", "warnLow", -1000);
    m_deviceClient->setAttribute("alarmTesterSchema", "intPropNeedsAck", "minInc", -10);

    const karabo::util::Schema& s = m_deviceClient->getDeviceSchema("alarmTesterSchema");

    CPPUNIT_ASSERT(s.getWarnLow<int>("intPropNeedsAck") == -1000);
    CPPUNIT_ASSERT(s.getMinInc<int>("intPropNeedsAck") == -10);

    std::clog << std::endl << "Tested application.. Ok" << std::endl;
}


void RunTimeSchemaAttributes_Test::testGuiServerApplication() {
    std::vector<karabo::util::Hash> schemaUpdates;
    schemaUpdates.push_back(Hash("path", "intPropNeedsAck", "attribute", "warnHigh", "value", 1000));
    schemaUpdates.push_back(Hash("path", "intPropNeedsAck", "attribute", "maxInc", "value", 10));

    Hash message("type", "updateAttributes", "instanceId", "alarmTesterSchema", "updates", schemaUpdates);
    karabo::TcpAdapter::QueuePtr messageQ =
          m_tcpAdapter->getNextMessages("attributesUpdated", 1, [&] { m_tcpAdapter->sendMessage(message); });
    Hash lastMessage;
    messageQ->pop(lastMessage);

    CPPUNIT_ASSERT(lastMessage.get<bool>("reply.success"));
    CPPUNIT_ASSERT(lastMessage.get<std::string>("reply.instanceId") == "alarmTesterSchema");
    CPPUNIT_ASSERT(lastMessage.get<std::vector<Hash> >("reply.requestedUpdate") == schemaUpdates);
    const Schema& s = lastMessage.get<Schema>("reply.updatedSchema");
    CPPUNIT_ASSERT(s.getWarnHigh<int>("intPropNeedsAck") == 1000);
    CPPUNIT_ASSERT(s.getMaxInc<int>("intPropNeedsAck") == 10);

    std::clog << "Tested GuiServer application.. Ok" << std::endl;
}


void RunTimeSchemaAttributes_Test::testGuiServerApplicationFailure() {
    // Retrieves the current values of the alarm attributes whose updates should fail.
    Schema currentSchema = m_deviceClient->getDeviceSchema("alarmTesterSchema");
    int currentWarnHigh = currentSchema.getWarnHigh<int>("intPropNeedsAck");
    int currentMaxInc = currentSchema.getMaxInc<int>("intPropNeedsAck");
    int currentAlarmHigh = currentSchema.getAlarmHigh<int>("intPropNeedsAck");

    std::vector<karabo::util::Hash> schemaUpdates;
    schemaUpdates.push_back(Hash("path", "intPropNeedsAck", "attribute", "warnHigh", "value", 50));
    schemaUpdates.push_back(Hash("path", "intPropNeedsAck", "attribute", "maxInc", "value", "this will Fail"));
    schemaUpdates.push_back(Hash("path", "nodeA.floatPropNeedsAck2", "attribute", "maxInc", "value", "this will Fail"));
    schemaUpdates.push_back(Hash("path", "intPropNeedsAck", "attribute", "alarmHigh", "value", 500));

    Hash message("type", "updateAttributes", "instanceId", "alarmTesterSchema", "updates", schemaUpdates);
    karabo::TcpAdapter::QueuePtr messageQ =
          m_tcpAdapter->getNextMessages("attributesUpdated", 1, [&] { m_tcpAdapter->sendMessage(message); });
    Hash lastMessage;
    messageQ->pop(lastMessage);

    CPPUNIT_ASSERT(lastMessage.get<bool>("reply.success") == false);
    CPPUNIT_ASSERT(lastMessage.get<std::string>("reply.instanceId") == "alarmTesterSchema");
    CPPUNIT_ASSERT(lastMessage.get<std::vector<Hash> >("reply.requestedUpdate") == schemaUpdates);
    const Schema& s = lastMessage.get<Schema>("reply.updatedSchema");

    // All the updates should have been rolled-back due to the failing 'maxInc' updates.
    CPPUNIT_ASSERT_EQUAL(currentWarnHigh, s.getWarnHigh<int>("intPropNeedsAck"));
    CPPUNIT_ASSERT_EQUAL(currentMaxInc, s.getMaxInc<int>("intPropNeedsAck"));
    CPPUNIT_ASSERT_EQUAL(currentAlarmHigh, s.getAlarmHigh<int>("intPropNeedsAck"));

    std::clog << "Tested GuiServer application failure.. Ok" << std::endl;
}


void RunTimeSchemaAttributes_Test::dummyMonitor(const std::string&, const karabo::util::Hash&) {

};
