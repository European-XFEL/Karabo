/*
 * File:   GetOutputChannelSchema_Test.hh
 * Author: haufs
 *
 * Created on Sep 20, 2016, 3:40:33 PM
 */

#ifndef PIPELINEDPROCESSING_TEST_HH
#define	PIPELINEDPROCESSING_TEST_HH

#include "karabo/karabo.hpp"
#include "karabo/core/DeviceServer.hh"
#include "karabo/core/DeviceClient.hh"
#include <boost/shared_ptr.hpp>
#include <cppunit/extensions/HelperMacros.h>

class PipelinedProcessing_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(PipelinedProcessing_Test);

    CPPUNIT_TEST(appTestRunner);

    CPPUNIT_TEST_SUITE_END();

public:
    PipelinedProcessing_Test();
    virtual ~PipelinedProcessing_Test() {}

    void setUp();
    void tearDown();

private:

    void appTestRunner();
    void testGetOutputChannelSchema();
    void testPipe();
    void testProfileTransferTimes(bool noShortCut=false);

    template <typename T>
    bool pollDeviceProperty(const std::string& deviceId,
                            const std::string& propertyName,
                            const T& expected,
                            const int maxTimeout) const;

    karabo::core::DeviceServer::Pointer m_deviceServer;
    karabo::core::DeviceServer::Pointer m_deviceServer2;
    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;

};

#endif	/* PIPELINEDPROCESSING_TEST_HH */

