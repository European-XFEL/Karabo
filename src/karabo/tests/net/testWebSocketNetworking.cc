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

#include <gtest/gtest.h>

#include <array>
#include <boost/bind/bind.hpp>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iosfwd>
#include <iostream>
#include <thread>

#include "karabo/data/io/BinarySerializer.hh"
#include "karabo/data/types/Exception.hh"
#include "karabo/log/Logger.hh"
#include "karabo/net/Channel.hh"
#include "karabo/net/Connection.hh"
#include "karabo/net/EventLoop.hh"


using namespace std::chrono;
using namespace std::literals::chrono_literals; // 10ms == 10 milliseconds
using namespace std::literals::string_literals; // For '"blabla"s'

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

using karabo::data::Hash;
using karabo::data::toString;
using karabo::net::Channel;
using karabo::net::Connection;
using karabo::net::EventLoop;


//<editor-fold desc="Server, client and parameters for testWriteAsync">

const karabo::data::Hash dataHash = karabo::data::Hash("Name", "DataHash", "PiField", 314159);
const int writePriority = 4;

bool equalsTestDataHash(const karabo::data::Hash& other) {
    return other.fullyEquals(dataHash);
}


enum class TestOutcome { UNKNOWN, SUCCESS, FAILURE };

/**
 * The server part for the WriteAsync tests. Reads the data sent by the predefined sequence of writeAsyncs
 * issued by the client part. After the last data in the sequence is read the server flags that it is done reading
 * to the client, and the client closes the connection.
 */
struct WsServer {
    KARABO_CLASSINFO(WsServer, "WsServer", "1.0");


    WsServer() : m_port(8123) {
        m_connection = Connection::create("WebSocket", Hash("type", "server", "port", m_port));
        m_port = m_connection->startAsync(std::bind(&WsServer::connectHandler, this, _1, _2));
    }

    int port() const {
        return m_port;
    }

   private:
    int m_port;
    karabo::net::Connection::Pointer m_connection;


    void connectHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
        std::clog << "[Srv]\t 0.1. Connect handler called." << std::endl;
        ASSERT_FALSE(ec) << "WsServer error: " << ec.value() << " -- " << ec.message();
        channel->readAsyncHash(std::bind(&WsServer::readAsyncHashHandlerCopyFalse, this, _1, channel, _2));
        std::clog << "[Srv]\t 0.2. First read handler registered." << std::endl;
    }


    void readAsyncHashHandlerCopyFalse(const boost::system::error_code& ec,
                                       const karabo::net::Channel::Pointer& channel, const karabo::data::Hash& hash) {
        ASSERT_FALSE(ec) << "WsServer error at readAysncHashHandlerCopyFalse: " << ec.value() << " -- " << ec.message();
        std::clog << "[Srv]\t 1.1. Read hash sent in body with copyAllData false." << std::endl;
        ASSERT_TRUE(equalsTestDataHash(hash)) << "[Srv]\t Not equal.";
        std::clog << "[Srv]\t 1.2. Hash checked to be OK." << std::endl;
        // Reads the next piece of data sent by WsClient as part of the test.
        channel->readAsyncHash(std::bind(&WsServer::readAsyncHashHandlerCopyTrue, this, _1, channel, _2));
    }


    void readAsyncHashHandlerCopyTrue(const boost::system::error_code& ec, const karabo::net::Channel::Pointer& channel,
                                      const karabo::data::Hash& hash) {
        ASSERT_FALSE(ec) << "WsServer error at readAysncHashHandlerCopyTrue: " << ec.value() << " -- " << ec.message();
        std::clog << "[Srv]\t 2.1. Read hash sent in body with copyAllData true." << std::endl;
        ASSERT_TRUE(equalsTestDataHash(hash)) << "[Srv]\t Not equal.";
        std::clog << "[Srv]\t 2.2. Hash checked to be OK." << std::endl;
        channel->readAsyncHash(std::bind(&WsServer::readAsyncHashHandlerCopyFalseAgain, this, _1, channel, _2));
    }


    void readAsyncHashHandlerCopyFalseAgain(const boost::system::error_code& ec,
                                            const karabo::net::Channel::Pointer& channel,
                                            const karabo::data::Hash& hash) {
        ASSERT_FALSE(ec) << "WsServer error at readAysncHashHandlerCopyFalse : " << ec.value() << " --" << ec.message();
        std::clog << "[Srv]\t 3.1. Read hash sent in body with copyAllData false." << std::endl;
        ASSERT_TRUE(equalsTestDataHash(hash));
        std::clog << "[Srv]\t 3.2. Hash checked to be OK." << std::endl;
        // Reads the next piece of data sent by WsClient as part of the test.
        // m_testReportFn(TestOutcome::SUCCESS, "Tests succeeded!", "");
        if (channel) {
            boost::asio::post(EventLoop::getIOService(), std::bind(&WsServer::stopTests, this, channel));
        }
        std::clog << "[Srv] ... server read all data in the sequence." << std::endl;
    }


    void stopTests(const karabo::net::Channel::Pointer& channel) {
        if (channel) {
            channel->close();                 // Here WebSocket close is synchroous, sends 'close' frame
            channel->getConnection()->stop(); // it stops the whole server
        }
        EventLoop::stop();
    }
};


struct WsClient {
    KARABO_CLASSINFO(WsClient, "WsClient", "1.0");

