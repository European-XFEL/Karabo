/*
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Created on Oct 30, 2012, 1:33:46 PM
 */

#ifndef TCPNETWORKING_TEST_HH
#define	TCPNETWORKING_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class TcpNetworking_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TcpNetworking_Test);
    CPPUNIT_TEST(testClientServer);
    CPPUNIT_TEST_SUITE_END();

public:

    TcpNetworking_Test();
    virtual ~TcpNetworking_Test();
    void setUp();
    void tearDown();

private:
    void testClientServer();

};

#endif	/* TCPNETWORKING_TEST_HH */

