/*
 * File:   GuiServer_Test.hh
 * Author: alessandro.silenzi@xfel.eu

 */

#ifndef GUISERVER_TEST_HH
#define GUISERVER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <boost/shared_ptr.hpp>

#include "TcpAdapter.hh"
#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/karabo.hpp"


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

    void testDisconnect();

    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
    boost::shared_ptr<karabo::TcpAdapter> m_tcpAdapter;
    static const karabo::util::Hash k_defaultLoginData;
};

#endif /* GUISERVER_TEST_HH */
