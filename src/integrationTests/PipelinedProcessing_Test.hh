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
    void testPipeQueue();
    void testPipeMinData();
    void testPipeTwoSharedReceiversWait();
    void testPipeTwoSharedReceiversDrop();

    /**
     * Tests pipe with two receivers with 'shared' value for their 'input.dataDistribution' setting
     * and a sender with 'queue' value for its 'output.NoInputShared' setting. Tests are performed
     * for both the 'round-robin' and 'load-balanced' values for the sender's 'output.distributionMode'.
     */
    void testPipeTwoSharedReceiversQueue();

    void testPipeTwoPots();
    void testProfileTransferTimes();

    void testPipeWait(unsigned int processingTime, unsigned int delayTime);


    /**
     * The "driver" method for the pipeline transmission performance test. This is the method to be
     * added to the test suite.
     */
    void testPipeWaitPerf();

    /**
     * Measures the pipeline "data transmission" performance by connecting a sender with zero
     * delay time to a receiver with zero processing time. The receiver also uses "wait" policy
     * for its "onSlowness" property.
     *
     * @param numOfDataItems Number of data items to be transmitted across the pipeline for each
     * test run.
     * @param numOfRuns The number of runs to be performed per invocation of the method.
     *
     * @note To minimize the effects of the property pollings used internally, it is recommended
     * to use a high number of data items to be transmitted along the pipe.
     */
    void testPipeWaitPerf(unsigned int numOfDataItems, unsigned int numOfRuns);

    void testPipeDrop(unsigned int processingTime, unsigned int delayTime, bool dataLoss);

    /**
     * Tests pipe for one receiver with 'queue' value for its 'input.onSlowness' setting and 'copy' value
     * for its 'input.dataDistribution' setting.
     *
     * In this scenario, the sender is expected to queue the data to be sent when the receiver has significantly
     * higher processing times than the sender's delayTime. If the opposite is true - the sender's delay time
     * is significantly higher than the receiver's processing time - no queuing on the sender is expected.
     *
     * This test asserts for those queuing behaviors.
     */
    void testPipeQueue(unsigned int processingTime, unsigned int delayTime);

    // roundRobin = true means that sender is supposed to be configured round-robin - extra tests of fair share are done
    void testPipeTwoSharedReceivers(unsigned int processingTime1,
                                    unsigned int processingTime2,
                                    unsigned int delayTime,
                                    bool dataLoss,
                                    bool roundRobin); // else load-balanced, i.e. the default

    /**
     * Tests the queuing behavior for pipes with two 'shared' receivers and the sender with 'queue' setting for
     * 'noInputShared' for its output channel. Queuing should be detected when the receivers processingTime are
     * significantly higher than the sender delayTime. Queuing should not be detected when the opposite is true -
     * the sender delayTime is significantly higher than the receivers processingTime.
     *
     * Note: the queuing behavior should be detected regardless of the sender's output distribution mode value 
     * being 'round-robin' or 'load-balanced'.
     */
    void testTwoSharedReceiversQueuing(unsigned int processingTime, unsigned int delayTime);

    /**
     * "Driving" method that takes care of calling testQueueClearOnDisconnectCopyQueue and
     * testQueueClearOnDisconnectSharedQueue.
     */
    void testQueueClearOnDisconnect();

    /**
     * Tests that the output queues kept by a sender are being properly cleared after the receiver disconnects
     * while there is still data to be sent. The receiver's 'input.onSlowness' is set to 'queue' and its
     * 'input.dataDistribution' is set to 'copy'.
     */
    void testQueueClearOnDisconnectCopyQueue();

    /**
     * Tests that the output queues kept by a sender are being properly cleared after the receiver disconnects
     * while there is still data to be sent. The receiver's 'input.dataDistribution' is set to 'shared' and
     * the sender's 'output.noInputShared' is set to 'queue'.
     *
     * @param useRoundRobin if true, the test is performed with the sender using 'round-robin' distribution mode.
     * Otherwise 'load-balanced' distribution mode is used.
     */
    void testQueueClearOnDisconnectSharedQueue(bool useRoundRobin);
    
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
