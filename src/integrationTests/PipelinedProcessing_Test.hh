/*
 * File:   PipelineProcessing_Test.hh
 * Author: haufs
 * 
 * Modified by J. Zhu
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
    virtual ~PipelinedProcessing_Test();

    void setUp();
    void tearDown();

private:

    void appTestRunner();

    void testGetOutputChannelSchema();
    void testPipeWait();
    void testPipeDrop();
    void testProfileTransferTimes();

    void testPipeWait(unsigned int processingTime, unsigned int delayTime);
    void testPipeDrop(unsigned int processingTime, unsigned int delayTime, bool dataLoss);
    void testProfileTransferTimes(bool noShortCut, bool copy);

    template <typename T>
    bool pollDeviceProperty(const std::string& deviceId,
                            const std::string& propertyName,
                            const T& expected,
                            const int maxTimeout,
                            bool checkForEqual = true) const; // if false, wait until not equal anymore

    void instantiateDeviceWithAssert(const std::string& classId, const karabo::util::Hash& configuration);
    void killDeviceWithAssert(const std::string& classId);

    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;

    unsigned int m_nDataPerRun;

    karabo::util::Hash m_receiverConfig;

    const std::string m_server = "testServerPP"; // server instance ID
    const std::string m_receiver = "pipeTestReceiver"; // receiver instance ID
    const std::string m_sender = "p2pTestSender"; // sender instance ID
    const std::string m_senderOutput1 = "p2pTestSender:output1"; // sender output channel 1
    const std::string m_senderOutput2 = "p2pTestSender:output2"; // sender output channel 2
};

#endif	/* PIPELINEDPROCESSING_TEST_HH */
