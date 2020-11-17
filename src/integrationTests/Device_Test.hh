/*
 * File:   Device_Test.hh
 * Author: gero.flucke@xfel.eu
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 *
 */

#ifndef DEVICE_TEST_HH
#define DEVICE_TEST_HH

#include "karabo/core/DeviceServer.hh"
#include "karabo/core/DeviceClient.hh"

#include <boost/thread.hpp>
#include <boost/function.hpp>

#include <cppunit/extensions/HelperMacros.h>

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

    void testGetTimestamp();
    void testSchemaInjection();
    void testGetSet();
    void testUpdateState();
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

    /**
     * Test calling a slot under a node
     */
    void testNodedSlot();

    bool waitForCondition(boost::function<bool() > checker, unsigned int timeoutMs);

    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
};

#endif	/* DEVICE_TEST_HH */

