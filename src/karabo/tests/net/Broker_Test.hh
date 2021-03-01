/* 
 * File:   Broker_Test.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on February 19, 2021, 3:09 PM
 */

#ifndef BROKER_TEST_HH
#define	BROKER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include <karabo/util/Hash.hh>
#include <karabo/net/Broker.hh>

class Broker_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(Broker_Test);
    CPPUNIT_TEST(testConnectDisconnect);
    CPPUNIT_TEST(testPublishSubscribe);
    CPPUNIT_TEST(testPublishSubscribeAsync);
    CPPUNIT_TEST(testReadingHeartbeatsAndLogs);
    CPPUNIT_TEST(testReadingGlobalCalls);
    CPPUNIT_TEST_SUITE_END();

public:

    Broker_Test();
    virtual ~Broker_Test();
    void setUp();
    void tearDown();

private:

    void testConnectDisconnect();
    void _testConnectDisconnect();
    void testPublishSubscribe();
    void _testPublishSubscribe();
    void testPublishSubscribeAsync();
    void _testPublishSubscribeAsync();
    void testReadingHeartbeatsAndLogs();
    void _testReadingHeartbeatsAndLogs();
    void testReadingGlobalCalls();
    void _testReadingGlobalCalls(const std::string& brokerAddress);

private:

    karabo::util::Hash m_config;
    std::string m_domain;
    boost::shared_ptr<boost::thread> m_thread;
};

#endif	/* BROKER_TEST_HH */

