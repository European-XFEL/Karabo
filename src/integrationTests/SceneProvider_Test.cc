/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   SceneProvider_Test.cc
 * Author: steffen.hauf@xfel.eu

 */

#include "SceneProvider_Test.hh"

#include <cstdlib>
#include <karabo/net/EventLoop.hh>


using namespace std;

#define KRB_TEST_MAX_TIMEOUT 5

USING_KARABO_NAMESPACES

CPPUNIT_TEST_SUITE_REGISTRATION(SceneProvider_Test);


SceneProvider_Test::SceneProvider_Test() {}


SceneProvider_Test::~SceneProvider_Test() {}


void SceneProvider_Test::setUp() {
    // uncomment this if ever testing against a local broker
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);
    // Start central event-loop
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));
    // Create and start server
    Hash config("serverId", "testServerSceneProvider", "scanPlugins", false, "Logger.priority", "FATAL");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();
    // Create client
    m_deviceClient = boost::make_shared<DeviceClient>(std::string(), false);
    m_deviceClient->initialize();
}


void SceneProvider_Test::tearDown() {
    m_deviceClient.reset();
    m_deviceServer.reset();
    EventLoop::stop();
    m_eventLoopThread.join();
}


void SceneProvider_Test::appTestRunner() {
    // bring up a GUI server and a tcp adapter to it
    std::pair<bool, std::string> success =
          m_deviceClient->instantiate("testServerSceneProvider", "GuiServerDevice",
                                      Hash("deviceId", "testGuiServerScenes", "port", 44447), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
    m_tcpAdapter =
          boost::shared_ptr<karabo::TcpAdapter>(new karabo::TcpAdapter(Hash("port", 44447u /*, "debug", true*/)));
    boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    m_tcpAdapter->login();

    // in order to avoid recurring setup and tear down call all tests are run in a single runner
    success = m_deviceClient->instantiate("testServerSceneProvider", "SceneProviderTestDevice",
                                          Hash("deviceId", "sceneProvider"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    success = m_deviceClient->instantiate("testServerSceneProvider", "NonSceneProviderTestDevice",
                                          Hash("deviceId", "noSceneProvider"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    testInstanceInfo();
    testRequestScenes();
    testRequestSceneFailure();

    if (m_tcpAdapter->connected()) {
        m_tcpAdapter->disconnect();
    }
}

void SceneProvider_Test::testInstanceInfo() {
    // Tests if the instance info correctly reports scene availability
    const Hash& topo = m_deviceClient->getSystemTopology();
    CPPUNIT_ASSERT(topo.has("device"));
    const Hash& device = topo.get<Hash>("device");
    CPPUNIT_ASSERT(device.hasAttribute("sceneProvider", "capabilities"));
    CPPUNIT_ASSERT((device.getAttribute<int>("sceneProvider", "capabilities") & karabo::core::PROVIDES_SCENES) == 1u);
    CPPUNIT_ASSERT(device.hasAttribute("noSceneProvider", "capabilities"));
    CPPUNIT_ASSERT((device.getAttribute<int>("noSceneProvider", "capabilities") & karabo::core::PROVIDES_SCENES) == 0u);
    std::clog << "Tested scene providers identified in instanceInfo.. Ok" << std::endl;
}

void SceneProvider_Test::testRequestScenes() {
    const Hash arg_hash = Hash("scenes", std::vector<std::string>(1, "foo"));
    Hash message("type", "requestGeneric", "instanceId", "sceneProvider", "slot", "slotGetScenes", "args", arg_hash);
    message.set("token", "notAVeryUniqueToken");
    karabo::TcpAdapter::QueuePtr messageQ =
          m_tcpAdapter->getNextMessages("requestGeneric", 1, [&] { m_tcpAdapter->sendMessage(message); });
    Hash lastMessage;
    messageQ->pop(lastMessage);

    CPPUNIT_ASSERT(lastMessage.has("type"));
    CPPUNIT_ASSERT(lastMessage.get<std::string>("type") == "requestGeneric");
    CPPUNIT_ASSERT(lastMessage.has("reply.foo"));
    CPPUNIT_ASSERT(lastMessage.get<std::string>("reply.foo") == "encoded(bar scene)");
    CPPUNIT_ASSERT(lastMessage.has("request.token"));
    CPPUNIT_ASSERT(lastMessage.get<std::string>("request.token") == "notAVeryUniqueToken");
    CPPUNIT_ASSERT(lastMessage.has("success"));
    CPPUNIT_ASSERT(lastMessage.get<bool>("success") == true);
    std::clog << "Tested scene retrieval via GUI server.. Ok" << std::endl;
}

void SceneProvider_Test::testRequestSceneFailure() {
    // here we request from a device that doesn't provide scenes
    const Hash arg_hash = Hash("scenes", std::vector<std::string>(1, "foo"));
    Hash message("type", "requestGeneric", "instanceId", "noSceneProvider", "slot", "slotGetScenes", "args", arg_hash);
    message.set("token", "notAVeryUniqueToken");
    karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages(
          "requestGeneric", 1, [&] { m_tcpAdapter->sendMessage(message); }, 10000);
    Hash lastMessage;
    messageQ->pop(lastMessage);

    CPPUNIT_ASSERT(lastMessage.has("request.type"));
    CPPUNIT_ASSERT(lastMessage.get<std::string>("request.type") == "requestGeneric");
    CPPUNIT_ASSERT(lastMessage.has("request.token"));
    CPPUNIT_ASSERT(lastMessage.get<std::string>("request.token") == "notAVeryUniqueToken");
    CPPUNIT_ASSERT(lastMessage.has("success"));
    CPPUNIT_ASSERT(lastMessage.get<bool>("success") == false);
    CPPUNIT_ASSERT(lastMessage.has("reason"));
    CPPUNIT_ASSERT(lastMessage.get<std::string>("reason").find("'noSceneProvider' has no slot 'slotGetScenes'") !=
                   std::string::npos);

    std::clog << "Tested scene retrieval failure (device doesn't provide scenes).. Ok" << std::endl;
}
