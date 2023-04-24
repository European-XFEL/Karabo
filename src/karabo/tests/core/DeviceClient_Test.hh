/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   DeviceClient_Test.hh
 * Author: flucke
 *
 * Created on August 24, 2017, 9:49 AM
 */

#ifndef DEVICECLIENT_TEST_HH
#define DEVICECLIENT_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"

class DeviceClient_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(DeviceClient_Test);
    CPPUNIT_TEST(testAll);
    CPPUNIT_TEST_SUITE_END();

   public:
    DeviceClient_Test();
    virtual ~DeviceClient_Test();
    void setUp();
    void tearDown();

   private:
    void testAll();

    /**
     * Checks that concurrent calls to DeviceClient::initTopology()
     * have the expected behavior - the broadcast of slotPing
     * followed by a sleep of almost 2 seconds done from
     * SignalSlotable::getAvailableInstances only happens once.
     * The way to assert the expected behavior is by confirming
     * that the calls take about the same time and obtain the
     * same results.
     *
     * NOTE: This test uses the DeviceClient::getDevices() function
     * to trigger the DeviceClient::initTopology() calls.
     */
    void testConcurrentInitTopology();

    void testGet();
    void testSet();
    void testGetSchema();
    void testGetSchemaNoWait();
    void testMonitorChannel();
    void testCurrentlyExecutableCommands();
    void testSlotsWithArgs();
    void testConnectionHandling();

    karabo::core::DeviceServer::Pointer m_deviceServer;
    karabo::core::DeviceClient::Pointer m_deviceClient;
};

#endif /* DEVICECLIENT_TEST_HH */
