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
#include <karabo/util/Hash.hh>


USING_KARABO_NAMESPACES;

CPPUNIT_TEST_SUITE_REGISTRATION(AlarmService_Test);

#define KRB_TEST_MAX_TIMEOUT 10

using namespace karabo;


AlarmService_Test::AlarmService_Test() {

}


AlarmService_Test::~AlarmService_Test() {

}


void AlarmService_Test::setUp() {

    // Start central event-loop
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));
    // Create and start server
    Hash config("serverId", "testServer", "scanPlugins", false, "Logger.priority", "ERROR");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();
    // Create client
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());

    //unlink persisted alarms if they exist
    if (boost::filesystem::exists(std::string(KARABO_TESTPATH) + "/testAlarmService.xml")) {
        boost::filesystem::remove(std::string(KARABO_TESTPATH) + "/testAlarmService.xml");
    }
}


void AlarmService_Test::tearDown() {
    
    if (m_tcpAdapter->connected()) {
        m_tcpAdapter->disconnect();
    }

    m_deviceServer.reset();
    EventLoop::stop();
    m_eventLoopThread.join();

    //unlink persisted alarms if they exist
    if (boost::filesystem::exists(std::string(KARABO_TESTPATH) + "/testAlarmService.xml")) {
        boost::filesystem::remove(std::string(KARABO_TESTPATH) + "/testAlarmService.xml");
    }

}


