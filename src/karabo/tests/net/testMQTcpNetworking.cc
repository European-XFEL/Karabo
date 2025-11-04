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
 * File:  testMQTcpNetworking.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on Oct 29, 2025, 5:16:16 PM
 */

#include <gtest/gtest.h>

#include <boost/system/error_code.hpp>

#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/log/Logger.hh"
#include "karabo/net/Channel.hh"
#include "karabo/net/Connection.hh"
#include "karabo/net/EventLoop.hh"


class TestMQTcpNetworking : public ::testing::Test {
   private:
    int m_numberOfMessages;
    karabo::data::Hash m_header;
    karabo::data::Hash m_data;
    // server
    int m_serverCount;
    int m_serverPort;
    std::jthread m_serverThread;
    karabo::net::Connection::Pointer m_serverConnection;
    std::chrono::steady_clock::time_point m_ts;
    // client
    int m_clientCount;
    karabo::net::Connection::Pointer m_connection;
    std::chrono::steady_clock::time_point m_clientTimestamp;

   public:
    KARABO_CLASSINFO(TestMQTcpNetworking, "TestMQTcpNetworking", "1.0");

   protected:
    TestMQTcpNetworking();

    ~TestMQTcpNetworking() override;

    void SetUp() override;

    void TearDown() override;

   private:
    void createServer();

    void serverConnectHandler(const karabo::net::ErrorCode& e, const karabo::net::Channel::Pointer& channel);

    void serverErrorHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);

    void serverReadHashHashHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel,
                                   karabo::data::Hash& header, karabo::data::Hash& body);

    void serverPublish(const karabo::net::Channel::Pointer& channel);

    void serverRun();

    void testClientServerMethod();

    void onClientConnected(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);

    void clientChannelErrorHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);

    void clientReadHashHashHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel,
                                   karabo::data::Hash& header, karabo::data::Hash& body);

    void onClientEnd(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);
};


using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;


TestMQTcpNetworking::TestMQTcpNetworking() : m_numberOfMessages(10000), m_serverCount(0), m_serverPort(0) {
    using namespace std;
    using namespace karabo::data;

    Hash tmp("a.b.c", 1, "a.b.d", vector<int>(5, 1), "a.b.e", vector<Hash>(2, Hash("a", 1)), "a.d",
             std::complex<double>(1.2, 4.2));
    tmp.setAttribute("a", "a1", true);
    tmp.setAttribute("a", "a2", 3.4);
    tmp.setAttribute("a.b", "b1", "3");
    tmp.setAttribute("a.b.c", "c1", 2);
    tmp.setAttribute("a.b.c", "c2", vector<string>(3, "bla"));
    m_data = tmp;

    Hash tmp2("policy", "LOSSLESS");
    m_header = tmp2;
}


TestMQTcpNetworking::~TestMQTcpNetworking() {}


void TestMQTcpNetworking::createServer() {
    m_serverConnection = karabo::net::Connection::create(karabo::data::Hash("Tcp.port", 0, "Tcp.type", "server"));
    KARABO_LOG_FRAMEWORK_DEBUG << "SERVER: connection object created. ";
    m_serverPort = m_serverConnection->startAsync(std::bind(&TestMQTcpNetworking::serverConnectHandler, this, _1, _2));
    EXPECT_GT(m_serverPort, 0);
    KARABO_LOG_FRAMEWORK_DEBUG << "SERVER: the allocated port is " << m_serverPort;
}


void TestMQTcpNetworking::serverConnectHandler(const karabo::net::ErrorCode& ec,
                                               const karabo::net::Channel::Pointer& channel) {
    EXPECT_EQ(ec.value(), boost::system::errc::success) << __FUNCTION__ << "; " << ec.message();
    if (ec) {
        serverErrorHandler(ec, channel);
        return;
    }
    KARABO_LOG_FRAMEWORK_DEBUG << "SERVER: connected";
    EXPECT_TRUE(channel->isOpen());
    // _1 -> header, _2 -> body, _3 -> error code
    channel->readAsyncHashHash(std::bind(&TestMQTcpNetworking::serverReadHashHashHandler, this, _1, channel, _2, _3));
}


void TestMQTcpNetworking::serverErrorHandler(const karabo::net::ErrorCode& ec,
                                             const karabo::net::Channel::Pointer& channel) {
    if (ec.value() == 2) {
        KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER: client has closed the connection!";
    } else {
        KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER_ERROR: " << ec.value() << " -- " << ec.message();
    }
    if (channel) channel->close();
    ASSERT_FALSE(channel->isOpen());
}


void TestMQTcpNetworking::serverReadHashHashHandler(const karabo::net::ErrorCode& ec,
                                                    const karabo::net::Channel::Pointer& channel,
                                                    karabo::data::Hash& header, karabo::data::Hash& body) {
    EXPECT_EQ(ec.value(), boost::system::errc::success) << __FUNCTION__ << "; " << ec.message();
    if (ec) {
        serverErrorHandler(ec, channel);
        return;
    }

    KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER : Request comes...\n" << header << body << "-----------------\n";

    channel->readAsyncHashHash(std::bind(&TestMQTcpNetworking::serverReadHashHashHandler, this, _1, channel, _2, _3));

    if (body.has("START")) {
        m_numberOfMessages = body.get<int>("START");
        KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER:  CLIENT sent START command with counter = " << m_numberOfMessages;
        m_serverCount = 0;
        m_ts = std::chrono::steady_clock::now();
        boost::asio::post(karabo::net::EventLoop::getIOService(),
                          std::bind(&TestMQTcpNetworking::serverPublish, this, channel));
    } else if (body.has("STOP")) {
        KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER:  CLIENT requests exiting together!\n";
    }
}


