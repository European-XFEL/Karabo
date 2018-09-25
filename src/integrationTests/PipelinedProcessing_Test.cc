/*
 * File:   PipelineProcessing_Test.cc
 * Author: haufs
 *
 * Modified by J. Zhu
 *
 * Created on Sep 20, 2016, 3:40:34 PM
 */

#include "PipelinedProcessing_Test.hh"
#include <karabo/net/EventLoop.hh>
#include <karabo/log/Logger.hh>

USING_KARABO_NAMESPACES;

CPPUNIT_TEST_SUITE_REGISTRATION(PipelinedProcessing_Test);


PipelinedProcessing_Test::PipelinedProcessing_Test() = default;


PipelinedProcessing_Test::~PipelinedProcessing_Test() = default;


void PipelinedProcessing_Test::setUp() {

    // set broker
    //putenv("KARABO_BROKER=tcp://localhost:7777");

    // Start central event-loop
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));

    // Create and start server
    Hash config("serverId", m_server, "scanPlugins", false, "Logger.priority", "ERROR");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();

    // Create client
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());
}


void PipelinedProcessing_Test::tearDown() {
    m_deviceClient.reset();
    m_deviceServer.reset();

    EventLoop::stop();
    m_eventLoopThread.join();
}


void PipelinedProcessing_Test::appTestRunner() {

    // in order to avoid recurring setup and tear down calls, all tests are run in a single runner
    instantiateDeviceWithAssert("P2PSenderDevice", Hash("deviceId", m_sender));
    m_nDataPerRun = m_deviceClient->get<unsigned int>(m_sender, "nData");

    // Create the base configuration for the two receivers
    m_receiverConfig = Hash("deviceId", m_receiver,
                            "input.connectedOutputChannels", m_senderOutput1,
                            "input2.connectedOutputChannels", m_senderOutput2);
    m_receiver2Config = Hash("deviceId", m_receiver2,
                             "input.connectedOutputChannels", m_senderOutput1,
                             "input2.connectedOutputChannels", m_senderOutput2);

    testGetOutputChannelSchema();

    // do not change the order of the following tests

    testPipeWait();

    testPipeDrop();

    testPipeTwoSharedReceiversWait();

    testPipeTwoSharedReceiversDrop();
    
    testTwoPots();

    testProfileTransferTimes();

    killDeviceWithAssert(m_sender);
}