void AlarmService_Test::appTestRunner() {
    //add a few threads to the event loop
    EventLoop::addThread(4);

    // in order to avoid recurring setup and tear down call all tests are run in a single runner
    // here we start the server and service devices, as well as an alarm test device
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServer", "GuiServerDevice", Hash("deviceId", "testGuiServer", "port", 44446), KRB_TEST_MAX_TIMEOUT);
    boost::this_thread::sleep(boost::posix_time::milliseconds(4000));
    m_tcpAdapter = boost::shared_ptr<karabo::TcpAdapter>(new karabo::TcpAdapter(Hash("port", 44446u/*, "debug", true*/)));
    boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    success = m_deviceClient->instantiate("testServer", "AlarmService", Hash("deviceId", "testAlarmService", "flushInterval", 1, "storagePath", std::string(KARABO_TESTPATH)), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
    success = m_deviceClient->instantiate("testServer", "AlarmTester", Hash("deviceId", "alarmTester"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    // the actual tests
    CPPUNIT_ASSERT_NO_THROW(testDeviceRegistration());
    CPPUNIT_ASSERT_NO_THROW(testAlarmPassing());
    CPPUNIT_ASSERT_NO_THROW(testAcknowledgement());
    CPPUNIT_ASSERT_NO_THROW(testTriggerGlobalAck());
    CPPUNIT_ASSERT_NO_THROW(testTriggerGlobal());
    CPPUNIT_ASSERT_NO_THROW(testFlushing());
    CPPUNIT_ASSERT_NO_THROW(testRecovery());
    CPPUNIT_ASSERT_NO_THROW(testDeviceKilled());
    // TODO: Comment in once it is figured out why the arriving Hash has
    // 'update' or 'acknowledgeable' coming from signalAlarmUpdate
    //testDeviceReappeared();
}


void AlarmService_Test::testDeviceRegistration() {
    //we give a bit of time for updates to propagate.
    boost::this_thread::sleep(boost::posix_time::milliseconds(5000));

    // test whether the device actually registers with the alarm service upon instanceNew
    std::vector<std::string> registeredDevices = m_deviceClient->get<std::vector<std::string> >("testAlarmService", "registeredDevices");
    CPPUNIT_ASSERT(std::find(registeredDevices.begin(), registeredDevices.end(), std::string("alarmTester")) != registeredDevices.end());

    std::clog << std::endl << "Tested device registration.. Ok" << std::endl;
}


void AlarmService_Test::testAlarmPassing() {
    // test if raising an alarm on alarmTester propagates to testAlarmService and updates the alarmTable there
    TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        m_deviceClient->execute("alarmTester", "triggerAlarmHighAck", KRB_TEST_MAX_TIMEOUT);
    });
    Hash lastMessage;
    messageQ->pop(lastMessage);

    CPPUNIT_ASSERT(m_deviceClient->get<std::string>("alarmTester", "result") == "triggerAlarmHighAck");
    CPPUNIT_ASSERT(m_deviceClient->get<int>("alarmTester", "intPropNeedsAck") == 5);
    
    //get row for first device
    CPPUNIT_ASSERT(lastMessage.has("rows"));
    std::string rowId = lastMessage.get<Hash>("rows").begin()->getKey();

    CPPUNIT_ASSERT(lastMessage.has("rows." + rowId + ".add"));
    Hash h = lastMessage.get<Hash>("rows." + rowId + ".add");
    //these should be the same as it is the first time the alarm is raised
    CPPUNIT_ASSERT(h.get<std::string>("timeOfOccurrence") == h.get<std::string>("timeOfFirstOccurrence"));
    CPPUNIT_ASSERT(karabo::util::Timestamp::fromHashAttributes(h.getAttributes("timeOfOccurrence")) == karabo::util::Timestamp::fromHashAttributes(h.getAttributes("timeOfFirstOccurrence")));
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "intPropNeedsAck");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmHigh");
    CPPUNIT_ASSERT(h.get<std::string>("description") == "A intPropNeedsAck alarmHigh");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);

    //go out of the alarm state
    messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        m_deviceClient->execute("alarmTester", "triggerNormalAck", KRB_TEST_MAX_TIMEOUT);
    });
    messageQ->pop(lastMessage);
    rowId = lastMessage.get<Hash>("rows").begin()->getKey();
    
    CPPUNIT_ASSERT(m_deviceClient->get<std::string>("alarmTester", "result") == "triggerNormalAck");
    CPPUNIT_ASSERT(m_deviceClient->get<int>("alarmTester", "intPropNeedsAck") == 0);

    CPPUNIT_ASSERT(lastMessage.has("rows." + rowId + ".acknowledgeable"));
    h = lastMessage.get<Hash>("rows." + rowId + ".acknowledgeable");

    CPPUNIT_ASSERT(karabo::util::Timestamp::fromHashAttributes(h.getAttributes("timeOfOccurrence")) == karabo::util::Timestamp::fromHashAttributes(h.getAttributes("timeOfFirstOccurrence")));
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "intPropNeedsAck");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmHigh");
    CPPUNIT_ASSERT(h.get<std::string>("description") == "A intPropNeedsAck alarmHigh");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == true);

    // Retrigger the alarm
    messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        m_deviceClient->execute("alarmTester", "triggerAlarmHighAck", KRB_TEST_MAX_TIMEOUT);
    });
    messageQ->pop(lastMessage);
    rowId = lastMessage.get<Hash>("rows").begin()->getKey();

    CPPUNIT_ASSERT(m_deviceClient->get<std::string>("alarmTester", "result") == "triggerAlarmHighAck");
    CPPUNIT_ASSERT(m_deviceClient->get<int>("alarmTester", "intPropNeedsAck") == 5);

    //now occurrences should mismatch and alarm should not be acknowledgeable
    CPPUNIT_ASSERT(lastMessage.has("rows." + rowId + ".update"));
    h = lastMessage.get<Hash>("rows." + rowId + ".update");

    CPPUNIT_ASSERT(h.get<std::string>("timeOfOccurrence") != h.get<std::string>("timeOfFirstOccurrence"));
    //not testing on train id, as it will be 0 in both cases without use of a time server
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "intPropNeedsAck");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmHigh");
    CPPUNIT_ASSERT(h.get<std::string>("description") == "A intPropNeedsAck alarmHigh");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    //the alarm should now not be acknowledgeable anymore
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);

    std::clog << "Tested alarm passing.. Ok" << std::endl;
}

