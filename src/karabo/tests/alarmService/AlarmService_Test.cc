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
    //set up the alarm service device
    
    m_pid = fork(); 

    switch (m_pid) {
        case -1: /* Error */
            std::cerr << "Uh-Oh! fork() failed.\n";
            exit(1);
        case 0: /* Child process */
            startServer();
            exit(1);
        default: /* Parent process */
            break;
    }


    
    Hash configClient();
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());
    
    
}

void AlarmService_Test::startServer(){
    
    Hash config("DeviceServer", Hash("serverId", "testServer", "scanPlugins", false, "visibility", 4, "Logger.priority", "DEBUG"));
    m_deviceServer = boost::shared_ptr<DeviceServer>(DeviceServer::create(config));
    m_deviceServer->run();
}


void AlarmService_Test::tearDown() {
    m_deviceClient->killServer("testServer", KRB_TEST_MAX_TIMEOUT);
    
    int status;
    while (!WIFEXITED(status)) {
        waitpid(m_pid, &status, 0); /* Wait for the process to complete */
    }
    
}


void AlarmService_Test::testDeviceRegistration() {
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServer", "AlarmService", Hash("deviceId", "testAlarmService"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
       
    success = m_deviceClient->instantiate("testServer", "AlarmTester", Hash("deviceId", "alarmTester"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
    std::vector<std::string> alarmDevices = m_deviceClient->get<std::vector<std::string> >("alarmTester", "alarmServiceDevices");
    CPPUNIT_ASSERT(alarmDevices.size() == 1);
    CPPUNIT_ASSERT(alarmDevices[0] == "testAlarmService");
    
   
    
    
}


#undef KRB_TEST_MAX_TIMEOUT

