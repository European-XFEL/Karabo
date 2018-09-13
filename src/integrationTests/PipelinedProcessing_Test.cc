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

#define KRB_TEST_MAX_TIMEOUT 20
#define N_RUNS_PER_TEST 5


PipelinedProcessing_Test::PipelinedProcessing_Test() = default;


PipelinedProcessing_Test::~PipelinedProcessing_Test() = default;


void PipelinedProcessing_Test::setUp() {

    // set broker
    //putenv("KARABO_BROKER=tcp://localhost:7777");

    // Start central event-loop
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));

    // Create and start server
    Hash config("serverId", "testServerPP", "scanPlugins", false, "Logger.priority", "ERROR");
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
    CPPUNIT_ASSERT(m_deviceClient->instantiate("testServerPP", "P2PSenderDevice", Hash("deviceId", "p2pTestSender"), KRB_TEST_MAX_TIMEOUT).first);
    m_nDataPerRun = m_deviceClient->get<unsigned int>("p2pTestSender", "nData");

    // Create the base configuration for the receiver
    m_receiverConfig = Hash("deviceId", "pipeTestReceiver",
                            "input.connectedOutputChannels", "p2pTestSender:output1",
                            "input2.connectedOutputChannels", "p2pTestSender:output2");

    testGetOutputChannelSchema();

    testPipeWait();

    testPipeDrop();

    testProfileTransferTimes();

    CPPUNIT_ASSERT(m_deviceClient->killDevice("p2pTestSender").first);
}


void PipelinedProcessing_Test::testGetOutputChannelSchema() {
    std::clog << "---\ntestGetOutputChannelSchema\n";

    karabo::util::Hash dataSchema = m_deviceClient->getOutputChannelSchema("p2pTestSender", "output1");

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
    std::clog << "---\ntestPipeWait\n";

    // use only one receiver for a group of tests
    CPPUNIT_ASSERT(m_deviceClient->instantiate("testServerPP", "PipeReceiverDevice", m_receiverConfig).first);
    CPPUNIT_ASSERT_EQUAL(std::string("wait"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));
    CPPUNIT_ASSERT_EQUAL(std::string("wait"), m_deviceClient->get<std::string>(m_receiver, "input2.onSlowness"));

    testPipeWait(0, 0);
    testPipeWait(100, 0);
    testPipeWait(0, 100);

    CPPUNIT_ASSERT(m_deviceClient->killDevice(m_receiver).first);
    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeWait(unsigned int processingTime, unsigned int delayTime) {

    std::clog << "Test with onSlowness = 'wait', processingTime = " 
              << processingTime << ", delayTime = " << delayTime << "\n";

    // make sure the sender has stopped sending data
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>("p2pTestSender", "state", karabo::util::State::NORMAL, KRB_TEST_MAX_TIMEOUT));

    const std::string receiver("pipeTestReceiver");

    m_deviceClient->set(receiver, "processingTime", processingTime);
    m_deviceClient->set("p2pTestSender", "delay", delayTime);

    int64_t elapsedTimeIn_microseconds = 0ll;
    // we use a single receiver device for several successive tests.
    unsigned int nTotalData0 = m_deviceClient->get<unsigned int>(receiver, "nTotalData");
    unsigned int nTotalDataOnEos0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos");
    unsigned int nDataExpected = nTotalData0;
    for (unsigned int nRun = 0; nRun < N_RUNS_PER_TEST; ++nRun) {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();
        // Then call its slot
        m_deviceClient->execute("p2pTestSender", "write", KRB_TEST_MAX_TIMEOUT);

        nDataExpected += m_nDataPerRun;
        // And poll for the correct answer
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(receiver, "nTotalData", nDataExpected, KRB_TEST_MAX_TIMEOUT));

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        // Note that duration contains overhead from message travel time and polling interval in pollDeviceProperty!
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        // Check that EOS handling is not called too early
        
        // EOS comes a bit later, so we have to poll client again to be sure...
        // Note: only one EOS arrives after receiving a train of data!
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(receiver, "nTotalDataOnEos", nDataExpected, KRB_TEST_MAX_TIMEOUT));

        // Test if data source was correctly passed
        std::vector<std::string> sources = m_deviceClient->get<std::vector<std::string> >(receiver, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(std::string("p2pTestSender:output1"), sources[0]);
        // Check that receiver did not post any problem on status:
        CPPUNIT_ASSERT_EQUAL(std::string(), m_deviceClient->get<std::string>(receiver, "status"));
        // This only can be tested if we used an input handler and not onData
        // FIXME: This fails in DeviceClient::get: "Key 'onData' does not exist" - due to DeviceClient caching bug?
        if (false && !m_deviceClient->get<bool>(receiver, "onData")) {
            std::vector<std::string> sources = m_deviceClient->get<std::vector<std::string> >(receiver, "dataSourcesFromIndex");
            CPPUNIT_ASSERT_EQUAL(std::string("p2pTestSender:output1"), sources[0]);
        }
    }

    const unsigned int dataItemSize = m_deviceClient->get<unsigned int>(receiver, "dataItemSize");
    double mbps = double(dataItemSize) * double(nDataExpected - nTotalData0) / double(elapsedTimeIn_microseconds);
    // Note that this measurement checks the inner-process shortcut - and includes timing overhead e.g. pollDeviceProperty
    // In addition, the process and delay times also affect mbps.
    std::clog << "testPipe: Megabytes per sec : " << mbps 
              << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds
              << ", dataItemSize = " << dataItemSize 
              << ", nTotalData = " << nDataExpected - nTotalData0
              << ", nTotalDataOnEos = " << nDataExpected - nTotalDataOnEos0 << std::endl;
}


