/*
 * File:   GetOutputChannelSchema_Test.cc
 * Author: haufs
 *
 * Created on Sep 20, 2016, 3:40:34 PM
 */

#include "PipelinedProcessing_Test.hh"
#include <karabo/net/EventLoop.hh>

USING_KARABO_NAMESPACES;

CPPUNIT_TEST_SUITE_REGISTRATION(PipelinedProcessing_Test);

#define KRB_TEST_MAX_TIMEOUT 10

PipelinedProcessing_Test::PipelinedProcessing_Test() {

}


void PipelinedProcessing_Test::setUp() {
    Hash config("DeviceServer", Hash("serverId", "testServerPP", "scanPlugins", false, "visibility", 4, "Logger.priority", "ERROR"));
    m_deviceServer = boost::shared_ptr<DeviceServer>(DeviceServer::create(config));
    m_deviceServerThread = boost::thread(&DeviceServer::run, m_deviceServer);
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());

}


void PipelinedProcessing_Test::tearDown() {
    m_deviceClient->killServer("testServerPP", KRB_TEST_MAX_TIMEOUT);
    m_deviceServerThread.join();
    m_deviceClient.reset();
}


void PipelinedProcessing_Test::appTestRunner() {

    // Start central event-loop
    boost::thread t(boost::bind(&EventLoop::work));

    // in order to avoid recurring setup and tear down calls, all tests are run in a single runner
    std::pair<bool, std::string> success =  m_deviceClient->instantiate("testServerPP", "P2PSenderDevice", Hash("deviceId", "p2pTestSender"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    testGetOutputChannelSchema();
    testPipe();

    EventLoop::stop();
    t.join();
}


void PipelinedProcessing_Test::testGetOutputChannelSchema(){
    karabo::util::Hash dataSchema =  m_deviceClient->getOutputChannelSchema("p2pTestSender", "output1");

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

    const Hash cfg("deviceId", "pipeTestReceiver", "processingTime", 100,
                   "input.connectedOutputChannels", "p2pTestSender:output1");

    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerPP", "PipeReceiverDevice",
                                                                       cfg, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    // Ask for a property of the device to wait until it is available
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>("p2pTestSender", "nData", 5, KRB_TEST_MAX_TIMEOUT));

    // Then call its slot
    m_deviceClient->execute("p2pTestSender", "write", KRB_TEST_MAX_TIMEOUT);

    // And poll for the correct answer
    CPPUNIT_ASSERT(pollDeviceProperty<unsigned int>("pipeTestReceiver", "nTotalData", 5, KRB_TEST_MAX_TIMEOUT));
    
    // Test if data source was correctly passed
    std::vector<std::string> sources = m_deviceClient->get<std::vector<std::string> >("pipeTestReceiver", "dataSources");
    CPPUNIT_ASSERT(sources[0] == "p2pTestSender/output1");
    
    // This only can be tested if we used an input handler and not onData
    if (!m_deviceClient->get<bool>("pipeTestReceiver", "onData")) {
        std::vector<std::string> sources = m_deviceClient->get<std::vector<std::string> >("pipeTestReceiver", "dataSourcesFromIndex");
        CPPUNIT_ASSERT(sources[0] == "p2pTestSender/output1");
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
