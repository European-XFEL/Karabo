/*
 * File:   GuiServer_Test.hh
 * Author: alessandro.silenzi@xfel.eu
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

#ifndef GUISERVER_TEST_HH
#define GUISERVER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <memory>

#include "TcpAdapter.hh"
#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/karabo.hpp"

#define CLIENT_SESSIONS_OPTION_KEY "onlyTempSessions"

class GuiServer_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(GuiServer_Test);

    CPPUNIT_TEST(appTestRunner);

    CPPUNIT_TEST_SUITE_END();

   public:
    GuiServer_Test();
    virtual ~GuiServer_Test();
    void setUp();
    void tearDown();

   private:
    void appTestRunner();
    void testVersionControl();
    void resetTcpConnection();
    void resetClientConnection();
    void resetClientConnection(const karabo::util::Hash& loginData);
    void testNotification();
    void testExecuteBeforeLogin();
    void testExecute();
    void testGetDeviceSchema();
    void testSlowSlots();
    void testGetClassSchema();
    void testReconfigure();
    void testReadOnly();
    void testRequestGeneric();
    void testRequestFailProtocol();
    void testRequestFailOldVersion();
    void testSlotNotify();
    void testSlotBroadcast();
    /**
     * Checks that messages of type 'deviceConfigurations' sent by
     * the GUI Server contain all the expected config updates for
     * all devices of interest of a connected client.
     */
    void testDeviceConfigUpdates();

    /**
     * @brief Checks that a "login" message with no "oneTimeToken" is refused by a GUI Server configured to require
     * authentication and an error notification is returned to the GUI Client.
     *
     */
    void testMissingTokenOnLogin();

    /**
     * @brief Checks that when a "login" message with an invalid "oneTimeToken" is received by the GUI Server, it
     * returns a corresponding error notification to the GUI Client.
     *
     */
    void testInvalidTokenOnLogin();

    /**
     * @brief Checks that when a "login" message with a valid "oneTimeToken" is received by the GUI Server, it returns
     * the resolved user's access level for the server's topic to the client.
     *
     */
    void testValidTokenOnLogin();

    /**
     * @brief Checks that a "beginTemporarySession" message with no "oneTimeToken" is refused by a GUI Server configured
     * to require authentication and an error notification is returned to the GUI Client.
     *
     */
    void testMissingTokenOnBeginTemporarySession();

    /**
     * @brief Checks that when a "beginTemporarySession" message with an invalid "oneTimeToken" is received by the GUI
     * Server, it returns a corresponding error notification to the GUI Client.
     *
     */
    void testInvalidTokenOnBeginTemporarySession();

    /**
     * @brief Checks that the "normal" login > beginTemporarySession > endTemporarySession sequence works. Also checks
     * that attempts of double temporary sessions and endTemporarySession requests are rejected.
     */
    void testBeginEndTemporarySession();

    /**
     * @brief Checks that an beginTemporarySession expires after the expected expiration time.
     */
    void testTemporarySessionExpiration();

    /**
     * @brief Checks that a GUI Server that only accepts Application Mode clients refuses
     * logins from standard GUI Clients.
     */
    void testOnlyAppModeClients();

    void testDisconnect();

    karabo::core::DeviceServer::Pointer m_deviceServer;
    std::jthread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
    std::shared_ptr<karabo::TcpAdapter> m_tcpAdapter;
    static const karabo::util::Hash k_defaultLoginData;
};

#endif /* GUISERVER_TEST_HH */
