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
 * File:   MQTcpNetworking.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on Sep 22, 2015, 3:06:16 PM
 */
#include "MQTcpNetworking.hh"

#include <karabo/net/Channel.hh>
#include <karabo/net/EventLoop.hh>

#include "boost/system/error_code.hpp"
#include "karabo/data/types/Hash.hh"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;


CPPUNIT_TEST_SUITE_REGISTRATION(MQTcpNetworking);


MQTcpNetworking::MQTcpNetworking() : m_numberOfMessages(10000), m_serverCount(0), m_serverPort(0) {
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


MQTcpNetworking::~MQTcpNetworking() {}


void MQTcpNetworking::createServer() {
    m_serverConnection = karabo::net::Connection::create(karabo::data::Hash("Tcp.port", 0, "Tcp.type", "server"));
    KARABO_LOG_FRAMEWORK_DEBUG << "SERVER: connection object created. ";
    m_serverPort = m_serverConnection->startAsync(std::bind(&MQTcpNetworking::serverConnectHandler, this, _1, _2));
    KARABO_LOG_FRAMEWORK_DEBUG << "SERVER: the allocated port is " << m_serverPort;
}


void MQTcpNetworking::serverConnectHandler(const karabo::net::ErrorCode& ec,
                                           const karabo::net::Channel::Pointer& channel) {
    if (ec) {
        serverErrorHandler(ec, channel);
        return;
    }
    KARABO_LOG_FRAMEWORK_DEBUG << "SERVER: connected";
    // _1 -> header, _2 -> body, _3 -> error code
    channel->readAsyncHashHash(std::bind(&MQTcpNetworking::serverReadHashHashHandler, this, _1, channel, _2, _3));
}


void MQTcpNetworking::serverErrorHandler(const karabo::net::ErrorCode& ec,
                                         const karabo::net::Channel::Pointer& channel) {
    if (ec.value() == 2) {
        KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER: client has closed the connection!";
    } else {
        KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER_ERROR: " << ec.value() << " -- " << ec.message();
    }
    if (channel) channel->close();
}


void MQTcpNetworking::serverReadHashHashHandler(const karabo::net::ErrorCode& ec,
                                                const karabo::net::Channel::Pointer& channel,
                                                karabo::data::Hash& header, karabo::data::Hash& body) {
    if (ec) {
        serverErrorHandler(ec, channel);
        return;
    }

    KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER : Request comes...\n" << header << body << "-----------------\n";

    channel->readAsyncHashHash(std::bind(&MQTcpNetworking::serverReadHashHashHandler, this, _1, channel, _2, _3));

    if (body.has("START")) {
        m_numberOfMessages = body.get<int>("START");
        KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER:  CLIENT sent START command with counter = " << m_numberOfMessages;
        m_serverCount = 0;
        m_ts = std::chrono::steady_clock::now();
        karabo::net::EventLoop::getIOService().post(std::bind(&MQTcpNetworking::serverPublish, this, channel));
    } else if (body.has("STOP")) {
        KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER:  CLIENT requests exiting together!\n";
    }
}


void MQTcpNetworking::serverPublish(const karabo::net::Channel::Pointer& channel) {
    channel->writeAsync(m_header, m_data);
    m_serverCount++;
    if (m_serverCount < m_numberOfMessages)
        karabo::net::EventLoop::getIOService().post(std::bind(&MQTcpNetworking::serverPublish, this, channel));
    else {
        auto t = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(t - m_ts);
        KARABO_LOG_FRAMEWORK_DEBUG << "SERVER : " << diff.count() << " ms";
        KARABO_LOG_FRAMEWORK_DEBUG << "\tpublishing rate = " << double(m_serverCount) / diff.count() << " per ms";
    }
}


void MQTcpNetworking::testClientServerMethod() {
    m_connection =
          karabo::net::Connection::create(karabo::data::Hash("Tcp.port", m_serverPort, "Tcp.hostname", "localhost"));
    m_connection->startAsync(std::bind(&MQTcpNetworking::onClientConnected, this, _1, _2));
}


void MQTcpNetworking::setUp() {
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


void MQTcpNetworking::tearDown() {
    karabo::net::EventLoop::run();
    KARABO_LOG_FRAMEWORK_DEBUG << "EventLoop::run() was left.";
    KARABO_LOG_FRAMEWORK_DEBUG << "=== MQTestClientServer STOPPED nThreads = "
                               << karabo::net::EventLoop::getNumberOfThreads();
    KARABO_LOG_FRAMEWORK_DEBUG << "==============================================";
}


void MQTcpNetworking::onClientConnected(const karabo::net::ErrorCode& e, const karabo::net::Channel::Pointer& channel) {
    if (e) {
        KARABO_LOG_FRAMEWORK_DEBUG << "MQTcpNetworking::onClientConnected  ErrorCode = " << e;
        clientChannelErrorHandler(e, channel);
        return;
    }

    karabo::data::Hash header("headline", "*** CLIENT ***");
    karabo::data::Hash data("START", 50000);

    // first sending
    channel->writeAsync(header, data);
    m_clientCount = 0;
    m_clientTimestamp = std::chrono::steady_clock::now();
    channel->readAsyncHashHash(std::bind(&MQTcpNetworking::clientReadHashHashHandler, this, _1, channel, _2, _3));
}


void MQTcpNetworking::clientChannelErrorHandler(const karabo::net::ErrorCode& ec,
                                                const karabo::net::Channel::Pointer& channel) {
    if (channel) channel->close();
    if (ec != boost::asio::error::eof) {
        KARABO_LOG_FRAMEWORK_DEBUG << "\nCLIENT ERROR: " << ec.value() << " -- " << ec.message();

        if (m_serverConnection) {
            m_serverConnection->stop();
            m_serverConnection.reset();
        }
    }
}


void MQTcpNetworking::clientReadHashHashHandler(const karabo::net::ErrorCode& e,
                                                const karabo::net::Channel::Pointer& channel,
                                                karabo::data::Hash& header, karabo::data::Hash& body) {
    if (e) {
        clientChannelErrorHandler(e, channel);
        return;
    }

    // inspect here the server reply.... just count
    m_clientCount++;
    if (m_clientCount < m_numberOfMessages) {
        channel->readAsyncHashHash(std::bind(&MQTcpNetworking::clientReadHashHashHandler, this, _1, channel, _2, _3));
    } else {
        karabo::data::Hash header("headline", "*** CLIENT ***");
        karabo::data::Hash data("STOP", karabo::data::Hash());
        channel->writeAsyncHashHash(
              header, data, std::bind(&MQTcpNetworking::onClientEnd, this, boost::asio::placeholders::error, channel));
    }
}


void MQTcpNetworking::onClientEnd(const karabo::net::ErrorCode& e, const karabo::net::Channel::Pointer& channel) {
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
}
