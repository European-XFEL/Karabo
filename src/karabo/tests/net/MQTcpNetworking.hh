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
 * File:   MQTcpNetworking.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on Sep 22, 2015, 3:06:16 PM
 */

#ifndef MQTCPNETWORKING_HH
#define MQTCPNETWORKING_HH

#include <cppunit/extensions/HelperMacros.h>

#include "karabo/log/Logger.hh"
#include "karabo/net/Channel.hh"
#include "karabo/net/Connection.hh"
#include "karabo/util/Configurator.hh"

class MQTcpNetworking : public CPPUNIT_NS::TestFixture {
    int m_numberOfMessages;

    karabo::util::Hash m_header;

    karabo::util::Hash m_data;

    // server
    int m_serverCount;
    int m_serverPort;
    boost::thread m_serverThread;
    karabo::net::Connection::Pointer m_serverConnection;
    std::chrono::steady_clock::time_point m_ts;

    // client
    int m_clientCount;
    karabo::net::Connection::Pointer m_connection;
    std::chrono::steady_clock::time_point m_clientTimestamp;

    CPPUNIT_TEST_SUITE(MQTcpNetworking);

    CPPUNIT_TEST(testClientServerMethod);

    CPPUNIT_TEST_SUITE_END();

   public:
    KARABO_CLASSINFO(MQTcpNetworking, "MQTcpNetworking", "1.0");

    MQTcpNetworking();
    virtual ~MQTcpNetworking();
    void setUp();
    void tearDown();

   private:
    void createServer();

    void serverConnectHandler(const karabo::net::ErrorCode& e, const karabo::net::Channel::Pointer& channel);

    void serverErrorHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);

    void serverReadHashHashHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel,
                                   karabo::util::Hash& header, karabo::util::Hash& body);

    void serverPublish(const karabo::net::Channel::Pointer& channel);

    void serverRun();

    void testClientServerMethod();

    void onClientConnected(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);

    void clientChannelErrorHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);

    void clientReadHashHashHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel,
                                   karabo::util::Hash& header, karabo::util::Hash& body);

    void onClientEnd(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel);
};

#endif /* MQTCPNETWORKING_HH */
