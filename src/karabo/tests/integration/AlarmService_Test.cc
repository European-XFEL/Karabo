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
    
    Hash config("DeviceServer", Hash("serverId", "testServer", "scanPlugins", false, "visibility", 4/*, "Logger.priority", "DEBUG"*/));
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
    //add a few threads to the event loop
    EventLoop::addThread(4);
    boost::asio::io_service::work work(EventLoop::getIOService());
    boost::thread t(boost::bind(&EventLoop::run));
    
    // in order to avoid recurring setup and tear down call all tests are run in a single runner
    // here we start the server and service devices, as well as an alarm test device
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServer", "GuiServerDevice", Hash("deviceId", "testGuiServer", "port", 44446), KRB_TEST_MAX_TIMEOUT);
    boost::this_thread::sleep(boost::posix_time::milliseconds(4000));
    m_tcpAdapter = boost::shared_ptr<karabo::TcpAdapter>(new karabo::TcpAdapter(Hash("port", 44446u/*, "debug", true*/)));
    boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    success = m_deviceClient->instantiate("testServer", "AlarmService", Hash("deviceId", "testAlarmService", "flushInterval", 1), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
    success = m_deviceClient->instantiate("testServer", "AlarmTester", Hash("deviceId", "alarmTester"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
    // the actual tests
    testDeviceRegistration();
    testAlarmPassing();
    testAcknowledgement();
    testFlushing();
    testRecovery();
    testDeviceKilled();
    testDeviceReappeared();
    EventLoop::stop();
}

void AlarmService_Test::testDeviceRegistration() {
    //we give a bit of time for updates to propagate.
    boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
    
    // test whether the device actually registers with the alarm service upon instanceNew
    std::vector<std::string> registeredDevices = m_deviceClient->get<std::vector<std::string> >("testAlarmService", "registeredDevices");
    CPPUNIT_ASSERT(registeredDevices[0] == "alarmTester");
    
}


void AlarmService_Test::testAlarmPassing() {
    // test if raising an alarm on alarmTester propagates to testAlarmService and updates the alarmTable there
    std::pair<bool, std::string> success;
    
    auto messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&]{success = m_deviceClient->execute("alarmTester", "triggerAlarmHigh", KRB_TEST_MAX_TIMEOUT);});
    Hash lastMessage;
    messageQ->pop(lastMessage);
    
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "triggeredAlarmHigh");
    
    CPPUNIT_ASSERT(lastMessage.has("rows.0.add"));
    Hash h = lastMessage.get<Hash>("rows.0.add");
    //these should be the same as it is the first time the alarm is raised
    CPPUNIT_ASSERT(h.get<std::string>("timeOfOccurrence") == h.get<std::string>("timeOfFirstOccurrence"));
    CPPUNIT_ASSERT(h.get<unsigned long long>("trainOfOccurrence") == h.get<unsigned long long>("trainOfFirstOccurrence"));
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmHigh");
    CPPUNIT_ASSERT(h.get<std::string>("description") == "A description for alarmHigh");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);

    
    //go out of the alarm state
    messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&]{success = m_deviceClient->execute("alarmTester", "triggerNormal", KRB_TEST_MAX_TIMEOUT);});
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "triggeredNormal");

    CPPUNIT_ASSERT(lastMessage.has("rows.0.acknowledgeable"));
    h = lastMessage.get<Hash>("rows.0.acknowledgeable");
    
    CPPUNIT_ASSERT(h.get<std::string>("timeOfOccurrence") == h.get<std::string>("timeOfFirstOccurrence"));
    CPPUNIT_ASSERT(h.get<unsigned long long>("trainOfOccurrence") == h.get<unsigned long long>("trainOfFirstOccurrence"));
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmHigh");
    CPPUNIT_ASSERT(h.get<std::string>("description") == "A description for alarmHigh");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == true);
    
   
    //retrigger the alarm
    messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&]{success = m_deviceClient->execute("alarmTester", "triggerAlarmHigh", KRB_TEST_MAX_TIMEOUT);});
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "triggeredAlarmHigh");
    
    
    //now occurrences should mismatch and alarm should not be acknowledgeable
    CPPUNIT_ASSERT(lastMessage.has("rows.0.update"));
    h = lastMessage.get<Hash>("rows.0.update");
    
    CPPUNIT_ASSERT(h.get<std::string>("timeOfOccurrence") != h.get<std::string>("timeOfFirstOccurrence"));
    //not testing on train id, as it will be 0 in both cases without use of a time server
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmHigh");
    CPPUNIT_ASSERT(h.get<std::string>("description") == "A description for alarmHigh");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    //the alarm should now not be acknowledgeable anymore
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);

    
}

