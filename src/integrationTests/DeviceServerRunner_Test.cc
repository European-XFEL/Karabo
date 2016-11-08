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

    // Start central event-loop
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));
    // Create and start server
    Hash config("serverId", "testDeviceServer_0", "scanPlugins", false, "Logger.priority", "ERROR");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeDeviceConstruction();
    // Create client
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());
}


void DeviceServerRunner_Test::tearDown() {
    m_deviceServer.reset();
    EventLoop::stop();
    m_eventLoopThread.join();
}


void DeviceServerRunner_Test::allTestsOnDeviceServer() {

    testRunConfigurationGroup();

}
