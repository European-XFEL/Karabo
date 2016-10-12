/*
 * File:   DeviceServerRunner_Test.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on Sep 16, 2016, 2:34:36 PM
 */

#include "DeviceServerRunner_Test.hh"


USING_KARABO_NAMESPACES;

#define KRB_TEST_MAX_TIMEOUT 10

CPPUNIT_TEST_SUITE_REGISTRATION(DeviceServerRunner_Test);


DeviceServerRunner_Test::DeviceServerRunner_Test() {
}


DeviceServerRunner_Test::~DeviceServerRunner_Test() {
}


void DeviceServerRunner_Test::setUp() {
    Hash config("DeviceServer", Hash("serverId", "testDeviceServer_0", "scanPlugins", false, "visibility", 4, "Logger.priority", "INFO"));
    m_deviceServer = boost::shared_ptr<DeviceServer>(DeviceServer::create(config));
    m_deviceServerThread = boost::thread(&DeviceServer::run, m_deviceServer);

    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());
}


void DeviceServerRunner_Test::tearDown() {
    m_deviceClient->killServer("testDeviceServer_0", KRB_TEST_MAX_TIMEOUT);
    m_deviceServerThread.join();
    m_deviceClient.reset();
}


void DeviceServerRunner_Test::allTestsOnDeviceServer() {
    testRunConfigurationGroup();
}