void TestMQTcpNetworking::serverPublish(const karabo::net::Channel::Pointer& channel) {
    channel->writeAsync(m_header, m_data);
    m_serverCount++;
    if (m_serverCount < m_numberOfMessages)
        boost::asio::post(karabo::net::EventLoop::getIOService(),
                          std::bind(&TestMQTcpNetworking::serverPublish, this, channel));
    else {
        auto t = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(t - m_ts);
        KARABO_LOG_FRAMEWORK_DEBUG << "SERVER : " << diff.count() << " ms";
        KARABO_LOG_FRAMEWORK_DEBUG << "\tpublishing rate = " << double(m_serverCount) / diff.count() << " per ms";
    }
}


void TestMQTcpNetworking::testClientServerMethod() {
    m_connection =
          karabo::net::Connection::create(karabo::data::Hash("Tcp.port", m_serverPort, "Tcp.hostname", "localhost"));
    ASSERT_NE(m_connection, nullptr);
    m_connection->startAsync(std::bind(&TestMQTcpNetworking::onClientConnected, this, _1, _2));
}


void TestMQTcpNetworking::SetUp() {
    KARABO_LOG_FRAMEWORK_DEBUG << "==============================================";
    KARABO_LOG_FRAMEWORK_DEBUG << "MQTestClientServer START nThreads = "
                               << karabo::net::EventLoop::getNumberOfThreads();
    try {
        createServer();
    } catch (const std::exception& e) {
        KARABO_LOG_FRAMEWORK_DEBUG << "SETUP exception: " << e.what();
        return;
    }
}


void TestMQTcpNetworking::TearDown() {
    karabo::net::EventLoop::run();
    KARABO_LOG_FRAMEWORK_DEBUG << "EventLoop::run() was left.";
    KARABO_LOG_FRAMEWORK_DEBUG << "=== MQTestClientServer STOPPED nThreads = "
                               << karabo::net::EventLoop::getNumberOfThreads();
    KARABO_LOG_FRAMEWORK_DEBUG << "==============================================";
}


void TestMQTcpNetworking::onClientConnected(const karabo::net::ErrorCode& e,
                                            const karabo::net::Channel::Pointer& channel) {
    EXPECT_EQ(e.value(), boost::system::errc::success) << __FUNCTION__ << "; " << e.message();
    if (e) {
        KARABO_LOG_FRAMEWORK_DEBUG << "TestMQTcpNetworking::onClientConnected  ErrorCode = " << e;
        clientChannelErrorHandler(e, channel);
        return;
    }

    karabo::data::Hash header("headline", "*** CLIENT ***");
    karabo::data::Hash data("START", 50000);

    // first sending
    channel->writeAsync(header, data);
    m_clientCount = 0;
    m_clientTimestamp = std::chrono::steady_clock::now();
    channel->readAsyncHashHash(std::bind(&TestMQTcpNetworking::clientReadHashHashHandler, this, _1, channel, _2, _3));
}


void TestMQTcpNetworking::clientChannelErrorHandler(const karabo::net::ErrorCode& ec,
                                                    const karabo::net::Channel::Pointer& channel) {
    EXPECT_NE(ec.value(), boost::system::errc::success) << __FUNCTION__ << "; " << ec.message();
    if (channel) channel->close();
    if (ec != boost::asio::error::eof) {
        KARABO_LOG_FRAMEWORK_DEBUG << "\nCLIENT ERROR: " << ec.value() << " -- " << ec.message();

        if (m_serverConnection) {
            m_serverConnection->stop();
            m_serverConnection.reset();
        }
    }
}


void TestMQTcpNetworking::clientReadHashHashHandler(const karabo::net::ErrorCode& e,
                                                    const karabo::net::Channel::Pointer& channel,
                                                    karabo::data::Hash& header, karabo::data::Hash& body) {
    EXPECT_EQ(e.value(), boost::system::errc::success) << __FUNCTION__ << "; " << e.message();
    if (e) {
        clientChannelErrorHandler(e, channel);
        return;
    }

    // inspect here the server reply.... just count
    m_clientCount++;
    if (m_clientCount < m_numberOfMessages) {
        channel->readAsyncHashHash(
              std::bind(&TestMQTcpNetworking::clientReadHashHashHandler, this, _1, channel, _2, _3));
    } else {
        karabo::data::Hash header("headline", "*** CLIENT ***");
        karabo::data::Hash data("STOP", karabo::data::Hash());
        channel->writeAsyncHashHash(
              header, data,
              std::bind(&TestMQTcpNetworking::onClientEnd, this, boost::asio::placeholders::error, channel));
    }
}


void TestMQTcpNetworking::onClientEnd(const karabo::net::ErrorCode& e, const karabo::net::Channel::Pointer& channel) {
    if (e) {
        if (e.value() == 2) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nCLIENT: server has closed the connection!";
        } else {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nCLIENT ERROR: " << e.value() << " -- " << e.message();
        }
        if (channel) channel->close();
        return;
    }

    auto t = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(t - m_clientTimestamp);
    double rate = double(m_clientCount) / diff.count();
    KARABO_LOG_FRAMEWORK_DEBUG << "CLIENT Summary : " << diff.count() << " ms, rate = " << rate << " 1/ms";
    channel->close();
    ASSERT_FALSE(channel->isOpen());
}