void PipelinedProcessing_Test::testPipeDrop() {
    std::clog << "---\ntestPipeDrop\n";

    m_receiverConfig += Hash("input.onSlowness", "drop", "input2.onSlowness", "drop");
    CPPUNIT_ASSERT(m_deviceClient->instantiate("testServerPP", "PipeReceiverDevice", m_receiverConfig).first);
    CPPUNIT_ASSERT_EQUAL(std::string("drop"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));
    CPPUNIT_ASSERT_EQUAL(std::string("drop"), m_deviceClient->get<std::string>(m_receiver, "input2.onSlowness"));

    testPipeDrop(0, 0, true);
    testPipeDrop(100, 0, true);
    testPipeDrop(0, 100, false);

    CPPUNIT_ASSERT(m_deviceClient->killDevice(m_receiver).first);
    std::clog << "Passed!\n\n";
}


void PipelinedProcessing_Test::testPipeDrop(unsigned int processingTime, unsigned int delayTime, bool dataLoss) {
    
    std::clog << "Test with onSlowness = 'drop', processingTime = " 
              << processingTime << ", delayTime = " << delayTime << "\n";

    // make sure the sender has stopped sending data
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>("p2pTestSender", "state", karabo::util::State::NORMAL, KRB_TEST_MAX_TIMEOUT));

    m_deviceClient->set(m_receiver, "processingTime", processingTime);
    m_deviceClient->set("p2pTestSender", "delay", delayTime);

    size_t elapsedTimeIn_microseconds = 0;
    unsigned int nTotalData0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    unsigned int nTotalDataOnEos0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalDataOnEos");
    unsigned int nDataExpected = nTotalData0;
    for (unsigned int nRun = 0; nRun < N_RUNS_PER_TEST; ++nRun) {
        auto startTimepoint = std::chrono::high_resolution_clock::now();
        m_deviceClient->execute("p2pTestSender", "write", KRB_TEST_MAX_TIMEOUT);

        // test data
        if (!dataLoss) {
            nDataExpected += m_nDataPerRun;
            CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataExpected, KRB_TEST_MAX_TIMEOUT));
        } else {
            // better way?
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
            // if the processing time is comparable to the delay time, the number of received data is random.
            unsigned int nTotalData = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
            CPPUNIT_ASSERT(nTotalData <= nDataExpected + m_nDataPerRun);
            CPPUNIT_ASSERT(nTotalData >= nDataExpected + 2);
            nDataExpected = nTotalData;
        }

        auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        // test EOS
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalDataOnEos", nDataExpected, KRB_TEST_MAX_TIMEOUT));

        // Test if data source was correctly passed
        auto sources = m_deviceClient->get<std::vector<std::string> >(m_receiver, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(std::string("p2pTestSender:output1"), sources[0]);
    }

    unsigned int dataItemSize = m_deviceClient->get<unsigned int>(m_receiver, "dataItemSize");
    double mbps = double(dataItemSize) * double(nDataExpected - nTotalData0) / double(elapsedTimeIn_microseconds);
    std::clog << "testPipe: Megabytes per sec : " << mbps 
              << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds
              << ", dataItemSize = " << dataItemSize 
              << ", nTotalData = " << nDataExpected - nTotalData0
              << ", nTotalDataOnEos = " << nDataExpected - nTotalDataOnEos0 << std::endl;
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

    // make sure the sender has stopped sending data
    CPPUNIT_ASSERT(pollDeviceProperty<karabo::util::State>("p2pTestSender", "state", karabo::util::State::NORMAL, KRB_TEST_MAX_TIMEOUT));

    std::string receiver = "pipeTestReceiver";
    if (noShortCut) {
        setenv("KARABO_NO_PIPELINE_SHORTCUT", "1", 1);
    }
    // Looks like to get "KARABO_NO_PIPELINE_SHORTCUT" active (some caching?),
    // we have to re-instantiate the receiver.
    CPPUNIT_ASSERT(m_deviceClient->instantiate("testServerPP", "PipeReceiverDevice", m_receiverConfig).first);
    m_deviceClient->set(receiver, "processingTime", 100); // why do we need this setup here?

    const unsigned int nDataPerRun = m_deviceClient->get<unsigned int>("p2pTestSender", "nData");

    // set the scenario
    m_deviceClient->set("p2pTestSender", "scenario", "profile");
    m_deviceClient->set("p2pTestSender", "copyAllData", copy);
    // Then call its slot
    m_deviceClient->execute("p2pTestSender", "write", KRB_TEST_MAX_TIMEOUT);
    
    // And poll for the correct answer
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(receiver, "nTotalData", nDataPerRun, KRB_TEST_MAX_TIMEOUT));

    pollDeviceProperty<float>(receiver, "averageTransferTime", 0.f, KRB_TEST_MAX_TIMEOUT, false); // until not zero anymore!
    float transferTime = m_deviceClient->get<float>(receiver, "averageTransferTime") / 1000;

    std::clog << "testProfileTransferTimes ("
            << (copy ? "copy" : "no copy") << ", " << (noShortCut ? "no short cut" : "short cut")
            << "): " << transferTime << " milliseconds average transfer time" << std::endl;

    if(noShortCut) {
        unsetenv("KARABO_NO_PIPELINE_SHORTCUT");
    }
    CPPUNIT_ASSERT(m_deviceClient->killDevice(receiver).first);
}


template <typename T>
bool PipelinedProcessing_Test::pollDeviceProperty(const std::string& deviceId,
                                                  const std::string& propertyName, const T& expected, const int maxTimeoutInSec,
                                                  bool checkForEqual) const {

    const int pollWaitTimeInMs = 5;
    int pollCounter = 0;

    // Poll the device until it responds with the correct answer or times out.
    while (pollWaitTimeInMs * pollCounter <= maxTimeoutInSec * 1000) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(pollWaitTimeInMs));
        const T nReceived = m_deviceClient->get<T>(deviceId, propertyName);
        if ((checkForEqual && nReceived == expected)
            || (!checkForEqual && nReceived != expected)) {
            return true;
        }
        ++pollCounter;
    }

    return false;
}
