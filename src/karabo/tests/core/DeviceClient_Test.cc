/* 
 * File:   DevcieClient_Test.cc
 * Author: flucke
 * 
 * Created on August 24, 2017, 9:49 AM
 */

#include <string>

#include "DeviceClient_Test.hh"

#include "karabo/util/Hash.hh"

#define KRB_TEST_MAX_TIMEOUT 5

CPPUNIT_TEST_SUITE_REGISTRATION(DeviceClient_Test);

using namespace karabo::core;
using namespace karabo::util;

DeviceClient_Test::DeviceClient_Test() {
}


DeviceClient_Test::~DeviceClient_Test() {
}


void DeviceClient_Test::setUp() {

    // Event loop is started in coreTestRunner.cc's main()

    const Hash config("serverId", "testServerDeviceClient", "scanPlugins", false, "Logger.priority", "FATAL");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();

    // Create client
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());

}


void DeviceClient_Test::tearDown() {

}


void DeviceClient_Test::testGet() {

    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerDeviceClient", "PropertyTest",
                                                                       Hash("deviceId", "TestedDevice"),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // int as normal type - test both get, i.e. return by reference argument or return by value
    int intProperty = 0;
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get<int>("TestedDevice", "int32Property", intProperty));
    CPPUNIT_ASSERT_EQUAL(32000000, intProperty);
    intProperty = 0;
    CPPUNIT_ASSERT_NO_THROW(intProperty = m_deviceClient->get<int>("TestedDevice", "int32Property"));
    CPPUNIT_ASSERT_EQUAL(32000000, intProperty);

    // State is specially treated: Internally it is a string, but that should stay internal!
    State state(State::UNKNOWN);
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get<State>("TestedDevice", "state", state));
    CPPUNIT_ASSERT(state != State::UNKNOWN); // still INIT or very likely NORMAL
    state = State::UNKNOWN;
    CPPUNIT_ASSERT_NO_THROW(state = m_deviceClient->get<State>("TestedDevice", "state"));
    CPPUNIT_ASSERT(state != State::UNKNOWN);
    //
    std::string dummy;
    CPPUNIT_ASSERT_THROW(m_deviceClient->get<std::string>("TestedDevice", "state", dummy), ParameterException);
    CPPUNIT_ASSERT_THROW(dummy = m_deviceClient->get<std::string>("TestedDevice", "state"), ParameterException);

    // The same for AlarmConditionState
    AlarmCondition alarm(AlarmCondition::ALARM);
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get<AlarmCondition>("TestedDevice", "alarmCondition", alarm));
    CPPUNIT_ASSERT(alarm == AlarmCondition::NONE); // no alarm on device!
    alarm = AlarmCondition::ALARM;
    CPPUNIT_ASSERT_NO_THROW(alarm = m_deviceClient->get<AlarmCondition>("TestedDevice", "alarmCondition"));
    CPPUNIT_ASSERT(alarm == AlarmCondition::NONE); // no alarm on device!
    //
    CPPUNIT_ASSERT_THROW(m_deviceClient->get<std::string>("TestedDevice", "alarmCondition", dummy), ParameterException);
    CPPUNIT_ASSERT_THROW(dummy = m_deviceClient->get<std::string>("TestedDevice", "alarmCondition"), ParameterException);

}


