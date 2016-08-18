/*
 * File:   AlarmService_Test.cc
 * Author: haufs
 *
 * Created on Aug 8, 2016, 3:22:00 PM
 */

#include "AlarmService_Test.hh"
#include <sys/wait.h>

USING_KARABO_NAMESPACES;

CPPUNIT_TEST_SUITE_REGISTRATION(AlarmService_Test);

#define KRB_TEST_MAX_TIMEOUT 10



AlarmService_Test::AlarmService_Test() {

}


AlarmService_Test::~AlarmService_Test() {
  
}



void AlarmService_Test::setUp() {
    
    Hash config("DeviceServer", Hash("serverId", "testServer", "scanPlugins", false, "visibility", 4, "Logger.priority", "DEBUG"));
    m_deviceServer = boost::shared_ptr<DeviceServer>(DeviceServer::create(config));
    m_deviceServerThread = boost::thread(&DeviceServer::run, m_deviceServer);
    Hash configClient();
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());
    
}



void AlarmService_Test::tearDown() {
    m_deviceClient->killServer("testServer", KRB_TEST_MAX_TIMEOUT);
    m_deviceServerThread.join();
}

void AlarmService_Test::appTestRunner() {
    // in order to avoid recurring setup and tear down call all tests are run in a single runner
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServer", "AlarmService", Hash("deviceId", "testAlarmService"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    success = m_deviceClient->instantiate("testServer", "AlarmTester", Hash("deviceId", "alarmTester"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
    testDeviceRegistration();
    testAlarmPassing();
    testAcknowledgement();
}

void AlarmService_Test::testDeviceRegistration() {
    // test whether the device actually registers with the alarm service upon instanceNew
    std::vector<std::string> registeredDevices = m_deviceClient->get<std::vector<std::string> >("testAlarmService", "registeredDevices");
    CPPUNIT_ASSERT(registeredDevices[0] == "alarmTester");
    
}


void AlarmService_Test::testAlarmPassing() {
    // test if raising an alarm on alarmTester propagates to testAlarmService and updates the alarmTable there
    std::pair<bool, std::string> success;
    success = m_deviceClient->execute("alarmTester", "triggerAlarmHigh", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "triggeredAlarmHigh");
    
    //allow some time for update to propagate
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    
    std::vector<Hash> alarmTable = m_deviceClient->get<std::vector<Hash> >("testAlarmService", "currentAlarms");
    CPPUNIT_ASSERT(alarmTable.size() == 1);
    Hash h = alarmTable[0];
    //these should be the same as it is the first time the alarm is raised
    CPPUNIT_ASSERT(h.get<std::string>("timeOfOccurrence") == h.get<std::string>("timeOfFirstOccurrence"));
    CPPUNIT_ASSERT(h.get<unsigned long long>("trainOfOccurrence") == h.get<unsigned long long>("trainOfFirstOccurrence"));
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmHigh");
    CPPUNIT_ASSERT(h.get<std::string>("description") == "A description for alarmHigh");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);
    CPPUNIT_ASSERT(h.get<bool>("acknowledged") == false);
    
    //go out of the alarm state
    success = m_deviceClient->execute("alarmTester", "triggerNormal", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "triggeredNormal");
    
    //allow some time for update to propagate
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

    alarmTable = m_deviceClient->get<std::vector<Hash> >("testAlarmService", "currentAlarms");
    CPPUNIT_ASSERT(alarmTable.size() == 1);
    h = alarmTable[0];
    CPPUNIT_ASSERT(h.get<std::string>("timeOfOccurrence") == h.get<std::string>("timeOfFirstOccurrence"));
    CPPUNIT_ASSERT(h.get<unsigned long long>("trainOfOccurrence") == h.get<unsigned long long>("trainOfFirstOccurrence"));
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmHigh");
    CPPUNIT_ASSERT(h.get<std::string>("description") == "A description for alarmHigh");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    //the alarm should now be acknowledgable
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledged") == false);
    
   
    //retrigger the alarm
    success = m_deviceClient->execute("alarmTester", "triggerAlarmHigh", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "triggeredAlarmHigh");
    
    //allow some time for update to propagate
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    
    //now occurrences should mismatch and alarm should not be acknowledgeable
    alarmTable = m_deviceClient->get<std::vector<Hash> >("testAlarmService", "currentAlarms");
    CPPUNIT_ASSERT(alarmTable.size() == 1);
    h = alarmTable[0];
    CPPUNIT_ASSERT(h.get<std::string>("timeOfOccurrence") != h.get<std::string>("timeOfFirstOccurrence"));
    //not testing on train id, as it will be 0 in both cases without use of a time server
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmHigh");
    CPPUNIT_ASSERT(h.get<std::string>("description") == "A description for alarmHigh");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    //the alarm should now not be acknowledgeable anymore
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);
    CPPUNIT_ASSERT(h.get<bool>("acknowledged") == false);
    
}

void AlarmService_Test::testAcknowledgement() {
    std::pair<bool, std::string> success;
    std::vector<Hash> alarmTable;
    
    //add another alarm to the table so we have two alarms pending
    //we will work only on the first one afterwards
    //now we go out of the alarm state, acknowledging should now be possible
    success = m_deviceClient->execute("alarmTester", "triggerWarnHigh2", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "triggeredWarnHigh2");
    
    //allow some time for update to propagate
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    
    //first test if we cannot acknowledge an not acknowledgeable alarm.
    //the alarm service should be in this state after the previous test.
    alarmTable = m_deviceClient->get<std::vector<Hash> >("testAlarmService", "currentAlarms");
    CPPUNIT_ASSERT(alarmTable.size() == 2);
    alarmTable[0].set("acknowledged", true);
    success = m_deviceClient->set("testAlarmService", "currentAlarms", alarmTable, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
    //the table should still be as was and acknowledged is set to false
    alarmTable = m_deviceClient->get<std::vector<Hash> >("testAlarmService", "currentAlarms");
    CPPUNIT_ASSERT(alarmTable.size() == 2);
    Hash h = alarmTable[0];
    CPPUNIT_ASSERT(h.get<std::string>("timeOfOccurrence") != h.get<std::string>("timeOfFirstOccurrence"));
    //not testing on train id, as it will be 0 in both cases without use of a time server
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmHigh");
    CPPUNIT_ASSERT(h.get<std::string>("description") == "A description for alarmHigh");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    //the alarm should now not be acknowledgeable anymore
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);
    CPPUNIT_ASSERT(h.get<bool>("acknowledged") == false);
    
    //now we go out of the alarm state, acknowledging should now be possible
    success = m_deviceClient->execute("alarmTester", "triggerNormal", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "triggeredNormal");
    
    //allow some time for update to propagate
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    
    //acknowledge alarm
    alarmTable = m_deviceClient->get<std::vector<Hash> >("testAlarmService", "currentAlarms");
    CPPUNIT_ASSERT(alarmTable.size() == 2);
    alarmTable[0].set("acknowledged", true);
    success = m_deviceClient->set("testAlarmService", "currentAlarms", alarmTable, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
    //alarm table entry should be deleted
    alarmTable = m_deviceClient->get<std::vector<Hash> >("testAlarmService", "currentAlarms");
    CPPUNIT_ASSERT(alarmTable.size() == 1);
    
    h = alarmTable[0];
    CPPUNIT_ASSERT(h.get<std::string>("timeOfOccurrence") == h.get<std::string>("timeOfFirstOccurrence"));
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty2");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "warnHigh");

}
#undef KRB_TEST_MAX_TIMEOUT

