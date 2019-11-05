/**
 * Author: steffen.hauf@xfel.eu
 *
 * Copyright (c) 2010-2019 European XFEL GmbH Hamburg. All rights reserved.
 *
 */

#include "GuiServer_Test.hh"
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
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testGuiVersionServer", "GuiServerDevice",
                                                                       Hash("deviceId", "testGuiServerDevice",
                                                                            "port", 44450,
                                                                            "minClientVersion", "2.2.3"),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    boost::this_thread::sleep(boost::posix_time::milliseconds(3000));

    testVersionControl();
    testNotification();
    testExecute();
    testReconfigure();

    if (m_tcpAdapter->connected()) {
        m_tcpAdapter->disconnect();
    }

}


void GuiVersion_Test::resetClientConnection() {
    int timeout = 5000;
    if (m_tcpAdapter) {
        if (m_tcpAdapter->connected()) {
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
    std::vector < std::tuple < std::string, std::string, bool>> tests;
    tests.push_back(std::tuple < std::string, std::string, bool>(
                                                                 "version control supported", "100.1.0", true));
    tests.push_back(std::tuple < std::string, std::string, bool>(
                                                                 "version control unsupported", "0.1.0", false));
    // Tests if the instance info correctly reports scene availability
    for (const auto &test : tests) {
        const std::string& testName = std::get<0>(test);
        const std::string& version = std::get<1>(test);
        const bool connected = std::get<2>(test);
        resetClientConnection();
        int timeout = 5000;
        loginInfo.set("version", version);
        m_tcpAdapter->sendMessage(loginInfo);
        while (m_tcpAdapter->connected() && timeout > 0) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(5));
            timeout -= 5;
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Failed :" + testName, connected, m_tcpAdapter->connected());
    }

    // change the minVersion
    m_deviceClient->set<std::string>("testGuiServerDevice", "minClientVersion", "");
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
    CPPUNIT_ASSERT_MESSAGE("GUIServer disconnects client if no version control is enabled", m_tcpAdapter->connected());
    std::clog << "testVersionControl: Ok" << std::endl;
}


void GuiVersion_Test::testNotification() {
    m_deviceClient->set<std::string>("testGuiServerDevice", "minClientVersion", "2.4.1");
    // connect again
    resetClientConnection();

    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    Hash loginInfo("type", "login", "username", "mrusp", "password", "12345", "version", "2.4.0");
    karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("notification", 1, [&] {
        m_tcpAdapter->sendMessage(loginInfo);
    });

    Hash lastMessage;
    messageQ->pop(lastMessage);
    const std::string message = lastMessage.get<std::string>("message");
    CPPUNIT_ASSERT(std::string("Your GUI client has version '2.4.0', but the minimum required is: 2.4.1") == message);

    int timeout = 1500;
    while (m_tcpAdapter->connected() && timeout > 0) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
        timeout -= 5;
    }
    // the GUI server will log us out
    CPPUNIT_ASSERT(!m_tcpAdapter->connected());
    std::clog << "testNotification: OK" << std::endl;
}


