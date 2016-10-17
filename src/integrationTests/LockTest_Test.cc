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
    
    // The test will raise exceptions by purpose, FATAL log level is higher then ERROR (max used in KARABO), so the test will be silent
    Hash config("DeviceServer", Hash("serverId", "testServerLock", "scanPlugins", false, "visibility", 4, "Logger.priority", "FATAL"));
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
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerLock", "LockTestDevice", Hash("deviceId", "lockTest3"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    success = m_deviceClient->instantiate("testServerLock", "LockTestDevice", Hash("deviceId", "lockTest1", "controlledDevice", "lockTest3"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    success = m_deviceClient->instantiate("testServerLock", "LockTestDevice", Hash("deviceId", "lockTest2", "controlledDevice", "lockTest3"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    testLocking();
    testUnlocking();
    testRecursiveLocking();
    testSettingOnLocked();
    testLockStealing();
}


void LockTest_Test::testLocking() {

    // This will lock "lockTest3" and work on it for 1s (asynchronously)
    m_deviceClient->executeNoWait("lockTest1", "lockAndWait");
    // We are waiting here to give the machinery time to really lock "lockTest3"
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    // This should fail, as lockAndWait is configured to throw once we are trying to also lock "lockTest3"
    CPPUNIT_ASSERT_THROW(m_deviceClient->execute("lockTest2", "lockAndWait", KRB_TEST_MAX_TIMEOUT), Exception);
    Exception::clearTrace();

    std::clog << "Tested locking.. Ok" << std::endl;

    //wait til lock clears
    while (m_deviceClient->get<std::string>("lockTest3", "lockedBy") != "") {
    };

    m_deviceClient->executeNoWait("lockTest1", "lockAndWaitLong");
    // We are waiting here to give the machinery time to really lock "lockTest3"
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    CPPUNIT_ASSERT_THROW(m_deviceClient->execute("lockTest2", "lockAndWaitTimeout", 10), Exception);
    Exception::clearTrace();
    std::clog << "Tested locking with timeout (fail).. Ok" << std::endl;

    m_deviceClient->execute("lockTest3", "slotClearLock");
    //wait til lock clears
    while (m_deviceClient->get<std::string>("lockTest3", "lockedBy") != "") {
    };

    m_deviceClient->executeNoWait("lockTest1", "lockAndWait");

    m_deviceClient->execute("lockTest2", "lockAndWaitTimeout", 10);
    std::clog << "Tested locking with timeout (success).. Ok" << std::endl;

}


void LockTest_Test::testUnlocking() {
    m_deviceClient->execute("lockTest3", "slotClearLock");
    while (m_deviceClient->get<std::string>("lockTest3", "lockedBy") != "") {
    };
    m_deviceClient->execute("lockTest2", "lockAndWait", KRB_TEST_MAX_TIMEOUT);
    std::clog << "Tested unlocking.. Ok" << std::endl;

}


void LockTest_Test::testRecursiveLocking() {
    m_deviceClient->execute("lockTest3", "slotClearLock");
    CPPUNIT_ASSERT_THROW(m_deviceClient->execute("lockTest1", "lockAndWaitRecursiveFail", KRB_TEST_MAX_TIMEOUT), Exception);
    Exception::clearTrace();
    m_deviceClient->execute("lockTest3", "slotClearLock");
    //recursive succeeds
    m_deviceClient->execute("lockTest1", "lockAndWaitRecursive", KRB_TEST_MAX_TIMEOUT);
    std::clog << "Tested recursive locking.. Ok" << std::endl;
}


void LockTest_Test::testSettingOnLocked() {
    m_deviceClient->execute("lockTest3", "slotClearLock");
    m_deviceClient->executeNoWait("lockTest1", "lockAndWait");
    // We are waiting here to give the machinery time to really lock "lockTest3"
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));   
    CPPUNIT_ASSERT_THROW(m_deviceClient->set("lockTest3", "intProperty", 100), Exception);
    Exception::clearTrace();

    const int value = m_deviceClient->get<int>("lockTest3", "intProperty");
    CPPUNIT_ASSERT(value <= 5);
    std::clog << "Tested rejection of setting on locked.. Ok" << std::endl;
}


void LockTest_Test::testLockStealing() {
    m_deviceClient->execute("lockTest3", "slotClearLock");
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    m_deviceClient->executeNoWait("lockTest1", "lockAndWait");
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    m_deviceClient->executeNoWait("lockTest3", "slotClearLock");
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    m_deviceClient->set("lockTest3", "intProperty", 100);
    CPPUNIT_ASSERT(m_deviceClient->get<int>("lockTest3", "intProperty") == 100);
    std::clog << "Tested stolen lock exception.. Ok" << std::endl;
}


