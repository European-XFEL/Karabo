/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   SceneProvider_Test.hh
 * Author: steffen.hauf@xfel.eu

 */

#ifndef SCENEPROVIDER_TEST_HH
#define SCENEPROVIDER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <boost/shared_ptr.hpp>

#include "TcpAdapter.hh"
#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/karabo.hpp"


class SceneProvider_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(SceneProvider_Test);

    CPPUNIT_TEST(appTestRunner);

    CPPUNIT_TEST_SUITE_END();

   public:
    SceneProvider_Test();
    virtual ~SceneProvider_Test();
    void setUp();
    void tearDown();

   private:
    void appTestRunner();
    void testInstanceInfo();
    void testRequestScenes();
    void testRequestSceneFailure();

    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
    boost::shared_ptr<karabo::TcpAdapter> m_tcpAdapter;
};

#endif /* SCENEPROVIDER_TEST_HH */