void AlarmService_Test::testAcknowledgement() {
    std::pair<bool, std::string> success;
   
    //add another alarm to the table so we have two alarms pending
    //we will work only on the first one afterwards
    auto messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&]{success = m_deviceClient->execute("alarmTester", "triggerWarnHigh2", KRB_TEST_MAX_TIMEOUT);});
    Hash lastMessage;
    messageQ->pop(lastMessage);

    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "triggeredWarnHigh2");
    CPPUNIT_ASSERT(lastMessage.has("rows.1.add"));
    
    //first test if we cannot acknowledge a not acknowledgeable alarm.
    //the alarm service should be in this state after the previous test.
    Hash message("type", "acknowledgeAlarm", "alarmInstanceId", "testAlarmService", "acknowledgedRows", Hash("0", true));
    messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&]{m_tcpAdapter->sendMessage(message);});
    messageQ->pop(lastMessage);
    
    //acknowledgement should have been refused
    CPPUNIT_ASSERT(lastMessage.has("rows.0.refuseAcknowledgement"));
    Hash h = lastMessage.get<Hash>("rows.0.refuseAcknowledgement");
    
    //and now updates
    CPPUNIT_ASSERT(h.get<std::string>("timeOfOccurrence") != h.get<std::string>("timeOfFirstOccurrence"));
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmHigh");
    CPPUNIT_ASSERT(h.get<std::string>("description") == "A description for alarmHigh");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);
    

    //now we go into the normal state. Acknowledging is now possible, and we are
    //made aware of this
    messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&]{success = m_deviceClient->execute("alarmTester", "triggerNormal", KRB_TEST_MAX_TIMEOUT);});
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "triggeredNormal");
    
   
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT(lastMessage.has("rows.0.acknowledgeable"));
    h = lastMessage.get<Hash>("rows.0.acknowledgeable");
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == true);
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
    
    // we can now acknowledge the alarm, by sending the appropriate message
    // this should trigger a message signalling that the acknowledged alarm is
    // to be removed
    messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&]{m_tcpAdapter->sendMessage(message);});
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT(lastMessage.has("rows.0.remove"));
    
    h = lastMessage.get<Hash>("rows.0.remove");

    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
  
    
    
    // requesting the alarm entries again, one entry, remains, this is the
    // second alarm we raised.
    message = Hash("type", "requestAlarms", "alarmInstanceId", "testAlarmService");
    messageQ = m_tcpAdapter->getNextMessages("alarmInit", 1, [&]{m_tcpAdapter->sendMessage(message);});
    //m_deviceClient->execute("testAlarmService", "slotReconfigure", Hash("currentAlarms", alarmTable), KRB_TEST_MAX_TIMEOUT);
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT(lastMessage.has("rows.1.init"));
    h = lastMessage.get<Hash>("rows.1.init");
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "nodeA/floatProperty2");
    

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
        propHash.set("deviceId", "alarmTester");
        propHash.set("property", "nodeA/floatProperty2");
        propHash.set<unsigned long long>("id", 1);
        Hash alarmHash("alarmTester", Hash("nodeA/floatProperty2", Hash("warnHigh", propHash)));
        Hash hTest("devices", registeredDevices, "alarms", alarmHash);
        
        //erase the occurance times, as the will not match!
        h.erase("alarms.alarmTester.nodeA/floatProperty2.warnHigh.timeOfFirstOccurrence");
        h.erase("alarms.alarmTester.nodeA/floatProperty2.warnHigh.timeOfOccurrence");
        
       

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
    
  
    
    success = m_deviceClient->instantiate("testServer", "AlarmTester", Hash("deviceId", "alarmTester2"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
   
    
    //trigger an alarm
    success = m_deviceClient->execute("alarmTester2", "triggerAlarmLow", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "triggeredAlarmLow");

    //now we bring the alarm service back up
    boost::shared_ptr<boost::lockfree::spsc_queue<Hash> > messageQ2;
    auto messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 3, [&]{messageQ2 = m_tcpAdapter->getNextMessages("alarmInit", 1, [&]{success = m_deviceClient->instantiate("testServer", "AlarmService", Hash("deviceId", "testAlarmService", "flushInterval", 1), KRB_TEST_MAX_TIMEOUT);});});
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.first);
    Hash lastMessage;
    messageQ2->pop(lastMessage);
    CPPUNIT_ASSERT(lastMessage.has("rows.1.init"));
    
    // alarmState should now be an alarm for floatProperty and floatProperty2 acknowledgeable, and alarm on alarmTester2
    // messages are unordered as they depend on async answers from other devices
    bool row2add = false;
    bool row1ack = false;
    bool row3add = false;

    for(size_t i = 0; i<3; ++i){
        messageQ->pop(lastMessage);
        if(lastMessage.has("rows.2.add")) row2add = true;
        if(lastMessage.has("rows.1.acknowledgeable")) row1ack = true;
        if(lastMessage.has("rows.3.add")) row3add = true;
        
    }
    CPPUNIT_ASSERT(row2add);
    CPPUNIT_ASSERT(row1ack);
    CPPUNIT_ASSERT(row3add);

}

