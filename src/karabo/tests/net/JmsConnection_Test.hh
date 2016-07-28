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
    CPPUNIT_TEST(testAll);
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

    /**
     * CppUnit creates a new instance for each registered test function
     * To test the broker business, I need a reliable state and only one instance
     * Hence, only one function will be registered that calls the other ones
     */
    void testAll();

    void testConnect();

    void testCommunication1();

    void testCommunication2();

    void readHandler1(karabo::net::JmsConsumer::Pointer consumer,
                      karabo::net::JmsProducer::Pointer producer,
                     karabo::util::Hash::Pointer header,
                      karabo::util::Hash::Pointer body);

    void readHandler2(karabo::net::JmsConsumer::Pointer channel,
                      karabo::util::Hash::Pointer header,
                      karabo::util::Hash::Pointer body);

    void readHandler3(karabo::net::JmsConsumer::Pointer channel,
                      karabo::util::Hash::Pointer header,
                      karabo::util::Hash::Pointer body);

    void readHandler4(karabo::net::JmsConsumer::Pointer channel,
                      karabo::util::Hash::Pointer header,
                      karabo::util::Hash::Pointer body);

    

    

};

#endif	/* JMSCONNECTION_TEST_HH */

