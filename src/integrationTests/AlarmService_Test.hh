/*
 * File:   AlarmService_Test.hh
 * Author: haufs
 *
 * Created on Aug 8, 2016, 3:21:59 PM
 */

#ifndef ALARMSERVICE_TEST_HH
#define	ALARMSERVICE_TEST_HH

#include "karabo/karabo.hpp"
#include "karabo/devices/AlarmService.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/core/DeviceClient.hh"
#include <boost/shared_ptr.hpp>
#include <cppunit/extensions/HelperMacros.h>

#include "AlarmTesterDevice.hh"
#include "TcpAdapter.hh"

class AlarmService_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(AlarmService_Test);

    CPPUNIT_TEST(appTestRunner);


    CPPUNIT_TEST_SUITE_END();

public:
    AlarmService_Test();
    virtual ~AlarmService_Test();
    void setUp();
    void tearDown();

private:

    void appTestRunner();
    void testDeviceRegistration();
    void testAlarmPassing();
    void testAcknowledgement();
    void testFlushing();
    void testRecovery();
    void testDeviceKilled();
    void testDeviceReappeared();


    //alarm service device
    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
    boost::shared_ptr<karabo::TcpAdapter> m_tcpAdapter;
    std::string m_killedDeviceRow;

    std::string m_rowForDevice0, m_rowForDevice1, m_rowForDevice2;

};

#endif	/* ALARMSERVICE_TEST_HH */

