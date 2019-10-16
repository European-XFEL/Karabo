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

struct TestClient {

    KARABO_CLASSINFO(TestClient, "TestClient", "1.0");


    TestClient(const karabo::util::Hash& input)
            : m_repetition(5)
            , m_expected("") {
        m_connection = karabo::net::Connection::create("Tcp", input);
        m_connection->startAsync(boost::bind(&TestClient::connectHandler, this, _1, _2));
    }


    virtual ~TestClient() {
    }


    void connectHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
        if (ec) {
            CPPUNIT_FAIL("Error connecting");
            return;
        }

        m_expected = "When the going gets tough...";

        const std::string& tmp = "When the going gets tough... the tough get going\r\n";
        auto datap = boost::make_shared<std::vector<char>>(tmp.begin(), tmp.end());
        channel->writeAsyncVectorPointer(datap, boost::bind(&TestClient::writeCompleteHandler, this, _1, channel, "..."));
    }


    void writeCompleteHandler(const karabo::net::ErrorCode& ec,
                              const karabo::net::Channel::Pointer& channel,
                              const std::string& terminator) {
        if (ec) {
            CPPUNIT_FAIL("error on write");
            channel->close();
        }
        auto readHandler = [this, channel](const karabo::net::ErrorCode& ec, const std::string& data) {
            if (ec) {
                CPPUNIT_FAIL("Error reading");
                channel->close();
                return;
            }

            //std::cerr << data;
            CPPUNIT_ASSERT(data == m_expected || data == " the tough get going\r\n");
            if (--m_repetition == 0) {
                channel->close();
                return;
            }

            m_expected = "Yet another test string\r\n";
            auto datap = boost::make_shared<std::vector<char>>(m_expected.begin(), m_expected.end());
            channel->writeAsyncVectorPointer(datap,
                                             boost::bind(&TestClient::writeCompleteHandler,
                                                         this, _1, channel, "\r\n"));
        };
        channel->readAsyncStringUntil(terminator, readHandler);
    }

private:
    karabo::net::Connection::Pointer m_connection;
    int m_timeout;
    int m_repetition;
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
    // Start echo server thread ...
    int port = 54345;
    auto fut = std::async(std::launch::async, [port]() {
        boost::asio::io_context io_context;
        tcp::acceptor a(io_context, tcp::endpoint(tcp::v4(), port));
        // Block on 'accept' function until client is knocked...
        auto sock = a.accept();
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
    });
    // Start client ...
    karabo::util::Hash input("hostname", "localhost", "port", port, "type", "client", "sizeofLength", 0);
    TestClient client(input);
    karabo::net::EventLoop::run();
    // Join server thread
    fut.wait();
}
