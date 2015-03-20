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

#include "TcpNetworking_Test.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(TcpNetworking_Test);


struct TcpServer {


    TcpServer() : m_count(0), m_port(0) {
        m_connection = karabo::net::Connection::create(karabo::util::Hash("Tcp.port", 0, "Tcp.type", "server"));
        m_port = m_connection->startAsync(boost::bind(&TcpServer::connectHandler, this, _1));
    }


    virtual ~TcpServer() {
    }


    int port() {
        return m_port;
    }


    void connectHandler(const karabo::net::Channel::Pointer& channel) {
        channel->setErrorHandler(boost::bind(&TcpServer::errorHandler, this, channel, _1));
        channel->readAsyncHashHash(boost::bind(&TcpServer::readHashHashHandler, this, channel, _1, _2));
    }


    void run() {
        karabo::net::IOService::Pointer io = m_connection->getIOService();
        io->run();
    }


    void errorHandler(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode& ec) {
        if (ec.value() == 2) {
            std::clog << "\nSERVER: client has closed the connection!" << std::endl;
        } else {
            std::clog << "\nSERVER_ERROR: " << ec.value() << " -- " << ec.message() << std::endl;
        }
        channel->close();
    }


    void readHashHashHandler(const karabo::net::Channel::Pointer& channel, karabo::util::Hash& header, karabo::util::Hash& body) {
        m_count++;
        //std::clog << "\nSERVER_INFO: count " << m_count << "\n" << header << body << "-----------------\n";

        CPPUNIT_ASSERT(header.get<std::string>("headline") == "*** CLIENT ***");

        header.set("headline", "----- SERVER -----");

        body.set("a.e", "server data");

        if (body.has("a") && body.get<std::string > ("a.b") == "?")
            body.set("a.b", "server reply");
        else
            body.set("a.b", "counter " + karabo::util::toString(m_count));


        channel->writeAsyncHashHash(header, body, boost::bind(&TcpServer::writeCompleteHandler, this, channel, "some string"));
    }


    void writeCompleteHandler(const karabo::net::Channel::Pointer& channel, const std::string& id) {
        BOOST_ASSERT(id == "some string");
        channel->readAsyncHashHash(boost::bind(&TcpServer::readHashHashHandler, this, channel, _1, _2));
    }

private:
    int m_count;
    int m_port;
    karabo::net::Connection::Pointer m_connection;
};


struct TcpClient {

    TcpClient(const std::string& host, int port) : m_count(0), m_port(port) {
    }


    virtual ~TcpClient() {
    }


    void run() {
        m_connection = karabo::net::Connection::create(karabo::util::Hash("Tcp.port", m_port, "Tcp.hostname", "localhost"));
        m_connection->setErrorHandler(boost::bind(&TcpClient::connectionErrorHandler, this, m_connection, _1));
        m_connection->startAsync(boost::bind(&TcpClient::connectHandler, this, _1));
        karabo::net::IOService::Pointer io = m_connection->getIOService();
        io->run();
    }


    void connectHandler(const karabo::net::Channel::Pointer& channel) {
        channel->setErrorHandler(boost::bind(&TcpClient::errorHandler, this, channel, _1));
        karabo::util::Hash header("headline", "*** CLIENT ***");
        karabo::util::Hash data("a.b", "?", "a.c", 42.22f, "a.d", 12);

        // first sending
        channel->writeAsyncHashHash(header, data, boost::bind(&TcpClient::writeCompleteHandler, this, channel, 42));
    }


    void connectionErrorHandler(const karabo::net::Connection::Pointer& connection, const karabo::net::ErrorCode& ec) {
        std::clog << "\nCLIENT_ERROR: Failed to connect to remote server. Stop...\n";
        boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
        connection->stop();
    }


    void errorHandler(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode& ec) {
        // Check if it is End-Of-File
        if (ec.value() == 2) {
            //std::clog << "\nCLIENT: server has closed the connection!" << std::endl;
        } else {
            std::clog << "\nCLIENT_ERROR: " << ec.value() << " -- " << ec.message() << std::endl;
        }
        channel->close();
    }


    void readHashHashHandler(const karabo::net::Channel::Pointer& channel, karabo::util::Hash& header, karabo::util::Hash& body) {
        // inspect here the server reply.... just count
        m_count++;
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
        channel->readAsyncHashHash(boost::bind(&TcpClient::readHashHashHandler, this, channel, _1, _2));
    }


private:
    int m_count;
    int m_port;
    karabo::net::Connection::Pointer m_connection;
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
    
    TcpServer server;
    boost::thread server_thread(boost::bind(&TcpServer::run, &server));
    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));

    TcpClient client("localhost", server.port());
    client.run();

    server_thread.join();
}