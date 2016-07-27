/* 
 * File:   JmsConnection_Test.hh
 * Author: heisenb
 *
 * Created on July 15, 2016, 5:05 PM
 */

#ifndef JMSCONNECTION_TEST_HH
#define	JMSCONNECTION_TEST_HH

#include <karabo/net/JmsChannel.hh>
#include <karabo/net/JmsConnection.hh>
#include <cppunit/extensions/HelperMacros.h>

class JmsConnection_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(JmsConnection_Test);
    CPPUNIT_TEST(testConnect);
    CPPUNIT_TEST(testChannel);
    CPPUNIT_TEST_SUITE_END();

    karabo::net::JmsConnection::Pointer m_connection;
    unsigned int m_counter;
    boost::posix_time::ptime tick;

public:
    JmsConnection_Test();
    virtual ~JmsConnection_Test();

private:

    void testConnect();

    void testChannel();

    void readHandler(karabo::net::JmsChannel::Pointer channel,
                     karabo::util::Hash::Pointer header,
                     karabo::util::Hash::Pointer body);

};

#endif	/* JMSCONNECTION_TEST_HH */

