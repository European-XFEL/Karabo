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

    // Create client
    m_deviceClient = boost::make_shared<DeviceClient>();

    // Create and start server
    // There could be running server from previous failed or killed tests.
    if (!m_deviceClient->exists(m_serverId).first) {
        Hash config("serverId", m_serverId, "scanPlugins", false, "Logger.priority", "ERROR");
        m_deviceServer = DeviceServer::create("DeviceServer", config);
        m_deviceServer->finalizeInternalInitialization();
    }

    // Instantiate the sender on the server
    const Hash cfg("deviceId", m_sender);
    CPPUNIT_ASSERT(m_deviceClient->instantiate(m_serverId, "P2PSenderDevice", cfg).first);
    m_nDataPerRun = m_deviceClient->get<unsigned int>(m_sender, "nData");
    CPPUNIT_ASSERT_EQUAL(m_nDataPerRun, 12u);

    // Create the base configuration for the receiver
    m_receiverConfig = Hash("deviceId", m_receiver,
                            "input.connectedOutputChannels", m_sender + ":output1",
                            "input2.connectedOutputChannels", m_sender + ":output2");
}


void PipelinedProcessing_Test::tearDown() {
    // we need to check here because testGetOutputChannelSchema does not start a PipeReceiverDevice.
    if (m_deviceClient->exists(m_receiver).first) CPPUNIT_ASSERT(m_deviceClient->killDevice(m_receiver).first);
    CPPUNIT_ASSERT(m_deviceClient->killDevice(m_sender).first);

    m_deviceClient.reset();
    CPPUNIT_ASSERT(m_deviceClient.use_count() == 0);

    m_deviceServer.reset();
    CPPUNIT_ASSERT(m_deviceServer.use_count() == 0);

    EventLoop::stop();
    m_eventLoopThread.join();
}


void PipelinedProcessing_Test::testGetOutputChannelSchema() {

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
}


void PipelinedProcessing_Test::testPipeWait() {
    CPPUNIT_ASSERT(m_deviceClient->instantiate(m_serverId, "PipeReceiverDevice", m_receiverConfig).first);
    CPPUNIT_ASSERT_EQUAL(std::string("wait"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));
    CPPUNIT_ASSERT_EQUAL(std::string("wait"), m_deviceClient->get<std::string>(m_receiver, "input2.onSlowness"));

    testPipeWait(0, 0);
    testPipeWait(100, 0);
    testPipeWait(0, 100);
}


void PipelinedProcessing_Test::testPipeWait(unsigned int processingTime, unsigned int delayTime) {

    std::clog << "Test with onSlowness = 'wait', processingTime = " 
              << processingTime << ", delayTime = " << delayTime << "\n";

    m_deviceClient->set(m_receiver, "processingTime", processingTime);
    m_deviceClient->set(m_sender, "delay", delayTime);

    int64_t elapsedTimeIn_microseconds = 0ll;
    auto nTotalData0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    auto nTotalOnEos0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalOnEos");
    auto nDataExpected = nTotalData0;
    auto nOnEosExpected = nTotalOnEos0;
    for (unsigned int nRun = 0; nRun < N_RUNS_PER_TEST; ++nRun) {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();
        // Then call its slot
        m_deviceClient->execute(m_sender, "write", KRB_TEST_MAX_TIMEOUT);

        nDataExpected += m_nDataPerRun;
        // And poll for the correct answer
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", nDataExpected, KRB_TEST_MAX_TIMEOUT));

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        // Note that duration contains overhead from message travel time and polling interval in pollDeviceProperty!
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        // Check that EOS handling is not called too early (how?)

        nOnEosExpected++;
        // EOS comes a bit later, so we have to poll client again to be sure...
        // Note: only one EOS arrives after receiving a train of data!
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(
                m_receiver, "nTotalOnEos", nOnEosExpected, KRB_TEST_MAX_TIMEOUT));

        // Test if data source was correctly passed
        std::vector<std::string> sources = m_deviceClient->get<std::vector<std::string> >(m_receiver, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(m_sender + ":output1", sources[0]);

        // This only can be tested if we used an input handler and not onData
        // FIXME: This fails in DeviceClient::get: "Key 'onData' does not exist" - due to DeviceClient caching bug?
        if (false && !m_deviceClient->get<bool>(m_receiver, "onData")) {
            auto sources = m_deviceClient->get<std::vector<std::string> >(m_receiver, "dataSourcesFromIndex");
            CPPUNIT_ASSERT_EQUAL(m_sender + ":output1", sources[0]);
        }
    }

    const unsigned int dataItemSize = m_deviceClient->get<unsigned int>(m_receiver, "dataItemSize");
    double mbps = double(dataItemSize) * double(nDataExpected - nTotalData0) / double(elapsedTimeIn_microseconds);
    // Note that this measurement checks the inner-process shortcut - and includes timing overhead e.g. pollDeviceProperty
    // In addition, the process and delay times also affect mbps.
    std::clog << "testPipe: Megabytes per sec : " << mbps 
              << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds
              << ", dataItemSize = " << dataItemSize 
              << ", nTotalData = " << nDataExpected - nTotalData0
              << ", nTotalOnEos = " << nOnEosExpected - nTotalOnEos0 << std::endl;
}


