/*
 * File:   LockTest_Test.cc
 * Author: steffen
 *
 * Created on Oct 2, 2016, 1:21:17 PM
 */

#include "LockTest_Test.hh"
using namespace std;

#define KRB_TEST_MAX_TIMEOUT 5

USING_KARABO_NAMESPACES

CPPUNIT_TEST_SUITE_REGISTRATION(LockTest_Test);

LockTest_Test::LockTest_Test() {
}

LockTest_Test::~LockTest_Test() {
}

void LockTest_Test::setUp() {

    Hash config("DeviceServer", Hash("serverId", "testServerLock", "scanPlugins", false, "visibility", 4, "Logger.priority", "ERROR"));
    m_deviceServer = boost::shared_ptr<DeviceServer>(DeviceServer::create(config));
    m_deviceServerThread = boost::thread(&DeviceServer::run, m_deviceServer);
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());

}



void LockTest_Test::tearDown() {
    m_deviceClient->killServer("testServerLock", KRB_TEST_MAX_TIMEOUT);
    m_deviceServerThread.join();
    m_deviceClient.reset();

}

void LockTest_Test::appTestRunner() {
 
    
    // in order to avoid recurring setup and tear down call all tests are run in a single runner
    std::pair<bool, std::string> success  = m_deviceClient->instantiate("testServerLock", "LockTestDevice", Hash("deviceId", "lockTest3"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
    success =  m_deviceClient->instantiate("testServerLock", "LockTestDevice", Hash("deviceId", "lockTest1", "controlledDevice", "lockTest3"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
    success = m_deviceClient->instantiate("testServerLock", "LockTestDevice", Hash("deviceId", "lockTest2", "controlledDevice", "lockTest3"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
  
    
    boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
    
    testLocking();
    testUnlocking();
    testRecursiveLocking();
    testSettingOnLocked();
    testLockStealing();
}

void LockTest_Test::testLocking() {
    m_deviceClient->executeNoWait("lockTest1", "lockAndWait");
   
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    
    std::pair<bool, std::string> success = m_deviceClient->execute("lockTest2", "lockAndWait", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    
    CPPUNIT_ASSERT(success.second.find("Could not acquire lock on lockTest3") != std::string::npos);
    std::clog<<"Tested locking.. Ok"<<std::endl;
    
    //wait til lock clears
    while(m_deviceClient->get<std::string>("lockTest3", "lockedBy") != ""){};
    
    m_deviceClient->set("lockTest1", "waitTime", 20000);
    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));
    m_deviceClient->executeNoWait("lockTest1", "lockAndWaitLong");
    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));
    success = m_deviceClient->execute("lockTest2", "lockAndWaitTimeout", 10);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second.find("Could not acquire lock on lockTest3") != std::string::npos);
    std::clog<<"Tested locking with timeout (fail).. Ok"<<std::endl;
    
    success = m_deviceClient->execute("lockTest3", "slotClearLock");
    //wait til lock clears
    while(m_deviceClient->get<std::string>("lockTest3", "lockedBy") != ""){};
    m_deviceClient->executeNoWait("lockTest1", "lockAndWait");
    success = m_deviceClient->execute("lockTest2", "lockAndWaitTimeout", 10);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second.find("Acquired Lock") != std::string::npos);
    std::clog<<"Tested locking with timeout (success).. Ok"<<std::endl;

}

void LockTest_Test::testUnlocking() {
    std::pair<bool, std::string> success = m_deviceClient->execute("lockTest3", "slotClearLock");
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    success = m_deviceClient->execute("lockTest2", "lockAndWait", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "Acquired Lock");
    std::clog<<"Tested unlocking.. Ok"<<std::endl;

}

void LockTest_Test::testRecursiveLocking() {
    std::pair<bool, std::string> success = m_deviceClient->execute("lockTest3", "slotClearLock");
    
   
    success = m_deviceClient->execute("lockTest1", "lockAndWaitRecursiveFail", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second.find("Could not acquire lock on lockTest3") != std::string::npos);
    
    success = m_deviceClient->execute("lockTest3", "slotClearLock");
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    //recursive succeeds
    
    success = m_deviceClient->execute("lockTest1", "lockAndWaitRecursive", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    CPPUNIT_ASSERT(success.second == "Acquired Lock");
    std::clog<<"Tested recursive locking.. Ok"<<std::endl;
}

void LockTest_Test::testSettingOnLocked(){
    std::pair<bool, std::string> success = m_deviceClient->execute("lockTest3", "slotClearLock");
    m_deviceClient->executeNoWait("lockTest1", "lockAndWait");
   
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    success = m_deviceClient->set("lockTest3", "intProperty", 100);
    CPPUNIT_ASSERT(!success.first);
    const int value = m_deviceClient->get<int>("lockTest3", "intProperty");
    CPPUNIT_ASSERT(value <= 5);
    std::clog<<"Tested rejection of setting on locked.. Ok"<<std::endl;
}

void LockTest_Test::testLockStealing(){
    std::pair<bool, std::string> success = m_deviceClient->execute("lockTest3", "slotClearLock");
     boost::this_thread::sleep(boost::posix_time::milliseconds(200));
    m_deviceClient->executeNoWait("lockTest1", "lockAndWait");
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    m_deviceClient->executeNoWait("lockTest3", "slotClearLock", KRB_TEST_MAX_TIMEOUT);
    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));
    const std::string& errorMsg = m_deviceClient->get<std::string>("lockTest1", "errorMessage");
    CPPUNIT_ASSERT(errorMsg.find("Lock was invalidated!") != std::string::npos);
    std::clog<<"Tested stolen lock exception.. Ok"<<std::endl;
}


