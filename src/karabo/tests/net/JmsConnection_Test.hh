/* 
 * File:   JmsConnection_Test.hh
 * Author: heisenb
 *
 * Created on July 15, 2016, 5:05 PM
 */

#ifndef JMSCONNECTION_TEST_HH
#define	JMSCONNECTION_TEST_HH

#include <karabo/net/JmsConsumer.hh>
#include <karabo/net/JmsProducer.hh>
#include <karabo/net/JmsConnection.hh>
#include <cppunit/extensions/HelperMacros.h>

class JmsConnection_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(JmsConnection_Test);
    CPPUNIT_TEST(testConnect);
    CPPUNIT_TEST(testCommunication1);
    CPPUNIT_TEST(testCommunication2);
    CPPUNIT_TEST(testPermanentRead);

    CPPUNIT_TEST_SUITE_END();

    karabo::net::JmsConnection::Pointer m_connection;
    unsigned int m_messageCount;
    boost::posix_time::ptime m_tick;
    boost::mutex m_mutex;
    

public:
    JmsConnection_Test();
    virtual ~JmsConnection_Test();

private:

    unsigned int incrementMessageCount();

    unsigned int getMessageCount();

    void testConnect();

    void testCommunication1();

    void testCommunication2();

    void testPermanentRead();

    void readHandler1(karabo::net::JmsConsumer::Pointer consumer,
                      karabo::net::JmsProducer::Pointer producer,
                     karabo::util::Hash::Pointer header,
                      karabo::util::Hash::Pointer body);

    void readHandler2(karabo::net::JmsConsumer::Pointer channel,
                      karabo::util::Hash::Pointer header,
                      karabo::util::Hash::Pointer body);

};

#endif	/* JMSCONNECTION_TEST_HH */

