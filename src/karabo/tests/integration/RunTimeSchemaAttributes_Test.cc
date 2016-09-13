/* 
 * File:   RunTimeSchemaAttributes.cc
 * Author: steffen
 * 
 * Created on September 13, 2016, 7:10 PM
 */

#include "RunTimeSchemaAttributes_Test.hh"


USING_KARABO_NAMESPACES;

CPPUNIT_TEST_SUITE_REGISTRATION(RunTimeSchemaAttributes_Test);

#define KRB_TEST_MAX_TIMEOUT 10



RunTimeSchemaAttributes_Test::RunTimeSchemaAttributes_Test() {

}


RunTimeSchemaAttributes_Test::~RunTimeSchemaAttributes_Test() {
  
}



void RunTimeSchemaAttributes_Test::setUp() {
    setenv("KARABO_BROKER_HOST", "localhost", 1);
    setenv("KARABO_BROKER_HOSTS", "localhost", 1);
    Hash config("DeviceServer", Hash("serverId", "testServerSchema", "scanPlugins", false, "visibility", 4, "Logger.priority", "DEBUG"));
    m_deviceServer = boost::shared_ptr<DeviceServer>(DeviceServer::create(config));
    m_deviceServerThread = boost::thread(&DeviceServer::run, m_deviceServer);
    Hash configClient();
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());
    
  
    
}



void RunTimeSchemaAttributes_Test::tearDown() {
    m_deviceClient->killServer("testServerSchema", KRB_TEST_MAX_TIMEOUT);
    m_deviceServerThread.join();
    

}

void RunTimeSchemaAttributes_Test::appTestRunner() {
    // in order to avoid recurring setup and tear down call all tests are run in a single runner

    std::vector<karabo::util::Hash> schemaUpdates;
    schemaUpdates.push_back(Hash("path", "floatProperty", "updateType", "setWarnLow", "updatedValue", -100.0));
    schemaUpdates.push_back(Hash("path", "floatProperty", "updateType", "setMinInc", "updatedValue", -100.0));
    schemaUpdates.push_back(Hash("path", "floatProperty", "updateType", "setAlarmHigh", "updatedValue", 1000.0));
    schemaUpdates.push_back(Hash("path", "floatProperty2", "updateType", "setAlarmLow", "updatedValue", 10.0));
    //note that if the schema were not to be updated by the runtime configuration this device should not init.
    //its value is configured to be below the minInc range. Only the update adjusts that.
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerSchema", "AlarmTester", Hash("deviceId", "alarmTesterSchema", "runtimeSchemaUpdates", schemaUpdates, "floatProperty", -1.), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
    boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
    
    testApplicationOnInit();
    testRuntimeApplication();

}

void RunTimeSchemaAttributes_Test::testApplicationOnInit(){
    //this should resemble our updated values
    const karabo::util::Schema& s = m_deviceClient->getDeviceSchema("alarmTesterSchema");
    CPPUNIT_ASSERT(s.getWarnLow<float>("floatProperty") == -100.0);
    CPPUNIT_ASSERT(s.getAlarmHigh<float>("floatProperty") == 1000);
    CPPUNIT_ASSERT(s.getAlarmLow<float>("floatProperty2") == 10.0);
    
}

void RunTimeSchemaAttributes_Test::testRuntimeApplication(){
    std::vector<karabo::util::Hash> schemaUpdates;
    schemaUpdates.push_back(Hash("path", "floatProperty", "updateType", "setWarnLow", "updatedValue", -1000.0));
    //register a dummy monitor to assure that signals from the device are tracked
    schemaUpdates.push_back(Hash("path", "floatProperty", "updateType", "setMinInc", "updatedValue", -10.0));
    m_deviceClient->registerDeviceMonitor("alarmTesterSchema", boost::bind(&RunTimeSchemaAttributes_Test::dummyMonitor, this, _1, _2));
    std::pair<bool, std::string> success = m_deviceClient->execute("alarmTesterSchema", "slotUpdateSchemaAttributes", schemaUpdates, KRB_TEST_MAX_TIMEOUT);
    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
    const karabo::util::Schema& s = m_deviceClient->getDeviceSchema("alarmTesterSchema");
    
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(s.getWarnLow<float>("floatProperty") == -1000.0);
    CPPUNIT_ASSERT(s.getMinInc<float>("floatProperty") == -10.0);
}

void RunTimeSchemaAttributes_Test::dummyMonitor(const std::string&, const karabo::util::Hash&){
    
};