void PipelinedProcessing_Test::testPipeDrop() {
    m_receiverConfig += Hash("input.onSlowness", "drop", "input2.onSlowness", "drop");
    CPPUNIT_ASSERT(m_deviceClient->instantiate(m_serverId, "PipeReceiverDevice", m_receiverConfig).first);
    CPPUNIT_ASSERT_EQUAL(std::string("drop"), m_deviceClient->get<std::string>(m_receiver, "input.onSlowness"));
    CPPUNIT_ASSERT_EQUAL(std::string("drop"), m_deviceClient->get<std::string>(m_receiver, "input2.onSlowness"));

    testPipeDrop(0, 0, DataLoss::RANDOM);
    testPipeDrop(0, 100, DataLoss::NONE);
    testPipeDrop(100, 0, DataLoss::MOST);
}


void PipelinedProcessing_Test::testPipeDrop(unsigned int processingTime, unsigned int delayTime, DataLoss dataLoss) {
    
    std::clog << "Test with onSlowness = 'drop', processingTime = " 
              << processingTime << ", delayTime = " << delayTime << "\n";
    
    // The error message 'Command "write" is not allowed in current state "ACTIVE" of device "p2pTestSender"' 
    // has been observed during testing with local "glassfish" broker.
    CPPUNIT_ASSERT_EQUAL(std::string("NORMAL"), m_deviceClient->get<karabo::util::State>(m_sender, "state").name());
    
    m_deviceClient->set(m_receiver, "processingTime", processingTime);
    m_deviceClient->set(m_sender, "delay", delayTime);

    std::size_t elapsedTimeIn_microseconds = 0;
    auto nTotalData0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
    auto nTotalOnEos0 = m_deviceClient->get<unsigned int>(m_receiver, "nTotalOnEos");
    auto nDataExpected = nTotalData0;
    auto nOnEosExpected = nTotalOnEos0;
    for (unsigned int nRun = 0; nRun < N_RUNS_PER_TEST; ++nRun) {
        auto startTimepoint = std::chrono::high_resolution_clock::now();
        m_deviceClient->execute(m_sender, "write", KRB_TEST_MAX_TIMEOUT);

        // test data
        if (dataLoss == DataLoss::NONE) {
            nDataExpected += m_nDataPerRun;
            CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(
                    m_receiver, "nTotalData", nDataExpected, KRB_TEST_MAX_TIMEOUT));
        } else if (dataLoss == DataLoss::MOST) {
            // if the processing time is much longer than the delay time, 
            // only two data are expected to be received: the first and the last?
            nDataExpected += 2;
            // better way?
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
            CPPUNIT_ASSERT_EQUAL(nDataExpected, m_deviceClient->get<unsigned int>(m_receiver, "nTotalData"));
        } else if (dataLoss == DataLoss::RANDOM) {
            // better way?
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
            // if the processing time is comparable to the delay time, the number of received data is random.
            auto nTotalData = m_deviceClient->get<unsigned int>(m_receiver, "nTotalData");
            CPPUNIT_ASSERT(nTotalData <= nDataExpected + m_nDataPerRun && nTotalData > nDataExpected);
            nDataExpected = nTotalData;
        } else {
            std::clog << "Unknown DataLoss!" << std::endl;
            CPPUNIT_ASSERT(false);
        }

        auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        // test EOS
        nOnEosExpected++;
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(
            m_receiver, "nTotalOnEos", nOnEosExpected, KRB_TEST_MAX_TIMEOUT));

        // Test if data source was correctly passed
        auto sources = m_deviceClient->get<std::vector<std::string> >(m_receiver, "dataSources");
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (1u), sources.size());
        CPPUNIT_ASSERT_EQUAL(m_sender + ":output1", sources[0]);
    }

    auto dataItemSize = m_deviceClient->get<unsigned int>(m_receiver, "dataItemSize");
    double mbps = double(dataItemSize) * double(nDataExpected - nTotalData0) / double(elapsedTimeIn_microseconds);
    std::clog << "testPipe: Megabytes per sec : " << mbps 
              << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds
              << ", dataItemSize = " << dataItemSize 
              << ", nTotalData = " << nDataExpected - nTotalData0
              << ", nTotalOnEos = " << nOnEosExpected - nTotalOnEos0 << std::endl;
}


