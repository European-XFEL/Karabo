/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   ReadAsyncStringUntil_Test.cc
 * Author: giovanet
 *
 * Created on May 16, 2018, 1:46 PM
 */

#include "ReadAsyncStringUntil_Test.hh"

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <cassert>
#include <fstream>
#include <iosfwd>
#include <iostream>
#include <karabo/karabo.hpp>

#include "karabo/net/Channel.hh"
#include "karabo/net/Connection.hh"
#include "karabo/net/EventLoop.hh"


using boost::asio::ip::tcp;
using namespace std;

using std::placeholders::_1;
using std::placeholders::_2;


CPPUNIT_TEST_SUITE_REGISTRATION(ReadAsyncStringUntil_Test);


#define MAX_DATA_LEN 1024


// We are going to test read-async-until function that Web protocols based on.
// Create simple synchronous echo server that will return us what we send.
// It runs on separate thread.

struct TestClient {
    KARABO_CLASSINFO(TestClient, "TestClient", "1.0");


    TestClient(const karabo::data::Hash& input)
        : m_repetition(5) // we want to read 5 times
          ,
          m_expected(""),
          m_promise(std::promise<std::string>()) {
        m_connection = karabo::net::Connection::create("Tcp", input);
        m_connection->startAsync(std::bind(&TestClient::connectHandler, this, _1, _2));
    }


    virtual ~TestClient() {}


    std::future<std::string> get_future() {
        return m_promise.get_future();
    }


    void connectHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
        if (ec) {
            m_promise.set_value("Error connecting");
            return;
        }

        m_expected = "When the going gets tough...";

        // The first statement will be read in two steps by changing terminator...
        const std::string& tmp = "When the going gets tough... the tough get going\r\n";
        auto datap = std::make_shared<std::vector<char>>(tmp.begin(), tmp.end());
        // NOTE: First we use "..." as a terminator!
        channel->writeAsyncVectorPointer(datap, std::bind(&TestClient::writeCompleteHandler, this, _1, channel, "..."));
    }


    void writeCompleteHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel,
                              const std::string& terminator) {
        if (ec) {
            channel->close();
            m_promise.set_value("Error on write");
            return;
        }
        auto readHandler = [this, channel](const karabo::net::ErrorCode& ec, const std::string& data) {
            if (ec) {
                channel->close();
                m_promise.set_value("Error reading");
                return;
            }

            // The first data message will be "When the going gets tough..."
            // The second one:                " the tough get going\r\n"
            // The third one and others:      "Yet another test string\r\n"
            // Uncomment print statement below to see how does it work
            // std::cerr << data;
            if (data != m_expected && data != " the tough get going\r\n") {
                channel->close();
                m_promise.set_value("Error on data comparison");
                return;
            }
            if (--m_repetition == 0) {
                channel->close();
                m_promise.set_value("OK");
                return;
            }

            m_expected = "Yet another test string\r\n";
            auto datap = std::make_shared<std::vector<char>>(m_expected.begin(), m_expected.end());
            // NOTE: After first reading we change terminator to "\r\n" (CRLF) to read
            //  a second part of first statement and all others ...
            channel->writeAsyncVectorPointer(datap,
                                             std::bind(&TestClient::writeCompleteHandler, this, _1, channel, "\r\n"));
        };
        channel->readAsyncStringUntil(terminator, readHandler);
    }

   private:
    karabo::net::Connection::Pointer m_connection;
    int m_repetition;
    std::string m_expected;
    std::promise<std::string> m_promise;
};


ReadAsyncStringUntil_Test::ReadAsyncStringUntil_Test() {}


ReadAsyncStringUntil_Test::~ReadAsyncStringUntil_Test() {}


void ReadAsyncStringUntil_Test::setUp() {}


void ReadAsyncStringUntil_Test::tearDown() {}


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
                if (ec == boost::asio::error::eof) break;      // The other side closed connection
                if (ec) throw boost::system::system_error(ec); // some other error.
                // no errors : synchronously write back
                boost::asio::write(sock, boost::asio::buffer(data, length));
            }
        } catch (std::exception& e) {
            std::cerr << "Exception in thread: " << e.what() << "\n";
        }
    });
    // Start client ...
    karabo::data::Hash input("hostname", "localhost", "port", port, "type", "client", "sizeofLength", 0);
    TestClient client(input);
    std::future<std::string> clientFut = client.get_future();
    karabo::net::EventLoop::run();
    // Join server thread
    fut.wait();
    std::string msg = clientFut.get();
    // Check result of testing
    CPPUNIT_ASSERT_MESSAGE(msg, "OK" == msg);
}
