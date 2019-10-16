/* 
 * File:   ReadAsyncStringUntil_Test.cc
 * Author: giovanet
 * 
 * Created on May 16, 2018, 1:46 PM
 */

#include "ReadAsyncStringUntil_Test.hh"

#include "karabo/net/Connection.hh"
#include "karabo/net/Channel.hh"
#include "karabo/net/EventLoop.hh"

#include <karabo/karabo.hpp>

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>

#include <iostream>
#include <fstream>
#include <cassert>
#include <iosfwd>


using boost::asio::ip::tcp;
using namespace std;


CPPUNIT_TEST_SUITE_REGISTRATION(ReadAsyncStringUntil_Test);


#define MAX_DATA_LEN 1024


// We are going to test read-async-until function that Web protocols based on.
// Create simple synchronous echo server that will return us what we send.
// It runs on separate thread.

struct EchoServer {


    EchoServer(unsigned short port) : m_port(port) {
    }


    /**
     * write back the data we read
     * @param sock connected to the client
     */
    void loopback(tcp::socket sock) {
        try {
            while (1) {
                char data[MAX_DATA_LEN];
                boost::system::error_code ec;

                size_t length = sock.read_some(boost::asio::buffer(data), ec);
                if (ec == boost::asio::error::eof) break; // The other side closed connection
                if (ec) throw boost::system::system_error(ec); // some other error.
                // no errors : synchronously write back
                boost::asio::write(sock, boost::asio::buffer(data, length));
            }
        } catch (std::exception& e) {
            std::cerr << "Exception in thread: " << e.what() << "\n";
        }
    }


    /**
     * Wait for incoming client connection
     */
    void listen() {
        boost::asio::io_context io_context;
        tcp::acceptor a(io_context, tcp::endpoint(tcp::v4(), m_port));
        // Block on 'accept' function until client is knocked...
        loopback(a.accept());   // one client can be accepted!
    }


    /**
     * Start thread where we are awaiting client connection
     */
    void start() {
        m_listener = new std::thread(&EchoServer::listen, this);
    }


    void join() {
        m_listener->join();
    }

private:
    unsigned short m_port;
    std::thread* m_listener;
};


struct TestClient {

    KARABO_CLASSINFO(TestClient, "TestClient", "1.0");


    TestClient(const karabo::util::Hash& input)
        : m_repetition(5)
        , m_terminator("\r\n")
        , m_expected("") {
        m_connection = karabo::net::Connection::create("Tcp", input);
        //                                                                     ec, channel
        m_connection->startAsync(boost::bind(&TestClient::connectHandler, this, _1, _2));
    }


    virtual ~TestClient() {
    }


    void connectHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
        if (ec) {
            CPPUNIT_FAIL("Error connecting");
            return;
        }

        const std::string& tmp = "When the going gets tough... the tough get going\r\n";
        m_terminator = "...";
        m_expected = "When the going gets tough...";

        boost::shared_ptr<vector <char>> datap(new vector <char>());
        datap->assign(tmp.begin(), tmp.end());
        channel->writeAsyncVectorPointer(datap, boost::bind(&TestClient::writeCompleteHandler, this, _1, channel));
    }


    void writeCompleteHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
        if (ec) {
            CPPUNIT_FAIL("error on write");
            channel->close();
        }

        channel->readAsyncStringUntil(m_terminator, boost::bind(&TestClient::readStringHandler, this, _1, channel, _2));
    }


    void readStringHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel, const std::string& read_str) {
        if (ec) {
            CPPUNIT_FAIL("Error reading");
            channel->close();
            return;
        }

        //std::cerr << read_str;

        CPPUNIT_ASSERT(read_str == m_expected || read_str == " the tough get going\r\n");
        
        --m_repetition;

        if (m_repetition == 0) {
            channel->close();
            return;
        }

        m_terminator = "\r\n";
        m_expected = "Yet another test string\r\n";
        boost::shared_ptr<vector <char>> datap(new vector <char>());
        datap->assign(m_expected.begin(), m_expected.end());
        channel->writeAsyncVectorPointer(datap, boost::bind(&TestClient::writeCompleteHandler, this, _1, channel));
    }

private:
    karabo::net::Connection::Pointer m_connection;
    int m_timeout;
    int m_repetition;
    std::string m_terminator;
    std::string m_expected;
};


ReadAsyncStringUntil_Test::ReadAsyncStringUntil_Test() {
}


ReadAsyncStringUntil_Test::~ReadAsyncStringUntil_Test() {
}


void ReadAsyncStringUntil_Test::setUp() {
}


void ReadAsyncStringUntil_Test::tearDown() {
}


void ReadAsyncStringUntil_Test::runTest() {
    using namespace std;

    // Start echo server thread ...
    EchoServer srv(54345);
    srv.start();

    // Start client ...
    karabo::util::Hash connectionCfg;
    connectionCfg.set("hostname", "localhost");
    connectionCfg.set("port", 54345);
    connectionCfg.set("type", "client");
    connectionCfg.set("sizeofLength", 0); //default is 4

    TestClient client(connectionCfg);

    // Start event loop
    karabo::net::EventLoop::run();
    // Join server thread
    srv.join();
}
