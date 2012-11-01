/*
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Created on Oct 30, 2012, 1:33:46 PM
 */

#ifndef UDPNETWORKING_TEST_HH
#define	UDPNETWORKING_TEST_HH

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

class UdpNetworking_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(UdpNetworking_Test);

    CPPUNIT_TEST(testMethod);

    CPPUNIT_TEST_SUITE_END();

public:
    UdpNetworking_Test();
    virtual ~UdpNetworking_Test();
    void setUp();
    void tearDown();
    
    private:
    void testMethod();
};

namespace karabo {
    namespace net {

        struct UdpServer {
            UdpServer();
            virtual ~UdpServer();
            void readVectorHandler(karabo::net::Channel::Pointer channel, const std::vector<char>& data);
            void writeCompleteHandler(karabo::net::Channel::Pointer channel);
            void errorHandler(karabo::net::Channel::Pointer channel, const std::string & errmsg);
            void run();

        private:
            int m_count;
            karabo::net::Connection::Pointer m_connection;
            std::vector<char> m_data;
        };

        struct UdpClient {
            UdpClient();
            virtual ~UdpClient();
            void run();
            void errorHandler(karabo::net::Channel::Pointer channel, const std::string & errmsg);
            void readVectorHandler(karabo::net::Channel::Pointer channel, const std::vector<char>& data);
            void timerHandler(karabo::net::Channel::Pointer channel);

        private:
            int m_count;
            karabo::net::Connection::Pointer m_connection;
            std::vector<char> m_data;
        };
    }
}

#endif	/* UDPNETWORKING_TEST_HH */

