/**
 * Author: steffen.hauf@xfel.eu
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include "GuiServer_Test.hh"

#include <chrono>
#include <cstdlib>
#include <karabo/devices/GuiServerTemporarySessionManager.hh>
#include <karabo/net/EventLoop.hh>

#include "TestKaraboAuthServer.hh"
#include "karabo/data/types/StringTools.hh"

using namespace std::chrono;
using namespace std::literals::chrono_literals;
using namespace std;

#define LOG_LEVEL "FATAL"
#define KRB_TEST_MAX_TIMEOUT 10
#define EXECUTE_SLOT_TIMEOUT 3
// Next line must be kept in sync with DeviceClient.hh:
#define CONNECTION_KEEP_ALIVE 15

#define TEST_GUI_SERVER_ID "testGuiServerDevice"

// Maximum temporary session duration time to be used in the tests (in seconds)
#define MAX_TEMPORARY_SESSION_TIME karabo::devices::CHECK_TEMPSESSION_EXPIRATION_INTERVAL_SECS * 2U
// Value to be used in the test for the "endTemporarySessionNoticeTime" property of the GUI Server
#define END_TEMPORARY_SESSION_NOTICE_TIME karabo::devices::CHECK_TEMPSESSION_EXPIRATION_INTERVAL_SECS - 1U

USING_KARABO_NAMESPACES

CPPUNIT_TEST_SUITE_REGISTRATION(GuiServer_Test);

bool waitForCondition(std::function<bool()> checker, unsigned int timeoutMillis) {
    const unsigned int sleepIntervalMillis = 5;
    unsigned int numOfWaits = 0;
    const unsigned int maxNumOfWaits = static_cast<unsigned int>(std::ceil(timeoutMillis / sleepIntervalMillis));
    while (numOfWaits < maxNumOfWaits && !checker()) {
        std::this_thread::sleep_for(milliseconds(sleepIntervalMillis));
        numOfWaits++;
    }
    return (numOfWaits < maxNumOfWaits);
}


GuiServer_Test::GuiServer_Test() {}


GuiServer_Test::~GuiServer_Test() {}


void GuiServer_Test::setUp() {
    // uncomment this if ever testing against a local broker
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);
    // Start central event-loop
    m_eventLoopThread = std::jthread([](std::stop_token stoken) { karabo::net::EventLoop::work(); });
    // Create and start server, but don't scan plugins
    Hash config("serverId", "testGuiVersionServer", "Logger.priority", LOG_LEVEL);
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();
    // Create client
    m_deviceClient = std::shared_ptr<DeviceClient>(new DeviceClient());
}


void GuiServer_Test::tearDown() {
    m_deviceServer.reset();
    m_deviceClient.reset();
    EventLoop::stop();
}


void GuiServer_Test::appTestRunner() {
    // bring up a GUI server
    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          "testGuiVersionServer", "GuiServerDevice",
          Hash("deviceId", TEST_GUI_SERVER_ID, "port", 44450, "minClientVersion", "2.2.3", "timeout", 0),
          KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    waitForCondition(
          [this]() {
              auto state = m_deviceClient->get<State>(TEST_GUI_SERVER_ID, "state");
              return state == State::ON;
          },
          KRB_TEST_MAX_TIMEOUT * 1000);

    testVersionControl();
    testExecuteBeforeLogin();
    testExecute();
    testSlowSlots();
    testGetDeviceSchema();
    testGetClassSchema();
    testReconfigure();
    testDeviceConfigUpdates();
    testDisconnect();
    testRequestGeneric();
    testRequestFailProtocol();
    testRequestFailOldVersion();
    testSlotNotify();
    testSlotBroadcast();

    if (m_tcpAdapter->connected()) {
        m_tcpAdapter->disconnect();
    }
    // Shutdown GUI Server device to reconfigure for readOnly
    success = m_deviceClient->killDevice(TEST_GUI_SERVER_ID, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // bring up a GUI server and a tcp adapter to it
    std::pair<bool, std::string> success_n =
          m_deviceClient->instantiate("testGuiVersionServer", "GuiServerDevice",
                                      Hash("deviceId", TEST_GUI_SERVER_ID, "port", 44450, "minClientVersion", "2.2.3",
                                           "isReadOnly", true, "timeout", 0),
                                      KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success_n.second, success_n.first);
    waitForCondition(
          [this]() {
              auto state = m_deviceClient->get<State>(TEST_GUI_SERVER_ID, "state");
              return state == State::ON;
          },
          KRB_TEST_MAX_TIMEOUT * 1000);

    testReadOnly();

    if (m_tcpAdapter->connected()) {
        m_tcpAdapter->disconnect();
    }

    // Shuts down the GUI Server device and brings it up again as an instance that requires user authentication.
    success = m_deviceClient->killDevice(TEST_GUI_SERVER_ID, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    const std::string authServerAddr = "127.0.0.1";
    const int authServerPort = 8052;

    // Instantiates the testing authentication server
    TestKaraboAuthServer tstAuthServer(authServerAddr, authServerPort);
    std::jthread srvRunner = std::jthread([&tstAuthServer](std::stop_token stoken) { tstAuthServer.run(); });

    // Instantiates a GUI Server in Authenticated mode with a maximum temporary session time of 15 seconds for
    // the purposes of the integrated tests.
    // clang-format off
    success_n = m_deviceClient->instantiate(
          "testGuiVersionServer", "GuiServerDevice",
          Hash("deviceId", TEST_GUI_SERVER_ID,
               "port", 44450,
               "minClientVersion", "2.20.0",
               "authServer", "http://" + authServerAddr + ":" + toString(authServerPort),
               "timeout", 0,
               "maxTemporarySessionTime", MAX_TEMPORARY_SESSION_TIME,
               "endTemporarySessionNoticeTime", END_TEMPORARY_SESSION_NOTICE_TIME),
          KRB_TEST_MAX_TIMEOUT);
    // clang-format on
    CPPUNIT_ASSERT_MESSAGE(success_n.second, success_n.first);
    bool serverOn = waitForCondition(
          [this]() {
              auto state = m_deviceClient->get<State>(TEST_GUI_SERVER_ID, "state");
              return state == State::ON;
          },
          KRB_TEST_MAX_TIMEOUT * 1000 * 2);
    CPPUNIT_ASSERT_MESSAGE("GUI Server didn't reach ON state in time", serverOn);

    testMissingTokenOnLogin();
    testInvalidTokenOnLogin();
    testValidTokenOnLogin();

    testValidTokenOnInSessionLogin();
    testInvalidTokenOnInSessionLogin();

    testMissingTokenOnBeginTemporarySession();
    testInvalidTokenOnBeginTemporarySession();
    testBeginEndTemporarySession();
    testTemporarySessionExpiration();

    // Shuts down the GUI Server device and brings it up again as an instance that only accepts
    // connections from ApplicationMode clients like cinema and theater.
    success = m_deviceClient->killDevice(TEST_GUI_SERVER_ID, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // clang-format off
    success_n = m_deviceClient->instantiate(
          "testGuiVersionServer", "GuiServerDevice",
          Hash("deviceId", TEST_GUI_SERVER_ID,
               "port", 44450,
               "minClientVersion", "2.20.0",
               "onlyAppModeClients", true),
          KRB_TEST_MAX_TIMEOUT);
    // clang-format on
    CPPUNIT_ASSERT_MESSAGE(success_n.second, success_n.first);

    testOnlyAppModeClients();

    srvRunner.detach();

    if (m_tcpAdapter->connected()) {
        m_tcpAdapter->disconnect();
    }
}


void GuiServer_Test::resetTcpConnection() {
    int timeout = 5000;
    if (m_tcpAdapter) {
        if (m_tcpAdapter->connected()) {
            m_tcpAdapter->disconnect();
        }
        while (m_tcpAdapter->connected() && timeout > 0) {
            std::this_thread::sleep_for(5ms);
            timeout -= 5;
        }
    }
    m_tcpAdapter =
          std::shared_ptr<karabo::TcpAdapter>(new karabo::TcpAdapter(Hash("port", 44450u /*, "debug", true*/)));
    timeout = 5000;
    while (!m_tcpAdapter->connected() && timeout > 0) {
        std::this_thread::sleep_for(5ms);
        timeout -= 5;
    }
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
}


void GuiServer_Test::resetClientConnection(const karabo::data::Hash& loginData) {
    resetTcpConnection();
    karabo::TcpAdapter::QueuePtr messageQ =
          m_tcpAdapter->getNextMessages("systemTopology", 1, [&] { m_tcpAdapter->sendMessage(loginData); });
    Hash lastMessage;
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT(lastMessage.has("systemTopology"));
}


void GuiServer_Test::resetClientConnection() {
    resetTcpConnection();
    m_tcpAdapter->login();
}


void GuiServer_Test::testVersionControl() {
    std::clog << "\ntestVersionControl: " << std::flush;
    Hash loginInfo("type", "login", "username", "mrusp", "password", "12345", "version", "100.1.0");
    // description , client version, server version, should connect
    typedef std::tuple<std::string, std::string, std::string, bool> TestData;
    std::vector<TestData> tests;
    tests.push_back(TestData("version control supported", "100.1.0", "2.11.0", true));
    tests.push_back(TestData("version control unsupported", "0.1.0", "2.11.0", false));
    tests.push_back(TestData("version control disabled", "0.1.0", "", true));
    for (const auto& test : tests) {
        std::clog << "." << std::flush;
        const std::string& testName = std::get<0>(test);
        const std::string& version = std::get<1>(test);
        const std::string& serverMinVersion = std::get<2>(test);
        const bool connected = std::get<3>(test);
        // set server minimum version
        m_deviceClient->set<std::string>(TEST_GUI_SERVER_ID, "minClientVersion", serverMinVersion);
        resetTcpConnection();
        loginInfo.set("version", version);
        Hash lastMessage;
        if (connected) {
            karabo::TcpAdapter::QueuePtr messageQ =
                  m_tcpAdapter->getNextMessages("systemTopology", 1, [&] { m_tcpAdapter->sendMessage(loginInfo); });
            messageQ->pop(lastMessage);
            CPPUNIT_ASSERT(lastMessage.has("systemTopology"));
        } else {
            karabo::TcpAdapter::QueuePtr messageQ =
                  m_tcpAdapter->getNextMessages("notification", 1, [&] { m_tcpAdapter->sendMessage(loginInfo); });
            messageQ->pop(lastMessage);
            const std::string& message = lastMessage.get<std::string>("message");
            CPPUNIT_ASSERT_MESSAGE(
                  message, message.rfind("Your GUI client has version '" + version + "', but the minimum required is:",
                                         0u) == 0u);
            int timeout = 1500;
            // wait for the GUI server to log us out
            while (m_tcpAdapter->connected() && timeout > 0) {
                std::this_thread::sleep_for(5ms);
                timeout -= 5;
            }
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Failed :" + testName, connected, m_tcpAdapter->connected());
    }

    std::clog << "OK" << std::endl;
}


void GuiServer_Test::testReadOnly() {
    resetClientConnection();
    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    //
    // Request execution of slot although the server is in readOnly mode!
    //
    std::vector<Hash> commands = {
          Hash("type", "execute"),
          Hash("type", "killDevice"),
          Hash("type", "initDevice"), //
          Hash("type", "killServer"),
          Hash("type", "reconfigure"),
          Hash("type", "requestGeneric", "slot", "slotSaveConfigurationFromName"),
    };
    for (const Hash& h : commands) {
        const std::string& type = h.get<std::string>("type");
        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("notification", 1, [&] { m_tcpAdapter->sendMessage(h); });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        const std::string& message = replyMessage.get<std::string>("message");
        CPPUNIT_ASSERT_EQUAL_MESSAGE(
              "Command: " + toString(h),
              std::string("Action '" + type + "' is not allowed on GUI servers in readOnly mode!"), message);
        std::clog << "testReadOnly: OK for " + type << std::endl;
    }
}

void GuiServer_Test::testExecuteBeforeLogin() {
    std::clog << "testExecuteBeforeLogin: " << std::flush;

    resetTcpConnection();
    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    //
    // Request of any type other than login, will fail.
    //
    std::vector<std::string> blockedTypes = {"execute",
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
                                             "requestGeneric"};
    for (const std::string& type : blockedTypes) {
        const Hash h("type", type);
        // no other argument should be needed, since the requests are rejected before
        // the arguments are parsed.
        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("notification", 1, [&] { m_tcpAdapter->sendMessage(h); });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        const std::string& message = replyMessage.get<std::string>("message");
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Received Hash: " + toString(replyMessage),
                                     std::string("Action '" + type + "' refused before log in"), message);
    }
    // The `login` type is implicitly tested by `resetClientConnection()`
    m_tcpAdapter->disconnect();
    std::clog << "OK" << std::endl;
}