void GuiVersion_Test::testExecute() {
    resetClientConnection();
    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    //
    // Request execution of slot of non-existing device
    //
    {
        const Hash h("type", "execute",
                     "deviceId", "not_there",
                     "command", "does.not.matter",
                     "reply", true,
                     "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("executeReply", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_MESSAGE(toString(replyMessage.get<Hash>("input")), h.fullyEquals(replyMessage.get<Hash>("input")));
        CPPUNIT_ASSERT(!replyMessage.get<bool>("success"));

        CPPUNIT_ASSERT_MESSAGE(replyMessage.get<std::string>("failureReason"),
                               replyMessage.get<std::string>("failureReason").find("Timeout Exception") != std::string::npos);
    }


    //
    // Request execution of non-existing slot of existing device (the GuiServerDevice itself...)
    //
    {
        const Hash h("type", "execute",
                     "deviceId", "testGuiServerDevice",
                     "command", "not.existing",
                     "reply", true,
                     "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("executeReply", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_MESSAGE(toString(replyMessage.get<Hash>("input")), h.fullyEquals(replyMessage.get<Hash>("input")));
        CPPUNIT_ASSERT(!replyMessage.get<bool>("success"));

        CPPUNIT_ASSERT_MESSAGE(replyMessage.get<std::string>("failureReason"),
                               replyMessage.get<std::string>("failureReason").find("Remote Exception") != std::string::npos);
    }

    //
    // Request execution of existing slot of existing device (the GuiServerDevice itself...)
    //
    {
        // Note that "slotGetTime" replies a Hash - but that does not matter, it is ignored.
        // Also, "execute" is meant for slots listed as SLOT_ELEMENTS - but it works for any argument less slot
        // as slotGetTime is one...
        const Hash h("type", "execute",
                     "deviceId", "testGuiServerDevice",
                     "command", "slotGetTime",
                     "reply", true,
                     "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("executeReply", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_MESSAGE(toString(replyMessage.get<Hash>("input")), h.fullyEquals(replyMessage.get<Hash>("input")));
        CPPUNIT_ASSERT(replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(!replyMessage.has("failureReason"));
    }

    //
    // Request execution of existing slot of existing device (the GuiServerDevice itself...),
    // but this time do not request for a reply.
    //
    {
        // We set the "lockedBy" property that is cleared by slotClearLock
        m_deviceClient->set("testGuiServerDevice", "lockedBy", "someone");
        CPPUNIT_ASSERT_EQUAL(std::string("someone"), m_deviceClient->get<std::string>("testGuiServerDevice", "lockedBy"));
        const Hash h("type", "execute",
                     "deviceId", "testGuiServerDevice",
                     "command", "slotClearLock");
        // "reply", false); is default
        m_tcpAdapter->sendMessage(h);

        // Just make sure that it really happened - we have to wait a bit for it:
        int timeout = 1500;
        while (!m_deviceClient->get<std::string>("testGuiServerDevice", "lockedBy").empty()) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(5));
            timeout -= 5;
        }
        CPPUNIT_ASSERT(m_deviceClient->get<std::string>("testGuiServerDevice", "lockedBy").empty());
    }

    std::clog << "testExecute: OK" << std::endl;
}


void GuiVersion_Test::testReconfigure() {
    resetClientConnection();
    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    //
    // Request reconfiguration of non-existing device
    //
    {
        const Hash h("type", "reconfigure",
                     "deviceId",
                     "not_there",
                     "configuration", Hash("whatever", 1),
                     "reply", true,
                     "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("reconfigureReply", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("reconfigureReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_MESSAGE(toString(replyMessage.get<Hash>("input")), h.fullyEquals(replyMessage.get<Hash>("input")));
        CPPUNIT_ASSERT(!replyMessage.get<bool>("success"));

        CPPUNIT_ASSERT_MESSAGE(replyMessage.get<std::string>("failureReason"),
                               replyMessage.get<std::string>("failureReason").find("Timeout Exception") != std::string::npos);
    }

    //
    // Request invalid reconfiguration of existing device (the GuiServerDevice itself...)
    //
    {
        const Hash h("type", "reconfigure",
                     "deviceId",
                     "testGuiServerDevice",
                     "configuration", Hash("whatever", 1),
                     "reply", true,
                     "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("reconfigureReply", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("reconfigureReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_MESSAGE(toString(replyMessage.get<Hash>("input")), h.fullyEquals(replyMessage.get<Hash>("input")));
        CPPUNIT_ASSERT(!replyMessage.get<bool>("success"));

        CPPUNIT_ASSERT_MESSAGE(replyMessage.get<std::string>("failureReason"),
                               replyMessage.get<std::string>("failureReason").find("Remote Exception") != std::string::npos);
    }

    //
    // Request valid reconfiguration of existing device (the GuiServerDevice itself...)
    //
    {
        const int newTarget = m_deviceClient->get<int>("testGuiServerDevice", "networkPerformance.sampleInterval") * 2;
        const Hash h("type", "reconfigure",
                     "deviceId",
                     "testGuiServerDevice",
                     "configuration", Hash("networkPerformance.sampleInterval", 10),
                     "reply", true,
                     "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("reconfigureReply", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("reconfigureReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_MESSAGE(toString(replyMessage.get<Hash>("input")), h.fullyEquals(replyMessage.get<Hash>("input")));
        CPPUNIT_ASSERT(replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(!replyMessage.has("failureReason"));
        // Just assure that it really happened:
        CPPUNIT_ASSERT_EQUAL(newTarget, m_deviceClient->get<int>("testGuiServerDevice", "networkPerformance.sampleInterval"));

    }

    //
    // Request valid reconfiguration of existing device (the GuiServerDevice itself...),
    // but this time do not request for a reply.
    //
    {
        const int newTarget = m_deviceClient->get<int>("testGuiServerDevice", "networkPerformance.sampleInterval") + 2;
        const Hash h("type", "reconfigure",
                     "deviceId",
                     "testGuiServerDevice",
                     "configuration", Hash("networkPerformance.sampleInterval", newTarget));
        // "reply", false); is default
        m_tcpAdapter->sendMessage(h);

        // Just make sure that it really happened - we have to wait a bit for it:
        int timeout = 1500;
        while (m_deviceClient->get<int>("testGuiServerDevice", "networkPerformance.sampleInterval") != newTarget && timeout > 0) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(5));
            timeout -= 5;
        }
        CPPUNIT_ASSERT_EQUAL(newTarget, m_deviceClient->get<int>("testGuiServerDevice", "networkPerformance.sampleInterval"));
    }

    std::clog << "testReconfigure: OK" << std::endl;
}