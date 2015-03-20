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

#include <karabo/net/IOService.hh>
#include <karabo/net/Connection.hh>
#include <karabo/net/Channel.hh>

namespace karabo {
    namespace nettcp {

        struct TcpServer {

            TcpServer();

            virtual ~TcpServer();

            int getPort() {
                return m_port;
            }
            
            void connectHandler(const karabo::net::Channel::Pointer& channel);
            
            void errorHandler(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode & ec);
            
            void readHashHashHandler(const karabo::net::Channel::Pointer& channel, karabo::util::Hash& header, karabo::util::Hash& data);
            
            void writeCompleteHandler(const karabo::net::Channel::Pointer& channel, const std::string& id);
            
            void run();

        private:
            int m_count;
            int m_port;
            karabo::net::Connection::Pointer m_connection;
        };

        struct TcpClient {

            TcpClient(const std::string& host, int port);

            virtual ~TcpClient();

            void run();
            void connectHandler(const karabo::net::Channel::Pointer& channel);
            void connectionErrorHandler(const karabo::net::Connection::Pointer& connection, const karabo::net::ErrorCode & ec);
            void errorHandler(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode & ec);
            void readHashHashHandler(const karabo::net::Channel::Pointer& channel, karabo::util::Hash & header, karabo::util::Hash& data);
            void writeCompleteHandler(const karabo::net::Channel::Pointer& channel, int id);

        private:
            int m_count;
            std::string m_host;
            int m_port;
            karabo::net::Connection::Pointer m_connection;
            karabo::util::Hash m_hash;
            std::string m_data;
        };
    }
}

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

#endif	/* TCPNETWORKING_TEST_HH */