void AlarmService_Test::testAcknowledgement() {
    //add another alarm to the table so we have two alarms pending
    //we will work only on the first one afterwards
    TcpAdapter::QueuePtr messageQ;
    CPPUNIT_ASSERT_NO_THROW(messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        m_deviceClient->execute("alarmTester", "triggerWarnHighAckNode", KRB_TEST_MAX_TIMEOUT);
    }));
    Hash lastMessage;
    messageQ->pop(lastMessage);

    CPPUNIT_ASSERT(m_deviceClient->get<std::string>("alarmTester", "result") == "triggerWarnHighAckNode");

    //get row for second device
    CPPUNIT_ASSERT(lastMessage.has("rows"));
    std::string rowId = lastMessage.get<Hash>("rows").begin()->getKey();
    m_rowForDevice1 = rowId;

    CPPUNIT_ASSERT_MESSAGE(toString(lastMessage), lastMessage.has("rows." + rowId + ".add"));

    //first test if we cannot acknowledge a not acknowledgeable alarm.
    //the alarm service should be in this state after the previous test.
    Hash message("type", "acknowledgeAlarm", "alarmInstanceId", "testAlarmService", "acknowledgedRows", Hash("0", true));
    CPPUNIT_ASSERT_NO_THROW(messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        m_tcpAdapter->sendMessage(message);
    }));
    messageQ->pop(lastMessage);
    rowId = lastMessage.get<Hash>("rows").begin()->getKey();

    //acknowledgement should have been refused
    CPPUNIT_ASSERT(lastMessage.has("rows." + rowId + ".refuseAcknowledgement"));
    Hash h = lastMessage.get<Hash>("rows." + rowId + ".refuseAcknowledgement");

    //and now updates
    CPPUNIT_ASSERT(h.get<std::string>("timeOfOccurrence") != h.get<std::string>("timeOfFirstOccurrence"));
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "intPropNeedsAck");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmHigh");
    CPPUNIT_ASSERT(h.get<std::string>("description") == "A intPropNeedsAck alarmHigh");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);


    //now we go into the normal state. Acknowledging is now possible, and we are
    //made aware of this
    CPPUNIT_ASSERT_NO_THROW(messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        m_deviceClient->execute("alarmTester", "triggerNormalAck", KRB_TEST_MAX_TIMEOUT);
    }));

    CPPUNIT_ASSERT(m_deviceClient->get<std::string>("alarmTester", "result") == "triggerNormalAck");

    messageQ->pop(lastMessage);
    rowId = lastMessage.get<Hash>("rows").begin()->getKey();

    CPPUNIT_ASSERT(lastMessage.has("rows." + rowId + ".acknowledgeable"));
    h = lastMessage.get<Hash>("rows." + rowId + ".acknowledgeable");
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == true);
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "intPropNeedsAck");

    // we can now acknowledge the alarm, by sending the appropriate message
    // this should trigger a message signaling that the acknowledged alarm is
    // to be removed
    CPPUNIT_ASSERT_NO_THROW(messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        m_tcpAdapter->sendMessage(message);
    }));
    messageQ->pop(lastMessage);
    rowId = lastMessage.get<Hash>("rows").begin()->getKey();

    CPPUNIT_ASSERT(lastMessage.has("rows." + rowId + ".remove"));

    h = lastMessage.get<Hash>("rows." + rowId + ".remove");
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "intPropNeedsAck");

    // requesting the alarm entries again, two entries remain, this is the
    // second alarm we raised.
    bool initMessageReceived = false;
    const int maxRepeats = 5;
    int rep = 0;
    while (!initMessageReceived) {
        if (rep++ == maxRepeats) CPPUNIT_ASSERT(false);
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        message = Hash("type", "requestAlarms", "alarmInstanceId", "testAlarmService");
        CPPUNIT_ASSERT_NO_THROW_MESSAGE(toString(rep) + " failures to receive message for " + toString(message),
                                        messageQ = m_tcpAdapter->getNextMessages("alarmInit", 1, [&] {
                                            m_tcpAdapter->sendMessage(message);
                                        }));
        //m_deviceClient->execute("testAlarmService", "slotReconfigure", Hash("currentAlarms", alarmTable), KRB_TEST_MAX_TIMEOUT);
        messageQ->pop(lastMessage);
        rowId = lastMessage.get<Hash>("rows").begin()->getKey();
        if (lastMessage.has("rows." + rowId + ".init")) {
            h = lastMessage.get<Hash>("rows." + rowId + ".init");
            CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
            CPPUNIT_ASSERT(h.get<std::string>("property") == "nodeA.floatPropNeedsAck2");
            initMessageReceived = true;
        }
    }

    std::clog << "Tested acknowledgement.. Ok" << std::endl;
}


