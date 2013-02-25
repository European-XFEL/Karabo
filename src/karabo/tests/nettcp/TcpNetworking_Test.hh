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
#include <iostream>
#include <fstream>
#include <cassert>
#include <iosfwd>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <karabo/util/Hash.hh>
#include <karabo/net/Connection.hh>
#include <karabo/net/Channel.hh>

class TcpNetworking_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TcpNetworking_Test);

    CPPUNIT_TEST(testMethod);

    CPPUNIT_TEST_SUITE_END();

public:
    TcpNetworking_Test();
    virtual ~TcpNetworking_Test();
    void setUp();
    void tearDown();

private:
    void testMethod();

};

namespace karabo {
    namespace tcpclientserver {

        class TcpClientServer {
        public:
            TcpClientServer(const std::string& rHost, unsigned short rPort, unsigned short lPort);
            ~TcpClientServer();
            void run();
            
            void clientConnectHandler(karabo::net::Channel::Pointer channel);
            void clientConnectionErrorHandler(karabo::net::Channel::Pointer channel, const std::string & errmsg);
            void clientErrorHandler(karabo::net::Channel::Pointer channel, const std::string & errmsg);
            void clientReadHashHandler(karabo::net::Channel::Pointer channel, const karabo::util::Hash & data);
            void clientWriteCompleteHandler(karabo::net::Channel::Pointer channel);
            void timerHandler(karabo::net::Channel::Pointer channel);
            
            void serverConnectHandler(karabo::net::Channel::Pointer channel);
            void serverErrorHandler(karabo::net::Channel::Pointer channel, const std::string & errmsg);
            void serverReadHashHandler(karabo::net::Channel::Pointer channel, const karabo::util::Hash& data);
            void serverWriteCompleteHandler(karabo::net::Channel::Pointer channel);
            
        private:
            int m_remoteCount;
            int m_localCount;
            std::string m_remoteHost;
            unsigned short m_remotePort;
            unsigned short m_localPort;
            karabo::net::Connection::Pointer m_clientConnection;
            karabo::net::Connection::Pointer m_serverConnection;
            karabo::util::Hash m_clientData;
            karabo::util::Hash m_serverData;
        };
    }
}

#endif	/* TCPNETWORKING_TEST_HH */

