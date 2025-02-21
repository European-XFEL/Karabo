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
 * File:   LockTest_Test.cc
 * Author: steffen
 *
 * Created on Oct 2, 2016, 1:21:17 PM
 */
#include "LockTest_Test.hh"

#include <chrono>
#include <karabo/net/EventLoop.hh>
#include <thread>

using namespace std::literals::chrono_literals;
using namespace std;

#define KRB_TEST_MAX_TIMEOUT 10

USING_KARABO_NAMESPACES

CPPUNIT_TEST_SUITE_REGISTRATION(LockTest_Test);


LockTest_Test::LockTest_Test() {}


LockTest_Test::~LockTest_Test() {}


void LockTest_Test::setUp() {
    // Start central event-loop
    m_eventLoopThread = boost::thread(std::bind(&EventLoop::work));
    // Create and start server
    Hash config("serverId", "testServerLock", "scanPlugins", false, "Logger.priority", "FATAL");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();
    // Create client
    m_deviceClient = std::make_shared<DeviceClient>(std::string(), false);
    m_deviceClient->initialize();
}


void LockTest_Test::tearDown() {
    m_deviceClient.reset();
    m_deviceServer.reset();
    EventLoop::stop();
    m_eventLoopThread.join();
}


void LockTest_Test::appTestRunner() {
    // in order to avoid recurring setup and tear down call all tests are run in a single runner
    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          "testServerLock", "LockTestDevice", Hash("deviceId", "lockTest3"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    success = m_deviceClient->instantiate("testServerLock", "LockTestDevice",
                                          Hash("deviceId", "lockTest1", "controlledDevice", "lockTest3"),
                                          KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    success = m_deviceClient->instantiate("testServerLock", "LockTestDevice",
                                          Hash("deviceId", "lockTest2", "controlledDevice", "lockTest3"),
                                          KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    testLocking();
    testUnlocking();
    testRecursiveLocking();
    testSettingOnLocked();
    testLockStealing();
}


void LockTest_Test::testLocking() {
    // This will lock "lockTest3" and work on it for 1s (asynchronously)
    m_deviceClient->executeNoWait("lockTest1", "lockAndWait");
    // We are waiting here to give the machinery time to really lock "lockTest3"
    std::this_thread::sleep_for(100ms);

    // This should fail, as lockAndWait is configured to throw once we are trying to also lock "lockTest3"
    CPPUNIT_ASSERT_THROW(m_deviceClient->execute("lockTest2", "lockAndWait", KRB_TEST_MAX_TIMEOUT), Exception);
    Exception::clearTrace();

    std::clog << "Tested locking.. Ok" << std::endl;

    CPPUNIT_ASSERT_NO_THROW(waitUntilLockClears("lockTest3"));

    m_deviceClient->executeNoWait("lockTest1", "lockAndWaitLong");
    // We are waiting here to give the machinery time to really lock "lockTest3"
    std::this_thread::sleep_for(100ms);

    CPPUNIT_ASSERT_THROW(m_deviceClient->execute("lockTest2", "lockAndWaitTimeout", 10), Exception);
    Exception::clearTrace();
    std::clog << "Tested locking with timeout (fail).. Ok" << std::endl;

    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("lockTest3", "slotClearLock"));

    CPPUNIT_ASSERT_NO_THROW(waitUntilLockClears("lockTest3"));

    m_deviceClient->executeNoWait("lockTest1", "lockAndWait");

    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("lockTest2", "lockAndWaitTimeout", 10));
    std::clog << "Tested locking with timeout (success).. Ok" << std::endl;
}


void LockTest_Test::testUnlocking() {
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("lockTest3", "slotClearLock"));

    // FIXME: should not be needed anymore with ordering guarantee:
    // CPPUNIT_ASSERT_NO_THROW(waitUntilLockClears("lockTest3"));

    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("lockTest2", "lockAndWait", KRB_TEST_MAX_TIMEOUT));
    std::clog << "Tested unlocking.. Ok" << std::endl;
}


void LockTest_Test::testRecursiveLocking() {
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("lockTest3", "slotClearLock"));
    CPPUNIT_ASSERT_THROW(m_deviceClient->execute("lockTest1", "lockAndWaitRecursiveFail", KRB_TEST_MAX_TIMEOUT),
                         Exception);
    Exception::clearTrace();

    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("lockTest3", "slotClearLock"));
    // recursive succeeds
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("lockTest1", "lockAndWaitRecursive", KRB_TEST_MAX_TIMEOUT));
    std::clog << "Tested recursive locking.. Ok" << std::endl;
}


void LockTest_Test::testSettingOnLocked() {
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("lockTest3", "slotClearLock"));
    m_deviceClient->executeNoWait("lockTest1", "lockAndWait");
    // We are waiting here to give the machinery time to really lock "lockTest3"
    std::this_thread::sleep_for(100ms);
    CPPUNIT_ASSERT_THROW(m_deviceClient->set("lockTest3", "intProperty", 100), Exception);
    Exception::clearTrace();

    const int value = m_deviceClient->get<int>("lockTest3", "intProperty");
    CPPUNIT_ASSERT(value <= 5);
    std::clog << "Tested rejection of setting on locked.. Ok" << std::endl;
}


void LockTest_Test::testLockStealing() {
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("lockTest3", "slotClearLock"));

    m_deviceClient->executeNoWait("lockTest1", "lockAndWait");
    // We are waiting here to give the machinery time to really lock "lockTest3"
    std::this_thread::sleep_for(100ms);

    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("lockTest3", "slotClearLock"));

    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set("lockTest3", "intProperty", 100));

    CPPUNIT_ASSERT_EQUAL(100, m_deviceClient->get<int>("lockTest3", "intProperty"));
    std::clog << "Tested stolen lock exception.. Ok" << std::endl;
}


void LockTest_Test::waitUntilLockClears(const std::string& deviceId) {
    unsigned int counter = 1000; // Do not wait forever...
    while (m_deviceClient->get<std::string>(deviceId, "lockedBy") != "" && counter-- > 0) {
        std::this_thread::sleep_for(5ms);
    };
    if (0 == counter) {
        throw KARABO_TIMEOUT_EXCEPTION("Lock on '" + deviceId + "' did not clear.");
    }
}
