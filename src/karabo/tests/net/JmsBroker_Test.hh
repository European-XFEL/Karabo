/*
 * File:   JmsBroker_Test.hh
 * Author: heisenb
 *
 * Created on Mar 20, 2013, 3:31:07 PM
 */

#ifndef JMSBROKER_TEST_HH
#define	JMSBROKER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/net/BrokerIOService.hh>
#include <karabo/net/BrokerConnection.hh>
#include <karabo/net/BrokerChannel.hh>

class JmsBroker_Test : public CPPUNIT_NS::TestFixture {
    
    int m_messagesRead;
    
    karabo::util::Hash m_hash;

    CPPUNIT_TEST_SUITE(JmsBroker_Test);

    CPPUNIT_TEST(testMethod);
    
    CPPUNIT_TEST(testBinaryTransport);

    CPPUNIT_TEST_SUITE_END();

public:
    JmsBroker_Test();
    
    virtual ~JmsBroker_Test();
    
    void readHandler1(karabo::net::BrokerChannel::Pointer channel, const karabo::util::Hash::Pointer& header, const std::string& body);
    
    void readHandler2(karabo::net::BrokerChannel::Pointer channel, const karabo::util::Hash::Pointer& header, const karabo::util::Hash::Pointer& body);
    
    
private:
    
    void testMethod();
    
    void testBinaryTransport();

};

#endif	/* JMSBROKER_TEST_HH */