void AlarmService_Test::testTriggerGlobalAck() {
    // Trigger global warn
    TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        m_deviceClient->execute("alarmTester", "triggerGlobalWarnAck", KRB_TEST_MAX_TIMEOUT);
    });
    Hash lastMessage;
    messageQ->pop(lastMessage);

    CPPUNIT_ASSERT(m_deviceClient->get<std::string>("alarmTester", "result") == "triggerGlobalWarnAck");

    //get row for first device
    CPPUNIT_ASSERT(lastMessage.has("rows"));
    std::string rowId = lastMessage.get<Hash>("rows").begin()->getKey();

    CPPUNIT_ASSERT(lastMessage.has("rows." + rowId + ".add"));
    Hash h = lastMessage.get<Hash>("rows." + rowId + ".add");
    //these should be the same as it is the first time the alarm is raised
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "global");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "warn");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);

    // Trigger global alarm
    messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        m_deviceClient->execute("alarmTester", "triggerGlobalAlarmAck", KRB_TEST_MAX_TIMEOUT);
    });
    messageQ->pop(lastMessage);

    CPPUNIT_ASSERT(m_deviceClient->get<std::string>("alarmTester", "result") == "triggerGlobalAlarmAck");

    //get row for first device
    CPPUNIT_ASSERT(lastMessage.has("rows"));
    rowId = lastMessage.get<Hash>("rows").begin()->getKey();
    
    CPPUNIT_ASSERT(lastMessage.has("rows." + rowId + ".add"));
    h = lastMessage.get<Hash>("rows." + rowId + ".add");
    //these should be the same as it is the first time the alarm is raised
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "global");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarm");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);
    
    // Trigger global warn
    messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        m_deviceClient->execute("alarmTester", "triggerGlobalWarnAck", KRB_TEST_MAX_TIMEOUT);
    });
    messageQ->pop(lastMessage);

    CPPUNIT_ASSERT(m_deviceClient->get<std::string>("alarmTester", "result") == "triggerGlobalWarnAck");

    //get row for first device
    CPPUNIT_ASSERT(lastMessage.has("rows"));
    auto rowIter = lastMessage.get<Hash>("rows").begin();
    rowId = rowIter->getKey();

    CPPUNIT_ASSERT(lastMessage.has("rows." + rowId + ".update"));
    h = lastMessage.get<Hash>("rows." + rowId + ".update");
    //these should be the same as it is the first time the alarm is raised
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "global");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "warn");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);

    //get row for first device
    CPPUNIT_ASSERT(lastMessage.has("rows"));
    rowId = (++rowIter)->getKey();
    
    CPPUNIT_ASSERT(lastMessage.has("rows." + rowId + ".acknowledgeable"));
    h = lastMessage.get<Hash>("rows." + rowId + ".acknowledgeable");
    //these should be the same as it is the first time the alarm is raised
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "global");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarm");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == true);
    
    //go out of the alarm state
    messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        m_deviceClient->execute("alarmTester", "triggerGlobalNormal", KRB_TEST_MAX_TIMEOUT);
    });
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT(m_deviceClient->get<std::string>("alarmTester", "result") == "triggerGlobalNormal");

    rowIter = lastMessage.get<Hash>("rows").begin();
    rowId = rowIter->getKey();
    CPPUNIT_ASSERT(lastMessage.has("rows." + rowId + ".acknowledgeable"));
    h = lastMessage.get<Hash>("rows." + rowId + ".acknowledgeable");
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "global");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarm");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == true);
    
    rowId = (++rowIter)->getKey();
    CPPUNIT_ASSERT(lastMessage.has("rows." + rowId + ".acknowledgeable"));
    h = lastMessage.get<Hash>("rows." + rowId + ".acknowledgeable");
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "global");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "warn");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == true);
    
    std::clog << "Tested global acknowledgeable triggering.. Ok" << std::endl;
}