void PipelinedProcessing_Test::testGetOutputChannelSchema() {
    std::clog << "---\ntestGetOutputChannelSchema\n";

    karabo::util::Hash dataSchema = m_deviceClient->getOutputChannelSchema(m_sender, "output1");

//    clog << "\nPipelinedProcessing_Test::testGetOutputChannelSchema() : dataSchema => \n" << dataSchema << endl;

    CPPUNIT_ASSERT(dataSchema.has("dataId"));
    CPPUNIT_ASSERT(dataSchema.getType("dataId") == karabo::util::Types::INT32);
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("dataId", "valueType") == "INT32");
    CPPUNIT_ASSERT(dataSchema.has("sha1"));
    CPPUNIT_ASSERT(dataSchema.getType("sha1") == karabo::util::Types::INT32);
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("sha1", "valueType") == "STRING");
    CPPUNIT_ASSERT(dataSchema.has("data"));
    CPPUNIT_ASSERT(dataSchema.getType("data") == karabo::util::Types::INT32);
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("data", "valueType") == "VECTOR_INT64");
    CPPUNIT_ASSERT(dataSchema.has("array"));
    CPPUNIT_ASSERT(dataSchema.hasAttribute("array", KARABO_HASH_CLASS_ID));
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array", KARABO_HASH_CLASS_ID) == "Hash");
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array", "classId") == "NDArray");
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array.data", "valueType") == "BYTE_ARRAY");
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array.shape", "valueType") == "VECTOR_UINT64");
    CPPUNIT_ASSERT(dataSchema.getAttributeAs<std::string>("array.shape", "defaultValue") == "100,200,0");
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array.type", "valueType") == "INT32");
    CPPUNIT_ASSERT(dataSchema.getAttributeAs<std::string>("array.type", "defaultValue") == "22");
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array.isBigEndian", "valueType") == "BOOL");
    CPPUNIT_ASSERT(dataSchema.getAttributeAs<std::string>("array.isBigEndian", "defaultValue") == "0");

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeWait() {
    std::clog << "---\ntestPipeWait (onSlowness = 'wait')\n";
    // use only one receiver for a group of tests
    instantiateDeviceWithAssert("PipeReceiverDevice", m_receiverConfig);
    CPPUNIT_ASSERT_EQUAL(std::string("wait"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));

    testPipeWait(0, 0);
    testPipeWait(100, 0);
    testPipeWait(0, 100);

    killDeviceWithAssert(m_receiver);
    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeWait(unsigned int processingTime, unsigned int delayTime) {

    std::clog << "- processingTime = " << processingTime << " ms, delayTime = " << delayTime << " ms\n";

    m_deviceClient->set(m_receiver, "processingTime", processingTime);
    m_deviceClient->set(m_sender, "delay", delayTime);

    int64_t elapsedTimeIn_microseconds = 0ll;
    // we use a single receiver device for several successive tests.
    unsigned int nTotalData0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    unsigned int nTotalDataOnEos0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos");
    unsigned int nDataExpected = nTotalData0;
    for (unsigned int nRun = 0; nRun < m_numRunsPerTest; ++nRun) {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();
        
        // make sure the sender has stopped sending data
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>(m_sender, "state", karabo::util::State::NORMAL));
        // Then call its slot
        m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

        nDataExpected += m_nDataPerRun;
        // And poll for the correct answer
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataExpected));

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        // Note that duration contains overhead from message travel time and polling interval in pollDeviceProperty!
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        // Check that EOS handling is not called too early
        
        // EOS comes a bit later, so we have to poll client again to be sure...
        // Note: only one EOS arrives after receiving a train of data!
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nDataExpected));

        // Test if data source was correctly passed
        auto sources = m_deviceClient->get<std::vector<std::string> >(m_receiver, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1, sources[0]);
        // Check that receiver did not post any problem on status:
        CPPUNIT_ASSERT_EQUAL(std::string(), m_deviceClient->get<std::string>(m_receiver, "status"));
        // This only can be tested if we used an input handler and not onData
        // FIXME: This fails in DeviceClient::get: "Key 'onData' does not exist" - due to DeviceClient caching bug?
        if (false && !m_deviceClient->get<bool>(m_receiver, "onData")) {
            auto sources = m_deviceClient->get<std::vector<std::string> >(m_receiver, "dataSourcesFromIndex");
            CPPUNIT_ASSERT_EQUAL(m_senderOutput1, sources[0]);
        }
    }

    const unsigned int dataItemSize = m_deviceClient->get<unsigned int>(m_receiver, "dataItemSize");
    double mbps = double(dataItemSize) * double(nDataExpected - nTotalData0) / double(elapsedTimeIn_microseconds);
    // Note that this measurement checks the inner-process shortcut - and includes timing overhead e.g. pollDeviceProperty
    // In addition, the process and delay times also affect mbps.
    std::clog << "  summary: Megabytes per sec : " << mbps
              << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds
              << ", dataItemSize = " << dataItemSize 
              << ", nTotalData = " << nDataExpected - nTotalData0
              << ", nTotalDataOnEos = " << nDataExpected - nTotalDataOnEos0 << std::endl;
}


