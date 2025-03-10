/*
 * File:   Device_Test.hh
 * Author: gero.flucke@xfel.eu
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
 *
 */

#ifndef DEVICE_TEST_HH
#define DEVICE_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <functional>
#include <thread>

#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"

class Device_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Device_Test);

    CPPUNIT_TEST(appTestRunner);

    CPPUNIT_TEST_SUITE_END();

   public:
    Device_Test();
    virtual ~Device_Test();
    void setUp();
    void tearDown();

   private:
    void appTestRunner();

    void testInstanceInfoServer();
    void testGetTimestampSystemInfo();
    void testSchemaInjection();
    void testGetconfigReconfig();
    void testUpdateState();
    void testSet();
    void testSetVectorUpdate();
    void testSignal();
    void testBadInit();

    /** Tests that updateSchema resets attributes in the static schema. */
    void testSchemaWithAttrUpdate();

    /** Tests that appendSchema preserves attributes in the static schema. */
    void testSchemaWithAttrAppend();

    /** Tests that updateSchema/appendSchema work well for tags, also inside schema of OutputChannel
     *
     * @param updateSlot which TestDevice slot to change the schema: "slotUpdateSchema" or "slotAppendSchema"
     */
    void testChangeSchemaOutputChannel(const std::string& updateSlot);

    /** Tests that updateSchema/appendSchema that change schema of output channel will trigger a reconnection
     *
     * @param updateSlot which TestDevice slot to change the schema: "slotUpdateSchema" or "slotAppendSchema"
     */
    void testOutputRecreatesOnSchemaChange(const std::string& updateSlot);

    /** Test that updateSchema/appendSchema properly creates (and destroys) input/output channels
     *
     * @param updateSlot which TestDevice slot to change the schema: "slotUpdateSchema" or "slotAppendSchema"
     */
    void testInputOutputChannelInjection(const std::string& updateSlot);

    /**
     * Test calling a slot under a node
     */
    void testNodedSlot();

    bool waitForCondition(std::function<bool()> checker, unsigned int timeoutMs);

    karabo::core::DeviceServer::Pointer m_deviceServer;
    std::jthread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
};

#endif /* DEVICE_TEST_HH */