void AlarmService_Test::testTriggerGlobal() {
    TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        m_deviceClient->execute("alarmTester", "triggerGlobalWarn", KRB_TEST_MAX_TIMEOUT);
    });
    Hash lastMessage;
    messageQ->pop(lastMessage);

    CPPUNIT_ASSERT(m_deviceClient->get<std::string>("alarmTester", "result") == "triggerGlobalWarn");

    //get row for first device
    CPPUNIT_ASSERT(lastMessage.has("rows"));
    std::string rowId = lastMessage.get<Hash>("rows").begin()->getKey();

    CPPUNIT_ASSERT(lastMessage.has("rows." + rowId + ".update"));
    Hash h = lastMessage.get<Hash>("rows." + rowId + ".update");
    //these should be the same as it is the first time the alarm is raised
    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "global");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "warn");
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == false);

    //go out of the alarm state
    messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        m_deviceClient->execute("alarmTester", "triggerGlobalNormal", KRB_TEST_MAX_TIMEOUT);
    });
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT(m_deviceClient->get<std::string>("alarmTester", "result") == "triggerGlobalNormal");

    std::clog << "Tested global triggering.. Ok" << std::endl;
}

void AlarmService_Test::testFlushing() {
    //we should find a flushed file in the working directory
    //assure the flushed file resembles the current state
    
    boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
    boost::interprocess::file_lock flock((std::string(KARABO_TESTPATH) + "/testAlarmService.xml").c_str());
    {
        boost::interprocess::sharable_lock<boost::interprocess::file_lock> shlock(flock);
        std::ifstream fin;
        fin.open(std::string(KARABO_TESTPATH) + "./testAlarmService.xml");
        std::ostringstream archive;
        std::string input;

        while (fin >> input) archive << input << std::endl;
        fin.close();
        TextSerializer<Hash>::Pointer serializer = TextSerializer<Hash>::create("Xml");
        Hash h;
        serializer->load(h, archive.str());
        
        //should be the same as what we currently hold in the table and devices sections
        std::vector<std::string> registeredDevices = m_deviceClient->get<std::vector<std::string> >("testAlarmService", "registeredDevices");

        Hash propHash;
        propHash.set("type", "warnHigh");
        propHash.set("description", "A floatPropNeedsAck2 warnHigh");
        propHash.set("needsAcknowledging", true);
        propHash.set("acknowledgeable", false);
        propHash.set("deviceId", "alarmTester");
        propHash.set("property", "nodeA" + Validator::kAlarmParamPathSeparator + "floatPropNeedsAck2");
        propHash.set<unsigned long long>("id", boost::lexical_cast<unsigned long long>(m_rowForDevice1));
        Hash alarmHash("alarmTester", Hash("nodeA" + Validator::kAlarmParamPathSeparator + "floatPropNeedsAck2", Hash("warnHigh", propHash)));
        Hash hTest("devices", registeredDevices, "alarms", alarmHash);

        //erase the occurance times, as the will not match!
        h.erase("alarms.alarmTester.nodeA" + Validator::kAlarmParamPathSeparator + "floatPropNeedsAck2.warnHigh.timeOfFirstOccurrence");
        h.erase("alarms.alarmTester.nodeA" + Validator::kAlarmParamPathSeparator + "floatPropNeedsAck2.warnHigh.timeOfOccurrence");
        // Erase the latest global alarm
        h.erase("alarms.alarmTester.global");
        
        CPPUNIT_ASSERT(karabo::util::similar(h, hTest));
    }

    std::clog << "Tested flushing.. Ok" << std::endl;
}


