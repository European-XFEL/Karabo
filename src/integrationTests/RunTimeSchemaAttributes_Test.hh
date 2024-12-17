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
 * File:   RunTimeSchemaAttributes.hh
 * Author: steffen
 *
 * Created on September 13, 2016, 7:11 PM
 */

#ifndef RUNTIMESCHEMAATTRIBUTES_HH
#define RUNTIMESCHEMAATTRIBUTES_HH

#include <cppunit/extensions/HelperMacros.h>

#include <boost/shared_ptr.hpp>

#include "AlarmTesterDevice.hh"
#include "TcpAdapter.hh"
#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/karabo.hpp"

class RunTimeSchemaAttributes_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(RunTimeSchemaAttributes_Test);

    CPPUNIT_TEST(appTestRunner);

    CPPUNIT_TEST_SUITE_END();

   public:
    RunTimeSchemaAttributes_Test();
    virtual ~RunTimeSchemaAttributes_Test();
    void setUp();
    void tearDown();

   private:
    void appTestRunner();
    void testRuntimeApplication();

    /**
     * Checks that after a valid sequence of updates of attributes in a
     * schema, the attributes have been updated.
     */
    void testGuiServerApplication();

    /**
     * Checks that after a sequence of updates of attributes that have
     * an invalid update among them, all the updates are rolled-back.
     */
    void testGuiServerApplicationFailure();

    void dummyMonitor(const std::string&, const karabo::util::Hash&);

    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;

    // m_tcpAdapter mocks a GUI Client for the test.
    std::shared_ptr<karabo::TcpAdapter> m_tcpAdapter;
};

#endif /* RUNTIMESCHEMAATTRIBUTES_HH */
