/*
 * File:   GuiVersion_Test.hh
 * Author: alessandro.silenzi@xfel.eu
 
 */

#ifndef GuiVersion_Test_HH
#define	GuiVersion_Test_HH

#include "karabo/karabo.hpp"
#include "karabo/core/DeviceServer.hh"
#include "karabo/core/DeviceClient.hh"
#include <boost/shared_ptr.hpp>
#include <cppunit/extensions/HelperMacros.h>

#include "TcpAdapter_5.hh"


class GuiVersion_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(GuiVersion_Test);

    CPPUNIT_TEST(appTestRunner);

    CPPUNIT_TEST_SUITE_END();

public:
    GuiVersion_Test();
    virtual ~GuiVersion_Test();
    void setUp();
    void tearDown();

private:
    void appTestRunner();
    void testVersionControl();
    void resetClientConnection();
    void testNotification();
    void testExecute();
    void testSlowSlots();
    void testReconfigure();
    void testReadOnly();
    void testRequestGeneric();
    void testRequestFailProtocol();

    /**
     * Checks that messages of type 'deviceConfigurations' sent by
     * the GUI Server contain all the expected config updates for
     * all devices of interest of a connected client.
     */
    void testDeviceConfigUpdates();

    void testDisconnect();

    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
    boost::shared_ptr<karabo::TcpAdapter> m_tcpAdapter;
};

#endif	/* GuiVersion_Test_HH */