void PipelinedProcessing_Test::testPipeDrop() {
    std::clog << "---\ntestPipeDrop (onSlowness = 'drop')\n";

    m_receiverConfig += Hash("input.onSlowness", "drop");
    instantiateDeviceWithAssert("PipeReceiverDevice", m_receiverConfig);
    CPPUNIT_ASSERT_EQUAL(std::string("drop"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));

    testPipeDrop(10, 0, true);
    testPipeDrop(100, 0, true);
    testPipeDrop(0, 100, false);

    killDeviceWithAssert(m_receiver);
    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeDrop(unsigned int processingTime, unsigned int delayTime, bool dataLoss) {

    std::clog << "- processingTime = " << processingTime << " ms, delayTime = " << delayTime << " ms\n";

    m_deviceClient->set(m_receiver, "processingTime", processingTime);
    m_deviceClient->set(m_sender, "delay", delayTime);

    size_t elapsedTimeIn_microseconds = 0;
    unsigned int nTotalData0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    unsigned int nTotalDataOnEos0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos");
    unsigned int nDataExpected = nTotalData0;
    for (unsigned int nRun = 0; nRun < m_numRunsPerTest; ++nRun) {
        auto startTimepoint = std::chrono::high_resolution_clock::now();
        // make sure the sender has stopped sending data
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>(m_sender, "state", karabo::util::State::NORMAL));
        m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

        // test data
        if (!dataLoss) {
            nDataExpected += m_nDataPerRun;
            CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataExpected));
        } else {
            // poll until nTotalDataOnEos changes (increases)
            CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nDataExpected, false));
            
            // if the processing time is comparable to or larger than the delay time, 
            // the number of received data is random, but should be larger than the 
            // number of local buffers (currently one 'active' and one 'inactive')
            unsigned int nTotalData = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
            CPPUNIT_ASSERT(nTotalData < nDataExpected + m_nDataPerRun);
            CPPUNIT_ASSERT(nTotalData >= nDataExpected + m_nPots);
            nDataExpected = nTotalData;
        }

        auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        // test EOS
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nDataExpected));

        // Test if data source was correctly passed
        auto sources = m_deviceClient->get<std::vector<std::string> >(m_receiver, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1, sources[0]);
    }

    unsigned int dataItemSize = m_deviceClient->get<unsigned int>(m_receiver, "dataItemSize");
    double mbps = double(dataItemSize) * double(nDataExpected - nTotalData0) / double(elapsedTimeIn_microseconds);
    std::clog << "  summary: Megabytes per sec : " << mbps
              << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds
              << ", dataItemSize = " << dataItemSize 
              << ", nTotalData = " << nDataExpected - nTotalData0
              << ", nTotalDataOnEos = " << nDataExpected - nTotalDataOnEos0 << std::endl;
}


