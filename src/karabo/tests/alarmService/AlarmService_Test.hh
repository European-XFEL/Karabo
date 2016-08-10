/*
 * File:   AlarmService_Test.hh
 * Author: haufs
 *
 * Created on Aug 8, 2016, 3:21:59 PM
 */

#ifndef ALARMSERVICE_TEST_HH
#define	ALARMSERVICE_TEST_HH

#include "karabo/karabo.hpp"
#include "karabo/core/AlarmService.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/core/DeviceClient.hh"
#include <boost/shared_ptr.hpp>
#include <cppunit/extensions/HelperMacros.h>

#include "AlarmTesterDevice.hh"

class AlarmService_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(AlarmService_Test);

    CPPUNIT_TEST(testDeviceRegistration);


    CPPUNIT_TEST_SUITE_END();

public:
    AlarmService_Test();
    virtual ~AlarmService_Test();
    void setUp();
    void tearDown();

private:
    void startServer();
    void testDeviceRegistration();

    pid_t m_pid;
    //alarm service device
    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_deviceServerThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
    karabo::core::AlarmService::Pointer m_serviceDevice;

    boost::condition_variable m_waitCond;
    boost::mutex m_waitMutex;

};

#endif	/* ALARMSERVICE_TEST_HH */

