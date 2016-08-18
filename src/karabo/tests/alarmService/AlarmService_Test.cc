/*
 * File:   AlarmService_Test.cc
 * Author: haufs
 *
 * Created on Aug 8, 2016, 3:22:00 PM
 */

#include "AlarmService_Test.hh"
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include "karabo/io/TextSerializer.hh"
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>

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
    
    //unlink persisted alarms if they exist
    if (boost::filesystem::exists("./testAlarmService.xml") ){
        boost::filesystem::remove("./testAlarmService.xml");
    }
    
}



void AlarmService_Test::tearDown() {
    m_deviceClient->killServer("testServer", KRB_TEST_MAX_TIMEOUT);
    m_deviceServerThread.join();
    
    //unlink persisted alarms if they exist
    if (boost::filesystem::exists("./testAlarmService.xml") ){
        boost::filesystem::remove("./testAlarmService.xml");
    }
}

void AlarmService_Test::appTestRunner() {
    // in order to avoid recurring setup and tear down call all tests are run in a single runner
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServer", "AlarmService", Hash("deviceId", "testAlarmService", "flushInterval", 1), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    success = m_deviceClient->instantiate("testServer", "AlarmTester", Hash("deviceId", "alarmTester"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
    testDeviceRegistration();
    testAlarmPassing();
    testAcknowledgement();
    testFlushing();
    testRecovery();
    testDeviceKilled();
    testDeviceReappeared();
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
    CPPUNIT_ASSERT(h.get<std::string>("instanceId") == "alarmTester");
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
    CPPUNIT_ASSERT(h.get<std::string>("instanceId") == "alarmTester");
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
    CPPUNIT_ASSERT(h.get<std::string>("instanceId") == "alarmTester");
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
    CPPUNIT_ASSERT(h.get<std::string>("instanceId") == "alarmTester");
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
    CPPUNIT_ASSERT(h.get<std::string>("instanceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty2");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "warnHigh");
    
    

}

void AlarmService_Test::testFlushing(){
    //we should find a flushed file in the working directory
    //assure the flushed file resembles the current state

    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
    boost::interprocess::file_lock flock("./testAlarmService.xml");
    {
        boost::interprocess::sharable_lock<boost::interprocess::file_lock> shlock(flock);
        std::ifstream fin;
        fin.open("./testAlarmService.xml");
        std::ostringstream archive;
        std::string input;
        while(fin>>input) archive<<input<<std::endl;
        fin.close();
        TextSerializer<Hash>::Pointer serializer = TextSerializer<Hash>::create("Xml");
        Hash h;
        serializer->load(h, archive.str());


        //should be the same as what we currently hold in the table and devices sections
        std::vector<std::string> registeredDevices = m_deviceClient->get<std::vector<std::string> >("testAlarmService", "registeredDevices");

        Hash propHash;
        propHash.set("type", "warnHigh");
        propHash.set("description", "A description for alarmHigh");
        propHash.set("needsAcknowledging", true);
        propHash.set<unsigned long long>("trainOfFirstOccurrence", 0);
        propHash.set<unsigned long long>("trainOfOccurrence", 0);
        propHash.set("acknowledgeable", false);
        Hash alarmHash("alarmTester", Hash("floatProperty2", Hash("warnHigh", propHash)));
        Hash hTest("devices", registeredDevices, "alarms", alarmHash);
        
        //erase the occurance times, as the will not match!
        h.erase("alarms.alarmTester.floatProperty2.warnHigh.timeOfFirstOccurrence");
        h.erase("alarms.alarmTester.floatProperty2.warnHigh.timeOfOccurrence");

        CPPUNIT_ASSERT(karabo::util::similar(h, hTest));
    }
    
}
    
void AlarmService_Test::testRecovery(){
    //first we bring down the alarm service.
    //at this state it should hold an warnHigh for floatProperty2 which cannot be acknowledged
    std::pair<bool, std::string> success = m_deviceClient->killDevice("testAlarmService", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    //know we raise an alarm on floatProperty again and ma floatProperty2 acknowledgeable
    success = m_deviceClient->execute("alarmTester", "triggerNormal2", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "triggeredNormal2");

    success = m_deviceClient->execute("alarmTester", "triggerAlarmHigh", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "triggeredAlarmHigh");
    
    //bring up a previously unknown device
    success = m_deviceClient->instantiate("testServer", "AlarmTester", Hash("deviceId", "alarmTester2"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    //trigger an alarm
    success = m_deviceClient->execute("alarmTester2", "triggerAlarmLow", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "triggeredAlarmLow");

    //now we bring the alarm service back up
    success = m_deviceClient->instantiate("testServer", "AlarmService", Hash("deviceId", "testAlarmService", "flushInterval", 1), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    //we give some time for message passing
    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));

    //alarmState should now be an alarm for floatProperty and floatProperty2 acknowledgeable
    const std::vector<Hash>& alarmTable = m_deviceClient->get<std::vector<Hash> >("testAlarmService", "currentAlarms");
    CPPUNIT_ASSERT(alarmTable.size() == 3);
    Hash h = alarmTable[0];

    CPPUNIT_ASSERT(h.get<std::string>("instanceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty2");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "warnHigh");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledged") == false);

    h = alarmTable[1];

    CPPUNIT_ASSERT(h.get<std::string>("instanceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmHigh");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);
    CPPUNIT_ASSERT(h.get<bool>("acknowledged") == false);
    
    h = alarmTable[2];

    CPPUNIT_ASSERT(h.get<std::string>("instanceId") == "alarmTester2");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmLow");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);



}

void AlarmService_Test::testDeviceKilled(){
    //kill device, alarms should become acknowledgeable
    std::pair<bool, std::string> success = m_deviceClient->killDevice("alarmTester2", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
    //we give some time so that device server notifies of death of device
    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
    
    //alarmState should now be an alarm for floatProperty2 and floatProperty2 acknowledgeable
    const std::vector<Hash>& alarmTable = m_deviceClient->get<std::vector<Hash> >("testAlarmService", "currentAlarms");
    CPPUNIT_ASSERT(alarmTable.size() == 3);
    
    Hash h = alarmTable[2];
    
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester2");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmLow");
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == true);
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
}

void AlarmService_Test::testDeviceReappeared(){
    //bring up a previously unknown device
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServer", "AlarmTester", Hash("deviceId", "alarmTester2", "floatProperty", -5.), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
    //we give some time so that device server notifies of death of device
    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
    
    //alarmState should now be an alarm for floatProperty2 and floatProperty2 not acknowledgeable anymore
    const std::vector<Hash>& alarmTable = m_deviceClient->get<std::vector<Hash> >("testAlarmService", "currentAlarms");
    CPPUNIT_ASSERT(alarmTable.size() == 3);
    
    Hash h = alarmTable[2];
    
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester2");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmLow");
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
}
    

#undef KRB_TEST_MAX_TIMEOUT