void PipelinedProcessing_Test::testPipeTwoSharedReceiversWait() {
    std::clog << "---\ntestPipeTwoSharedReceiversWait (onSlowness = 'wait', dataDistribution = 'shared')\n";

    m_receiverConfig += Hash("input.onSlowness", "wait", "input.dataDistribution", "shared");
    m_receiver2Config += Hash("input.onSlowness", "wait", "input.dataDistribution", "shared");

    instantiateDeviceWithAssert("PipeReceiverDevice", m_receiverConfig);
    instantiateDeviceWithAssert("PipeReceiverDevice", m_receiver2Config);

    CPPUNIT_ASSERT_EQUAL(std::string("wait"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));
    CPPUNIT_ASSERT_EQUAL(std::string("wait"), m_deviceClient->get<std::string>(m_receiver2, "input.onSlowness"));
    CPPUNIT_ASSERT_EQUAL(std::string("shared"), m_deviceClient->get<std::string>(m_receiver, "input.dataDistribution"));
    CPPUNIT_ASSERT_EQUAL(std::string("shared"), m_deviceClient->get<std::string>(m_receiver2, "input.dataDistribution"));
    // check that the default value is "copy"
    CPPUNIT_ASSERT_EQUAL(std::string("copy"), m_deviceClient->get<std::string>(m_receiver, "input2.dataDistribution"));

    testPipeTwoSharedReceivers(0, 0, 0, false);
    testPipeTwoSharedReceivers(0, 300, 0, false);
    testPipeTwoSharedReceivers(300, 0, 0, false);
    
    killDeviceWithAssert(m_receiver);
    killDeviceWithAssert(m_receiver2);
    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeTwoSharedReceiversDrop() {
    std::clog << "---\ntestPipeTwoSharedReceiversDrop (onSlowness = 'drop', dataDistribution = 'shared')\n";

    m_receiverConfig += Hash("input.onSlowness", "drop", "input.dataDistribution", "shared");
    m_receiver2Config += Hash("input.onSlowness", "drop", "input.dataDistribution", "shared");

    instantiateDeviceWithAssert("PipeReceiverDevice", m_receiverConfig);
    instantiateDeviceWithAssert("PipeReceiverDevice", m_receiver2Config);

    CPPUNIT_ASSERT_EQUAL(std::string("drop"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));
    CPPUNIT_ASSERT_EQUAL(std::string("drop"), m_deviceClient->get<std::string>(m_receiver2, "input.onSlowness"));
    CPPUNIT_ASSERT_EQUAL(std::string("shared"), m_deviceClient->get<std::string>(m_receiver, "input.dataDistribution"));
    CPPUNIT_ASSERT_EQUAL(std::string("shared"), m_deviceClient->get<std::string>(m_receiver2, "input.dataDistribution"));

    testPipeTwoSharedReceivers(0, 0, 0, false);
    // TODO: we should expect to see data loss in the following two cases, however ...
    testPipeTwoSharedReceivers(100, 0, 0, false);
    testPipeTwoSharedReceivers(300, 300, 0, false);

    killDeviceWithAssert(m_receiver);
    killDeviceWithAssert(m_receiver2);
    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeTwoSharedReceivers(unsigned int processingTime, 
                                                          unsigned int processingTime2,
                                                          unsigned int delayTime,
                                                          bool dataLoss) {

    std::clog << "- processingTime = " << processingTime
            << " ms, processingTime2 = " << processingTime2
            << " ms, delayTime = " << delayTime << " ms\n";

    m_deviceClient->set(m_receiver, "processingTime", processingTime);
    m_deviceClient->set(m_receiver2, "processingTime", processingTime2);
    m_deviceClient->set(m_sender, "delay", delayTime);
    // we use the same two receiver devices for several successive tests.
    unsigned int nTotalData0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    unsigned int nTotalData02 = m_deviceClient->get<unsigned int>(m_receiver2, "nTotalData");
    unsigned int nTotalData = nTotalData0;
    unsigned int nTotalData2 = nTotalData02;
    CPPUNIT_ASSERT_EQUAL(nTotalData, m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos"));
    CPPUNIT_ASSERT_EQUAL(nTotalData2, m_deviceClient->get<unsigned int>(m_receiver2, "nTotalDataOnEos"));
    for (unsigned int nRun = 0; nRun < m_numRunsPerTest; ++nRun) {
        // make sure the sender has stopped sending data
        CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>(m_sender, "state", karabo::util::State::NORMAL));
        // Then call its slot
        m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);

        // poll until nTotalDataOnEos(s) of both receivers change (increase)
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nTotalData, false));
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver2, "nTotalDataOnEos", nTotalData2, false));

        unsigned int nTotalDataNew = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
        unsigned int nTotalData2New = m_deviceClient->get<unsigned int>(m_receiver2, "nTotalData");

        // test nTotalDataOnEos == nTotalData
        CPPUNIT_ASSERT_EQUAL(nTotalDataNew, m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos"));
        CPPUNIT_ASSERT_EQUAL(nTotalData2New, m_deviceClient->get<unsigned int>(m_receiver2, "nTotalDataOnEos"));

        // test the total data received
        // A receiver should receive at least m_nPots data no mater how long the processingTime is.
        CPPUNIT_ASSERT(nTotalDataNew >= nTotalData + m_nPots);
        CPPUNIT_ASSERT(nTotalData2New >= nTotalData2 + m_nPots);
        if (!dataLoss) {
            CPPUNIT_ASSERT_EQUAL(nTotalData + nTotalData2 + m_nDataPerRun, nTotalDataNew + nTotalData2New);
        } else {
            CPPUNIT_ASSERT(nTotalDataNew + nTotalData2New < nTotalData + nTotalData2 + m_nDataPerRun);
        }

        // update nTotalData
        nTotalData = nTotalDataNew;
        nTotalData2 = nTotalData2New;
        
        // Test if data source was correctly passed
        auto sources = m_deviceClient->get<std::vector<std::string> >(m_receiver, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1, sources[0]);
        auto sources2 = m_deviceClient->get<std::vector<std::string> >(m_receiver2, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (1u), sources2.size());
        CPPUNIT_ASSERT_EQUAL(m_senderOutput1, sources2[0]);

        // Check that receiver did not post any problem on status:
        CPPUNIT_ASSERT_EQUAL(std::string(), m_deviceClient->get<std::string>(m_receiver, "status"));
        CPPUNIT_ASSERT_EQUAL(std::string(), m_deviceClient->get<std::string>(m_receiver2, "status"));
    }

    std::clog << "  summary: nTotalData = " << nTotalData - nTotalData0 << ", " << nTotalData2 - nTotalData02 << std::endl;
}


