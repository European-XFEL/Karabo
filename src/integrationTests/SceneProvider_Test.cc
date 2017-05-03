/*
 * File:   SceneProvider_Test.cc
 * Author: steffen.hauf@xfel.eu
 
 */

#include "SceneProvider_Test.hh"
#include <karabo/net/EventLoop.hh>

using namespace std;

#define KRB_TEST_MAX_TIMEOUT 5

USING_KARABO_NAMESPACES

CPPUNIT_TEST_SUITE_REGISTRATION(SceneProvider_Test);


SceneProvider_Test::SceneProvider_Test() {
}


SceneProvider_Test::~SceneProvider_Test() {
}


void SceneProvider_Test::setUp() {

    // Start central event-loop
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));
    // Create and start server
    Hash config("serverId", "testServerSceneProvider", "scanPlugins", false, "Logger.priority", "FATAL");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();
    // Create client
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());

}


void SceneProvider_Test::tearDown() {
    m_deviceServer.reset();
    EventLoop::stop();
    m_eventLoopThread.join();
}


void SceneProvider_Test::appTestRunner() {

    // in order to avoid recurring setup and tear down call all tests are run in a single runner
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerSceneProvider", "SceneProviderTestDevice",
                                                                       Hash("deviceId", "sceneProvider"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    success = m_deviceClient->instantiate("testServerSceneProvider", "NonSceneProviderTestDevice",
                                          Hash("deviceId", "noSceneProvider"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    testInstanceInfo();

}

void SceneProvider_Test::testInstanceInfo() {
    // Tests if the instance info correctly reports scene availability
    const Hash& topo = m_deviceClient->getSystemTopology();
    CPPUNIT_ASSERT(topo.has("device"));
    const Hash& device = topo.get<Hash>("device");
    CPPUNIT_ASSERT(device.hasAttribute("sceneProvider", "capabilities"));
    CPPUNIT_ASSERT((device.getAttribute<unsigned int>("sceneProvider", "capabilities") & karabo::core::PROVIDES_SCENES) == 1u );
    CPPUNIT_ASSERT(device.hasAttribute("noSceneProvider", "capabilities"));
    CPPUNIT_ASSERT((device.getAttribute<unsigned int>("noSceneProvider", "capabilities") & karabo::core::PROVIDES_SCENES) == 0u);

}