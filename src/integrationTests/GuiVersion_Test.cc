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
                                                        "strictClientVersion", true, "minClientVersion", "2.2.3"),
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
    // Tests if the instance info correctly reports scene availability
    resetClientConnection();
    Hash loginInfo("type", "login", "username", "mrusp", "password", "12345", "version", "100.1.0");
    int timeout = 5000;
    m_tcpAdapter->sendMessage(loginInfo);
    while (m_tcpAdapter->connected() && timeout > 0) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
        timeout -= 5;
    }
    // Should be still connected!
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    std::clog << "Tested version control Correct Version.. Ok" << std::endl;

    // connect again
    resetClientConnection();

    // still connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    // send old version
    loginInfo.set("version", "0.1.0");
    m_tcpAdapter->sendMessage(loginInfo);

    timeout = 500;
    while (m_tcpAdapter->connected() && timeout > 0) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
        timeout -= 5;
    }

    // the GUI server will log us out
    CPPUNIT_ASSERT(!m_tcpAdapter->connected());
    std::clog << "Tested version control Unsupported Version.. Ok" << std::endl;

    // connect again
    resetClientConnection();

    // check if still connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    // send funny version
    loginInfo.set("version", "10.");
    m_tcpAdapter->sendMessage(loginInfo);
    timeout = 500;
    while (m_tcpAdapter->connected() && timeout > 0) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
        timeout -= 5;
    }
    // the GUI server will log us out
    CPPUNIT_ASSERT(!m_tcpAdapter->connected());
    std::clog << "Tested version control malformatted Version.. Ok" << std::endl;

    // connect again
    resetClientConnection();

    // check if still connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    // send sha1-like shtuff
    loginInfo.set("version", "2f1c169");
    m_tcpAdapter->sendMessage(loginInfo);
    timeout = 500;
    while (m_tcpAdapter->connected() && timeout > 0) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
        timeout -= 5;
    }
    // the GUI server will log us out
    CPPUNIT_ASSERT(!m_tcpAdapter->connected());
    std::clog << "Tested SHA1-like Version.. Ok" << std::endl;

    // connect again
    resetClientConnection();

    // check if still connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    // send overspecified versions
    loginInfo.set("version", "2.2.3.5");
    m_tcpAdapter->sendMessage(loginInfo);
    timeout = 500;
    while (m_tcpAdapter->connected() && timeout > 0) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
        timeout -= 5;
    }
    // the GUI server will not log us out
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    std::clog << "Tested overspecified Version.. Ok" << std::endl;

    // connect again
    resetClientConnection();

    // check if still connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());


    // send pre-released versions
    loginInfo.set("version", "2.2.3a5");
    m_tcpAdapter->sendMessage(loginInfo);
    timeout = 500;
    while (m_tcpAdapter->connected() && timeout > 0) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
        timeout -= 5;
    }
    // the GUI server will not log us out
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    std::clog << "Tested alpha Version.. Ok" << std::endl;

    // connect again
    resetClientConnection();

    // check if still connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    // send overspecified development versions
    loginInfo.set("version", "2.2.3dev5");
    m_tcpAdapter->sendMessage(loginInfo);
    timeout = 500;
    while (m_tcpAdapter->connected() && timeout > 0) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
        timeout -= 5;
    }
    // the GUI server will not log us out
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    std::clog << "Tested dev Version.. Ok" << std::endl;
    
}
