/*
 * File:   LockTest_Test.hh
 * Author: steffen
 *
 * Created on Oct 2, 2016, 1:21:16 PM
 */

#ifndef LOCKTEST_TEST_HH
#define	LOCKTEST_TEST_HH

#include "karabo/karabo.hpp"
#include "karabo/core/DeviceServer.hh"
#include "karabo/core/DeviceClient.hh"
#include <boost/shared_ptr.hpp>
#include <cppunit/extensions/HelperMacros.h>

#include "LockTestDevice.hh"



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
    
    void waitUntilLockClears();
    
    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_deviceServerThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
};

#endif	/* LOCKTEST_TEST_HH */

