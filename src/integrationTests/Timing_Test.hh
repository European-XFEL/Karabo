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
 * File:   Timing_Test.hh
 * Author: steffen.hauf@xfel.eu

 */

#ifndef TIMING_TEST_HH
#define TIMING_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <memory>

#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/data/time/Epochstamp.hh"
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
    std::jthread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
};

#endif /* TIMING_TEST_HH */
