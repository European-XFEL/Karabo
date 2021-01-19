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

bool waitForCondition(boost::function<bool() > checker, unsigned int timeoutMillis) {
    const unsigned int sleepIntervalMillis = 5;
    unsigned int numOfWaits = 0;
    const unsigned int maxNumOfWaits = static_cast<unsigned int> (std::ceil(timeoutMillis / sleepIntervalMillis));
    while (numOfWaits < maxNumOfWaits && !checker()) {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(sleepIntervalMillis));
        numOfWaits++;
    }
    return (numOfWaits < maxNumOfWaits);
}


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

    // bring up a GUI server
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testGuiVersionServer", "GuiServerDevice",
                                                                       Hash("deviceId", "testGuiServerDevice",
                                                                            "port", 44450,
                                                                            "minClientVersion", "2.2.3",
                                                                            "timeout", 0),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    waitForCondition([this](){
        auto state = m_deviceClient->get<State>("testGuiServerDevice", "state");
        return state == State::ON;
    }, KRB_TEST_MAX_TIMEOUT * 1000);

    testVersionControl();
    testExecuteBeforeLogin();
    testExecute();
    testSlowSlots();
    testReconfigure();
    testDeviceConfigUpdates();
    testDisconnect();
    testRequestGeneric();
    testRequestFailProtocol();
    testRequestFailOldVersion();

    if (m_tcpAdapter->connected()) {
        m_tcpAdapter->disconnect();
    }
    // Shutdown GUI Server device to reconfigure for readOnly
    success = m_deviceClient->killDevice("testGuiServerDevice", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // bring up a GUI server and a tcp adapter to it
    std::pair<bool, std::string> success_n = m_deviceClient->instantiate("testGuiVersionServer", "GuiServerDevice",
                                                                         Hash("deviceId", "testGuiServerDevice",
                                                                              "port", 44450,
                                                                              "minClientVersion", "2.2.3",
                                                                              "isReadOnly", true,
                                                                              "timeout", 0),
                                                                         KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success_n.second, success_n.first);
    waitForCondition([this](){
        auto state = m_deviceClient->get<State>("testGuiServerDevice", "state");
        return state == State::ON;
    }, KRB_TEST_MAX_TIMEOUT * 1000);

    testReadOnly();

    if (m_tcpAdapter->connected()) {
        m_tcpAdapter->disconnect();
    }
}


