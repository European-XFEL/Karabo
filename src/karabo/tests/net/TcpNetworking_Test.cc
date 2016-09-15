/*
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Created on Oct 30, 2012, 1:33:46 PM
 */

#include <iostream>
#include <fstream>
#include <cassert>
#include <iosfwd>
#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <karabo/net/IOService.hh>
#include <karabo/net/Connection.hh>
#include <karabo/net/Channel.hh>
#include <karabo/net/EventLoop.hh>
#include "TcpNetworking_Test.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(TcpNetworking_Test);


struct TcpServer {

    KARABO_CLASSINFO(TcpServer, "TcpServer", "1.0");

    TcpServer() : m_count(0), m_port(0) {
        m_connection = karabo::net::Connection::create(karabo::util::Hash("Tcp.port", 0, "Tcp.type", "server"));
        m_port = m_connection->startAsync(boost::bind(&TcpServer::connectHandler, this, _1, _2));
    }


    virtual ~TcpServer() {
    }


    int port() {
        return m_port;
    }


    void connectHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER_ERROR: " << ec.value() << " -- " << ec.message();
            if (channel) channel->close();
            return;
        }
        channel->readAsyncHashHash(boost::bind(&TcpServer::readHashHashHandler, this, _1, channel, _2, _3));
    }


    void errorHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
        if (ec.value() == 2) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER: client has closed the connection!";
        } else {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER_ERROR: " << ec.value() << " -- " << ec.message();
        }
        if (channel) channel->close();
    }


    void readHashHashHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel,
                             karabo::util::Hash& header, karabo::util::Hash& body) {
        if (ec) {
            errorHandler(ec, channel);
            return;
        }

        m_count++;
        KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER_INFO: count " << m_count << "\n" << header << body << "-----------------\n";

        CPPUNIT_ASSERT(header.get<std::string>("headline") == "*** CLIENT ***");

        header.set("headline", "----- SERVER -----");

        body.set("a.e", "server data");

        if (body.has("a") && body.get<std::string > ("a.b") == "?")
            body.set("a.b", "server reply");
        else
            body.set("a.b", "counter " + karabo::util::toString(m_count));


        channel->writeAsyncHashHash(header, body, boost::bind(&TcpServer::writeCompleteHandler, this, _1, channel, "some string"));
    }


    void writeCompleteHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel, const std::string& id) {
        if (ec) {
            errorHandler(ec, channel);
            return;
        }

        BOOST_ASSERT(id == "some string");
        channel->readAsyncHashHash(boost::bind(&TcpServer::readHashHashHandler, this, _1, channel, _2, _3));
    }

private:
    int m_count;
    int m_port;
    karabo::net::Connection::Pointer m_connection;
};


struct TcpClient {

    KARABO_CLASSINFO(TcpClient, "TcpClient", "1.0");

    TcpClient(const std::string& host, int port)
        : m_count(0)
        , m_port(port)
        , m_connection(karabo::net::Connection::create(karabo::util::Hash("Tcp.port", m_port, "Tcp.hostname", "sample.example.org")))
        , m_deadline(karabo::net::EventLoop::getIOService()) { //                                                           timeout repetition channel ec 
        m_connection->startAsync(boost::bind(&TcpClient::connectHandler, this, _1, 1000, 3, _2));
    }


    virtual ~TcpClient() {
    }


    void connectHandler(const karabo::net::ErrorCode& ec, int timeout, int repetition, const karabo::net::Channel::Pointer& channel) {
        if (ec) {
            errorHandler(ec, channel);
            if (ec != boost::asio::error::eof && repetition >= 0) {

                m_deadline.expires_from_now(boost::posix_time::milliseconds(timeout));
                m_deadline.async_wait(boost::bind(&TcpClient::waitHandler, this, boost::asio::placeholders::error, timeout, repetition));
            }
            return;
        }
        KARABO_LOG_FRAMEWORK_DEBUG << "\nTcpClient connectHandler";
        karabo::util::Hash header("headline", "*** CLIENT ***");
        karabo::util::Hash data("a.b", "?", "a.c", 42.22f, "a.d", 12);

        // first sending
        channel->writeAsyncHashHash(header, data, boost::bind(&TcpClient::writeCompleteHandler, this, channel, 42));
    }


