/*
 * File:   Device_Test.hh
 * Author: gero.flucke@xfel.eu
 
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

    bool waitForCondition(boost::function<bool() > checker, unsigned int timeoutMs);

    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
};

#endif	/* DEVICE_TEST_HH */

