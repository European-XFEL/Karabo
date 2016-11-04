/*
 * File:   DeviceServerRunner_Test.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on Sep 16, 2016, 2:34:36 PM
 */

#ifndef DEVICESERVERRUNNER_TEST_HH
#define	DEVICESERVERRUNNER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include "karabo/karabo.hpp"


class DeviceServerRunner_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(DeviceServerRunner_Test);

    CPPUNIT_TEST(allTestsOnDeviceServer);

    CPPUNIT_TEST_SUITE_END();

public:
    DeviceServerRunner_Test();
    virtual ~DeviceServerRunner_Test();
    void setUp();
    void tearDown();

private:
    void allTestsOnDeviceServer();
    
    // RunConfigurationGroup_Test
    void testRunConfigurationGroup();
    void testGetGroup();
    
    //  Other tests ....
    
    
private:
        
    karabo::core::DeviceServer::Pointer m_deviceServer;

    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
};

#endif	/* DEVICESERVERRUNNER_TEST_HH */

