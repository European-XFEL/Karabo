/* 
 * File:   DeviceClient_Test.hh
 * Author: flucke
 *
 * Created on August 24, 2017, 9:49 AM
 */

#ifndef DEVICECLIENT_TEST_HH
#define	DEVICECLIENT_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include "karabo/core/DeviceServer.hh"
#include "karabo/core/DeviceClient.hh"

class DeviceClient_Test : public CPPUNIT_NS::TestFixture {



    CPPUNIT_TEST_SUITE(DeviceClient_Test);
    CPPUNIT_TEST(testGet);
    CPPUNIT_TEST_SUITE_END();

public:
    DeviceClient_Test();
    virtual ~DeviceClient_Test();
    void setUp();
    void tearDown();

private:
    void testGet();

    karabo::core::DeviceServer::Pointer m_deviceServer;
    karabo::core::DeviceClient::Pointer m_deviceClient;
};

#endif	/* DEVICECLIENT_TEST_HH */

