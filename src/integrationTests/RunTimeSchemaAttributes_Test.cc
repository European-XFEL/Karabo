/* 
 * File:   RunTimeSchemaAttributes.cc
 * Author: steffen
 * 
 * Created on September 13, 2016, 7:10 PM
 */

#include "RunTimeSchemaAttributes_Test.hh"


USING_KARABO_NAMESPACES;

CPPUNIT_TEST_SUITE_REGISTRATION(RunTimeSchemaAttributes_Test);

#define KRB_TEST_MAX_TIMEOUT 10


RunTimeSchemaAttributes_Test::RunTimeSchemaAttributes_Test() {

}


RunTimeSchemaAttributes_Test::~RunTimeSchemaAttributes_Test() {

}


void RunTimeSchemaAttributes_Test::setUp() {

    // Start central event-loop
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));
    // Create and start server
    Hash config("serverId", "testServerSchema", "scanPlugins", false, "Logger.priority", "ERROR");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeDeviceConstruction();
    // Create client
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());
}


void RunTimeSchemaAttributes_Test::tearDown() {
    m_deviceServer.reset();
    EventLoop::stop();
    m_eventLoopThread.join();
}


void RunTimeSchemaAttributes_Test::appTestRunner() {

    // in order to avoid recurring setup and tear down call all tests are run in a single runner
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerSchema", "GuiServerDevice", Hash("deviceId", "testGuiServerSchema", "port", 44447), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    boost::this_thread::sleep(boost::posix_time::milliseconds(3000));
    m_tcpAdapter = boost::shared_ptr<karabo::TcpAdapter>(new karabo::TcpAdapter(Hash("port", 44447u/*, "debug", true*/)));
    boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
    CPPUNIT_ASSERT(m_tcpAdapter->connected());


    success = m_deviceClient->instantiate("testServerSchema", "AlarmTester", Hash("deviceId", "alarmTesterSchema"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    boost::this_thread::sleep(boost::posix_time::milliseconds(5000));

    testRuntimeApplication();
    testGuiServerApplication();
    testGuiServerApplicationFailure();

    if (m_tcpAdapter->connected()) {
        m_tcpAdapter->disconnect();
    }

}


void RunTimeSchemaAttributes_Test::testRuntimeApplication() {
    std::vector<karabo::util::Hash> schemaUpdates;
    schemaUpdates.push_back(Hash("path", "floatProperty", "attribute", "warnLow", "value", -1000.0));
    schemaUpdates.push_back(Hash("path", "floatProperty", "attribute", "minInc", "value", -10.0));
    //register a dummy monitor to assure that signals from the device are tracked
    m_deviceClient->registerDeviceMonitor("alarmTesterSchema", boost::bind(&RunTimeSchemaAttributes_Test::dummyMonitor, this, _1, _2));
    m_deviceClient->setAttribute("alarmTesterSchema", "floatProperty", "warnLow", -1000.0);
    m_deviceClient->setAttribute("alarmTesterSchema", "floatProperty", "minInc", -10.0);
    boost::this_thread::sleep(boost::posix_time::milliseconds(5000));
    const karabo::util::Schema& s = m_deviceClient->getDeviceSchema("alarmTesterSchema");


    CPPUNIT_ASSERT(s.getWarnLow<float>("floatProperty") == -1000.0);
    CPPUNIT_ASSERT(s.getMinInc<float>("floatProperty") == -10.0);
}


void RunTimeSchemaAttributes_Test::testGuiServerApplication() {

    std::vector<karabo::util::Hash> schemaUpdates;
    schemaUpdates.push_back(Hash("path", "floatProperty", "attribute", "warnHigh", "value", 1000.0));
    schemaUpdates.push_back(Hash("path", "floatProperty", "attribute", "maxInc", "value", 10.0));

    Hash message("type", "updateAttributes", "instanceId", "alarmTesterSchema", "updates", schemaUpdates);
    karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("attributesUpdated", 1, [&] {
        m_tcpAdapter->sendMessage(message);
    });
    Hash lastMessage;
    messageQ->pop(lastMessage);

    CPPUNIT_ASSERT(lastMessage.get<bool>("reply.success"));
    CPPUNIT_ASSERT(lastMessage.get<std::string>("reply.instanceId") == "alarmTesterSchema");
    CPPUNIT_ASSERT(lastMessage.get<std::vector<Hash> > ("reply.requestedUpdate") == schemaUpdates);
    const Schema& s = lastMessage.get<Schema>("reply.updatedSchema");
    CPPUNIT_ASSERT(s.getWarnHigh<float>("floatProperty") == 1000.0);
    CPPUNIT_ASSERT(s.getMaxInc<float>("floatProperty") == 10.0);
}


void RunTimeSchemaAttributes_Test::testGuiServerApplicationFailure() {

    std::vector<karabo::util::Hash> schemaUpdates;
    schemaUpdates.push_back(Hash("path", "floatProperty", "attribute", "warnHigh", "value", 50.0));
    schemaUpdates.push_back(Hash("path", "floatProperty", "attribute", "maxInc", "value", "this will Fail"));
    schemaUpdates.push_back(Hash("path", "floatProperty", "attribute", "alarmHigh", "value", 500.0));

    Hash message("type", "updateAttributes", "instanceId", "alarmTesterSchema", "updates", schemaUpdates);
    karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("attributesUpdated", 1, [&] {
        m_tcpAdapter->sendMessage(message);
    });
    Hash lastMessage;
    messageQ->pop(lastMessage);

    CPPUNIT_ASSERT(lastMessage.get<bool>("reply.success") == false);
    CPPUNIT_ASSERT(lastMessage.get<std::string>("reply.instanceId") == "alarmTesterSchema");
    CPPUNIT_ASSERT(lastMessage.get<std::vector<Hash> > ("reply.requestedUpdate") == schemaUpdates);
    const Schema& s = lastMessage.get<Schema>("reply.updatedSchema");
    //this update should have passed through
    CPPUNIT_ASSERT(s.getWarnHigh<float>("floatProperty") == 50.0);
    //we failed at updating this one, it is at its previous value
    CPPUNIT_ASSERT(s.getMaxInc<float>("floatProperty") == 10.0);
    //this update is okay again
    CPPUNIT_ASSERT(s.getAlarmHigh<float>("floatProperty") == 500.0);
}


void RunTimeSchemaAttributes_Test::dummyMonitor(const std::string&, const karabo::util::Hash&) {

};