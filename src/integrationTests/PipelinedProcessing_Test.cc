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

    Hash config2("serverId", "testServerPP2", "scanPlugins", false, "Logger.priority", "ERROR");
    m_deviceServer2 = DeviceServer::create("DeviceServer", config2);
    m_deviceServer2->finalizeInternalInitialization();

    // Create client
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());
}


void PipelinedProcessing_Test::tearDown() {
    m_deviceServer.reset();
    m_deviceServer2.reset();
    EventLoop::stop();
    m_eventLoopThread.join();    
}


void PipelinedProcessing_Test::appTestRunner() {

    // in order to avoid recurring setup and tear down calls, all tests are run in a single runner
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerPP", "P2PSenderDevice", Hash("deviceId", "p2pTestSender"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    testGetOutputChannelSchema();
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

    const Hash cfg("deviceId", "pipeTestReceiver", "processingTime", 0,
                   "input.connectedOutputChannels", "p2pTestSender:output1",
                   "input2.connectedOutputChannels", "p2pTestSender:output2");

    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerPP2", "PipeReceiverDevice",
                                                                       cfg, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    unsigned int n = 0;
    auto startTimepoint = std::chrono::high_resolution_clock::now();
    unsigned int nTotalOnEos = 0;
    
    for(; n < 10; ++n) {
        // Ask for a property of the device to wait until it is available
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>("p2pTestSender", "nData", 12, KRB_TEST_MAX_TIMEOUT));

        // Then call its slot
        m_deviceClient->execute("p2pTestSender", "write", KRB_TEST_MAX_TIMEOUT);

        // And poll for the correct answer
        CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>("pipeTestReceiver", "nTotalData", 12*(n+1), KRB_TEST_MAX_TIMEOUT));

        // Test if data source was correctly passed
        std::vector<std::string> sources = m_deviceClient->get<std::vector<std::string> >("pipeTestReceiver", "dataSources");
        CPPUNIT_ASSERT(sources[0] == "p2pTestSender:output1");

        // This only can be tested if we used an input handler and not onData
        if (!m_deviceClient->get<bool>("pipeTestReceiver", "onData")) {
            std::vector<std::string> sources = m_deviceClient->get<std::vector<std::string> >("pipeTestReceiver", "dataSourcesFromIndex");
            CPPUNIT_ASSERT(sources[0] == "p2pTestSender:output1");
        }

        // Check that EOS handling is not called too early
        nTotalOnEos = m_deviceClient->get<unsigned int>("pipeTestReceiver", "nTotalOnEos");
        CPPUNIT_ASSERT_EQUAL(12u*(n+1), nTotalOnEos);
    }

    const unsigned int dataItemSize = m_deviceClient->get<unsigned int>("pipeTestReceiver", "dataItemSize");
    auto endTimepoint = std::chrono::high_resolution_clock::now();
    auto dur = endTimepoint - startTimepoint;
    int64_t elapsedTimeIn_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
    double mbps = double(dataItemSize) * double(nTotalOnEos) / double(elapsedTimeIn_microseconds);
    KARABO_LOG_FRAMEWORK_INFO_C("PipelinedProcessing_Test") << "Megabytes per sec : " << mbps
            << ", elapsedTimeIn_microseconds = " << elapsedTimeIn_microseconds
            << ", dataItemSize = " << dataItemSize << ", nTotalOnEos=" << nTotalOnEos;
    CPPUNIT_ASSERT_EQUAL(120u, nTotalOnEos);
}

void PipelinedProcessing_Test::testProfileTransferTimes(bool noShortCut) {
    std::string receiver = "pipeTestReceiver";
    if(noShortCut) {
        
        putenv("KARABO_NO_PIPELINE_SHORTCUT=1");

        const Hash cfg("deviceId", "pipeTestReceiver2", "processingTime", 0,
                   "input.connectedOutputChannels", "p2pTestSender:output1",
                   "input2.connectedOutputChannels", "p2pTestSender:output2");

        std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerPP2", "PipeReceiverDevice",
                                                                           cfg, KRB_TEST_MAX_TIMEOUT);
        CPPUNIT_ASSERT(success.first);
        receiver = "pipeTestReceiver2";
    }

    m_deviceClient->execute(receiver, "reset", KRB_TEST_MAX_TIMEOUT);
    m_deviceClient->set(receiver, "processingTime", 100);
    

    // Ask for a property of the device to wait until it is available
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>("p2pTestSender", "nData", 12, KRB_TEST_MAX_TIMEOUT));

    // set the scenario
    m_deviceClient->set("p2pTestSender", "scenario", "profile");
    // Then call its slot
    m_deviceClient->execute("p2pTestSender", "write", KRB_TEST_MAX_TIMEOUT);
    
    // And poll for the correct answer
    
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(receiver, "nTotalData", 132-(int)noShortCut*120, KRB_TEST_MAX_TIMEOUT));
    
    float transferTime = m_deviceClient->get<float>(receiver, "averageTransferTime") / 1000;

    KARABO_LOG_FRAMEWORK_INFO_C("PipelinedProcessing_Test") << "Average transfer time (copy) is "<< transferTime << " milliseconds";

    // Reset receiver and ask sender "not to copy"
    m_deviceClient->execute(receiver, "reset", KRB_TEST_MAX_TIMEOUT);
    m_deviceClient->set("p2pTestSender", "copyAllData", false);
    
    // Then call its slot
    m_deviceClient->execute("p2pTestSender", "write", KRB_TEST_MAX_TIMEOUT);
    
    // And poll for the correct answer
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>(receiver, "nTotalData", 144-(int)noShortCut*120, KRB_TEST_MAX_TIMEOUT));
    
    float transferTime2 = m_deviceClient->get<float>(receiver, "averageTransferTime") / 1000;
    
    KARABO_LOG_FRAMEWORK_INFO_C("PipelinedProcessing_Test") << "Average transfer time (no copy) is " << transferTime2 << " milliseconds";

    if(!noShortCut) {
        CPPUNIT_ASSERT(transferTime2 < 1); //should take less than a ms.
    } else {
        putenv("KARABO_NO_PIPELINE_SHORTCUT=");
    }
}



template <typename T>
bool PipelinedProcessing_Test::pollDeviceProperty(const std::string& deviceId,
                                                  const std::string& propertyName, const T& expected, const int maxTimeout) const {

    const int pollWaitTime = 1;
    int pollCounter = 0;

    // Poll the device until it responds with the correct answer or times out.
    while (pollWaitTime * pollCounter <= maxTimeout) {
        boost::this_thread::sleep(boost::posix_time::seconds(pollWaitTime));
        const T nReceived = m_deviceClient->get<T>(deviceId, propertyName);
        if (nReceived == expected) {
            return true;
        }
        ++pollCounter;
    }

    return false;
}