void PipelinedProcessing_Test::testTwoPots() {
    std::clog << "---\ntestTwoPots\n";

    // start a new sender with a long delay time
    const std::string sender = "potTestSender";
    instantiateDeviceWithAssert("P2PSenderDevice", Hash("deviceId", sender));
    unsigned int nDataExpected = m_deviceClient->get<unsigned int>(m_sender, "nData");

    // start a receiver
    m_receiverConfig += Hash("input.onSlowness", "wait", "input.dataDistribution", "copy", "processingTime", 0);
    instantiateDeviceWithAssert("PipeReceiverDevice", m_receiverConfig);

    // write data asynchronously
    m_deviceClient->execute(m_sender, "write");

    unsigned int nDataWhenStop = 6;
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataWhenStop));
    // stop sending data after receiving nDataWhenStop data!
    m_deviceClient->execute(m_sender, "stop");
    // make sure only nDataWhenStop data have been received up to now
    CPPUNIT_ASSERT_EQUAL(nDataWhenStop, m_deviceClient->get<unsigned int>(m_receiver, "nTotalData"));
    // The receiver is expected to get two more data when EOS arrives. One is the data being sending 
    // during the "stop" slot is called (the active pot). Another comes from the inactive pot.
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nDataWhenStop + 2));

    killDeviceWithAssert(sender);
    killDeviceWithAssert(m_receiver);
    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testProfileTransferTimes() {
    std::clog << "---\ntestProfileTransferTimes\n";

    testProfileTransferTimes(false, true);
    testProfileTransferTimes(false, false);
    testProfileTransferTimes(true, true);
    testProfileTransferTimes(true, false);

    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testProfileTransferTimes(bool noShortCut, bool copy) {

    if (noShortCut) {
        setenv("KARABO_NO_PIPELINE_SHORTCUT", "1", 1);
    }
    // Looks like to get "KARABO_NO_PIPELINE_SHORTCUT" active (some caching?),
    // we have to re-instantiate the receiver.
    instantiateDeviceWithAssert("PipeReceiverDevice", m_receiverConfig);

    const unsigned int nDataPerRun = m_deviceClient->get<unsigned int>(m_sender, "nData");

    // set the scenario
    m_deviceClient->set(m_sender, "scenario", "profile");
    m_deviceClient->set(m_sender, "copyAllData", copy);
    // make sure the sender has stopped sending data
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>(m_sender, "state", karabo::util::State::NORMAL));
    // Then call its slot
    m_deviceClient->execute(m_sender, "write", m_maxTestTimeOut);
    
    // And poll for the correct answer
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataPerRun));

    pollDeviceProperty<float>(m_receiver, "averageTransferTime", 0.f, false); // until not zero anymore!
    float transferTime = m_deviceClient->get<float>(m_receiver, "averageTransferTime") / 1000;

    std::clog << "- ("
              << (copy ? "copy" : "no copy") << ", " << (noShortCut ? "no short cut" : "short cut")
              << "): " << transferTime << " milliseconds average transfer time" << std::endl;

    if (noShortCut) {
        unsetenv("KARABO_NO_PIPELINE_SHORTCUT");
    }
    killDeviceWithAssert(m_receiver);
}


template <typename T>
bool PipelinedProcessing_Test::pollDeviceProperty(const std::string& deviceId,
                                                  const std::string& propertyName, const T& expected, bool checkForEqual, 
                                                  const int maxTimeoutInSec) const {

    const int pollWaitTimeInMs = 5;
    int pollCounter = 0;

    // Poll the device until it responds with the correct answer or times out.
    while (pollWaitTimeInMs * pollCounter <= maxTimeoutInSec * 1000) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(pollWaitTimeInMs));
        const T nReceived = m_deviceClient->get<T>(deviceId, propertyName);
        if ((checkForEqual && nReceived == expected) || (!checkForEqual && nReceived != expected)) {
            return true;
        }
        ++pollCounter;
    }

    return false;
}


void PipelinedProcessing_Test::instantiateDeviceWithAssert(const std::string& classId,
                                                           const karabo::util::Hash& configuration) {
    std::pair<bool, std::string> success = m_deviceClient->instantiate(m_server, classId, configuration, m_maxTestTimeOut);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
}


void PipelinedProcessing_Test::killDeviceWithAssert(const std::string& deviceId) {
    std::pair<bool, std::string> success = m_deviceClient->killDevice(deviceId, m_maxTestTimeOut);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
}
