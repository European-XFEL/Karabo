/*
 * File:   GuiVersion_Test.cc
 * Author: steffen.hauf@xfel.eu
 
 */

#include "GuiVersion_Test.hh"
#include <karabo/net/EventLoop.hh>
#include <cstdlib>


using namespace std;

#define KRB_TEST_MAX_TIMEOUT 5

USING_KARABO_NAMESPACES

CPPUNIT_TEST_SUITE_REGISTRATION(GuiVersion_Test);


GuiVersion_Test::GuiVersion_Test() {
}


GuiVersion_Test::~GuiVersion_Test() {
}


void GuiVersion_Test::setUp() {
    // uncomment this if ever testing against a local broker
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);
    // Start central event-loop
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));
    // Create and start server
    Hash config("serverId", "testGuiVersionServer", "scanPlugins", false, "Logger.priority", "FATAL");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();
    // Create client
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());

}


void GuiVersion_Test::tearDown() {
    m_deviceServer.reset();
    EventLoop::stop();
    m_eventLoopThread.join();
}


void GuiVersion_Test::appTestRunner() {
    
    // bring up a GUI server and a tcp adapter to it
    std::pair<bool, std::string> success = m_deviceClient->instantiate(
        "testGuiVersionServer", "GuiServerDevice", Hash("deviceId", "testGuiServerDevice", "port", 44450,
                                                        "minClientVersion", "2.2.3"),
        KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    boost::this_thread::sleep(boost::posix_time::milliseconds(3000));

    testVersionControl();
    
    if (m_tcpAdapter->connected()) {
        m_tcpAdapter->disconnect();
    }

}

void GuiVersion_Test::resetClientConnection() {
    int timeout = 5000;
    if (m_tcpAdapter){
        if (m_tcpAdapter->connected()){
            m_tcpAdapter->disconnect();
        }
        while (m_tcpAdapter->connected() && timeout > 0) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(5));
            timeout -= 5;
        }
    }
    m_tcpAdapter = boost::shared_ptr<karabo::TcpAdapter>(new karabo::TcpAdapter(Hash("port", 44450u/*, "debug", true*/)));
    timeout = 5000;
    while (!m_tcpAdapter->connected() && timeout > 0) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
        timeout -= 5;
    }
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
}

void GuiVersion_Test::testVersionControl() {
    Hash loginInfo("type", "login", "username", "mrusp", "password", "12345", "version", "100.1.0");
    std::clog << std::endl;
    std::vector<std::tuple<std::string, std::string, bool>> tests;
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "version control supported", "100.1.0", true ));
    tests.push_back(std::tuple<std::string, std::string, bool>(
        "version control unsupported", "0.1.0", false ));
    // Tests if the instance info correctly reports scene availability
    for (auto test : tests){
        const std::string testName = std::get<0>(test);
        const std::string version = std::get<1>(test);
        const bool connected = std::get<2>(test);
        std::clog << "Test " << testName << "... ";
        resetClientConnection();
        int timeout = 5000;
        loginInfo.set("version", version);
        m_tcpAdapter->sendMessage(loginInfo);
        while (m_tcpAdapter->connected() && timeout > 0) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(5));
            timeout -= 5;
        }
        CPPUNIT_ASSERT_EQUAL(connected, m_tcpAdapter->connected());
        std::clog << "Ok" << std::endl;
            
    }
 
    std::clog << "Test no Version control.. ";
    // change the minVersion
    m_deviceClient->set<std::string>("testGuiServerDevice","minClientVersion", "");
    // connect again
    resetClientConnection();

    // check if still connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    // send overspecified development versions
    loginInfo.set("version", "1.5.4");
    m_tcpAdapter->sendMessage(loginInfo);
    int timeout = 500;
    while (m_tcpAdapter->connected() && timeout > 0) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
        timeout -= 5;
    }
    // the GUI server will not log us out
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    std::clog << "Ok" << std::endl;
}
