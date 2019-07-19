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
    void testTriggerGlobalAck();
    void testTriggerGlobal();

    /**
     * Waits, for a maximum amount of time, for a condition checked by a
     * given function.
     *
     * @param checker the function that will evaluate if the target condition has
     *        been reached.
     *
     * @param timeoutMillis the maximum amount of time to wait for the condition
     *        (in milliseconds).
     *
     * @return true if the condition has been reached; false if time expired before
     *         the condition could have been reached.
     */
    bool waitForCondition(boost::function<bool() > checker, unsigned int timeoutMillis);


    //alarm service device
    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
    boost::shared_ptr<karabo::TcpAdapter> m_tcpAdapter;
    std::string m_killedDeviceRow;

    std::string m_rowForDevice1;

};

#endif	/* ALARMSERVICE_TEST_HH */

