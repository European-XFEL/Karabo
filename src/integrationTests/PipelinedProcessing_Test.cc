/*
 * File:   GetOutputChannelSchema_Test.cc
 * Author: haufs
 *
 * Created on Sep 20, 2016, 3:40:34 PM
 */

#include "PipelinedProcessing_Test.hh"
#include <karabo/net/EventLoop.hh>
#include <karabo/log/Logger.hh>

USING_KARABO_NAMESPACES;

CPPUNIT_TEST_SUITE_REGISTRATION(PipelinedProcessing_Test);

#define KRB_TEST_MAX_TIMEOUT 20


PipelinedProcessing_Test::PipelinedProcessing_Test() {

}


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
    m_deviceServer.reset();

    EventLoop::stop();
    m_eventLoopThread.join();    
}


void PipelinedProcessing_Test::appTestRunner() {

    // in order to avoid recurring setup and tear down calls, all tests are run in a single runner
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerPP", "P2PSenderDevice", Hash("deviceId", "p2pTestSender"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    testGetOutputChannelSchema();
    // Order of the next three matters :-( :
    // testPipe instantiates "pipeTestReceiver",
    // testProfileTransferTimes(false) makes use of its existence,
    // and testProfileTransferTimes(true) kills it and instantiates a "pipeTestReceiver2" device instead.
    testPipe();
    testProfileTransferTimes(false); // allows local short-cutting on PP
    testProfileTransferTimes(true); // forces PP data to go via loopback TCP
}


void PipelinedProcessing_Test::testGetOutputChannelSchema() {
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
}


void PipelinedProcessing_Test::testPipe() {

    const std::string receiver("pipeTestReceiver");
    const Hash cfg("deviceId", receiver, "processingTime", 0,
                   "input.connectedOutputChannels", "p2pTestSender:output1",
                   "input2.connectedOutputChannels", "p2pTestSender:output2");

    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerPP", "PipeReceiverDevice",
                                                                       cfg, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    const unsigned int nDataPerRun = m_deviceClient->get<unsigned int>("p2pTestSender", "nData");

    int64_t elapsedTimeIn_microseconds = 0ll;
    const unsigned int nRuns = 10;
    for (unsigned int nRun = 0; nRun < nRuns; ++nRun) {

        const auto startTimepoint = std::chrono::high_resolution_clock::now();
        // Then call its slot
        m_deviceClient->execute("p2pTestSender", "write", KRB_TEST_MAX_TIMEOUT);

        // And poll for the correct answer
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(receiver, "nTotalData", nDataPerRun * (nRun + 1), KRB_TEST_MAX_TIMEOUT));
        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        // Note that duration contains overhead from message travel time and polling interval in pollDeviceProperty!
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        // Check that EOS handling is not called too early
        // (EOS comes a bit later, so we have to poll client again to be sure...)
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(receiver, "nTotalOnEos", nDataPerRun * (nRun + 1), KRB_TEST_MAX_TIMEOUT));

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
    unsigned int nTotalOnEos = m_deviceClient->get<unsigned int>(receiver, "nTotalOnEos");
    double mbps = double(dataItemSize) * double(nTotalOnEos) / double(elapsedTimeIn_microseconds);
    // Note that this measurement checks the inner-process shortcut - and includes timing overhead e.g. pollDeviceProperty
    std::clog << "testPipe: Megabytes per sec : " << mbps
            << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds
            << ", dataItemSize = " << dataItemSize << ", nTotalOnEos=" << nTotalOnEos << std::endl;
    CPPUNIT_ASSERT_EQUAL(nDataPerRun * nRuns, nTotalOnEos);
}


void PipelinedProcessing_Test::testProfileTransferTimes(bool noShortCut) {
    std::string receiver = "pipeTestReceiver";
    if(noShortCut) {

        setenv("KARABO_NO_PIPELINE_SHORTCUT", "1", 1);

        // Looks like to get "KARABO_NO_PIPELINE_SHORTCUT" active (some caching?),
        // we have to re-instantiate the receiver.
        // Problems with bad caching in the DeviceClient makes the test safer if we use a new deviceId...
        std::pair<bool, std::string> success = m_deviceClient->killDevice(receiver, KRB_TEST_MAX_TIMEOUT);
        CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

        receiver = "pipeTestReceiver2";

        const Hash cfg("deviceId", receiver, "processingTime", 0,
                   "input.connectedOutputChannels", "p2pTestSender:output1",
                   "input2.connectedOutputChannels", "p2pTestSender:output2");
        success = m_deviceClient->instantiate("testServerPP", "PipeReceiverDevice", cfg, KRB_TEST_MAX_TIMEOUT);
        CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    }

    m_deviceClient->execute(receiver, "reset", KRB_TEST_MAX_TIMEOUT);
    m_deviceClient->set(receiver, "processingTime", 100);

    const unsigned int nDataPerRun = m_deviceClient->get<unsigned int>("p2pTestSender", "nData");

    // set the scenario
    m_deviceClient->set("p2pTestSender", "scenario", "profile");
    // Then call its slot
    m_deviceClient->execute("p2pTestSender", "write", KRB_TEST_MAX_TIMEOUT);
    
    // And poll for the correct answer
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(receiver, "nTotalData", nDataPerRun, KRB_TEST_MAX_TIMEOUT));

    pollDeviceProperty<float>(receiver, "averageTransferTime", 0.f, KRB_TEST_MAX_TIMEOUT, false); // until not zero anymore!
    float transferTime = m_deviceClient->get<float>(receiver, "averageTransferTime") / 1000;

    std::clog << "testProfileTransferTimes (copy, " << (noShortCut ? "no short cut" : "short cut")
            << "): " << transferTime << " milliseconds average transfer time" << std::endl;

    // Reset receiver and ask sender "not to copy"
    m_deviceClient->execute(receiver, "reset", KRB_TEST_MAX_TIMEOUT);
    m_deviceClient->set("p2pTestSender", "copyAllData", false);
    
    // Then call its slot
    m_deviceClient->execute("p2pTestSender", "write", KRB_TEST_MAX_TIMEOUT);
    
    // And poll for the correct answer
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(receiver, "nTotalData", nDataPerRun, KRB_TEST_MAX_TIMEOUT));

    pollDeviceProperty<float>(receiver, "averageTransferTime", 0.f, KRB_TEST_MAX_TIMEOUT, false); // until not zero anymore!
    float transferTime2 = m_deviceClient->get<float>(receiver, "averageTransferTime") / 1000;

    std::clog << "testProfileTransferTimes (no copy, " << (noShortCut ? "no short cut" : "short cut")
            << "): " << transferTime2 << " milliseconds average transfer time" << std::endl;

    if(noShortCut) {
        unsetenv("KARABO_NO_PIPELINE_SHORTCUT");
    }
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