void AlarmService_Test::testRecovery() {
    //first we bring down the alarm service.
    //at this state it should hold a warnHigh for floatPropNeedsAck2 which cannot be acknowledged
    std::pair<bool, std::string> success;
    TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("instanceGone", 1, [&] {
        m_deviceClient->killDevice("testAlarmService", KRB_TEST_MAX_TIMEOUT);
    });
    Hash lastMessage;
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT(lastMessage.has("instanceId"));
    CPPUNIT_ASSERT(lastMessage.get<std::string>("instanceId") == "testAlarmService");

    // now we raise an alarm on intPropNeedsAck again and ma nodeA.floatPropNoAck2 acknowledgeable
    m_deviceClient->execute("alarmTester", "triggerNormalAckNode", KRB_TEST_MAX_TIMEOUT);
    // Wait a bit until our internal cache will be updated
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    CPPUNIT_ASSERT(m_deviceClient->get<std::string>("alarmTester", "result") == "triggerNormalAckNode");

    m_deviceClient->execute("alarmTester", "triggerAlarmHighAck", KRB_TEST_MAX_TIMEOUT);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    CPPUNIT_ASSERT(m_deviceClient->get<std::string>("alarmTester", "result") == "triggerAlarmHighAck");

    CPPUNIT_ASSERT_NO_THROW(success = m_deviceClient->instantiate("testServer", "AlarmTester", Hash("deviceId", "alarmTester2"), KRB_TEST_MAX_TIMEOUT));
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    CPPUNIT_ASSERT(success.first);

    //trigger an alarm
    m_deviceClient->execute("alarmTester2", "triggerAlarmLowAck", KRB_TEST_MAX_TIMEOUT);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    CPPUNIT_ASSERT(m_deviceClient->get<std::string>("alarmTester2", "result") == "triggerAlarmLowAck");
    CPPUNIT_ASSERT(m_deviceClient->get<int>("alarmTester2", "intPropNeedsAck") == -5);

    // Clear all messages received so far:
    m_tcpAdapter->clearAllMessages();
    // now we bring the alarm service back up
    CPPUNIT_ASSERT(m_deviceClient->instantiate("testServer", "AlarmService",
                                               Hash("deviceId", "testAlarmService", "flushInterval", 1,
                                                    "storagePath", std::string(KARABO_TESTPATH)),
                                               KRB_TEST_MAX_TIMEOUT).first);

    // We expect to receive several messages on the adapter:
    bool topologyMessage = false; // testAlarmService device has started
    bool alarmInit = false; // alarmTester.nodeA.floatPropNeedsAck2 initial state from file storage
    bool rowAddedTester1 = false; // alarmTester has alarm on intPropNeedsAck
    bool rowAddedTester2 = false; // alarmTester2 has alarm on intPropNeedsAck

    // We cannot use nested 'm_tcpAdapter->getNextMessages("alarmUpdate", 1,...' since we do not know whether throttling
    // sends a single or two alarmUpdate messages - and waiting twice for one message and accepting a timeout on the
    // second does not work either since getNextMessages(..) clears its result container...
    // So we just look into all messages that are received.
    int stillWaitInMs = KRB_TEST_MAX_TIMEOUT * 1000;
    do {
        // Give time for message travel, but give up at some point...
        if (stillWaitInMs <= 0) {
            break;
        }
        stillWaitInMs -= 50;
        boost::this_thread::sleep(boost::posix_time::milliseconds(50));
        // Check testAlarmService is started
        if (!topologyMessage) {
            for (const Hash& msg : m_tcpAdapter->getAllMessages("instanceNew")) {
                if (msg.has("topologyEntry.device.testAlarmService")) {
                    topologyMessage = true; // testAlarmService is started
                }
            }
        }
        // Check that testAlarmService provides old state from file
        if (!alarmInit) {
            for (const Hash& msg : m_tcpAdapter->getAllMessages("alarmInit")) {
                if (msg.has("rows")) {
                    const Hash& rows = msg.get<Hash>("rows");
                    for (auto itNode = rows.begin(); itNode != rows.end(); ++itNode) {
                        // rows contains any stringified number as key for a Hash - but key does not matter
                        const Hash& row = itNode->getValue<Hash>();
                        // Check whether msg contains "rows.<whateverNumber>.init.deviceId"
                        // and "rows.<whateverNumber>.init.property" with the correct content, i.e.
                        // alarmTester.nodeA.floatPropNeedsAck2 was in error before the alarmTester stopped
                        if (row.has("init.deviceId") && row.get<std::string>("init.deviceId") == "alarmTester"
                            && row.has("init.property") && row.get<std::string>("init.property") == "nodeA.floatPropNeedsAck2") {
                            alarmInit = true;
                        }
                    }
                }
            }
        }
        // Check that alarmTester sends updates
        if (!rowAddedTester1 || !rowAddedTester2) {
            for (const Hash& msg : m_tcpAdapter->getAllMessages("alarmUpdate")) {
                if (msg.has("rows")) {
                    const Hash& rows = msg.get<Hash>("rows");
                    for (auto itNode = rows.begin(); itNode != rows.end(); ++itNode) {
                        // rows contains any stringified number as key for a Hash - but key does not matter
                        const Hash& row = itNode->getValue<Hash>();
                        // Check whether msg contains "rows.<whateverNumber>.add.deviceId"
                        // and "rows.<whateverNumber>.add.property" with the correct content.
                        if (row.has("add.deviceId") && row.has("add.property")
                            && row.get<std::string>("add.property") == "intPropNeedsAck") {
                            // OK - which one of our testers is it?
                            if (row.get<std::string>("add.deviceId") == "alarmTester") {
                                rowAddedTester1 = true;
                            } else if (row.get<std::string>("add.deviceId") == "alarmTester2") {
                                rowAddedTester2 = true;
                            }
                        }
                    }
                }
            }
        }
    } while (!topologyMessage || !rowAddedTester1 || !rowAddedTester2 || !alarmInit);

    CPPUNIT_ASSERT(topologyMessage);
    CPPUNIT_ASSERT(alarmInit);

    CPPUNIT_ASSERT(rowAddedTester2);
    CPPUNIT_ASSERT(rowAddedTester1);

    std::clog << "Tested service recovery.. Ok" << std::endl;
}


