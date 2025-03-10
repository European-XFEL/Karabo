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
 * File:   LockTest_Test.hh
 * Author: steffen
 *
 * Created on Oct 2, 2016, 1:21:16 PM
 */

#ifndef LOCKTEST_TEST_HH
#define LOCKTEST_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <memory>
#include <thread>

#include "LockTestDevice.hh"
#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/karabo.hpp"


class LockTest_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(LockTest_Test);

    CPPUNIT_TEST(appTestRunner);

    CPPUNIT_TEST_SUITE_END();

   public:
    LockTest_Test();
    virtual ~LockTest_Test();
    void setUp();
    void tearDown();

   private:
    void appTestRunner();
    void testLocking();
    void testUnlocking();
    void testRecursiveLocking();
    void testSettingOnLocked();
    void testLockStealing();

    void waitUntilLockClears(const std::string& deviceId);

    karabo::core::DeviceServer::Pointer m_deviceServer;
    std::jthread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
};

#endif /* LOCKTEST_TEST_HH */
