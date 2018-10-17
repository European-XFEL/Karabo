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


static const int m_numRunsPerTest = 5;
static const int m_maxTestTimeOut = 20;


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
    void testPipeMinData();
    void testPipeTwoSharedReceiversWait();
    void testPipeTwoSharedReceiversDrop();
    void testPipeTwoPots();
    void testProfileTransferTimes();

    void testPipeWait(unsigned int processingTime, unsigned int delayTime);
    void testPipeDrop(unsigned int processingTime, unsigned int delayTime, bool dataLoss);
    // roundRobin = true means that sender is supposed to be configured round-robin - extra tests of fair share are done
    void testPipeTwoSharedReceivers(unsigned int processingTime1,
                                    unsigned int processingTime2,
                                    unsigned int delayTime,
                                    bool dataLoss,
                                    bool roundRobin = false); // else load-balanced, i.e. the default
    void testProfileTransferTimes(bool noShortCut, bool copy);

    template <typename T>
    bool pollDeviceProperty(const std::string& deviceId,
                            const std::string& propertyName,
                            const T& expected,
                            bool checkForEqual = true, // if false, wait until not equal anymore
                            const int maxTimeout = m_maxTestTimeOut) const;

    void instantiateDeviceWithAssert(const std::string& classId, const karabo::util::Hash& configuration);
    void killDeviceWithAssert(const std::string& deviceId);

    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;

    const unsigned int m_nPots = 2; // number of local buffers
    unsigned int m_nDataPerRun;

    const std::string m_server = "testServerPP"; // server instance ID
    const std::string m_receiver = "pipeTestReceiver"; // receiver instance ID
    const std::string m_receiver1 = "pipeTestReceiver1";
    const std::string m_receiver2 = "pipeTestReceiver2";
    const std::string m_sender = "p2pTestSender"; // sender instance ID
    const std::string m_senderOutput1 = "p2pTestSender:output1"; // sender output channel 1
    const std::string m_senderOutput2 = "p2pTestSender:output2"; // sender output channel 2
    
    const karabo::util::Hash m_receiverBaseConfig{"input.connectedOutputChannels", m_senderOutput1,
                                                  "input2.connectedOutputChannels", m_senderOutput2};
};

#endif	/* PIPELINEDPROCESSING_TEST_HH */