void AlarmService_Test::testDeviceKilled(){
     //kill device, alarms should become acknowledgeable
     std::pair<bool, std::string> success;
     auto messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&]{success = m_deviceClient->killDevice("alarmTester2", KRB_TEST_MAX_TIMEOUT);});
     CPPUNIT_ASSERT(success.first);
     
     Hash lastMessage;
     messageQ->pop(lastMessage);
     //the following depends on async messaging of the reappearance of the
     // alarm service in a previous test. We cannot know for sure which row
    
     if (lastMessage.has("rows.3.deviceKilled")) m_killedDeviceRow = "3";
     if (lastMessage.has("rows.2.deviceKilled")) m_killedDeviceRow = "2";
     if (lastMessage.has("rows.1.deviceKilled")) m_killedDeviceRow = "1";
     
     CPPUNIT_ASSERT(!m_killedDeviceRow.empty()); 
     Hash h = lastMessage.get<Hash>("rows."+m_killedDeviceRow+".deviceKilled");
     
     CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester2");
     CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
     CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmLow");
     CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == true);
     CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
 }
 
 void AlarmService_Test::testDeviceReappeared(){
     // bring up a previously known device
     // this should trigger that the previously acknowledgeable alarms of this device
     // (they became acknowledgeable when it was killed), are now not acknowledgeable
     // any more, as we start it into an alarm state.
     std::pair<bool, std::string> success;
     auto messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&]{success = m_deviceClient->instantiate("testServer", "AlarmTester", Hash("deviceId", "alarmTester2", "floatProperty", -5.), KRB_TEST_MAX_TIMEOUT);});
     CPPUNIT_ASSERT(success.first);
     
     Hash lastMessage;
     messageQ->pop(lastMessage);

     CPPUNIT_ASSERT(lastMessage.has("rows."+m_killedDeviceRow+".update")); 
     Hash h = lastMessage.get<Hash>("rows."+m_killedDeviceRow+".update");
     
     CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester2");
     CPPUNIT_ASSERT(h.get<std::string>("property") == "floatProperty");
     CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmLow");
     CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);
     CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
	
 }
    

#undef KRB_TEST_MAX_TIMEOUT