    WsClient(const std::string& h, int p) : m_port(p) {
        std::string host = h.empty() ? "localhost" : h;
        m_connection = Connection::create("WebSocket", Hash("port", p, "hostname", host, "type", "client"));
        m_connection->startAsync(std::bind(&WsClient::connectHandler, this, _1, _2));
    }

   private:
    int m_port;
    karabo::net::Connection::Pointer m_connection;
    karabo::net::Channel::Pointer m_channel;

    void connectHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
        ASSERT_FALSE(ec) << "WsClient error: " << ec.value() << " -- " << ec.message();

        std::clog << "[Cli] Write async client connected. Sending data ..." << std::endl;
        try {
            channel->writeAsync(dataHash, writePriority, false);
            std::clog << "[Cli]\t1. sent hash as body with copyAllData false." << std::endl;
            channel->writeAsync(dataHash, writePriority, true);
            std::clog << "[Cli]\t2. sent hash as body with copyAllData true." << std::endl;
            channel->writeAsync(dataHash, writePriority, false);
            std::clog << "[Cli]\t3. sent hash as body with copyAllData false again." << std::endl;

            std::clog << "[Cli] ... all test data sent by the client" << std::endl;
        } catch (karabo::data::Exception& ke) {
            std::clog << "Error during write sequence by the client: " << ke.what() << std::endl;
            std::clog << "Details: " << std::endl;
            ke.showTrace(std::clog);
            if (channel) channel->close();
        } catch (std::exception& e) {
            std::clog << "Error during write sequence by the client: " << e.what() << std::endl;
            if (channel) channel->close();
        }

        // Keep channel alive to avoid that [Srv] gets informed about disconnection before all data has been read
        // (but do not block the thread by sleeping).
        m_channel = channel;
    }
};


//</editor-fold>


TEST(TestWebSocketNetworking, testWriteAsync) {
    using namespace std;

    // Data items for the test results - to be called by either the server or the client to report test
    // results.
    decltype(high_resolution_clock::now()) startTime;
    decltype(high_resolution_clock::now()) finishTime;

    // Mutex for test results access
    std::mutex testResultsMutex;

    startTime = high_resolution_clock::now();

    WsServer server;
    WsClient client("localhost", server.port() /*, testReportFn*/);

    karabo::net::EventLoop::run();

    finishTime = high_resolution_clock::now();

    std::lock_guard<std::mutex> lock(testResultsMutex);
    auto testDuration = finishTime - startTime;
    std::clog << "Test took " << duration_cast<milliseconds>(testDuration).count() << " milliseconds." << std::endl;
}


TEST(TestWebSocketNetworking, testAcceptAfterPeerDisconnectDuringHandshake) {
    namespace asio = boost::asio;
    using asio::ip::tcp;

    auto server = Connection::create("WebSocket", Hash("type", "server"));
    Channel::Pointer serverChannel;
    Channel::Pointer clientChannel;
    auto timeout = std::make_shared<asio::steady_timer>(EventLoop::getIOService(), 5s);
    timeout->async_wait([](const boost::system::error_code& ec) {
        if (!ec) EventLoop::stop();
    });
    const auto finishWhenConnected = [&serverChannel, &clientChannel, &timeout]() {
        if (serverChannel && clientChannel) {
            timeout->cancel();
            EventLoop::stop();
        }
    };
    // Construct the onConnect handler of the server. Once a client is connected, the server must be enabled to take
    // more connections. A recursive function would suit here. Since I failed to do that with a lambda expression
    // (maybe since C++20 has still some restriction, did not check deeply), we prepare the server only for a second
    // client connecting as fits to this test.
    bool onConnectFailed = false;
    auto onConnect = [&server, &serverChannel, &finishWhenConnected, &onConnectFailed](
                           const karabo::net::ErrorCode& ec, const Channel::Pointer& channel) {
        if (!ec) {
            EXPECT_TRUE(false) << "First connection attempt is expected to fail (no handshake)";
        } else {
            onConnectFailed = true;
            auto onConnectInner = [&](const karabo::net::ErrorCode& ecIn, const Channel::Pointer& channel) {
                if (!ecIn) {
                    serverChannel = channel;
                    finishWhenConnected();
                }
            };
            server->startAsync(std::bind(onConnectInner, _1, _2));
        }
    };
    const int port = server->startAsync(std::bind(onConnect, _1, _2));

    // Queue a TCP peer which disconnects without completing the WebSocket handshake.
    asio::io_context peerContext;
    tcp::socket disconnectingPeer(peerContext);
    disconnectingPeer.connect(tcp::endpoint(asio::ip::address_v4::loopback(), port));
    disconnectingPeer.close();

    auto client = Connection::create("WebSocket", Hash("type", "client", "hostname", "localhost", "port", port));
    client->startAsync(
          [&clientChannel, &finishWhenConnected](const karabo::net::ErrorCode& ec, const Channel::Pointer& channel) {
              if (!ec) {
                  clientChannel = channel;
                  finishWhenConnected();
              }
          });

    EventLoop::run();

    EXPECT_TRUE(onConnectFailed);
    EXPECT_TRUE(serverChannel) << "Server stopped accepting after a peer disconnected during the handshake";
    EXPECT_TRUE(clientChannel) << "Client could not establish a WebSocket after the failed handshake";

    client->stop();
    server->stop();
}