void GuiServer_Test::testExecute() {
    std::clog << "testExecute: " << std::flush;
    resetClientConnection();
    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    // As GuiServerTest::testExecute is called on a non-authenticated GUI Server
    // (call made from GuiServer_Test::appTestRunner) after instantiation of the
    // GUI Server, only one client session is expected with no token or temporary
    // session information.
    const auto clientSessionsHash = m_deviceClient->execute1<Hash>(TEST_GUI_SERVER_ID, "slotGetClientSessions",
                                                                   EXECUTE_SLOT_TIMEOUT, Hash("dummy", ""));
    const auto clientSessions = clientSessionsHash.get<std::vector<Hash>>("clientSessions");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Number of client connections differs from expected", 1U,
                                 static_cast<unsigned>(clientSessions.size()));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Client version should not be empty", false,
                                 clientSessions[0].get<std::string>("clientVersion").empty());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("A session token is not expected (empty sessionToken)", true,
                                 clientSessions[0].get<std::string>("sessionToken").empty());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("A session start time is expected (non-empty sessionStartTime)", false,
                                 clientSessions[0].get<std::string>("sessionStartTime").empty());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("No temporary session is expected (empty temporarySessionToken)", true,
                                 clientSessions[0].get<std::string>("temporarySessionToken").empty());
    const std::string tempSessionTime = clientSessions[0].get<std::string>("temporarySessionStartTime");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("No temporary session is expected (empty temporarySessionStartTime)", true,
                                 clientSessions[0].get<std::string>("temporarySessionStartTime").empty());

    //
    // Request execution of slot of non-existing device
    //
    {
        const Hash h("type", "execute", "deviceId", "not_there", "command", "does.not.matter", "reply", true, "timeout",
                     1);
        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("executeReply", 1, [&] { m_tcpAdapter->sendMessage(h); });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_MESSAGE(toString(replyMessage.get<Hash>("input")),
                               h.fullyEquals(replyMessage.get<Hash>("input")));
        CPPUNIT_ASSERT(!replyMessage.get<bool>("success"));

        CPPUNIT_ASSERT_EQUAL(std::string("Request not answered within 1 seconds."),
                             replyMessage.get<std::string>("reason"));
    }


    //
    // Request execution of non-existing slot of existing device (the GuiServerDevice itself...)
    //
    {
        const Hash h("type", "execute", "deviceId", TEST_GUI_SERVER_ID, "command", "not.existing", "reply", true,
                     "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("executeReply", 1, [&] { m_tcpAdapter->sendMessage(h); });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_MESSAGE(toString(replyMessage.get<Hash>("input")),
                               h.fullyEquals(replyMessage.get<Hash>("input")));
        CPPUNIT_ASSERT(!replyMessage.get<bool>("success"));

        // Failure reason has two parts for this, separated by "\nDetails:\n".
        // First part of fail message is fixed, followed by details that contain the
        // remote exception trace. Details of the trace do not matter here.
        const std::string& reason = replyMessage.get<std::string>("reason");
        const char* part1Delim = "'testGuiServerDevice' has no slot 'not.existing'\nDetails:\n";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(reason, 0ul, reason.find(part1Delim));
        CPPUNIT_ASSERT_MESSAGE(reason, reason.find("1. Exception =====>", strlen(part1Delim)) != std::string::npos);
    }

    //
    // Request execution of existing slot of existing device (the GuiServerDevice itself...)
    //
    {
        // Note that "slotGetConfiguration" replies with a Hash carrying the configuration and a string
        // with the deviceId - but that does not matter, they are ignored.
        // Also, "execute" is meant for slots listed as SLOT_ELEMENTS - but it works for any argument less slot
        // as slotGetConfiguration is one...
        const Hash h("type", "execute", "deviceId", TEST_GUI_SERVER_ID, "command", "slotGetConfiguration", "reply",
                     true, "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("executeReply", 1, [&] { m_tcpAdapter->sendMessage(h); });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_MESSAGE(toString(replyMessage.get<Hash>("input")),
                               h.fullyEquals(replyMessage.get<Hash>("input")));
        CPPUNIT_ASSERT(replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(!replyMessage.has("reason"));
    }

    //
    // Request execution of existing slot of existing device (the GuiServerDevice itself...),
    // but this time do not request for a reply.
    //
    {
        // We set the "lockedBy" property that is cleared by slotClearLock
        m_deviceClient->set(TEST_GUI_SERVER_ID, "lockedBy", "someone");
        CPPUNIT_ASSERT_EQUAL(std::string("someone"), m_deviceClient->get<std::string>(TEST_GUI_SERVER_ID, "lockedBy"));
        const Hash h("type", "execute", "deviceId", TEST_GUI_SERVER_ID, "command", "slotClearLock");
        // "reply", false); is default
        m_tcpAdapter->sendMessage(h);

        // Just make sure that it really happened - we have to wait a bit for it:
        int timeout = 1500;
        while (!m_deviceClient->get<std::string>(TEST_GUI_SERVER_ID, "lockedBy").empty()) {
            std::this_thread::sleep_for(5ms);
            timeout -= 5;
        }
        CPPUNIT_ASSERT(m_deviceClient->get<std::string>(TEST_GUI_SERVER_ID, "lockedBy").empty());
    }

    std::clog << "OK" << std::endl;
}


void GuiServer_Test::testRequestFailProtocol() {
    resetClientConnection();
    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    const unsigned int messageTimeout = 2000u;
    {
        const std::string type = "GuiServerDoesNotHaveThisType";
        Hash h("type", type);

        const Hash conf = m_deviceClient->get(TEST_GUI_SERVER_ID);
        const std::string& classVersion = conf.get<string>("classVersion");

        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("notification", 1, [&] { m_tcpAdapter->sendMessage(h); }, messageTimeout);
        Hash replyMessage;
        messageQ->pop(replyMessage);

        std::string assert_message = "The gui server with version " + classVersion +
                                     " does not support the client application request of " + type;
        CPPUNIT_ASSERT_EQUAL(assert_message, replyMessage.get<std::string>("message"));

        std::clog << "testRequestFailProtocol: OK" << std::endl;
    }
}


void GuiServer_Test::testRequestFailOldVersion() {
    // independently from the minimum Client version configured,
    // we want to block certain actions to be performed.
    // for example: `projectSaveItems` can be poisonous for the database.
    m_deviceClient->set<std::string>(TEST_GUI_SERVER_ID, "minClientVersion", "2.9.1");
    std::clog << "testRequestFailOldVersion: " << std::flush;
    // connect again
    resetClientConnection(Hash("type", "login", "username", "mrusp", "password", "12345", "version", "2.9.1"));

    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    const unsigned int messageTimeout = 2000u;
    {
        const std::string type = "projectSaveItems";
        Hash h("type", type); // no other arguments are needed.
        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("notification", 1, [&] { m_tcpAdapter->sendMessage(h); }, messageTimeout);
        Hash replyMessage;
        messageQ->pop(replyMessage);

        std::string assert_message =
              "Action '" + type + "' is not allowed on this GUI client version. Please upgrade your GUI client";
        CPPUNIT_ASSERT_EQUAL(assert_message, replyMessage.get<std::string>("message"));

        std::clog << "OK" << std::endl;
    }
}