void AlarmService_Test::testDeviceKilled() {
    //kill device, alarms should become acknowledgeable
    std::pair<bool, std::string> success;
    TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        success = m_deviceClient->killDevice("alarmTester2", KRB_TEST_MAX_TIMEOUT);
    });
    CPPUNIT_ASSERT(success.first);

    Hash lastMessage;
    messageQ->pop(lastMessage);
    m_killedDeviceRow = lastMessage.get<Hash>("rows").begin()->getKey();
    Hash h = lastMessage.get<Hash>("rows." + m_killedDeviceRow + ".deviceKilled");

    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester2");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "intPropNeedsAck");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmLow");
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == true);
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);

    std::clog << "Tested device killing.. Ok" << std::endl;
}


void AlarmService_Test::testDeviceReappeared() {
    // bring up a previously known device
    // this should trigger that the previously acknowledgeable alarms of this device
    // (they became acknowledgeable when it was killed), are now not acknowledgeable
    // any more, as we start it into an alarm state.
    std::pair<bool, std::string> success;
    TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("alarmUpdate", 1, [&] {
        success = m_deviceClient->instantiate("testServer", "AlarmTester", Hash("deviceId", "alarmTester2", "intPropNeedsAck", -5), KRB_TEST_MAX_TIMEOUT);
    });
    CPPUNIT_ASSERT(success.first);

    Hash lastMessage;
    messageQ->pop(lastMessage);
    
    CPPUNIT_ASSERT(lastMessage.has("rows." + m_killedDeviceRow + ".update"));
    Hash h = lastMessage.get<Hash>("rows." + m_killedDeviceRow + ".update");

    CPPUNIT_ASSERT(h.get<std::string>("deviceId") == "alarmTester2");
    CPPUNIT_ASSERT(h.get<std::string>("property") == "intPropNeedsAck");
    CPPUNIT_ASSERT(h.get<std::string>("type") == "alarmLow");
    CPPUNIT_ASSERT(h.get<bool>("acknowledgeable") == false);
    CPPUNIT_ASSERT(h.get<bool>("needsAcknowledging") == true);

    std::clog << "Tested device reappearance.. Ok" << std::endl;
}

#undef KRB_TEST_MAX_TIMEOUT

