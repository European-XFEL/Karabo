/*
 * File:   Timing_Test.hh
 * Author: steffen.hauf@xfel.eu
 
 */

#ifndef SCENEPROVIDER_TEST_HH
#define	SCENEPROVIDER_TEST_HH

#include "karabo/karabo.hpp"
#include "karabo/core/DeviceServer.hh"
#include "karabo/core/DeviceClient.hh"
#include <karabo/util/Epochstamp.hh>
#include <boost/shared_ptr.hpp>
#include <cppunit/extensions/HelperMacros.h>



class Timing_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Timing_Test);

    CPPUNIT_TEST(appTestRunner);

    CPPUNIT_TEST_SUITE_END();

public:
    Timing_Test();
    virtual ~Timing_Test();
    void setUp();
    void tearDown();

private:
    void appTestRunner();
    void testCorrelated();
    void testIntermittantUpdates();
    
    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
    karabo::util::Epochstamp m_lastCheck;
};

#endif	/* SCENEPROVIDER_TEST_HH */