    void errorHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
        if (ec != boost::asio::error::eof)
            KARABO_LOG_FRAMEWORK_DEBUG << "\nCLIENT_ERROR: " << ec.value() << " -- " << ec.message();
        if (channel) channel->close();
    }


    void waitHandler(const karabo::net::ErrorCode& ec, int timeout, int repetition) {
        if (ec == boost::asio::error::operation_aborted) return;
        --repetition;
        timeout *= 2;
        if (repetition == 1)
            m_connection = karabo::net::Connection::create(karabo::util::Hash("Tcp.port", m_port, "Tcp.hostname", "exflserv04"));
        if (repetition == 0)
            m_connection = karabo::net::Connection::create(karabo::util::Hash("Tcp.port", m_port, "Tcp.hostname", "localhost"));
        m_connection->startAsync(boost::bind(&TcpClient::connectHandler, this, _1, timeout, repetition, _2));
    }


    void readHashHashHandler(const karabo::net::ErrorCode& ec,
                             const karabo::net::Channel::Pointer& channel,
                             karabo::util::Hash& header,
                             karabo::util::Hash& body) {
        if (ec) {
            errorHandler(ec, channel);
            return;
        }

        // inspect here the server reply.... just count
        m_count++;

        KARABO_LOG_FRAMEWORK_DEBUG << "TcpClient readHashHashHandler count = " << m_count;

        if (m_count >= 3) { // stop after 3 attempts
            channel->close();
            return;
        }

        CPPUNIT_ASSERT(header.get<std::string>("headline") == "----- SERVER -----");
        if (body.has("a.e")) {
            CPPUNIT_ASSERT(body.get<std::string>("a.e") == "server data");
            body.erase("a.e");
        }

        header.set("headline", "*** CLIENT ***");

        // Prepare new data
        body.set("a.b", "John Doe");
        body.set("a.c", 1.0f * (static_cast<unsigned int> (::rand()) % 1000));
        body.set("a.d", static_cast<int> (::rand()) % 100);
        std::vector<unsigned char> pixels;
        body.set("a.v", pixels);
        std::vector<unsigned char>& x = body.get<std::vector<unsigned char> >("a.v");
        for (int i = 1; i <= 20; i++) x.push_back(static_cast<unsigned char> (i % 256));

        // send client data asynchronous: define "write" completion handler
        channel->writeAsyncHashHash(header, body, boost::bind(&TcpClient::writeCompleteHandler, this, channel, 42));
    }


    void writeCompleteHandler(const karabo::net::Channel::Pointer& channel, int id) {
        CPPUNIT_ASSERT(id == 42);
        // data was sent successfully! Prepare to read a reply asynchronous from a server: placeholder _1 is a Hash
        channel->readAsyncHashHash(boost::bind(&TcpClient::readHashHashHandler, this, _1, channel, _2, _3));
    }


private:
    int m_count;
    int m_port;
    karabo::net::Connection::Pointer m_connection;
    boost::asio::deadline_timer m_deadline;
    int m_timeout;
    int m_repetition;
};


TcpNetworking_Test::TcpNetworking_Test() {
}


TcpNetworking_Test::~TcpNetworking_Test() {
}


void TcpNetworking_Test::setUp() {
}


void TcpNetworking_Test::tearDown() {
}


void TcpNetworking_Test::testClientServer() {
    using namespace std;
    int nThreads = karabo::net::EventLoop::getNumberOfThreads();
    CPPUNIT_ASSERT(nThreads == 0);

    TcpServer server;
    TcpClient client("localhost", server.port());
    
    nThreads = karabo::net::EventLoop::getNumberOfThreads();
    CPPUNIT_ASSERT(nThreads == 0);

    karabo::net::EventLoop::run();

    nThreads = karabo::net::EventLoop::getNumberOfThreads();
    CPPUNIT_ASSERT(nThreads == 0);
}