void GuiServer_Test::testRequestGeneric() {
    resetClientConnection();
    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());
    const unsigned int messageTimeout = 2000u;
    {
        Hash h("type", "requestGeneric", "instanceId", "isnotonline", "timeout", 1, "slot", "requestScene");
        h.set("args", Hash("name", "scene"));

        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("requestGeneric", 1, [&] { m_tcpAdapter->sendMessage(h); }, messageTimeout);
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(false, replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT_EQUAL(std::string("requestGeneric"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_EQUAL(std::string("scene"), replyMessage.get<std::string>("request.args.name"));
        std::clog << "requestGeneric: OK without specified replyType" << std::endl;
    }
    {
        Hash h("type", "requestGeneric", "instanceId", "isnotonline", "timeout", 1);
        h.set("args", Hash("name", "scene"));
        // Note: h is ill-formed as it misses "slot" element

        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("requestGeneric", 1, [&] { m_tcpAdapter->sendMessage(h); }, messageTimeout);
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(false, replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT_EQUAL(std::string("requestGeneric"), replyMessage.get<std::string>("type"));
        const std::string& reason = replyMessage.get<std::string>("reason");
        CPPUNIT_ASSERT_MESSAGE(reason, reason.find("Key 'slot' does not exist") != std::string::npos);
        std::clog << "requestGeneric: OK (since fails with ill formed message)" << std::endl;
    }
    {
        Hash h("type", "requestGeneric", "instanceId", "isnotonline", "timeout", 1, "replyType", "requestSuperScene",
               "slot", "slotDumpDebugInfo");
        h.set("args", Hash("name", "noname"));

        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages(
              "requestSuperScene", 1, [&] { m_tcpAdapter->sendMessage(h); }, messageTimeout);
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(false, replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(h.fullyEquals(replyMessage.get<Hash>("request")));
        CPPUNIT_ASSERT_EQUAL(std::string("Request not answered within 1 seconds."),
                             replyMessage.get<std::string>("reason"));
        CPPUNIT_ASSERT_EQUAL(std::string("requestSuperScene"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_EQUAL(std::string("noname"), replyMessage.get<std::string>("request.args.name"));

        std::clog << "requestGeneric: OK different replyType" << std::endl;
    }
    {
        Hash h("type", "requestGeneric", "instanceId", TEST_GUI_SERVER_ID, "timeout", 1, "replyType", "debug", "empty",
               true, "slot", "slotDumpDebugInfo");
        h.set("args", Hash("clients", true));

        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("debug", 1, [&] { m_tcpAdapter->sendMessage(h); }, messageTimeout);
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
    {
        Hash h("type", "requestGeneric", "instanceId", TEST_GUI_SERVER_ID, "timeout", 1, "replyType", "debug", "empty",
               true, "token", "here is a token of my appreciation", "slot", "slotDumpDebugInfo");
        h.set("args", Hash("clients", true));

        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("debug", 1, [&] { m_tcpAdapter->sendMessage(h); }, messageTimeout);
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(true, replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT_EQUAL(std::string("debug"), replyMessage.get<std::string>("type"));
        const Hash& request = replyMessage.get<Hash>("request");
        CPPUNIT_ASSERT_EQUAL(1ul, request.size());
        CPPUNIT_ASSERT(request.has("token"));
        CPPUNIT_ASSERT_EQUAL(std::string("here is a token of my appreciation"), request.get<std::string>("token"));
        const Hash& clients = replyMessage.get<Hash>("reply");
        const int number_clients = clients.size();
        CPPUNIT_ASSERT_EQUAL(1, number_clients);

        std::clog << "requestGeneric: OK with online device and empty request and a token" << std::endl;
    }
}

void GuiServer_Test::testGetDeviceSchema() {
    std::clog << "testGetDeviceSchema: " << std::flush;

    resetClientConnection();
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    // Will request schema twice to trigger both code paths:
    // * The one that will actually request the schema.
    // * The one that will get it from the cache.
    // Caveat:
    // If any of the previously running tests access the device schema of the device used here (TEST_GUI_SERVER_ID),
    // it might already be in the cache of the gui server's device client and both test runs get it from there :-(.
    // When this test was implemented, it was proven that this was not the case.

    karabo::TcpAdapter::QueuePtr messageQ; // Just re-use
    Hash replyMessage;                     // Cache the first reply
    // Request is of course identical both times
    const Hash h("type", "getDeviceSchema", "deviceId", TEST_GUI_SERVER_ID);

    // First request
    {
        CPPUNIT_ASSERT_NO_THROW(
              messageQ = m_tcpAdapter->getNextMessages("deviceSchema", 1, [&] { m_tcpAdapter->sendMessage(h); }));
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("deviceSchema"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_EQUAL(std::string(TEST_GUI_SERVER_ID), replyMessage.get<std::string>("deviceId"));
        const Schema& schema = replyMessage.get<karabo::data::Schema>("schema");
        CPPUNIT_ASSERT(!schema.empty());
    }

    // Second request
    {
        CPPUNIT_ASSERT_NO_THROW(
              messageQ = m_tcpAdapter->getNextMessages("deviceSchema", 1, [&] { m_tcpAdapter->sendMessage(h); }));
        Hash replyMessage2;
        messageQ->pop(replyMessage2);
        CPPUNIT_ASSERT(replyMessage.fullyEquals(replyMessage2));
    }
}


void GuiServer_Test::testSlowSlots() {
    resetClientConnection();
    // bring up a PropertyTestDevice
    std::pair<bool, std::string> success =
          m_deviceClient->instantiate("testGuiVersionServer", "PropertyTest",
                                      Hash("deviceId", "testGuiServerDevicePropertyTest"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    //
    // Request execution of existing slot of the PropertyTest device `testGuiServerDevicePropertyTest`
    // The slot `slowSlot` does not appear in the schema, but the slowSlot has the same signature
    // as a karabo command, i.e. no arguments. The slot takes 2 seconds therefore it should timeout.
    //
    {
        const Hash h("type", "execute", "deviceId", "testGuiServerDevicePropertyTest", "command", "slowSlot", "reply",
                     true, "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("executeReply", 1, [&] { m_tcpAdapter->sendMessage(h); });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(!replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(replyMessage.has("reason"));
        const std::string& failureMsg = replyMessage.get<std::string>("reason");
        CPPUNIT_ASSERT_MESSAGE(failureMsg,
                               failureMsg.find("Request not answered within 1 seconds") != std::string::npos);
    }

    ////////////////////////////////////////////////////////////////
    //
    // Request execution of existing slot of the PropertyTest device `testGuiServerDevicePropertyTest`
    // After setting the `ignoreTimeoutClasses` the call will succeed.
    //
    m_deviceClient->set(TEST_GUI_SERVER_ID, "ignoreTimeoutClasses", std::vector<std::string>({"PropertyTest"}));
    {
        const Hash h("type", "execute", "deviceId", "testGuiServerDevicePropertyTest", "command", "slowSlot", "reply",
                     true, "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("executeReply", 1, [&] { m_tcpAdapter->sendMessage(h); });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        std::string message = (replyMessage.has("reason")) ? replyMessage.get<std::string>("reason") : "NO REASON";
        CPPUNIT_ASSERT_MESSAGE(message, replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(!replyMessage.has("reason"));
    }
    //
    // Test that the server will handle timeout after removing "PropertyTest" from the list of bad guys
    // before shutting down the test device.
    //
    m_deviceClient->set(TEST_GUI_SERVER_ID, "ignoreTimeoutClasses", std::vector<std::string>());
    {
        const Hash h("type", "execute", "deviceId", "testGuiServerDevicePropertyTest", "command", "slowSlot", "reply",
                     true, "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("executeReply", 1, [&] { m_tcpAdapter->sendMessage(h); });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(!replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(replyMessage.has("reason"));
        const std::string& failureMsg = replyMessage.get<std::string>("reason");
        CPPUNIT_ASSERT_MESSAGE(failureMsg,
                               failureMsg.find("Request not answered within 1 seconds") != std::string::npos);
    }

    ////////////////////////////////////////////////////////////////
    //
    // Request execution of existing slot of the PropertyTest device `testGuiServerDevicePropertyTest`
    // After setting the a larger`"timeout" the call will succeed.
    //
    const int previousTimeout = m_deviceClient->get<int>(TEST_GUI_SERVER_ID, "timeout");
    m_deviceClient->set(TEST_GUI_SERVER_ID, "timeout", 30);
    {
        const Hash h("type", "execute", "deviceId", "testGuiServerDevicePropertyTest", "command", "slowSlot", "reply",
                     true, "timeout", 1); // smaller than the "timeout" property of the server, so gets ignored
        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("executeReply", 1, [&] { m_tcpAdapter->sendMessage(h); });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        std::string message = (replyMessage.has("reason")) ? replyMessage.get<std::string>("reason") : "NO REASON";
        CPPUNIT_ASSERT_MESSAGE(message, replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(!replyMessage.has("reason"));
    }
    //
    // Test that the server will handle timeout after  resetting the "timeout" property.
    //
    m_deviceClient->set(TEST_GUI_SERVER_ID, "timeout", previousTimeout);
    {
        const Hash h("type", "execute", "deviceId", "testGuiServerDevicePropertyTest", "command", "slowSlot", "reply",
                     true, "timeout", 1); // now this rules again, so the 2s slow slowSlot will timeout again
        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("executeReply", 1, [&] { m_tcpAdapter->sendMessage(h); });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("executeReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(!replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(replyMessage.has("reason"));
        const std::string& failureMsg = replyMessage.get<std::string>("reason");
        CPPUNIT_ASSERT_MESSAGE(failureMsg,
                               failureMsg.find("Request not answered within 1 seconds") != std::string::npos);
    }

    // Clean up. Shutdown the PropertyTest device.

    success = m_deviceClient->killDevice("testGuiServerDevicePropertyTest", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);


    std::clog << "testSlowSlots: OK" << std::endl;
}

void GuiServer_Test::testGetClassSchema() {
    std::clog << "testGetClassSchema: " << std::flush;

    resetClientConnection();
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    // Will request schema twice to trigger both code paths:
    // * The one that will actually request the class schema.
    // * The one that will get it from the cache.

    karabo::TcpAdapter::QueuePtr messageQ; // Just re-use
    Hash replyMessage;                     // Cache the first reply
    // Request is of course identical both times
    const Hash h("type", "getClassSchema", "serverId", m_deviceServer->getInstanceId(), "classId", "PropertyTest");

    // First request
    {
        CPPUNIT_ASSERT_NO_THROW(
              messageQ = m_tcpAdapter->getNextMessages("classSchema", 1, [&] { m_tcpAdapter->sendMessage(h); }));
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("classSchema"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_EQUAL(m_deviceServer->getInstanceId(), replyMessage.get<std::string>("serverId"));
        CPPUNIT_ASSERT_EQUAL(std::string("PropertyTest"), replyMessage.get<std::string>("classId"));
        const Schema& schema = replyMessage.get<karabo::data::Schema>("schema");
        CPPUNIT_ASSERT(!schema.empty());
    }

    // Second request
    {
        CPPUNIT_ASSERT_NO_THROW(
              messageQ = m_tcpAdapter->getNextMessages("classSchema", 1, [&] { m_tcpAdapter->sendMessage(h); }));
        Hash replyMessage2;
        messageQ->pop(replyMessage2);
        CPPUNIT_ASSERT(replyMessage.fullyEquals(replyMessage2));
    }

    // Finally test that for non-existing class an empty schema is returned
    {
        Hash h2(h);
        h2.set("classId", "NonExistingDeviceClass");
        CPPUNIT_ASSERT_NO_THROW(
              messageQ = m_tcpAdapter->getNextMessages("classSchema", 1, [&] { m_tcpAdapter->sendMessage(h2); }));
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("classSchema"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_EQUAL(m_deviceServer->getInstanceId(), replyMessage.get<std::string>("serverId"));
        CPPUNIT_ASSERT_EQUAL(std::string("NonExistingDeviceClass"), replyMessage.get<std::string>("classId"));
        const Schema& schema = replyMessage.get<karabo::data::Schema>("schema");
        CPPUNIT_ASSERT(schema.empty());
    }
    std::clog << "OK" << std::endl;
}

void GuiServer_Test::testReconfigure() {
    resetClientConnection();
    // check if we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    //
    // Request reconfiguration of non-existing device
    //
    {
        const Hash h("type", "reconfigure", "deviceId", "not_there", "configuration", Hash("whatever", 1), "reply",
                     true, "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("reconfigureReply", 1, [&] { m_tcpAdapter->sendMessage(h); });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("reconfigureReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_MESSAGE(toString(replyMessage.get<Hash>("input")),
                               h.fullyEquals(replyMessage.get<Hash>("input")));
        CPPUNIT_ASSERT(!replyMessage.get<bool>("success"));

        CPPUNIT_ASSERT_EQUAL(std::string("Request not answered within 1 seconds."),
                             replyMessage.get<std::string>("reason"));
    }

    //
    // Request invalid reconfiguration of existing device (the GuiServerDevice itself...)
    //
    {
        const Hash h("type", "reconfigure", "deviceId", TEST_GUI_SERVER_ID, "configuration", Hash("whatever", 1),
                     "reply", true, "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("reconfigureReply", 1, [&] { m_tcpAdapter->sendMessage(h); });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("reconfigureReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_MESSAGE(toString(replyMessage.get<Hash>("input")),
                               h.fullyEquals(replyMessage.get<Hash>("input")));
        CPPUNIT_ASSERT(!replyMessage.get<bool>("success"));

        // Failure reason has two parts for this, separated by "\nDetails:\n".
        // First part of fail message is fixed, followed by details that contain the
        // remote exception trace. Details of the trace do not matter here.
        const std::string& reason = replyMessage.get<std::string>("reason");
        const char* part1Delim =
              "Error in slot \"slotReconfigure\"\n"
              "  because: Encountered unexpected configuration parameter: \"whatever\"\n"
              "Details:\n";
        CPPUNIT_ASSERT_EQUAL_MESSAGE(reason, 0ul, reason.find(part1Delim));
        CPPUNIT_ASSERT_MESSAGE(reason, reason.find("1. Exception =====>", strlen(part1Delim)) != std::string::npos);
    }

    //
    // Request valid reconfiguration of existing device (the GuiServerDevice itself...)
    //
    {
        const int newTarget = m_deviceClient->get<int>(TEST_GUI_SERVER_ID, "networkPerformance.sampleInterval") * 2;
        const Hash h("type", "reconfigure", "deviceId", TEST_GUI_SERVER_ID, "configuration",
                     Hash("networkPerformance.sampleInterval", 10), "reply", true, "timeout", 1);
        karabo::TcpAdapter::QueuePtr messageQ =
              m_tcpAdapter->getNextMessages("reconfigureReply", 1, [&] { m_tcpAdapter->sendMessage(h); });
        Hash replyMessage;
        messageQ->pop(replyMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("reconfigureReply"), replyMessage.get<std::string>("type"));
        CPPUNIT_ASSERT_MESSAGE(toString(replyMessage.get<Hash>("input")),
                               h.fullyEquals(replyMessage.get<Hash>("input")));
        CPPUNIT_ASSERT(replyMessage.get<bool>("success"));
        CPPUNIT_ASSERT(!replyMessage.has("reason"));
        // Just assure that it really happened:
        CPPUNIT_ASSERT_EQUAL(newTarget,
                             m_deviceClient->get<int>(TEST_GUI_SERVER_ID, "networkPerformance.sampleInterval"));
    }

    //
    // Request valid reconfiguration of existing device (the GuiServerDevice itself...),
    // but this time do not request for a reply.
    //
    {
        const int newTarget = m_deviceClient->get<int>(TEST_GUI_SERVER_ID, "networkPerformance.sampleInterval") + 2;
        const Hash h("type", "reconfigure", "deviceId", TEST_GUI_SERVER_ID, "configuration",
                     Hash("networkPerformance.sampleInterval", newTarget));
        // "reply", false); is default
        m_tcpAdapter->sendMessage(h);

        // Just make sure that it really happened - we have to wait a bit for it:
        int timeout = 1500;
        while (m_deviceClient->get<int>(TEST_GUI_SERVER_ID, "networkPerformance.sampleInterval") != newTarget &&
               timeout > 0) {
            std::this_thread::sleep_for(5ms);
            timeout -= 5;
        }
        CPPUNIT_ASSERT_EQUAL(newTarget,
                             m_deviceClient->get<int>(TEST_GUI_SERVER_ID, "networkPerformance.sampleInterval"));
    }

    std::clog << "testReconfigure: OK" << std::endl;
}


void GuiServer_Test::testDeviceConfigUpdates() {
    resetClientConnection();
    // checks that we are connected
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    // Need a 2nd client for parts of this test to test that a badly behaving client does not harm the other one:
    auto tcpAdapter2 = std::make_shared<karabo::TcpAdapter>(Hash("port", 44450u /*, "debug", true*/));
    int timeout = 5000;
    while (!tcpAdapter2->connected() && timeout > 0) {
        std::this_thread::sleep_for(5ms);
        timeout -= 5;
    }
    CPPUNIT_ASSERT(tcpAdapter2->connected());
    tcpAdapter2->login();

    const unsigned int propertyUpdateInterval =
          1500u; // A propertyUpdateInterval that is large enough so that the distance
    // between a reference timestamp gathered right after an update interval
    // "pulse" and the real "pulse" timestamp is at least one order of magnitude
    // smaller than the interval duration - (with 1500 we are allowing that
    // distance to be up to 150 ms, which is quite reasonable even in situations
    // where the running system is under a heavy load).
    m_deviceClient->set<int>(TEST_GUI_SERVER_ID, "propertyUpdateInterval", propertyUpdateInterval);
    const unsigned int nextMessageTimeout = propertyUpdateInterval + 500u;

    // Instantiate two property test devices
    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          "testGuiVersionServer", "PropertyTest", Hash("deviceId", "PropTest_1"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    success = m_deviceClient->instantiate("testGuiVersionServer", "PropertyTest", Hash("deviceId", "PropTest_2"),
                                          KRB_TEST_MAX_TIMEOUT);

    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Changes a property of one of the test devices and makes sure that no message 'deviceConfigurations' arrives
    // within the propertyUpdateInterval.
    {
        const Hash h("type", "reconfigure", "deviceId", "PropTest_1", "configuration", Hash("int32Property", 10));
        CPPUNIT_ASSERT_THROW(
              m_tcpAdapter->getNextMessages(
                    "deviceConfigurations", 1, [&] { m_tcpAdapter->sendMessage(h); }, nextMessageTimeout),
              karabo::data::TimeoutException);
        // Makes sure that the property has been set.
        CPPUNIT_ASSERT_EQUAL(10, m_deviceClient->get<int>("PropTest_1", "int32Property"));
    }

    // "Subscribes" to one of the test property devices by sending the GUI Server a 'startMonitoringDevice' message.
    // But 2nd client that is not monitoring does not get any message.
    {
        const Hash h("type", "startMonitoringDevice", "deviceId", "PropTest_1");
        // After receiving a startMonitoringDevice, the GUI Server sends a 'deviceConfigurations' message with
        // the full configuration it has for the device.
        karabo::TcpAdapter::QueuePtr messageQ;
        CPPUNIT_ASSERT_NO_THROW(
              messageQ = m_tcpAdapter->getNextMessages(
                    "deviceConfigurations", 1, [&] { m_tcpAdapter->sendMessage(h); }, nextMessageTimeout));
        Hash nextMessage;
        messageQ->pop(nextMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("deviceConfigurations"), nextMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(nextMessage.has("configurations.PropTest_1"));
        // key _deviceId_ present means: full config is received, not just an update from signal[State]Changed
        CPPUNIT_ASSERT_MESSAGE(toString(nextMessage), nextMessage.has("configurations.PropTest_1._deviceId_"));
        // 2nd client not yet subscribed
        CPPUNIT_ASSERT_EQUAL(0ul, tcpAdapter2->getAllMessages("deviceConfigurations").size());
    }

    // Now 2nd client also starts to monitor the device - should work in the same way although under the hood just
    // accesses data from cache
    {
        const Hash h("type", "startMonitoringDevice", "deviceId", "PropTest_1");
        // After receiving a startMonitoringDevice, the GUI Server sends a 'deviceConfigurations' message with
        // the full configuration it has for the device.
        karabo::TcpAdapter::QueuePtr messageQ;
        CPPUNIT_ASSERT_NO_THROW(
              messageQ = tcpAdapter2->getNextMessages(
                    "deviceConfigurations", 1, [&] { tcpAdapter2->sendMessage(h); }, nextMessageTimeout));
        Hash nextMessage;
        messageQ->pop(nextMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("deviceConfigurations"), nextMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(nextMessage.has("configurations.PropTest_1"));
        // key _deviceId_ present means: full config is received, not just an update from signal[State]Changed
        CPPUNIT_ASSERT_MESSAGE(toString(nextMessage), nextMessage.has("configurations.PropTest_1._deviceId_"));
    }

    {
        // 2nd clients unsubscribes again - but once too often.
        // That must not harm the 1st client in the following tests.
        const Hash h("type", "stopMonitoringDevice", "deviceId", "PropTest_1");
        tcpAdapter2->sendMessage(h);
        tcpAdapter2->sendMessage(h);
        tcpAdapter2->clearAllMessages(); // to check that no device updates arrive after stop of monitoring

        // The pre-2.15.X problem of a connection miscount by this duplicated "stopMonitoringDevice" is
        // unfortunately only seen after the device has "aged to death" inside the DeviceClient.
        // That requires this very long sleep to be sure to test that the issue is fixed - without it, the
        // next m_tcpAdapter->getNextMessages("deviceConfigurations", ...) does NOT timeout despite of the bug.
        std::this_thread::sleep_for(milliseconds(CONNECTION_KEEP_ALIVE * 1000 + 250));
    }
    // Changes properties on the two devices and assures that an update message arrives containing only the change
    // to the subscribed one - and 2nd client does not receive anything anymore.
    // NOTE: From this point on, the order of the operations matters - there's a synchronization code before an
    //       upcoming property change test that is based on the timestamp that will be stored in propUpdateTime during
    //       the test below.
    Epochstamp propUpdateTime;
    {
        const Hash h_1("type", "reconfigure", "deviceId", "PropTest_1", "configuration", Hash("int32Property", 12));
        const Hash h_2("type", "reconfigure", "deviceId", "PropTest_2", "configuration", Hash("int32Property", 22));

        karabo::TcpAdapter::QueuePtr messageQ;
        CPPUNIT_ASSERT_NO_THROW(messageQ = m_tcpAdapter->getNextMessages(
                                      "deviceConfigurations", 1,
                                      [&] {
                                          m_tcpAdapter->sendMessage(h_2);
                                          m_tcpAdapter->sendMessage(h_1);
                                      },
                                      nextMessageTimeout));

        propUpdateTime.now(); // Captures a timestamp that is as close as possible to the update "pulse".

        Hash nextMessage;
        messageQ->pop(nextMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("deviceConfigurations"), nextMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(nextMessage.has("configurations"));
        Hash configs = nextMessage.get<Hash>("configurations");
        CPPUNIT_ASSERT(configs.has("PropTest_1"));
        Hash propTest1Config = configs.get<Hash>("PropTest_1");
        CPPUNIT_ASSERT(propTest1Config.has("int32Property"));
        CPPUNIT_ASSERT(propTest1Config.get<int>("int32Property") == 12);
        CPPUNIT_ASSERT(configs.size() == 1u);
        // 2nd client did not get any update
        CPPUNIT_ASSERT_EQUAL(0ul, tcpAdapter2->getAllMessages("deviceConfigurations").size());
    }
    // Now test that tcpAdapter2 (that previously unsubscribed twice), gets updates again after a single request
    {
        const Hash h("type", "startMonitoringDevice", "deviceId", "PropTest_1");
        karabo::TcpAdapter::QueuePtr messageQ;
        CPPUNIT_ASSERT_NO_THROW(
              messageQ = tcpAdapter2->getNextMessages(
                    "deviceConfigurations", 1, [h, tcpAdapter2] { tcpAdapter2->sendMessage(h); }, nextMessageTimeout));
        Hash nextMessage;
        messageQ->pop(nextMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("deviceConfigurations"), nextMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(nextMessage.has("configurations.PropTest_1"));
    }
    tcpAdapter2.reset(); // Not needed anymore.

    // "Subscribes" to the yet unsubscribed test device.
    {
        const Hash h("type", "startMonitoringDevice", "deviceId", "PropTest_2", "reply", true, "timeout", 1);
        // After receiving a startMonitoringDevice, the GUI Server sends a 'deviceConfigurations' message with
        // the full configuration it has for the device.
        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages(
              "deviceConfigurations", 1, [&] { m_tcpAdapter->sendMessage(h); }, nextMessageTimeout);
        Hash nextMessage;
        messageQ->pop(nextMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("deviceConfigurations"), nextMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(nextMessage.has("configurations.PropTest_2"));
        CPPUNIT_ASSERT(nextMessage.has("configurations.PropTest_2._deviceId_"));
        CPPUNIT_ASSERT_EQUAL(22, nextMessage.get<int>("configurations.PropTest_2.int32Property"));
    }

    // Changes properties on both test devices and assures that an update message arrives containing the changes
    // to both devices.
    {
        const Hash h_1("type", "reconfigure", "deviceId", "PropTest_1", "configuration", Hash("int32Property", 14));
        const Hash h_2("type", "reconfigure", "deviceId", "PropTest_2", "configuration", Hash("int32Property", 24));

        // Syncs as close as possible to the next update "pulse" - we'll need that for the next check, which is
        // supposed to get the two updates in the same cycle.
        Epochstamp targetTime(propUpdateTime);
        Epochstamp currentTime;
        // Duration constructor takes care of overflow of fractions.
        const TimeDuration duration(
              0ull,
              propertyUpdateInterval * 1'000'000'000'000'000ull); // 10^15 => factor from ms. to attosecs.
        const int tolerance = propertyUpdateInterval / 15;
        do {
            targetTime += duration;
        } while (targetTime < currentTime &&
                 targetTime.elapsed(currentTime).getFractions(TIME_UNITS::MILLISEC) <= tolerance);
        std::this_thread::sleep_for(std::chrono::milliseconds(targetTime.elapsed().getFractions(TIME_UNITS::MILLISEC)));

        karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages(
              "deviceConfigurations", 1,
              [&] {
                  m_tcpAdapter->sendMessage(h_2);
                  m_tcpAdapter->sendMessage(h_1);
              },
              nextMessageTimeout);

        Hash nextMessage;
        messageQ->pop(nextMessage);
        CPPUNIT_ASSERT_EQUAL(std::string("deviceConfigurations"), nextMessage.get<std::string>("type"));
        CPPUNIT_ASSERT(nextMessage.has("configurations"));
        Hash configs = nextMessage.get<Hash>("configurations");
        CPPUNIT_ASSERT(configs.has("PropTest_1"));
        Hash propTest1Config = configs.get<Hash>("PropTest_1");
        CPPUNIT_ASSERT(propTest1Config.has("int32Property"));
        CPPUNIT_ASSERT(propTest1Config.get<int>("int32Property") == 14);
        CPPUNIT_ASSERT(configs.has("PropTest_2"));
        Hash propTest2Config = configs.get<Hash>("PropTest_2");
        CPPUNIT_ASSERT(propTest2Config.has("int32Property"));
        CPPUNIT_ASSERT(propTest2Config.get<int>("int32Property") == 24);
        CPPUNIT_ASSERT(configs.size() == 2u);
    }


    // "Unsubscribes" for both devices by sending the corresponding 'stopMonitoringDevice' for both devices to the
    // GUI Server.
    {
        const Hash h_1("type", "stopMonitoringDevice", "deviceId", "PropTest_1");
        m_tcpAdapter->sendMessage(h_1);

        const Hash h_2("type", "stopMonitoringDevice", "deviceId", "PropTest_2");
        m_tcpAdapter->sendMessage(h_2);
    }


    // Changes properties on both test devices and assures that no message 'deviceConfigurations' arrives within the
    // propertyUpdateInterval.
    {
        const Hash h_1("type", "reconfigure", "deviceId", "PropTest_1", "configuration", Hash("int32Property", 16));

        const Hash h_2("type", "reconfigure", "deviceId", "PropTest_2", "configuration", Hash("int32Property", 26));

        CPPUNIT_ASSERT_THROW(m_tcpAdapter->getNextMessages(
                                   "deviceConfigurations", 1,
                                   [&] {
                                       m_tcpAdapter->sendMessage(h_2);
                                       m_tcpAdapter->sendMessage(h_1);
                                   },
                                   nextMessageTimeout),
                             karabo::data::TimeoutException);

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


void GuiServer_Test::testDisconnect() {
    std::clog << "testDisconnect: " << std::flush;
    const int timeoutMs = KRB_TEST_MAX_TIMEOUT * 1000;

    resetClientConnection();
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    // Use server to send message (instead of creating an extra SignalSlotable for that)
    // until DeviceClient understands slots with arguments.

    //
    // Test bad client identifier
    //
    bool disconnected = true;
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request(TEST_GUI_SERVER_ID, "slotDisconnectClient", "BLAnoPORT")
                                  .timeout(timeoutMs)
                                  .receive(disconnected));
    CPPUNIT_ASSERT(!disconnected);
    CPPUNIT_ASSERT(m_tcpAdapter->connected());

    //
    // Test valid client identifier
    //
    Hash result;
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request(TEST_GUI_SERVER_ID, "slotDumpDebugInfo", Hash("clients", 0))
                                  .timeout(timeoutMs)
                                  .receive(result));
    std::vector<std::string> keys;
    result.getKeys(keys);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Expected single key for one client only, but there are " + toString(keys), 1ul,
                                 result.size()); // Just a single client
    std::string clientIdentifier = result.begin()->getKey();
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request(TEST_GUI_SERVER_ID, "slotDisconnectClient", clientIdentifier)
                                  .timeout(timeoutMs)
                                  .receive(disconnected));
    CPPUNIT_ASSERT_MESSAGE("Failed to disconnect '" + clientIdentifier + "'", disconnected);
    // Wait until disconnected (disconnection delayed by one second in GUI server)
    int timeout = 2000;
    while (m_tcpAdapter->connected() && timeout > 0) {
        std::this_thread::sleep_for(50ms);
        timeout -= 50;
    }
    CPPUNIT_ASSERT(!m_tcpAdapter->connected());
    std::clog << "OK" << std::endl;
}


void GuiServer_Test::testSlotNotify() {
    std::clog << "testSlotNotify: " << std::flush;
    const int timeoutMs = KRB_TEST_MAX_TIMEOUT * 1000;
    const std::string messageToSend("Banner for everyone!");
    const Hash arg("message", messageToSend, "contentType", "banner", "foreground", "red");
    const std::vector<string> expectedMessageData = {messageToSend, "", "red"};
    Hash reply;
    karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages(
          "notification", 1,
          [&reply, &arg, timeoutMs, this] {
              m_deviceServer->request(TEST_GUI_SERVER_ID, "slotNotify", arg).timeout(timeoutMs).receive(reply);
          },
          timeoutMs);
    CPPUNIT_ASSERT_MESSAGE(toString(reply), reply.empty());

    // Test that client received the notification
    Hash messageReceived;
    messageQ->pop(messageReceived);
    CPPUNIT_ASSERT(messageReceived.has("message"));
    CPPUNIT_ASSERT(messageReceived.has("contentType"));
    CPPUNIT_ASSERT(messageReceived.has("foreground"));
    CPPUNIT_ASSERT(!messageReceived.has("background"));
    CPPUNIT_ASSERT_EQUAL(messageToSend, messageReceived.get<std::string>("message"));
    CPPUNIT_ASSERT_EQUAL(std::string("banner"), messageReceived.get<std::string>("contentType"));
    CPPUNIT_ASSERT_EQUAL(std::string("red"), messageReceived.get<std::string>("foreground"));

    // Since it is type "banner", GUI server device stores message as "bannerData":
    // Note: Better wait to ensure that deviceClient received update - no guarantee since server sent the message...
    waitForCondition(
          [this, &messageToSend]() {
              return 3ul == m_deviceClient->get<std::vector<std::string>>(TEST_GUI_SERVER_ID, "bannerData").size();
          },
          timeoutMs);
    const std::vector<std::string> messageData(
          m_deviceClient->get<std::vector<std::string>>(TEST_GUI_SERVER_ID, "bannerData"));
    CPPUNIT_ASSERT_EQUAL(expectedMessageData.size(), messageData.size());
    for (int i = 0; i < 3; i++) {
        CPPUNIT_ASSERT_EQUAL(expectedMessageData[i], messageData[i]);
    }

    // Create second adapter that connects - it should receive the stored notification "banner"
    auto tcpAdapter2 = std::make_shared<karabo::TcpAdapter>(Hash("port", 44450u /*, "debug", true*/));
    int timeout = 5000;
    while (!tcpAdapter2->connected() && timeout > 0) {
        std::this_thread::sleep_for(5ms);
        timeout -= 5;
    }
    CPPUNIT_ASSERT(tcpAdapter2->connected());
    std::vector<Hash> messages(tcpAdapter2->getAllMessages("notification"));
    timeout = 1000;
    while (messages.empty() && timeout > 0) {
        std::this_thread::sleep_for(2ms);
        timeout -= 2;
        messages = tcpAdapter2->getAllMessages("notification");
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(messages), 1ul, messages.size());

    CPPUNIT_ASSERT(messages[0].has("message"));
    CPPUNIT_ASSERT(messages[0].has("contentType"));
    CPPUNIT_ASSERT(messages[0].has("foreground"));
    CPPUNIT_ASSERT(!messages[0].has("background"));
    CPPUNIT_ASSERT_EQUAL(messageToSend, messages[0].get<std::string>("message"));
    CPPUNIT_ASSERT_EQUAL(std::string("banner"), messages[0].get<std::string>("contentType"));
    CPPUNIT_ASSERT_EQUAL(std::string("red"), messages[0].get<std::string>("foreground"));

    tcpAdapter2->disconnect();

    const Hash clear_arg("message", "", "contentType", "banner");
    messageQ = m_tcpAdapter->getNextMessages(
          "notification", 1,
          [&reply, &clear_arg, timeoutMs, this] {
              m_deviceServer->request(TEST_GUI_SERVER_ID, "slotNotify", clear_arg).timeout(timeoutMs).receive(reply);
          },
          timeoutMs);
    CPPUNIT_ASSERT_MESSAGE(toString(reply), reply.empty());
    // Banner data is cleared
    CPPUNIT_ASSERT_EQUAL(0ul, m_deviceClient->get<std::vector<std::string>>(TEST_GUI_SERVER_ID, "bannerData").size());

    messageQ->pop(messageReceived);
    CPPUNIT_ASSERT(messageReceived.has("message"));
    CPPUNIT_ASSERT(messageReceived.has("contentType"));
    CPPUNIT_ASSERT(!messageReceived.has("foreground"));
    CPPUNIT_ASSERT(!messageReceived.has("background"));
    CPPUNIT_ASSERT_EQUAL(std::string(), messageReceived.get<std::string>("message"));
    CPPUNIT_ASSERT_EQUAL(std::string("banner"), messageReceived.get<std::string>("contentType"));

    // new clients do not get the banner
    auto tcpAdapter3 = std::make_shared<karabo::TcpAdapter>(Hash("port", 44450u /*, "debug", true*/));
    timeout = 5000;
    while (!tcpAdapter2->connected() && timeout > 0) {
        std::this_thread::sleep_for(5ms);
        timeout -= 5;
    }
    CPPUNIT_ASSERT(tcpAdapter3->connected());
    messages = tcpAdapter3->getAllMessages("notification");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(messages), 0ul, messages.size());
    tcpAdapter3->disconnect();

    std::clog << "OK" << std::endl;
}

void GuiServer_Test::testSlotBroadcast() {
    std::clog << "testSlotBroadcast: " << std::flush;

    const int timeoutMs = KRB_TEST_MAX_TIMEOUT * 1000;

    const Hash message("isSkookum", true, "type", "unimplementedDangerousCall");
    const Hash arg("message", message, "clientAddress", "");
    Hash reply;
    karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages(
          "unimplementedDangerousCall", 1,
          [&reply, &arg, timeoutMs, this] {
              m_deviceServer->request(TEST_GUI_SERVER_ID, "slotBroadcast", arg).timeout(timeoutMs).receive(reply);
          },
          timeoutMs);
    CPPUNIT_ASSERT_EQUAL(true, reply.get<bool>("success"));
    CPPUNIT_ASSERT_EQUAL(1ul, reply.size());

    // Test that client received the notification
    Hash messageReceived;
    messageQ->pop(messageReceived);
    CPPUNIT_ASSERT_MESSAGE(toString(messageReceived), message.fullyEquals(messageReceived));
    std::clog << "." << std::flush;

    // A message should have a type
    const Hash bad_arg("isSkookum", false);
    CPPUNIT_ASSERT_THROW(
          m_deviceServer->request(TEST_GUI_SERVER_ID, "slotBroadcast", bad_arg).timeout(timeoutMs).receive(reply),
          std::exception);
    std::clog << "." << std::flush;

    const Hash bad_msg("isSkookum", false, "type", "unimplementedDangerousCall");
    const Hash bad_client_arg("message", bad_msg, "clientAddress", "pinneberg");

    m_deviceServer->request(TEST_GUI_SERVER_ID, "slotBroadcast", bad_client_arg).timeout(timeoutMs).receive(reply);

    // success is false since we did not send the message to anybody
    CPPUNIT_ASSERT_EQUAL(false, reply.get<bool>("success"));
    CPPUNIT_ASSERT_EQUAL(1ul, reply.size());
    std::clog << "." << std::flush;

    // now send a message to a specific client.

    std::string clientAddress;
    Hash debugInfo;
    m_deviceServer->request(TEST_GUI_SERVER_ID, "slotDumpDebugInfo", Hash("clients", true))
          .timeout(timeoutMs)
          .receive(debugInfo);
    // Failed with timeout(1000) for above request in https://git.xfel.eu/Karabo/Framework/-/jobs/290411
    CPPUNIT_ASSERT_EQUAL(1ul, debugInfo.size());
    clientAddress = debugInfo.begin()->getKey();

    const Hash client_msg("skookumFactor", 42, "type", "unimplementedDangerousCall");
    const Hash client_arg("clientAddress", clientAddress, "message", client_msg);

    // Test that client received the notification
    messageQ = m_tcpAdapter->getNextMessages(
          "unimplementedDangerousCall", 1,
          [&reply, &client_arg, timeoutMs, this] {
              m_deviceServer->request(TEST_GUI_SERVER_ID, "slotBroadcast", client_arg)
                    .timeout(timeoutMs)
                    .receive(reply);
          },
          timeoutMs);

    CPPUNIT_ASSERT_EQUAL(true, reply.get<bool>("success"));
    CPPUNIT_ASSERT_EQUAL(1ul, reply.size());
    // Test that client received the notification
    messageQ->pop(messageReceived);
    CPPUNIT_ASSERT_MESSAGE(toString(messageReceived), client_msg.fullyEquals(messageReceived));
    std::clog << "." << std::flush;

    std::clog << " OK" << std::endl;
}


void GuiServer_Test::testMissingTokenOnLogin() {
    std::clog << "testMissingTokenOnLogin: " << std::flush;

    // From version 2.20.0 (or a development version of 2.20.0) a missing one time token is interpreted as the login for
    // a a read-only session with no user authentication involved.
    Hash loginReadOnlySession("type", "login", "clientId", "bobHost(pid 10264)", "version", "2.20.0");

    resetTcpConnection();

    Hash lastMessage;
    karabo::TcpAdapter::QueuePtr messageQ = m_tcpAdapter->getNextMessages(
          "loginInformation", 1, [&] { m_tcpAdapter->sendMessage(loginReadOnlySession); });
    messageQ->pop(lastMessage);
    bool readOnly = lastMessage.get<bool>("readOnly");
    int accessLevel = lastMessage.get<int>("accessLevel");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong readOnly mode value", true, readOnly);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong access level value for readOnly login", int(Schema::AccessLevel::OBSERVER),
                                 accessLevel);

    // Starting a temporary session from a readOnly session should not be allowed.
    Hash beginTempSessionInfo("type", "beginTemporarySession", "temporarySessionToken",
                              TestKaraboAuthServer::VALID_TOKEN, "version", "2.20.0", "levelBeforeTemporarySession",
                              static_cast<int>(Schema::AccessLevel::OPERATOR));
    messageQ =
          m_tcpAdapter->getNextMessages("notification", 1, [&] { m_tcpAdapter->sendMessage(beginTempSessionInfo); });
    messageQ->pop(lastMessage);
    const std::string message = lastMessage.get<std::string>("message");
    CPPUNIT_ASSERT_MESSAGE(
          "Expected notification message that GuiServer cannot start a temporary session from a readOnly session'. Got "
          "'" + message +
                "'",
          message.find("cannot be started from a readOnly") != std::string::npos);


    int timeout = 1500;
    // wait for the GUI server to log us out
    while (m_tcpAdapter->connected() && timeout > 0) {
        std::this_thread::sleep_for(5ms);
        timeout -= 5;
    }

    std::clog << "OK" << std::endl;
}


void GuiServer_Test::testInvalidTokenOnLogin() {
    std::clog << "testInvalidTokenOnLogin: " << std::flush;

    Hash loginInfo("type", "login", "username", "bob", "oneTimeToken", "abcd", "version", "2.20.0");

    resetTcpConnection();

    Hash lastMessage;
    const string expectedMsg = "Error validating token: " + TestKaraboAuthServer::INVALID_TOKEN_MSG;
    karabo::TcpAdapter::QueuePtr messageQ =
          m_tcpAdapter->getNextMessages("notification", 1, [&] { m_tcpAdapter->sendMessage(loginInfo); });
    messageQ->pop(lastMessage);
    const std::string& message = lastMessage.get<std::string>("message");
    CPPUNIT_ASSERT_MESSAGE("Expected notification message '" + expectedMsg + "'. Got '" + message + "'",
                           message == expectedMsg);

    int timeout = 1500;
    // wait for the GUI server to log us out
    while (m_tcpAdapter->connected() && timeout > 0) {
        std::this_thread::sleep_for(5ms);
        timeout -= 5;
    }

    std::clog << "OK" << std::endl;
}

void GuiServer_Test::testValidTokenOnLogin() {
    std::clog << "testValidTokenOnLogin: " << std::flush;

    Hash loginInfo("type", "login", "username", TestKaraboAuthServer::VALID_USER_ID, "oneTimeToken",
                   TestKaraboAuthServer::VALID_TOKEN, "version", "2.20.0");

    resetTcpConnection();

    Hash lastMessage;
    karabo::TcpAdapter::QueuePtr messageQ =
          m_tcpAdapter->getNextMessages("loginInformation", 1, [&] { m_tcpAdapter->sendMessage(loginInfo); });
    messageQ->pop(lastMessage);
    const int accessLevel = lastMessage.get<int>("accessLevel");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("AccessLevel differs from expected", TestKaraboAuthServer::VALID_ACCESS_LEVEL,
                                 accessLevel);
    const string& userId = lastMessage.get<std::string>("username");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("User differs from expected", TestKaraboAuthServer::VALID_USER_ID, userId);

    int timeout = 1500;
    // wait for the GUI server to log us out
    while (m_tcpAdapter->connected() && timeout > 0) {
        std::this_thread::sleep_for(5ms);
        timeout -= 5;
    }

    std::clog << "OK" << std::endl;
}

void GuiServer_Test::testValidTokenOnInSessionLogin() {
    std::clog << "testValidTokenOnInSessionLogin: " << std::flush;

    Hash loginInfo("type", "login", "username", TestKaraboAuthServer::VALID_USER_ID, "oneTimeToken",
                   TestKaraboAuthServer::VALID_TOKEN, "version", "2.20.0");

    resetTcpConnection();

    Hash lastMessage;
    karabo::TcpAdapter::QueuePtr messageQ =
          m_tcpAdapter->getNextMessages("loginInformation", 1, [&] { m_tcpAdapter->sendMessage(loginInfo); });
    messageQ->pop(lastMessage);
    int accessLevel = lastMessage.get<int>("accessLevel");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("AccessLevel differs from expected", TestKaraboAuthServer::VALID_ACCESS_LEVEL,
                                 accessLevel);
    string userId = lastMessage.get<std::string>("username");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("User differs from expected", TestKaraboAuthServer::VALID_USER_ID, userId);

    // Sends another login with the same valid credentials - the GUI Server will reauthorize the same user
    // successfully - the "side-effect" is that the startTime of the session will be "refreshed" in this case
    // as the user is the same.
    messageQ = m_tcpAdapter->getNextMessages("loginInformation", 1, [&] { m_tcpAdapter->sendMessage(loginInfo); });
    messageQ->pop(lastMessage);
    accessLevel = lastMessage.get<int>("accessLevel");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("AccessLevel differs from expected", TestKaraboAuthServer::VALID_ACCESS_LEVEL,
                                 accessLevel);
    userId = lastMessage.get<std::string>("username");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("User differs from expected", TestKaraboAuthServer::VALID_USER_ID, userId);

    std::clog << "OK" << std::endl;
};

void GuiServer_Test::testInvalidTokenOnInSessionLogin() {
    std::clog << "testInvalidTokenOnInSessionLogin: " << std::flush;

    Hash loginInfo("type", "login", "username", TestKaraboAuthServer::VALID_USER_ID, "oneTimeToken",
                   TestKaraboAuthServer::VALID_TOKEN, "version", "2.20.0");

    Hash invalidLoginInfo("type", "login", "username", TestKaraboAuthServer::VALID_USER_ID, "oneTimeToken", "abcd",
                          "version", "2.20.0");

    resetTcpConnection();

    Hash lastMessage;
    karabo::TcpAdapter::QueuePtr messageQ =
          m_tcpAdapter->getNextMessages("loginInformation", 1, [&] { m_tcpAdapter->sendMessage(loginInfo); });
    messageQ->pop(lastMessage);
    int accessLevel = lastMessage.get<int>("accessLevel");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("AccessLevel differs from expected", TestKaraboAuthServer::VALID_ACCESS_LEVEL,
                                 accessLevel);
    string userId = lastMessage.get<std::string>("username");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("User differs from expected", TestKaraboAuthServer::VALID_USER_ID, userId);

    // Sends another login with the an invalid token - the GUI Server will reject the user change.
    const string expectedErrorMsg = "Error validating token: " + TestKaraboAuthServer::INVALID_TOKEN_MSG;
    messageQ = m_tcpAdapter->getNextMessages("notification", 1, [&] { m_tcpAdapter->sendMessage(invalidLoginInfo); });
    messageQ->pop(lastMessage);
    const std::string& message = lastMessage.get<std::string>("message");
    CPPUNIT_ASSERT_MESSAGE("Expected notification message '" + expectedErrorMsg + "'. Got '" + message + "'",
                           message == expectedErrorMsg);

    // Checks that the previous session is still active with the expected one-time token
    auto clientSessionsHash = m_deviceClient->execute1<Hash>(
          TEST_GUI_SERVER_ID, "slotGetClientSessions", EXECUTE_SLOT_TIMEOUT, Hash(CLIENT_SESSIONS_OPTION_KEY, false));
    auto clientSessions = clientSessionsHash.get<std::vector<Hash>>("clientSessions");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Number of client connections differs from expected", 1U,
                                 static_cast<unsigned>(clientSessions.size()));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Session token differs from expected", TestKaraboAuthServer::VALID_TOKEN,
                                 clientSessions[0].get<std::string>("sessionToken"));

    std::clog << "OK" << std::endl;
};

void GuiServer_Test::testMissingTokenOnBeginTemporarySession() {
    std::clog << "testMissingTokenOnBeginTemporarySession: " << std::flush;

    Hash loginInfo("type", "login", "clientId", "desk010", "oneTimeToken", TestKaraboAuthServer::VALID_TOKEN, "version",
                   "2.20.0", "levelBeforeTemporarySession", static_cast<int>(Schema::AccessLevel::OPERATOR));

    Hash beginTempSessionInfo("type", "beginTemporarySession", "version", "2.20.0");

    resetTcpConnection();

    // Performs an authenticated login successfully - temporary sessions are only available from that point on.
    Hash lastMessage;
    karabo::TcpAdapter::QueuePtr messageQ =
          m_tcpAdapter->getNextMessages("loginInformation", 1, [&] { m_tcpAdapter->sendMessage(loginInfo); });
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT_MESSAGE("'accessLevel' missing in loginInformation sent by the GUI Server",
                           lastMessage.has("accessLevel"));
    // Attempt at begining a temporary session without the required token
    messageQ = m_tcpAdapter->getNextMessages("onBeginTemporarySession", 1,
                                             [&] { m_tcpAdapter->sendMessage(beginTempSessionInfo); });
    messageQ->pop(lastMessage);
    bool success = lastMessage.get<bool>("success");
    const std::string& reason = lastMessage.get<std::string>("reason");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("beginTemporarySession request succeeded unexpectedly", false, success);
    CPPUNIT_ASSERT_MESSAGE("Error reason differs from the expected. Received error reason: '" + reason + "'",
                           reason.find("Required \"temporarySessionToken\" field missing") != std::string::npos);

    int timeout = 1500;
    // wait for the GUI server to log us out
    while (m_tcpAdapter->connected() && timeout > 0) {
        std::this_thread::sleep_for(5ms);
        timeout -= 5;
    }
    CPPUNIT_ASSERT_MESSAGE("Disconnection from GUI Server timedout.", m_tcpAdapter->connected());

    std::clog << "OK" << std::endl;
}


void GuiServer_Test::testInvalidTokenOnBeginTemporarySession() {
    std::clog << "testInvalidTokenOnBeginTemporarySession: " << std::flush;

    Hash loginInfo("type", "login", "clientId", "desk010", "oneTimeToken", TestKaraboAuthServer::VALID_TOKEN, "version",
                   "2.20.0");

    Hash beginTempSessionInfo("type", "beginTemporarySession", "temporarySessionToken", "abcd", "version", "2.20.0",
                              "levelBeforeTemporarySession", static_cast<int>(Schema::AccessLevel::OPERATOR));

    resetTcpConnection();

    // Performs an authenticated login successfully - temporary sessions are only available from that point on.
    Hash lastMessage;
    karabo::TcpAdapter::QueuePtr messageQ =
          m_tcpAdapter->getNextMessages("loginInformation", 1, [&] { m_tcpAdapter->sendMessage(loginInfo); });
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT_MESSAGE("'accessLevel' missing in loginInformation sent by the GUI Server",
                           lastMessage.has("accessLevel"));
    // Attempts at an beginTemporarySession with an invalid token
    messageQ = m_tcpAdapter->getNextMessages("onBeginTemporarySession", 1,
                                             [&] { m_tcpAdapter->sendMessage(beginTempSessionInfo); });
    messageQ->pop(lastMessage);
    bool success = lastMessage.get<bool>("success");
    const std::string& reason = lastMessage.get<std::string>("reason");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("beginTemporarySession request succeeded unexpectedly", false, success);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("beginTemporarySession failure reason", TestKaraboAuthServer::INVALID_TOKEN_MSG,
                                 reason);

    int timeout = 1500;
    // wait for the GUI server to log us out
    while (m_tcpAdapter->connected() && timeout > 0) {
        std::this_thread::sleep_for(5ms);
        timeout -= 5;
    }
    CPPUNIT_ASSERT_MESSAGE("Disconnection from GUI Server timedout.", m_tcpAdapter->connected());

    std::clog << "OK" << std::endl;
}

void GuiServer_Test::testBeginEndTemporarySession() {
    std::clog << "testBeginEndTemporarySession: " << std::flush;

    Hash loginInfo("type", "login", "clientId", "desk010", "oneTimeToken", TestKaraboAuthServer::VALID_TOKEN, "version",
                   "2.20.0");

    Hash beginTempSessionInfo("type", "beginTemporarySession", "temporarySessionToken",
                              TestKaraboAuthServer::VALID_TOKEN, "version", "2.20.0", "levelBeforeTemporarySession",
                              static_cast<int>(Schema::AccessLevel::OPERATOR));

    Hash endTempSessionInfo("type", "endTemporarySession", "temporarySessionToken", TestKaraboAuthServer::VALID_TOKEN,
                            "version", "2.20.0");

    resetTcpConnection();

    // Before any client connection is established, slotGetClientSessions should return no connection data
    // An options hash-in without the expected "onlyTempSessions" key should behave the same as specifying
    // the key with false.
    auto clientSessionsHash = m_deviceClient->execute1<Hash>(TEST_GUI_SERVER_ID, "slotGetClientSessions",
                                                             EXECUTE_SLOT_TIMEOUT, Hash("dummy", ""));
    auto clientSessions = clientSessionsHash.get<std::vector<Hash>>("clientSessions");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("No client connection was expected", 0U, static_cast<unsigned>(clientSessions.size()));

    // Performs an authenticated login successfully - temporary sessions are only available from that point on.
    Hash lastMessage;
    karabo::TcpAdapter::QueuePtr messageQ =
          m_tcpAdapter->getNextMessages("loginInformation", 1, [&] { m_tcpAdapter->sendMessage(loginInfo); });
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT_MESSAGE("'accessLevel' missing in loginInformation sent by the GUI Server",
                           lastMessage.has("accessLevel"));

    // Retrieves information about existing client sessions. There should be one
    // session with no temporary session on top.
    clientSessionsHash = m_deviceClient->execute1<Hash>(TEST_GUI_SERVER_ID, "slotGetClientSessions",
                                                        EXECUTE_SLOT_TIMEOUT, Hash(CLIENT_SESSIONS_OPTION_KEY, false));
    clientSessions = clientSessionsHash.get<std::vector<Hash>>("clientSessions");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Number of client connections differs from expected", 1U,
                                 static_cast<unsigned>(clientSessions.size()));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Client version differs from expected", std::string("2.20.0"),
                                 clientSessions[0].get<std::string>("clientVersion"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("A session token is expected (non-empty sessionToken)", false,
                                 clientSessions[0].get<std::string>("sessionToken").empty());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("A session start time is expected (non-empty sessionStartTime)", false,
                                 clientSessions[0].get<std::string>("sessionStartTime").empty());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("No temporary session is expected (empty temporarySessionToken)", true,
                                 clientSessions[0].get<std::string>("temporarySessionToken").empty());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("No temporary session is expected (empty temporarySessionStartTime)", true,
                                 clientSessions[0].get<std::string>("temporarySessionStartTime").empty());
    const auto sessionToken = clientSessions[0].get<std::string>("sessionToken");
    const auto sessionStartTime = clientSessions[0].get<std::string>("sessionStartTime");
    // If "onlyTempSessions" is set to true, the reply at this point should be empty
    clientSessionsHash = m_deviceClient->execute1<Hash>(TEST_GUI_SERVER_ID, "slotGetClientSessions",
                                                        EXECUTE_SLOT_TIMEOUT, Hash(CLIENT_SESSIONS_OPTION_KEY, true));
    clientSessions = clientSessionsHash.get<std::vector<Hash>>("clientSessions");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Number of client connections differs from expected", 0U,
                                 static_cast<unsigned>(clientSessions.size()));

    // Request a temporary session begining with a valid token.
    messageQ = m_tcpAdapter->getNextMessages("onBeginTemporarySession", 1,
                                             [&] { m_tcpAdapter->sendMessage(beginTempSessionInfo); });
    messageQ->pop(lastMessage);
    bool success = lastMessage.get<bool>("success");
    std::string reason = lastMessage.get<std::string>("reason");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("beginTemporarySession request failed", true, success);
    CPPUNIT_ASSERT_MESSAGE("Field 'reason' should be empty. Instead it has '" + reason + "'", reason.empty());
    CPPUNIT_ASSERT_MESSAGE("'onBeginTemporarySession' message should have an 'accessLevel' field",
                           lastMessage.has("accessLevel"));
    CPPUNIT_ASSERT_MESSAGE("'onBeginTemporarySession' message should have an 'temporarySessionDurationSecs' field",
                           lastMessage.has("temporarySessionDurationSecs"));
    CPPUNIT_ASSERT_MESSAGE("'onBeginTemporarySession' message should have an 'username' field",
                           lastMessage.has("username"));
    const std::string& userId = lastMessage.get<std::string>("username");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("User differs from expected", TestKaraboAuthServer::VALID_USER_ID, userId);

    // Retrieves information about existing client sessions. There should be one
    // session with a temporary session on top. The sessionToken and the
    // sessionStartTime should match what was retrieved before starting the
    // temporary session.
    clientSessionsHash = m_deviceClient->execute1<Hash>(TEST_GUI_SERVER_ID, "slotGetClientSessions",
                                                        EXECUTE_SLOT_TIMEOUT, Hash(CLIENT_SESSIONS_OPTION_KEY, true));
    clientSessions = clientSessionsHash.get<std::vector<Hash>>("clientSessions");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Number of client connections differs from expected", 1U,
                                 static_cast<unsigned>(clientSessions.size()));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Client version differs from expected", std::string("2.20.0"),
                                 clientSessions[0].get<std::string>("clientVersion"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("A session token is expected (non-empty sessionToken)", false,
                                 clientSessions[0].get<std::string>("sessionToken").empty());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("The session token differs from the expected value", sessionToken,
                                 clientSessions[0].get<std::string>("sessionToken"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("The session start time differs from the expected value", sessionStartTime,
                                 clientSessions[0].get<std::string>("sessionStartTime"));
    const std::string temporarySessionToken = clientSessions[0].get<std::string>("temporarySessionToken");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("A temporary session is expected (non-empty temporarySessionToken)", false,
                                 temporarySessionToken.empty());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("A temporary session is expected (non-empty temporarySessionStartTime)", false,
                                 clientSessions[0].get<std::string>("temporarySessionStartTime").empty());
    // At this point "onlyTempSessions" set to false should also return the
    // same single existing session with the active temporary session.
    clientSessionsHash = m_deviceClient->execute1<Hash>(TEST_GUI_SERVER_ID, "slotGetClientSessions",
                                                        EXECUTE_SLOT_TIMEOUT, Hash(CLIENT_SESSIONS_OPTION_KEY, false));
    clientSessions = clientSessionsHash.get<std::vector<Hash>>("clientSessions");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Number of client connections differs from expected", 1U,
                                 static_cast<unsigned>(clientSessions.size()));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Temporary Session Token differs from expected", temporarySessionToken,
                                 clientSessions[0].get<std::string>("temporarySessionToken"));

    // Request a begin temporary session with a valid token - should be rejected since we are already in a temporary
    // session
    messageQ = m_tcpAdapter->getNextMessages("onBeginTemporarySession", 1,
                                             [&] { m_tcpAdapter->sendMessage(beginTempSessionInfo); });
    messageQ->pop(lastMessage);
    success = lastMessage.get<bool>("success");
    reason = lastMessage.get<std::string>("reason");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("beginTemporarySession request didn't fail as expected", false, success);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Reason for failed temporary session over temporary session",
                                 std::string{"There's already an active temporary session."}, reason);

    // Request a login (over an existing login session) while a temporary session is active - login must be
    // rejected because of the active temporary session.
    const string expectedErrorMsg =
          "There's an active temporary session. Please terminate it before trying to "
          "login again.";
    messageQ = m_tcpAdapter->getNextMessages("notification", 1, [&] { m_tcpAdapter->sendMessage(loginInfo); });
    messageQ->pop(lastMessage);
    const std::string& message = lastMessage.get<std::string>("message");
    CPPUNIT_ASSERT_MESSAGE("Expected notification message '" + expectedErrorMsg + "'. Got '" + message + "'",
                           message == expectedErrorMsg);

    // Request an end of temporary session
    messageQ = m_tcpAdapter->getNextMessages("onEndTemporarySession", 1,
                                             [&] { m_tcpAdapter->sendMessage(endTempSessionInfo); });
    messageQ->pop(lastMessage);
    success = lastMessage.get<bool>("success");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("endTemporarySession request failed", true, success);
    Schema::AccessLevel levelBeforeTemporarySession =
          static_cast<Schema::AccessLevel>(lastMessage.get<int>("levelBeforeTemporarySession"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("levelBeforeTemporarySession", Schema::AccessLevel::OPERATOR,
                                 levelBeforeTemporarySession);
    std::string loggedUserId = lastMessage.get<std::string>("loggedUserId");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("loggedUserId", TestKaraboAuthServer::VALID_USER_ID, loggedUserId);
    // Request endTemporarySession again - should fail as there is no active temporary session
    messageQ = m_tcpAdapter->getNextMessages("onEndTemporarySession", 1,
                                             [&] { m_tcpAdapter->sendMessage(endTempSessionInfo); });
    messageQ->pop(lastMessage);
    success = lastMessage.get<bool>("success");
    reason = lastMessage.get<std::string>("reason");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
          "endTemporarySession unexpectedly succeeded - there should be no active temporary session.", false, success);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
          "Reason for failed temporary session over temporary session unexpected",
          std::string{"There's no active temporary session associated with the requesting client."}, reason);

    int timeout = 1500;
    // wait for the GUI server to log us out
    while (m_tcpAdapter->connected() && timeout > 0) {
        std::this_thread::sleep_for(5ms);
        timeout -= 5;
    }
    CPPUNIT_ASSERT_MESSAGE("Disconnection from GUI Server timedout.", m_tcpAdapter->connected());

    std::clog << "OK" << std::endl;
}

void GuiServer_Test::testTemporarySessionExpiration() {
    std::clog << "testTemporarySessionExpiration: " << std::flush;

    Hash loginInfo("type", "login", "clientId", "desk010", "oneTimeToken", TestKaraboAuthServer::VALID_TOKEN, "version",
                   "2.20.0");

    Hash beginTempSessionInfo("type", "beginTemporarySession", "temporarySessionToken",
                              TestKaraboAuthServer::VALID_TOKEN, "version", "2.20.0", "levelBeforeTemporarySession",
                              static_cast<int>(Schema::AccessLevel::OPERATOR));

    resetTcpConnection();

    // Performs an authenticated login successfully - temporary sessions are only available from that point on.
    Hash lastMessage;
    karabo::TcpAdapter::QueuePtr messageQ =
          m_tcpAdapter->getNextMessages("loginInformation", 1, [&] { m_tcpAdapter->sendMessage(loginInfo); });
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT_MESSAGE("'accessLevel' missing in loginInformation sent by the GUI Server",
                           lastMessage.has("accessLevel"));
    // Request a temporary session with a valid token - waits for 3 messages: the "onBeginTemporarySession" result
    // message (should be successful) immediately after, an "onEndTemporarySessionNotice" after the first check
    // cycle of the GuiServerTemporarySessionManager and an "onTemporarySessionExpired" after the second check
    // cycle.
    messageQ = m_tcpAdapter->getNextMessages("onBeginTemporarySession", 1,
                                             [&] { m_tcpAdapter->sendMessage(beginTempSessionInfo); });
    messageQ->pop(lastMessage);
    bool success = lastMessage.get<bool>("success");
    std::string reason = lastMessage.get<std::string>("reason");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("beginTemporarySession request failed", true, success);
    CPPUNIT_ASSERT_MESSAGE("Field 'reason' should be empty.", reason.empty());
    CPPUNIT_ASSERT_MESSAGE("'onBeginTemporarySession' message should have an 'accessLevel' field",
                           lastMessage.has("accessLevel"));
    CPPUNIT_ASSERT_MESSAGE("'onBeginTemporarySession' message should have an 'temporarySessionDurationSecs' field",
                           lastMessage.has("temporarySessionDurationSecs"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Value of temporarySessionDurationSecs", MAX_TEMPORARY_SESSION_TIME,
                                 lastMessage.get<unsigned int>("temporarySessionDurationSecs"));

    // No triggering needed and timeout large enough to guarantee reception of an eminent expiration notice.
    const size_t lifetimePlusCheckInterval =
          1'000ul * (MAX_TEMPORARY_SESSION_TIME + karabo::devices::CHECK_TEMPSESSION_EXPIRATION_INTERVAL_SECS);
    messageQ = m_tcpAdapter->getNextMessages("onEndTemporarySessionNotice", 1ul, [] {}, lifetimePlusCheckInterval);
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT_MESSAGE("'onEndTemporarySessionNotice' message should have an 'aboutToExpireToken' field",
                           lastMessage.has("aboutToExpireToken"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Unexpected about to expire token value", TestKaraboAuthServer::VALID_TOKEN,
                                 lastMessage.get<std::string>("aboutToExpireToken"));
    CPPUNIT_ASSERT_MESSAGE("'onBeginTemporarySession' message should have a 'secondsToExpiration' field",
                           lastMessage.has("secondsToExpiration"));
    const karabo::data::TimeValue secsToExpiration = lastMessage.get<karabo::data::TimeValue>("secondsToExpiration");
    CPPUNIT_ASSERT_LESSEQUAL(END_TEMPORARY_SESSION_NOTICE_TIME, static_cast<unsigned int>(secsToExpiration));

    // No triggering needed and timeout large enough to guarantee an expiration being received.
    messageQ = m_tcpAdapter->getNextMessages("onTemporarySessionExpired", 1ul, [] {}, lifetimePlusCheckInterval);
    messageQ->pop(lastMessage);
    CPPUNIT_ASSERT_MESSAGE("'onTemporarySessionExpired' message should have an 'expiredToken' field",
                           lastMessage.has("expiredToken"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Unexpected expired token value", TestKaraboAuthServer::VALID_TOKEN,
                                 lastMessage.get<std::string>("expiredToken"));
    CPPUNIT_ASSERT_MESSAGE("'onBeginTemporarySession' message should have an 'expirationTime' field",
                           lastMessage.has("expirationTime"));
    Epochstamp expiredAt = Epochstamp(lastMessage.get<std::string>("expirationTime"));
    Epochstamp currentTime;
    CPPUNIT_ASSERT_GREATER(expiredAt, currentTime);

    Schema::AccessLevel levelBeforeTemporarySession =
          static_cast<Schema::AccessLevel>(lastMessage.get<int>("levelBeforeTemporarySession"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("levelBeforeTemporarySession", Schema::AccessLevel::OPERATOR,
                                 levelBeforeTemporarySession);
    std::string loggedUserId = lastMessage.get<std::string>("loggedUserId");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("loggedUserId", TestKaraboAuthServer::VALID_USER_ID, loggedUserId);

    int timeout = 1500;
    // wait for the GUI server to log us out
    while (m_tcpAdapter->connected() && timeout > 0) {
        std::this_thread::sleep_for(5ms);
        timeout -= 5;
    }
    CPPUNIT_ASSERT_MESSAGE("Disconnection from GUI Server timedout.", m_tcpAdapter->connected());

    std::clog << "OK" << std::endl;
}


void GuiServer_Test::testOnlyAppModeClients() {
    std::clog << "testOnlyAppModeClients: " << std::flush;

    Hash loginInfo("type", "login", "username", "bob", "applicationMode", false, "version", "2.20.0");

    resetTcpConnection();

    // Checks that a login from a non applicationMode client is rejected by a GUI Server set to only
    // accept applicationMode clients.
    Hash lastMessage;
    const string expectedMsgPart = "configured to refuse connections from the standard Karabo GUI Client";
    karabo::TcpAdapter::QueuePtr messageQ =
          m_tcpAdapter->getNextMessages("notification", 1, [&] { m_tcpAdapter->sendMessage(loginInfo); });
    messageQ->pop(lastMessage);
    const std::string& message = lastMessage.get<std::string>("message");
    CPPUNIT_ASSERT_MESSAGE(
          "The returned message, '" + message + "' doesn't contain the expected part, '" + expectedMsgPart + "'.",
          message.find(expectedMsgPart) != std::string::npos);

    int timeout = 1500;
    // wait for the GUI server to log us out
    while (m_tcpAdapter->connected() && timeout > 0) {
        std::this_thread::sleep_for(5ms);
        timeout -= 5;
    }
    CPPUNIT_ASSERT_MESSAGE("GUI Server should have closed the connection after sending the error notification",
                           !m_tcpAdapter->connected());

    resetTcpConnection();

    // Checks that a login from an applicationMode client is accepted by a GUI Server set to only accept
    // applicationMode clients. A successful login (no auth in this case) is followed by the GUI Server
    // sending the system topology to the client.
    Hash loginInfoAppMode("type", "login", "username", "alice", "applicationMode", true, "version", "2.20.0");
    CPPUNIT_ASSERT_NO_THROW(messageQ = m_tcpAdapter->getNextMessages(
                                  "systemTopology", 1, [&] { m_tcpAdapter->sendMessage(loginInfoAppMode); }));

    std::clog << "OK" << std::endl;
}