void GuiVersion_Test::resetTcpConnection() {
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


void GuiVersion_Test::resetClientConnection(const karabo::util::Hash& loginData) {
    resetTcpConnection();
    karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("systemTopology", 1, [&] {
        m_tcpAdapter->sendMessage(loginData);
    });
    Hash lastMessage;
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT(lastMessage.has("systemTopology"));
}


void GuiVersion_Test::resetClientConnection() {
    resetTcpConnection();
    m_tcpAdapter->login();
}


void GuiVersion_Test::testVersionControl() {
    Hash loginInfo("type", "login", "username", "mrusp", "password", "12345", "version", "100.1.0");
    // description , client version, server version, should connect
    typedef std::tuple < std::string, std::string, std::string, bool> TestData;
    std::vector <TestData> tests;
    tests.push_back(TestData("version control supported", "100.1.0", "2.11.0", true));
    tests.push_back(TestData("version control unsupported", "0.1.0", "2.11.0", false));
    tests.push_back(TestData("version control disabled", "0.1.0", "", true));
    for (const auto &test : tests) {
        const std::string& testName = std::get<0>(test);
        const std::string& version = std::get<1>(test);
        const std::string& serverMinVersion = std::get<2>(test);
        const bool connected = std::get<3>(test);
        // set server minimum version
        m_deviceClient->set<std::string>("testGuiServerDevice", "minClientVersion", serverMinVersion);
        resetTcpConnection();
        loginInfo.set("version", version);
        Hash lastMessage;
        if (connected) {
            karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("systemTopology", 1, [&] {
                m_tcpAdapter->sendMessage(loginInfo);
            });
            messageQ->pop(lastMessage);
            CPPUNIT_ASSERT(lastMessage.has("systemTopology"));
        } else {
            karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("notification", 1, [&] {
                m_tcpAdapter->sendMessage(loginInfo);
            });
            messageQ->pop(lastMessage);
            const std::string& message = lastMessage.get<std::string>("message");
            CPPUNIT_ASSERT_MESSAGE(message, message.rfind("Your GUI client has version '" + version + "', but the minimum required is:", 0u) == 0u);
            int timeout = 1500;
            // wait for the GUI server to log us out
            while (m_tcpAdapter->connected() && timeout > 0) {
                boost::this_thread::sleep(boost::posix_time::milliseconds(5));
                timeout -= 5;
            }
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Failed :" + testName, connected, m_tcpAdapter->connected());
    }

    std::clog << "testVersionControl: Ok" << std::endl;
}


void GuiVersion_Test::testReadOnly() {
    resetClientConnection();
    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    //
    // Request execution of slot although the server is in readOnly mode!
    //
    std::vector<Hash> commands = {
        Hash("type", "execute"),
        Hash("type", "killDevice"),
        Hash("type", "projectSaveItems"),
        Hash("type", "initDevice"),
        Hash("type", "requestFromSlot", "slot", "thisSlotShouldBeRejected"),
        Hash("type", "killServer"),
        Hash("type", "acknowledgeAlarm"),
        Hash("type", "projectUpdateAttribute"),
        Hash("type", "updateAttributes"),
        Hash("type", "reconfigure"),
        Hash("type", "requestGeneric", "slot", "slotSaveConfigurationFromName"),
        };
    for (const Hash &h : commands) {
        const std::string& type = h.get<std::string>("type");
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("notification", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        const std::string& message = replyMessage.get<std::string>("message");
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Command: " + toString(h), std::string("Action '" + type + "' is not allowed on GUI servers in readOnly mode!"), message);
        std::clog << "testReadOnly: OK for " + type  << std::endl;
    }
    const unsigned int messageTimeout = 500u;
    Hash reqScene("type", "requestFromSlot",
                  "deviceId", "doesNotMatter",
                  "slot", "requestScene");
    CPPUNIT_ASSERT_THROW(
                         m_tcpAdapter->getNextMessages("notification", 1, [&] {
                             m_tcpAdapter->sendMessage(reqScene);
                         }, messageTimeout), karabo::util::TimeoutException);
    std::clog << "testReadOnly: OK for requestFromSlot with requestScene" << std::endl;

    Hash reqSlotScene("type", "requestFromSlot",
                      "deviceId", "doesNotMatter",
                      "slot", "slotGetScene");
    CPPUNIT_ASSERT_THROW(
                         m_tcpAdapter->getNextMessages("notification", 1, [&] {
                             m_tcpAdapter->sendMessage(reqSlotScene);
                         }, messageTimeout), karabo::util::TimeoutException);
    std::clog << "testReadOnly: OK for requestFromSlot with slotGetScene" << std::endl;
}

void GuiVersion_Test::testExecuteBeforeLogin() {
    std::clog << "testExecuteBeforeLogin: ";

    resetTcpConnection();
    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    //
    // Request of any type other than login, will fail.
    //
    std::vector<std::string> blockedTypes = {
        "execute",
        "requestFromSlot",
        "reconfigure",
        "getDeviceConfiguration",
        "getDeviceSchema",
        "getClassSchema",
        "initDevice",
        "killServer",
        "killDevice",
        "startMonitoringDevice",
        "stopMonitoringDevice",
        "getPropertyHistory",
        "getConfigurationFromPast",
        "subscribeNetwork",
        "requestNetwork",
        "error",
        "acknowledgeAlarm",
        "requestAlarms",
        "updateAttributes",
        "projectBeginUserSession",
        "projectEndUserSession",
        "projectSaveItems",
        "projectLoadItems",
        "projectListProjectManagers",
        "projectListItems",
        "projectListDomains",
        "projectUpdateAttribute",
        "requestGeneric"
    };
    for (const std::string& type : blockedTypes) {
        const Hash h("type", type);
        // no other argument should be needed, since the requests are rejected before
        // the arguments are parsed.
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("notification", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        const std::string& message = replyMessage.get<std::string>("message");
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Received Hash: " + toString(replyMessage),
                                     std::string("Action '" + type + "' refused before log in"),
                                     message);
    }
    // The `login` type is implicitly tested by `resetClientConnection()`
    m_tcpAdapter->disconnect();
    std::clog << "OK" << std::endl;
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

        CPPUNIT_ASSERT_EQUAL(std::string("Failure on request to execute 'does.not.matter' on device 'not_there'. Request "\
                                         "not answered within 1 seconds."),
                             replyMessage.get<std::string>("failureReason"));
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

        // First part of fail message is fixed, details contain 'Remote Exception':
        CPPUNIT_ASSERT_EQUAL_MESSAGE(replyMessage.get<std::string>("failureReason"), 0ul,
                                     replyMessage.get<std::string>("failureReason")
                                     .find("Failure on request to execute 'not.existing' on device 'testGuiServerDevice', details:\n"));
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


void GuiVersion_Test::testRequestFailProtocol() {
    resetClientConnection();
    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    const unsigned int messageTimeout = 2000u;
    {
        const std::string type = "GuiServerDoesNotHaveThisType";
        Hash h("type", type);

        const Hash conf = m_deviceClient->get("testGuiServerDevice");
        const std::string& classVersion = conf.get<string>("classVersion");

        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("notification", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        }, messageTimeout);
        Hash replyMessage;
        messageQ->pop(replyMessage);

        std::string assert_message = "The gui server with version " + classVersion + " does not support the client application request of " + type;
        CPPUNIT_ASSERT_EQUAL(assert_message, replyMessage.get<std::string>("message"));

        std::clog << "testRequestFailProtocol: OK" << std::endl;
    }
}


void GuiVersion_Test::testRequestFailOldVersion() {
    // independently from the minimum Client version configured,
    // we want to block certain actions to be performed.
    // for example: `projectSaveItems` can be poisonous for the database.
    m_deviceClient->set<std::string>("testGuiServerDevice", "minClientVersion", "2.9.1");
    std::clog << "testRequestFailOldVersion: " ;
    // connect again
    resetClientConnection(Hash("type", "login", "username", "mrusp", "password", "12345", "version", "2.9.1"));

    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    const unsigned int messageTimeout = 2000u;
    {
        const std::string type = "projectSaveItems";
        Hash h("type", type);  // no other arguments are needed.
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("notification", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        }, messageTimeout);
        Hash replyMessage;
        messageQ->pop(replyMessage);

        std::string assert_message = "Action '" + type + "' is not allowed on this GUI client version. Please upgrade your GUI client";
        CPPUNIT_ASSERT_EQUAL(assert_message, replyMessage.get<std::string>("message"));

        std::clog << "OK" << std::endl;
    }
}


void GuiVersion_Test::testRequestGeneric() {
    resetClientConnection();
    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    const unsigned int messageTimeout = 2000u;
    {
        Hash h("type", "requestGeneric",
               "instanceId", "isnotonline",
               "timeout", 1,
               "slot", "requestScene");
        h.set("args", Hash("name", "scene"));

        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("requestGeneric", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        }, messageTimeout);
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(false, replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT_EQUAL(std::string("requestGeneric"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_EQUAL(std::string("scene"), replyMessage.get<std::string>("request.args.name"));
        std::clog << "requestGeneric: OK without specified replyType" << std::endl;
    }
    {
        Hash h("type", "requestGeneric",
               "instanceId", "isnotonline",
               "timeout", 1,
               "replyType", "requestSuperScene",
               "slot", "slotDumpDebugInfo");
        h.set("args", Hash("name", "noname"));

        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("requestSuperScene", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        }, messageTimeout);
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(false, replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(h.fullyEquals(replyMessage.get<Hash>("request")));
        CPPUNIT_ASSERT_EQUAL(std::string("Failure on request to isnotonline.slotDumpDebugInfo, not answered within 1 seconds."),
                             replyMessage.get<std::string>("reason"));
        CPPUNIT_ASSERT_EQUAL(std::string("requestSuperScene"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_EQUAL(std::string("noname"), replyMessage.get<std::string>("request.args.name"));

        std::clog << "requestGeneric: OK different replyType" << std::endl;
    }
    {
        Hash h("type", "requestGeneric",
               "instanceId", "testGuiServerDevice",
               "timeout", 1,
               "replyType", "debug",
               "empty", true,
               "slot", "slotDumpDebugInfo");
        h.set("args", Hash("clients", true));

        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("debug", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        }, messageTimeout);
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(true, replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT_EQUAL(std::string("debug"), replyMessage.get<std::string>("type"));
        const Hash& request = replyMessage.get<Hash>("request");
        CPPUNIT_ASSERT(request.empty());
        const Hash& clients = replyMessage.get<Hash>("reply");
        const int number_clients = clients.size();
        CPPUNIT_ASSERT_EQUAL(1, number_clients);

        std::clog << "requestGeneric: OK with online device and empty request" << std::endl;
    }
}


void GuiVersion_Test::testSlowSlots() {
    resetClientConnection();
    // bring up a PropertyTestDevice
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testGuiVersionServer",
                                                                       "PropertyTest",
                                                                       Hash("deviceId", "testGuiServerDevicePropertyTest"),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    //
    // Request execution of existing slot of the PropertyTest device `testGuiServerDevicePropertyTest`
    // The slot `slowSlot` does not appear in the schema, but the slowSlot has the same signature
    // as a karabo command, i.e. no arguments. The slot takes 2 seconds therefore it should timeout.
    //
    {
        const Hash h("type", "execute",
                     "deviceId", "testGuiServerDevicePropertyTest",
                     "command", "slowSlot",
                     "reply", true,
                     "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("executeReply", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(!replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(replyMessage.has("failureReason"));
        const std::string& failureMsg = replyMessage.get<std::string>("failureReason");
        CPPUNIT_ASSERT_MESSAGE(failureMsg, failureMsg.find("Request not answered within 1 seconds") != std::string::npos);
    }

    ////////////////////////////////////////////////////////////////
    //
    // Request execution of existing slot of the PropertyTest device `testGuiServerDevicePropertyTest`
    // After setting the `ignoreTimeoutClasses` the call will succeed.
    //
    m_deviceClient->set("testGuiServerDevice", "ignoreTimeoutClasses", std::vector<std::string>({"PropertyTest"}));
    {
        const Hash h("type", "execute",
                     "deviceId", "testGuiServerDevicePropertyTest",
                     "command", "slowSlot",
                     "reply", true,
                     "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("executeReply", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        std::string message = (replyMessage.has("failureReason"))? replyMessage.get<std::string>("failureReason") : "NO REASON";
        CPPUNIT_ASSERT_MESSAGE(message, replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(!replyMessage.has("failureReason"));
    }
    //
    // Test that the server will handle timeout after removing "PropertyTest" from the list of bad guys
    // before shutting down the test device.
    //
    m_deviceClient->set("testGuiServerDevice", "ignoreTimeoutClasses", std::vector<std::string>());
    {
        const Hash h("type", "execute",
                     "deviceId", "testGuiServerDevicePropertyTest",
                     "command", "slowSlot",
                     "reply", true,
                     "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("executeReply", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(!replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(replyMessage.has("failureReason"));
        const std::string& failureMsg = replyMessage.get<std::string>("failureReason");
        CPPUNIT_ASSERT_MESSAGE(failureMsg, failureMsg.find("Request not answered within 1 seconds") != std::string::npos);
    }

    ////////////////////////////////////////////////////////////////
    //
    // Request execution of existing slot of the PropertyTest device `testGuiServerDevicePropertyTest`
    // After setting the a larger`"timeout" the call will succeed.
    //
    const int previousTimeout = m_deviceClient->get<int>("testGuiServerDevice", "timeout");
    m_deviceClient->set("testGuiServerDevice", "timeout", 30);
    {
        const Hash h("type", "execute",
                     "deviceId", "testGuiServerDevicePropertyTest",
                     "command", "slowSlot",
                     "reply", true,
                     "timeout", 1); // smaller than the "timeout" property of the server, so gets ignored
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("executeReply", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        std::string message = (replyMessage.has("failureReason")) ? replyMessage.get<std::string>("failureReason") : "NO REASON";
        CPPUNIT_ASSERT_MESSAGE(message, replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(!replyMessage.has("failureReason"));
    }
    //
    // Test that the server will handle timeout after  resetting the "timeout" property.
    //
    m_deviceClient->set("testGuiServerDevice", "timeout", previousTimeout);
    {
        const Hash h("type", "execute",
                     "deviceId", "testGuiServerDevicePropertyTest",
                     "command", "slowSlot",
                     "reply", true,
                     "timeout", 1); // now this rules again, so the 2s slow slowSlot will timeout again
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("executeReply", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(!replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(replyMessage.has("failureReason"));
        const std::string& failureMsg = replyMessage.get<std::string>("failureReason");
        CPPUNIT_ASSERT_MESSAGE(failureMsg, failureMsg.find("Request not answered within 1 seconds") != std::string::npos);
    }

    // Clean up. Shutdown the PropertyTest device.

    success = m_deviceClient->killDevice("testGuiServerDevicePropertyTest", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);


    std::clog << "testSlowSlots: OK" << std::endl;
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

        CPPUNIT_ASSERT_EQUAL(std::string("Failure on request to reconfigure 'whatever' of device 'not_there'. Request not answered within 1 seconds."),
                             replyMessage.get<std::string>("failureReason"));
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

        // Start of fail message is well defined, details contain 'Remote Exception':
        CPPUNIT_ASSERT_EQUAL_MESSAGE(replyMessage.get<std::string>("failureReason"), 0ul,
                                     replyMessage.get<std::string>("failureReason")
                                     .find("Failure on request to reconfigure 'whatever' of device 'testGuiServerDevice', details:\n"));
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


void GuiVersion_Test::testDeviceConfigUpdates() {
    resetClientConnection();
    // checks that we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    const unsigned int propertyUpdateInterval = 1500u; // A propertyUpdateInterval that is large enough so that the distance
    // between a reference timestamp gathered right after an update interval
    // "pulse" and the real "pulse" timestamp is at least one order of magnitu-
    // de smaller than the interval duration - (with 1500 we are allowing that
    // distance to be up to 150 ms, which is quite reasonable even in situations
    // where the running system is under a heavy load).
    m_deviceClient->set<int>("testGuiServerDevice", "propertyUpdateInterval", propertyUpdateInterval);
    const unsigned int nextMessageTimeout = propertyUpdateInterval + 500u;

    // Instantiate two property test devices
    std::pair<bool, std::string> success = m_deviceClient->instantiate("testGuiVersionServer", "PropertyTest",
                                                                       Hash("deviceId", "PropTest_1"),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    success = m_deviceClient->instantiate("testGuiVersionServer", "PropertyTest",
                                          Hash("deviceId", "PropTest_2"),
                                          KRB_TEST_MAX_TIMEOUT);

    CPPUNIT_ASSERT(success.first);

    // Changes a property of one of the test devices and makes sure that no message 'deviceConfigurations' arrives
    // within the propertyUpdateInterval.
    {
        const Hash h("type", "reconfigure",
                     "deviceId",
                     "PropTest_1",
                     "configuration", Hash("int32Property", 10));
        CPPUNIT_ASSERT_THROW(
                             m_tcpAdapter->getNextMessages("deviceConfigurations", 1, [&] {
                                 m_tcpAdapter->sendMessage(h);
                             }, nextMessageTimeout), karabo::util::TimeoutException);
        // Makes sure that the property has been set.
        CPPUNIT_ASSERT_EQUAL(10, m_deviceClient->get<int>("PropTest_1", "int32Property"));
    }

    // "Subscribes" to one of the test property devices by sending the GUI Server a 'startMonitoringDevice' message.
    {
        const Hash h("type", "startMonitoringDevice",
                     "deviceId",
                     "PropTest_1");
        // After receiving a startMonitoringDevice, the GUI Server sends a 'deviceConfigurations' message with
        // the full configuration it has for the device.
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("deviceConfigurations", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        }, nextMessageTimeout);
        Hash nextMessage;
        messageQ->pop(nextMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("deviceConfigurations"), nextMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(nextMessage.has("configurations.PropTest_1"));
    }

    // Changes properties on the two devices and assures that an update message arrives containing only the change
    // to the subscribed one.
    // NOTE: From this point on, the order of the operations matters - there's a synchronization code before an upcoming
    //       property change test that is based on the timestamp that will be stored in propUpdateTime during the test
    //       below.
    Epochstamp propUpdateTime;
    {
        const Hash h_1("type", "reconfigure",
                       "deviceId",
                       "PropTest_1",
                       "configuration", Hash("int32Property", 12));
        const Hash h_2("type", "reconfigure",
                       "deviceId",
                       "PropTest_2",
                       "configuration", Hash("int32Property", 22));

        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("deviceConfigurations", 1, [&] {
            m_tcpAdapter->sendMessage(h_2);
            m_tcpAdapter->sendMessage(h_1);
        }, nextMessageTimeout);

        propUpdateTime.now(); // Captures a timestamp that is as close as possible to the update "pulse".

        Hash nextMessage;
        messageQ->pop(nextMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("deviceConfigurations"), nextMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(nextMessage.has("configurations"));
        Hash configs = nextMessage.get<Hash>("configurations");
        CPPUNIT_ASSERT(configs.has("PropTest_1"));
        Hash propTest1Config = configs.get<Hash>("PropTest_1");
        CPPUNIT_ASSERT(propTest1Config.get<int>("int32Property") == 12);
        CPPUNIT_ASSERT(configs.size() == 1u);
    }

    // "Subscribes" to the yet unsubscribed test device.
    {
        const Hash h("type", "startMonitoringDevice",
                     "deviceId",
                     "PropTest_2",
                     "reply", true,
                     "timeout", 1);
        // After receiving a startMonitoringDevice, the GUI Server sends a 'deviceConfigurations' message with
        // the full configuration it has for the device.
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("deviceConfigurations", 1, [&] {
            m_tcpAdapter->sendMessage(h);
        }, nextMessageTimeout);
        Hash nextMessage;
        messageQ->pop(nextMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("deviceConfigurations"), nextMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(nextMessage.has("configurations.PropTest_2"));
    }

    // Changes properties on both test devices and assures that an update message arrives containing the changes
    // to both devices.
    {
        const Hash h_1("type", "reconfigure",
                       "deviceId",
                       "PropTest_1",
                       "configuration", Hash("int32Property", 14));
        const Hash h_2("type", "reconfigure",
                       "deviceId",
                       "PropTest_2",
                       "configuration", Hash("int32Property", 24));

        // Syncs as close as possible to the next update "pulse" - we'll need that for the next check, which is supposed
        // to get the two updates in the same cycle.
        Epochstamp targetTime(propUpdateTime);
        Epochstamp currentTime;
        // Duration constructor takes care of overflow of fractions.
        const TimeDuration duration(0ull,
                                    propertyUpdateInterval * 1000000000000000ull); // 10^15 => factor from ms. to attosecs.
        const int tolerance = propertyUpdateInterval / 15;
        do {
            targetTime += duration;
        } while (targetTime < currentTime &&
                 targetTime.elapsed(currentTime).getFractions(TIME_UNITS::MILLISEC) <= tolerance);
        boost::this_thread::sleep(boost::posix_time::milliseconds(targetTime.elapsed().getFractions(TIME_UNITS::MILLISEC)));

        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages("deviceConfigurations", 1, [&] {
            m_tcpAdapter->sendMessage(h_2);
            m_tcpAdapter->sendMessage(h_1);
        }, nextMessageTimeout);

        Hash nextMessage;
        messageQ->pop(nextMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("deviceConfigurations"), nextMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(nextMessage.has("configurations"));
        Hash configs = nextMessage.get<Hash>("configurations");
        CPPUNIT_ASSERT(configs.has("PropTest_1"));
        Hash propTest1Config = configs.get<Hash>("PropTest_1");
        CPPUNIT_ASSERT(propTest1Config.get<int>("int32Property") == 14);
        CPPUNIT_ASSERT(configs.has("PropTest_2"));
        Hash propTest2Config = configs.get<Hash>("PropTest_2");
        CPPUNIT_ASSERT(propTest2Config.get<int>("int32Property") == 24);
        CPPUNIT_ASSERT(configs.size() == 2u);
    }

    // "Unsubscribes" for both devices by sending the corresponding 'stopMonitoringDevice' for both devices to the
    // GUI Server.
    {
        const Hash h_1("type", "stopMonitoringDevice",
                       "deviceId", "PropTest_1");
        m_tcpAdapter->sendMessage(h_1);

        const Hash h_2("type", "stopMonitoringDevice",
                       "deviceId", "PropTest_2");
        m_tcpAdapter->sendMessage(h_2);
    }


    // Changes properties on both test devices and assures that no message 'deviceConfigurations' arrives within the
    // propertyUpdateInterval.
    {
        const Hash h_1("type", "reconfigure",
                       "deviceId",
                       "PropTest_1",
                       "configuration", Hash("int32Property", 16));

        const Hash h_2("type", "reconfigure",
                       "deviceId",
                       "PropTest_2",
                       "configuration", Hash("int32Property", 26));

        CPPUNIT_ASSERT_THROW(
                             m_tcpAdapter->getNextMessages("deviceConfigurations", 1, [&] {
                                m_tcpAdapter->sendMessage(h_2);
                                m_tcpAdapter->sendMessage(h_1);
                             }, nextMessageTimeout), karabo::util::TimeoutException);

        // Makes sure that the properties have been set.
        CPPUNIT_ASSERT_EQUAL(16, m_deviceClient->get<int>("PropTest_1", "int32Property"));
        CPPUNIT_ASSERT_EQUAL(26, m_deviceClient->get<int>("PropTest_2", "int32Property"));
    }

    // Shuts down both test devices.
    success = m_deviceClient->killDevice("PropTest_1", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);
    success = m_deviceClient->killDevice("PropTest_2", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT(success.first);

    std::clog << "testDeviceConfigUpdates: OK" << std::endl;
}


void GuiVersion_Test::testDisconnect() {
    std::clog << "testDisconnect: " << std::flush;

    resetClientConnection();
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    // Use server to send message (instead of creating an extra SignalSlotable for that)
    // until DeviceClient understands slots with arguments.

    //
    // Test bad client identifier
    //
    bool disconnected = true;
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request("testGuiServerDevice", "slotDisconnectClient", "BLAnoPORT")
                            .timeout(1000).receive(disconnected));
    CPPUNIT_ASSERT(!disconnected);
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    //
    // Test valid client identifier
    //
    Hash result;
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request("testGuiServerDevice", "slotDumpDebugInfo", Hash("clients", 0))
                            .timeout(1000).receive(result));
    std::vector<std::string> keys;
    result.getKeys(keys);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Expected single key for one client only, but there are " + toString(keys),
                                 1ul, result.size()); // Just a single client
    std::string clientIdentifier = result.begin()->getKey();
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request("testGuiServerDevice", "slotDisconnectClient", clientIdentifier)
                            .timeout(1000).receive(disconnected));
    CPPUNIT_ASSERT_MESSAGE("Failed to disconnect '" + clientIdentifier + "'", disconnected);
    // Wait until disconnected (disconnection delayed by one second in GUI server)
    int timeout = 2000;
    while (m_tcpAdapter->connected() && timeout > 0) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(50));
        timeout -= 50;
    }
    CPPUNIT_ASSERT(!m_tcpAdapter->connected());
    std::clog << "OK" << std::endl;
}
