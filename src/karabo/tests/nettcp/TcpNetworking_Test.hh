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
    namespace net {

        struct TcpServer {

            TcpServer() : m_count(0) {
            }

            virtual ~TcpServer() {
            }

            void connectHandler(karabo::net::Channel::Pointer channel);
            void readVectorHashHandler(karabo::net::Channel::Pointer channel, const std::vector<char>& data, const karabo::util::Hash & hash);
            void writeCompleteHandler(karabo::net::Channel::Pointer channel);
            void errorHandler(karabo::net::Channel::Pointer channel, const std::string & errmsg);
            void run();

        private:
            int m_count;
            karabo::net::Connection::Pointer m_connection;
            karabo::util::Hash m_hash;
            std::vector<char> m_data;
        };

        struct TcpClient {

            TcpClient() : m_count(0){
            }

            virtual ~TcpClient() {
            }

            void run();
            void errorHandler(karabo::net::Channel::Pointer channel, const std::string & errmsg);
            void readStringHashHandler(karabo::net::Channel::Pointer channel, const std::string& data, const karabo::util::Hash & hash);
            void connectHandler(karabo::net::Channel::Pointer channel);
            void timerHandler(karabo::net::Channel::Pointer channel);

        private:
            int m_count;
            karabo::net::Connection::Pointer m_connection;
            karabo::util::Hash m_hash;
            std::string m_data;
        };
    }
}

#endif	/* TCPNETWORKING_TEST_HH */

