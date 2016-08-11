/*
 * File:   MQTcpNetworking.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on Sep 22, 2015, 3:06:16 PM
 */

#ifndef MQTCPNETWORKING_HH
#define	MQTCPNETWORKING_HH

#include <boost/date_time/posix_time/posix_time.hpp>
#include <cppunit/extensions/HelperMacros.h>

#include <karabo/net/IOService.hh>
#include <karabo/net/Connection.hh>
#include <karabo/net/Channel.hh>

class MQTcpNetworking : public CPPUNIT_NS::TestFixture {


    int m_numberOfMessages;

    karabo::util::Hash m_header;

    karabo::util::Hash m_data;

    // server
    int m_serverCount;
    int m_serverPort;
    boost::thread m_serverThread;
    karabo::net::Connection::Pointer m_serverConnection;
    boost::posix_time::ptime m_ts;

    // client
    int m_clientCount;
    karabo::net::Connection::Pointer m_connection;
    boost::posix_time::ptime m_clientTimestamp;

    CPPUNIT_TEST_SUITE(MQTcpNetworking);

    CPPUNIT_TEST(testClientServerMethod);

    CPPUNIT_TEST_SUITE_END();

public:
    MQTcpNetworking();
    virtual ~MQTcpNetworking();
    void setUp();
    void tearDown();

private:

    void createServer();

    void serverConnectHandler(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode& e);

    void serverErrorHandler(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode& ec);
    
    void serverReadHashHashHandler(const karabo::net::Channel::Pointer& channel,
                                   karabo::util::Hash& header,
                                   karabo::util::Hash& body,
                                   const karabo::net::ErrorCode& ec);

    void serverPublish(const karabo::net::Channel::Pointer& channel);

    void serverRun();

    void testClientServerMethod();

    void onClientConnected(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode& ec);

    void clientChannelErrorHandler(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode& ec);
    
    void clientReadHashHashHandler(const karabo::net::Channel::Pointer& channel,
                                   karabo::util::Hash& header,
                                   karabo::util::Hash& body,
                                   const karabo::net::ErrorCode& ec);

    void onClientEnd(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode& ec);
};

#endif	/* MQTCPNETWORKING_HH */