void PipelinedProcessing_Test::testProfileTransferTimes() {
    testProfileTransferTimes(false, true);
    testProfileTransferTimes(false, false);
    testProfileTransferTimes(true, true);
    testProfileTransferTimes(true, false);
}


void PipelinedProcessing_Test::testProfileTransferTimes(bool noShortCut, bool copy) {
    if (noShortCut) {
        setenv("KARABO_NO_PIPELINE_SHORTCUT", "1", 1);
    }
    // Looks like to get "KARABO_NO_PIPELINE_SHORTCUT" active (some caching?),
    // we have to re-instantiate the receiver.
    CPPUNIT_ASSERT(m_deviceClient->instantiate(m_serverId, "PipeReceiverDevice", m_receiverConfig).first);
    m_deviceClient->set(m_receiver, "processingTime", 100); // why do we need this setup here?

    m_deviceClient->set(m_sender, "scenario", "profile");
    m_deviceClient->set(m_sender, "copyAllData", copy);
    m_deviceClient->execute(m_sender, "write", KRB_TEST_MAX_TIMEOUT);
    
    // And poll for the correct answer
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(m_receiver, "nTotalData", m_nDataPerRun, KRB_TEST_MAX_TIMEOUT));

    pollDeviceProperty<float>(m_receiver, "averageTransferTime", 0.f, KRB_TEST_MAX_TIMEOUT, false); // until not zero anymore!
    auto transferTime = m_deviceClient->get<float>(m_receiver, "averageTransferTime") / 1000;

    std::clog << "testProfileTransferTimes ("
            << (copy ? "copy" : "no copy") << ", " << (noShortCut ? "no short cut" : "short cut")
            << "): " << transferTime << " milliseconds average transfer time" << std::endl;

    if (noShortCut) {
        unsetenv("KARABO_NO_PIPELINE_SHORTCUT");
    }
    CPPUNIT_ASSERT(m_deviceClient->killDevice(m_receiver).first);
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
        if ((checkForEqual && nReceived == expected) || (!checkForEqual && nReceived != expected)) {
            return true;
        }
        ++pollCounter;
    }

    return false;
}
