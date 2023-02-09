/*
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Created on Oct 30, 2012, 1:33:46 PM
 */

#ifndef TCPNETWORKING_TEST_HH
#define TCPNETWORKING_TEST_HH
#include <cppunit/extensions/HelperMacros.h>

#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>

class TcpNetworking_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TcpNetworking_Test);
    CPPUNIT_TEST(testConfig);
    CPPUNIT_TEST(testClientServer);
    CPPUNIT_TEST(testWriteAsync);
    CPPUNIT_TEST(testBufferSet);
    CPPUNIT_TEST(testConsumeBytesAfterReadUntil);
    CPPUNIT_TEST_SUITE_END();

   public:
    KARABO_CLASSINFO(TcpNetworking_Test, "TcpNetworking_Test", "1.0");

    TcpNetworking_Test();
    virtual ~TcpNetworking_Test();
    void setUp();
    void tearDown();

   private:
    void testConfig();
    void testClientServer();

    /**
     * Test the set of writeAsync methods of type write and "forget" in the
     * karabo::net::TcpChannel class (acessed through its abstract base
     * class, karabo::net::Channel).
     *
     * The write and "forget" do not call write handlers specified by their
     * callers once they have done their job (or failed at trying to do it).
     */
    void testWriteAsync();

    void testBufferSet();

    void testConsumeBytesAfterReadUntil();
};

#endif /* TCPNETWORKING_TEST_HH */
