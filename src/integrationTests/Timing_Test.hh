/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Timing_Test.hh
 * Author: steffen.hauf@xfel.eu

 */

#ifndef TIMING_TEST_HH
#define TIMING_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <boost/shared_ptr.hpp>
#include <karabo/util/Epochstamp.hh>

#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/karabo.hpp"


class Timing_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Timing_Test);

    CPPUNIT_TEST(testWrongPeriod);
    CPPUNIT_TEST(testIdReset);

    CPPUNIT_TEST_SUITE_END();

   public:
    Timing_Test();
    virtual ~Timing_Test();
    void setUp();
    void tearDown();

   private:
    void testWrongPeriod();
    void testIdReset();

    karabo::core::DeviceServer::Pointer m_deviceServer;
    karabo::core::DeviceServer::Pointer m_deviceServer2;
    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
};

#endif /* TIMING_TEST_HH */
