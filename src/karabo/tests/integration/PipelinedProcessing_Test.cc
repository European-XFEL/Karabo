/*
 * File:   GetOutputChannelSchema_Test.cc
 * Author: haufs
 *
 * Created on Sep 20, 2016, 3:40:34 PM
 */

#include "PipelinedProcessing_Test.hh"


USING_KARABO_NAMESPACES;

CPPUNIT_TEST_SUITE_REGISTRATION(PipelinedProcessing_Test);

#define KRB_TEST_MAX_TIMEOUT 10



PipelinedProcessing_Test::PipelinedProcessing_Test() {

}


PipelinedProcessing_Test::~PipelinedProcessing_Test() {
  
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
    //add a few threads to the event loop
    
    
    // in order to avoid recurring setup and tear down call all tests are run in a single runner
    std::pair<bool, std::string> success =  m_deviceClient->instantiate("testServerPP", "P2PSenderDevice", Hash("deviceId", "p2pTestSender"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    testGetOutputChannelSchema();
   
}



void PipelinedProcessing_Test::testGetOutputChannelSchema(){
    karabo::util::Hash dataSchema =  m_deviceClient->getOutputChannelSchema("p2pTestSender", "output1");
    
    CPPUNIT_ASSERT(dataSchema.has("dataId"));
    CPPUNIT_ASSERT(dataSchema.getType("dataId") == karabo::util::Types::INT32);
    CPPUNIT_ASSERT(dataSchema.has("sha1"));
    CPPUNIT_ASSERT(dataSchema.getType("sha1") == karabo::util::Types::STRING);
    CPPUNIT_ASSERT(dataSchema.has("data"));
    CPPUNIT_ASSERT(dataSchema.getType("data") == karabo::util::Types::VECTOR_INT64);
    CPPUNIT_ASSERT(dataSchema.has("array"));
    CPPUNIT_ASSERT(dataSchema.hasAttribute("array", KARABO_HASH_CLASS_ID));
    CPPUNIT_ASSERT(dataSchema.getAttribute<std::string>("array", KARABO_HASH_CLASS_ID) == "NDArray");
    const karabo::util::NDArray& arr = dataSchema.get<karabo::util::NDArray>("array");
    CPPUNIT_ASSERT(arr.getType() == karabo::util::Types::DOUBLE);
    CPPUNIT_ASSERT(arr.getShape() == karabo::util::Dims(100,200,0));
    CPPUNIT_ASSERT(arr.isBigEndian() == false);            
}


