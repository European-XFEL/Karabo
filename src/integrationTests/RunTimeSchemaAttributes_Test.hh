/* 
 * File:   RunTimeSchemaAttributes.hh
 * Author: steffen
 *
 * Created on September 13, 2016, 7:11 PM
 */

#ifndef RUNTIMESCHEMAATTRIBUTES_HH
#define	RUNTIMESCHEMAATTRIBUTES_HH

#include "karabo/karabo.hpp"
#include "karabo/devices/AlarmService.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/core/DeviceClient.hh"
#include <boost/shared_ptr.hpp>
#include <cppunit/extensions/HelperMacros.h>

#include "AlarmTesterDevice.hh"
#include "TcpAdapter.hh"

class RunTimeSchemaAttributes_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(RunTimeSchemaAttributes_Test);

    CPPUNIT_TEST(appTestRunner);

    CPPUNIT_TEST_SUITE_END();

public:
    RunTimeSchemaAttributes_Test();
    virtual ~RunTimeSchemaAttributes_Test();
    void setUp();
    void tearDown();

private:

    void appTestRunner();
    void testRuntimeApplication();
    void testGuiServerApplication();
    void testGuiServerApplicationFailure();
    void dummyMonitor(const std::string&, const karabo::util::Hash&);

    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
    boost::shared_ptr<karabo::TcpAdapter> m_tcpAdapter;




};

#endif	/* RUNTIMESCHEMAATTRIBUTES_HH */

