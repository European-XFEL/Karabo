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
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Created on Oct 30, 2012, 1:33:46 PM
 */

#include "TcpNetworking_Test.hh"

#include <array>
#include <boost/bind/bind.hpp>
#include <boost/thread.hpp>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iosfwd>
#include <iostream>

#include "karabo/io/BinarySerializer.hh"
#include "karabo/io/BufferSet.hh"
#include "karabo/net/Channel.hh"
#include "karabo/net/Connection.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/Queues.hh"
#include "karabo/net/TcpChannel.hh"
#include "karabo/util/Dims.hh"
#include "karabo/util/Exception.hh"
#include "karabo/util/NDArray.hh"

using namespace std::chrono;
using namespace std::literals::chrono_literals; // 10ms == 10 milliseconds
using namespace std::literals::string_literals; // For '"blabla"s'

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

using karabo::net::Channel;
using karabo::net::Connection;
using karabo::net::EventLoop;
using karabo::util::Hash;
using karabo::util::toString;

/**
 * Create client and server that are connected
 *
 * @returns pair (client/server) of pairs (connection, channel)
 */
std::pair<std::pair<Connection::Pointer, Channel::Pointer>, std::pair<Connection::Pointer, Channel::Pointer>>
createClientServer(Hash clientCfg, Hash serverCfg) {
    CPPUNIT_ASSERT_MESSAGE("Must not specify type for server", !serverCfg.erase("type"));
    CPPUNIT_ASSERT_MESSAGE("Must not specify type for client", !clientCfg.erase("type"));
    CPPUNIT_ASSERT_MESSAGE("Must not specify port for client", !clientCfg.erase("port"));

    serverCfg.merge(Hash("type", "server"));
    auto serverConn = Connection::create("Tcp", serverCfg);

    Channel::Pointer serverChannel;
    std::string failureReasonServ;
    auto serverConnectHandler = [&serverChannel, &failureReasonServ](const karabo::net::ErrorCode& ec,
                                                                     const Channel::Pointer& channel) {
        if (ec) {
            failureReasonServ = "Server connection failed: " + toString(ec.value()) += " -- " + ec.message();
        } else {
            serverChannel = channel;
        }
    };
    const unsigned int serverPort = serverConn->startAsync(serverConnectHandler);
    CPPUNIT_ASSERT(serverPort != 0);

    clientCfg.merge(Hash("type", "client", "port", serverPort));
    Connection::Pointer clientConn = Connection::create("Tcp", clientCfg);
    Channel::Pointer clientChannel = clientConn->start();

    int timeout = 10000;
    while (timeout >= 0) {
        if (serverChannel) break;
        std::this_thread::sleep_for(10ms);
        timeout -= 10;
    }
    CPPUNIT_ASSERT_MESSAGE(failureReasonServ + ", timeout: " + toString(timeout), serverChannel);
    return std::make_pair(std::make_pair(serverConn, serverChannel), std::make_pair(clientConn, clientChannel));
}


CPPUNIT_TEST_SUITE_REGISTRATION(TcpNetworking_Test);


struct TcpServer {
    KARABO_CLASSINFO(TcpServer, "TcpServer", "1.0");

    TcpServer() : m_count(0), m_port(0) {
        m_connection = karabo::net::Connection::create(karabo::util::Hash("Tcp.port", 0, "Tcp.type", "server"));
        m_port = m_connection->startAsync(std::bind(&TcpServer::connectHandler, this, _1, _2));
    }


    virtual ~TcpServer() {}


    int port() {
        return m_port;
    }


    void connectHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER_ERROR: " << ec.value() << " -- " << ec.message();
            if (channel) channel->close();
            return;
        }
        channel->readAsyncHashHash(std::bind(&TcpServer::readHashHashHandler, this, _1, channel, _2, _3));
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
        KARABO_LOG_FRAMEWORK_DEBUG << "\nSERVER_INFO: count " << m_count << "\n"
                                   << header << body << "-----------------\n";

        CPPUNIT_ASSERT(header.get<std::string>("headline") == "*** CLIENT ***");

        header.set("headline", "----- SERVER -----");

        body.set("a.e", "server data");

        if (body.has("a") && body.get<std::string>("a.b") == "?") body.set("a.b", "server reply");
        else body.set("a.b", "counter " + karabo::util::toString(m_count));


        channel->writeAsyncHashHash(header, body,
                                    std::bind(&TcpServer::writeCompleteHandler, this, _1, channel, "some string"));
    }


    void writeCompleteHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel,
                              const std::string& id) {
        if (ec) {
            errorHandler(ec, channel);
            return;
        }

        BOOST_ASSERT(id == "some string");
        channel->readAsyncHashHash(std::bind(&TcpServer::readHashHashHandler, this, _1, channel, _2, _3));
    }

   private:
    int m_count;
    int m_port;
    karabo::net::Connection::Pointer m_connection;
};


struct TcpClient {
    KARABO_CLASSINFO(TcpClient, "TcpClient", "1.0");

    TcpClient(const std::string& host, int port)
        : m_count(0),
          m_port(port),
          m_connection(karabo::net::Connection::create(
                karabo::util::Hash("Tcp.port", m_port, "Tcp.hostname", "sample.example.org"))),
          m_deadline(karabo::net::EventLoop::getIOService()) { // timeout repetition channel ec
        m_connection->startAsync(std::bind(&TcpClient::connectHandler, this, _1, 1000, 3, _2));
    }


    virtual ~TcpClient() {}


    void connectHandler(const karabo::net::ErrorCode& ec, int timeout, int repetition,
                        const karabo::net::Channel::Pointer& channel) {
        if (ec) {
            errorHandler(ec, channel);
            if (ec != boost::asio::error::eof && repetition >= 0) {
                m_deadline.expires_from_now(milliseconds(timeout));
                m_deadline.async_wait(
                      std::bind(&TcpClient::waitHandler, this, boost::asio::placeholders::error, timeout, repetition));
            }
            return;
        }
        KARABO_LOG_FRAMEWORK_DEBUG << "\nTcpClient connectHandler";
        karabo::util::Hash header("headline", "*** CLIENT ***");
        karabo::util::Hash data("a.b", "?", "a.c", 42.22f, "a.d", 12);

        // first sending
        channel->writeAsyncHashHash(header, data, std::bind(&TcpClient::writeCompleteHandler, this, channel, 42));
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
            m_connection =
                  karabo::net::Connection::create(karabo::util::Hash("Tcp.port", m_port, "Tcp.hostname", "exflserv04"));
        if (repetition == 0)
            m_connection =
                  karabo::net::Connection::create(karabo::util::Hash("Tcp.port", m_port, "Tcp.hostname", "localhost"));
        m_connection->startAsync(std::bind(&TcpClient::connectHandler, this, _1, timeout, repetition, _2));
    }


    void readHashHashHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel,
                             karabo::util::Hash& header, karabo::util::Hash& body) {
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
        body.set("a.c", 1.0f * (static_cast<unsigned int>(::rand()) % 1000));
        body.set("a.d", static_cast<int>(::rand()) % 100);
        std::vector<unsigned char> pixels;
        body.set("a.v", pixels);
        std::vector<unsigned char>& x = body.get<std::vector<unsigned char>>("a.v");
        for (int i = 1; i <= 20; i++) x.push_back(static_cast<unsigned char>(i % 256));

        // send client data asynchronous: define "write" completion handler
        channel->writeAsyncHashHash(header, body, std::bind(&TcpClient::writeCompleteHandler, this, channel, 42));
    }


    void writeCompleteHandler(const karabo::net::Channel::Pointer& channel, int id) {
        CPPUNIT_ASSERT(id == 42);
        // data was sent successfully! Prepare to read a reply asynchronous from a server: placeholder _1 is a Hash
        channel->readAsyncHashHash(std::bind(&TcpClient::readHashHashHandler, this, _1, channel, _2, _3));
    }


   private:
    int m_count;
    int m_port;
    karabo::net::Connection::Pointer m_connection;
    boost::asio::steady_timer m_deadline;
    int m_timeout;
    int m_repetition;
};


//<editor-fold desc="Server, client and parameters for testWriteAsync">

#define CHAR_ARRAY_SIZE 4

struct WriteAsyncTestsParams {
    const karabo::util::Hash dataHash = karabo::util::Hash("Name", "DataHash", "PiField", 3.14159);
    const karabo::util::Hash dataHashNDArray =
          karabo::util::Hash("Data", karabo::util::NDArray(karabo::util::Dims(5000, 6000), 1000u));
    const std::string dataString = std::string("Sample of std::string");
    const karabo::util::Hash headerHash = karabo::util::Hash("Header", "hdr", "NumOfFields", 3, "required", true);
    const karabo::net::VectorCharPointer vectorCharPointer =
          std::make_shared<std::vector<char>>(std::vector<char>(10, 'A'));
    const std::vector<char> vectorChar = std::vector<char>(20, 'B');
    const std::size_t charArraySize = CHAR_ARRAY_SIZE;
    const char charArray[CHAR_ARRAY_SIZE] = {'1', '2', '5', 'A'};
    const int writePriority = 4;


    bool equalsTestDataHash(const karabo::util::Hash& other) {
        return (other == dataHash && other.get<std::string>("Name") == dataHash.get<std::string>("Name") &&
                other.get<double>("PiField") - dataHash.get<double>("PiField") < 1.e-14);
    }


    bool equalsTestHeaderHash(const karabo::util::Hash& other) {
        return (other == headerHash && other.get<std::string>("Header") == headerHash.get<std::string>("Header") &&
                other.get<int>("NumOfFields") == headerHash.get<int>("NumOfFields") &&
                other.get<bool>("required") == headerHash.get<bool>("required"));
    }


    bool equalsTestNDArrayHash(const karabo::util::Hash& other) {
        return (other.size() == dataHashNDArray.size() &&
                other.get<karabo::util::NDArray>("Data").getShape() ==
                      dataHashNDArray.get<karabo::util::NDArray>("Data").getShape() &&
                other.get<karabo::util::NDArray>("Data").getData<unsigned int>()[0] == 1000u);
    }
};


enum class TestOutcome { UNKNOWN, SUCCESS, FAILURE };

/**
 * The server part for the WriteAsync tests. Reads the data sent by the predefined sequence of writeAsyncs
 * issued by the client part. After the last data in the sequence is read the server flags that it is done reading
 * to the client, and the client closes the connection.
 */
struct WriteAsyncSrv {
    KARABO_CLASSINFO(WriteAsyncSrv, "WriteAsyncSrv", "1.0");


    WriteAsyncSrv(std::function<void(const TestOutcome&, const std::string&, const std::string&)> testReportFn)
        : m_port(0), m_testReportFn(testReportFn) {
        m_connection = karabo::net::Connection::create(karabo::util::Hash("Tcp.port", 0, "Tcp.type", "server"));
        m_port = m_connection->startAsync(std::bind(&WriteAsyncSrv::connectHandler, this, _1, _2));
    }

    int port() const {
        return m_port;
    }

   private:
    int m_port;
    std::function<void(const TestOutcome&, const std::string&, const std::string&)> m_testReportFn;
    karabo::net::Connection::Pointer m_connection;
    WriteAsyncTestsParams m_params;


    void connectHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
        std::clog << "[Srv]\t 0.1. Connect handler called." << std::endl;
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "WriteAsync connection");
            if (channel) channel->close();
            return;
        }
        channel->readAsyncHash(std::bind(&WriteAsyncSrv::readAsyncHashHandlerCopyFalse, this, _1, channel, _2));
        std::clog << "[Srv]\t 0.2. First read handler registered." << std::endl;
    }


    void readAsyncHashHandlerCopyFalse(const boost::system::error_code& ec,
                                       const karabo::net::Channel::Pointer& channel, karabo::util::Hash& hash) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashHandlerCopyFalse: " << ec.value()
                                       << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashHandlerCopyFalse");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 1.1. Read hash sent in body with copyAllData false." << std::endl;
        if (!m_params.equalsTestDataHash(hash)) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Hash read differs from hash written:\n") + "Expected:\n" +
                                 karabo::util::toString(m_params.dataHash) + "\nActual:\n" +
                                 karabo::util::toString(hash),
                           "#1. readAsyncHashHandlerCopyFalse");
            if (channel) channel->close();
        } else {
            std::clog << "[Srv]\t 1.2. Hash checked to be OK." << std::endl;
            // Reads the next piece of data sent by WriteAsyncCli as part of the test.
            channel->readAsyncHash(std::bind(&WriteAsyncSrv::readAsyncHashHandlerCopyTrue, this, _1, channel, _2));
        }
    }


    void readAsyncHashHandlerCopyTrue(const boost::system::error_code& ec, const karabo::net::Channel::Pointer& channel,
                                      karabo::util::Hash& hash) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashHandlerCopyTrue: " << ec.value()
                                       << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashHandlerCopyTrue");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 2.1. Read hash sent in body with copyAllData true." << std::endl;
        if (!m_params.equalsTestDataHash(hash)) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Hash read differs from hash written:\n") + "Expected:\n" +
                                 karabo::util::toString(m_params.dataHash) + "\nActual:\n" +
                                 karabo::util::toString(hash),
                           "#2. readAsyncHashHandlerCopyTrue");
            if (channel) channel->close();
        } else {
            std::clog << "[Srv]\t 2.2. Hash checked to be OK." << std::endl;
            // Reads the next piece of data sent by WriteAsyncCli as part of the test.
            channel->readAsyncString(std::bind(&WriteAsyncSrv::readAsyncStringHandler, this, _1, channel, _2));
        }
    }


    void readAsyncStringHandler(const boost::system::error_code& ec, const karabo::net::Channel::Pointer& channel,
                                std::string& str) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncStringHandler: " << ec.value() << " -- "
                                       << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncStringHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 3.1. Read string sent in body." << std::endl;
        if (str != m_params.dataString) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("String read differs from string written:\n") + "Expected:\n" +
                                 m_params.dataString + "\nActual:\n" + str,
                           "#3. readAsyncStringHandler");
            if (channel) channel->close();
        } else {
            std::clog << "[Srv]\t 3.2. String checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHashHash(
                  std::bind(&WriteAsyncSrv::readAsyncHashHashHandlerCopyFalse, this, _1, channel, _2, _3));
        }
    }


    void readAsyncHashHashHandlerCopyFalse(const boost::system::error_code& ec,
                                           const karabo::net::Channel::Pointer& channel, karabo::util::Hash& header,
                                           karabo::util::Hash& body) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashHashHandlerCopyFalse: " << ec.value()
                                       << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashHashHandlerCopyFalse");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 4.1. Read hashes sent in header and body with copyAllData false." << std::endl;
        if (!m_params.equalsTestHeaderHash(header) || !m_params.equalsTestDataHash(body)) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Hashes read don't match the ones written:\n") + "Expected header:\n" +
                                 karabo::util::toString(m_params.headerHash) + "\nActual header:\n" +
                                 karabo::util::toString(header) + "\nExpected body:\n" +
                                 karabo::util::toString(m_params.dataHash) + "\nActual body:\n" +
                                 karabo::util::toString(body),
                           "#4. readAsyncHashHashHandlerCopyFalse");
            if (channel) channel->close();
        } else {
            std::clog << "[Srv]\t 4.2. Hashes checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHashHash(
                  std::bind(&WriteAsyncSrv::readAsyncHashHashHandlerCopyTrue, this, _1, channel, _2, _3));
        }
    }


    void readAsyncHashHashHandlerCopyTrue(const boost::system::error_code& ec,
                                          const karabo::net::Channel::Pointer& channel, karabo::util::Hash& header,
                                          karabo::util::Hash& body) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashHashHandlerCopyTrue: " << ec.value()
                                       << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashHashHandlerCopyTrue");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 5.1. Read hashes sent in header and body with copyAllData true." << std::endl;
        if (!m_params.equalsTestHeaderHash(header) || !m_params.equalsTestDataHash(body)) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Hashes read don't match the ones written:\n") + "Expected header:\n" +
                                 karabo::util::toString(m_params.headerHash) + "\nActual header:\n" +
                                 karabo::util::toString(header) + "\nExpected body:\n" +
                                 karabo::util::toString(m_params.dataHash) + "\nActual body:\n" +
                                 karabo::util::toString(body),
                           "#5. readAsyncHashHashHandlerCopyTrue");
            if (channel) channel->close();
        } else {
            std::clog << "[Srv]\t 5.2. Hashes checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncVector(std::bind(&WriteAsyncSrv::readAsyncCharArrayHandler, this, _1, channel, _2));
        }
    }


    void readAsyncCharArrayHandler(const boost::system::error_code& ec, const karabo::net::Channel::Pointer& channel,
                                   std::vector<char>& vector) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncVectorHandler: " << ec.value() << " -- "
                                       << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncVectorHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 6.1. Read vector of char sent in body." << std::endl;
        if (vector != std::vector<char>(std::begin(m_params.charArray), std::end(m_params.charArray))) {
            m_testReportFn(TestOutcome::FAILURE, std::string("Vector read doesn't match the one written."),
                           "#6. readAsyncVectorHandler");
            if (channel) channel->close();
        } else {
            std::clog << "[Srv]\t 6.2. Vector checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncVectorPointer(
                  std::bind(&WriteAsyncSrv::readAsyncVectorPointerHandler, this, _1, channel, _2));
        }
    }


    void readAsyncVectorPointerHandler(const boost::system::error_code& ec,
                                       const karabo::net::Channel::Pointer& channel,
                                       std::shared_ptr<std::vector<char>>& vectorCharPointer) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncVectorPointerHandler: " << ec.value()
                                       << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncVectorHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 7.1. Read VetorCharPointer sent in body." << std::endl;
        if (*vectorCharPointer != *(m_params.vectorCharPointer)) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Vector read doesn't match the one written.") +
                                 "Expected vector size:" + karabo::util::toString(m_params.vectorCharPointer->size()) +
                                 std::string("\nActual vector size: ") +
                                 karabo::util::toString(vectorCharPointer->size()) +
                                 std::string("\nExpected first position content: ") + (*m_params.vectorCharPointer)[0] +
                                 std::string("\nActual first position content: ") + (*vectorCharPointer)[0],
                           "#7. readAsyncVectorPointerHandler");
            if (channel) channel->close();
        } else {
            std::clog << "[Srv]\t 7.2. VectorCharPointer checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHashVectorPointer(
                  std::bind(&WriteAsyncSrv::readAsyncHashVectorPointerHandler, this, _1, channel, _2, _3));
        }
    }


    void readAsyncHashVectorPointerHandler(const boost::system::error_code& ec,
                                           const karabo::net::Channel::Pointer& channel,
                                           const karabo::util::Hash& header,
                                           const karabo::net::VectorCharPointer& data) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashVectorPointerHandler: " << ec.value()
                                       << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashVectorPointerHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 8.1. Read header hash and VectorCharPointer body." << std::endl;

        if (!m_params.equalsTestHeaderHash(header) || *data != *(m_params.vectorCharPointer)) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Data read doesn't match the data written:\n") + "Expected header:\n" +
                                 karabo::util::toString(m_params.headerHash) + "\nActual header:\n" +
                                 karabo::util::toString(header) + "Expected body vector:" +
                                 karabo::util::toString(std::vector<char>((*m_params.vectorCharPointer).begin(),
                                                                          (*m_params.vectorCharPointer).end()),
                                                        80) +
                                 "\nActual body vector: " +
                                 karabo::util::toString(std::vector<char>((*data).begin(), (*data).end()), 80),
                           "#8. readAsyncHashVectorPointerHandler");
        } else {
            std::clog << "[Srv]\t 8.2. Hash header and VectorCharPointer body checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHash(
                  std::bind(&WriteAsyncSrv::readAsyncHashNDArrayHandlerCopyFalse, this, _1, channel, _2));
        }
    }


    void readAsyncHashNDArrayHandlerCopyFalse(const boost::system::error_code& ec,
                                              const karabo::net::Channel::Pointer& channel,
                                              const karabo::util::Hash& dataHash) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashNDArrayHandlerCopyFalse: "
                                       << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashNDArrayHandlerCopyFalse");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 9.1. Read hash with NDArray sent in body with copyAllData false." << std::endl;
        if (!m_params.equalsTestNDArrayHash(dataHash)) {
            m_testReportFn(
                  TestOutcome::FAILURE,
                  std::string("Hash with NDArray read doesn't match the hash written:\n.") +
                        "Expected hash size: " + karabo::util::toString(m_params.dataHashNDArray.size()) +
                        "\nActual hash size: " + karabo::util::toString(dataHash.size()) + "\nExpected NDArray size: " +
                        karabo::util::toString(m_params.dataHashNDArray.get<karabo::util::NDArray>("Data").size()) +
                        "\nActual NDArray size: " +
                        karabo::util::toString(dataHash.get<karabo::util::NDArray>("Data").size()),
                  "#9. readAsyncHashNDArrayHandlerCopyFalse");
        } else {
            std::clog << "[Srv]\t 9.2. Hash with NDArray checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHash(
                  std::bind(&WriteAsyncSrv::readAsyncHashNDArrayHandlerCopyTrue, this, _1, channel, _2));
        }
    }


    void readAsyncHashNDArrayHandlerCopyTrue(const boost::system::error_code& ec,
                                             const karabo::net::Channel::Pointer& channel,
                                             const karabo::util::Hash& dataHash) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashNDArrayHandlerCopyTrue: " << ec.value()
                                       << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashNDArrayHandlerCopyTrue");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 10.1. Read hash with NDArray sent in body with copyAllData true." << std::endl;
        if (!m_params.equalsTestNDArrayHash(dataHash)) {
            m_testReportFn(
                  TestOutcome::FAILURE,
                  std::string("Hash with NDArray read doesn't match the hash written:\n.") +
                        "Expected hash size: " + karabo::util::toString(m_params.dataHashNDArray.size()) +
                        "\nActual hash size: " + karabo::util::toString(dataHash.size()) + "\nExpected NDArray size: " +
                        karabo::util::toString(m_params.dataHashNDArray.get<karabo::util::NDArray>("Data").size()) +
                        "\nActual NDArray size: " +
                        karabo::util::toString(dataHash.get<karabo::util::NDArray>("Data").size()),
                  "#10. readAsyncHashNDArrayHandlerCopyTrue");
        } else {
            std::clog << "[Srv]\t 10.2. Hash with NDArray checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHashVector(
                  std::bind(&WriteAsyncSrv::readAsyncHashCharArrayHandler, this, _1, channel, _2, _3));
        }
    }


    void readAsyncHashCharArrayHandler(const boost::system::error_code& ec,
                                       const karabo::net::Channel::Pointer& channel,
                                       const karabo::util::Hash& headerHash, const std::vector<char>& dataVect) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashCharArrayHandler: " << ec.value()
                                       << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAysncHashCharArrayHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 11.1. Read header hash and body as a vector of chars." << std::endl;

        if (!m_params.equalsTestHeaderHash(headerHash) || dataVect.size() != m_params.charArraySize) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Data read doesn't match the data written:\n") + "Expected header:\n" +
                                 karabo::util::toString(m_params.headerHash) + "\nActual header:\n" +
                                 karabo::util::toString(headerHash) +
                                 "\nExpected body vector size: " + karabo::util::toString(m_params.charArraySize) +
                                 "\nActual body vector size: " + karabo::util::toString(dataVect.size()),
                           "#11. readAysncHashCharArrayHandler");
        } else {
            std::clog << "[Srv]\t 11.2. Header hash and array of char for body matched." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHashString(
                  std::bind(&WriteAsyncSrv::readAsyncHashStringHandler, this, _1, channel, _2, _3));
        }
    }


    void readAsyncHashStringHandler(const boost::system::error_code& ec, const karabo::net::Channel::Pointer& channel,
                                    const karabo::util::Hash& headerHash, const std::string& dataStr) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAsyncHashStringHandler: " << ec.value()
                                       << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashStringHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 12.1. Read header hash and body as a string." << std::endl;

        if (!m_params.equalsTestHeaderHash(headerHash) || dataStr != m_params.dataString) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Data read doesn't match the data written:\n") + "Expected header:\n" +
                                 karabo::util::toString(m_params.headerHash) + "\nActual header:\n" +
                                 karabo::util::toString(headerHash) + "\nExpected body string: " + m_params.dataString +
                                 "\nActual body string: " + dataStr,
                           "#12. readAsyncHashStringHandler");
        } else {
            std::clog << "[Srv]\t 12.2. Header hash and string for body matched." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHashVector(
                  std::bind(&WriteAsyncSrv::readAsyncHashVectorHandler, this, _1, channel, _2, _3));
        }
    }


    void readAsyncHashVectorHandler(const boost::system::error_code& ec, const karabo::net::Channel::Pointer& channel,
                                    const karabo::util::Hash& headerHash, const std::vector<char>& dataVect) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAsyncHashVectorHandler: " << ec.value()
                                       << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashVectorHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 13.1. Read header hash and body as a vector of char." << std::endl;


        if (!m_params.equalsTestHeaderHash(headerHash) || dataVect != m_params.vectorChar) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Data read doesn't match the data written:\n") + "Expected header:\n" +
                                 karabo::util::toString(m_params.headerHash) + "\nActual header:\n" +
                                 karabo::util::toString(headerHash) +
                                 "\nExpected body size: " + karabo::util::toString(m_params.vectorChar.size()) +
                                 "\nActual body size: " + karabo::util::toString(dataVect.size()),
                           "#13. readAsyncHashVectorHandler");
        } else {
            std::clog << "[Srv]\t 13.2. Header hash and vector of char for body matched." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncVector(std::bind(&WriteAsyncSrv::readAsyncVectorHandler, this, _1, channel, _2));
        }
    }


    void readAsyncVectorHandler(const boost::system::error_code& ec, const karabo::net::Channel::Pointer& channel,
                                const std::vector<char>& dataVect) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAsyncVectorHandler: " << ec.value() << " -- "
                                       << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncVectorHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 14.1. Read body as a vector of char." << std::endl;

        if (dataVect != m_params.vectorChar) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Data read doesn't match the data written:\n") +
                                 "\nExpected vector size: " + karabo::util::toString(m_params.vectorChar.size()) +
                                 "\nActual vector size: " + karabo::util::toString(dataVect.size()),
                           "#14. readAsyncVectorHandler");
        } else {
            std::clog << "[Srv]\t 14.2. Vector of char for body matched." << std::endl;
            m_testReportFn(TestOutcome::SUCCESS, "Tests succeeded!", "");
            if (channel) channel->close();
            std::clog << "[Srv] ... server read all data in the sequence." << std::endl;
        }
    }
};


struct WriteAsyncCli {
    KARABO_CLASSINFO(WriteAsyncCli, "WriteAndForgetCli", "1.0");

    WriteAsyncCli(const std::string& host, int port,
                  std::function<void(const TestOutcome&, const std::string&, const std::string&)> testReportFn)
        : m_port(port),
          m_testReportFn(testReportFn),
          m_connection(karabo::net::Connection::create(karabo::util::Hash("Tcp.port", m_port, "Tcp.hostname", host))) {
        m_connection->startAsync(std::bind(&WriteAsyncCli::connectHandler, this, _1, _2));
    }

   private:
    int m_port;
    std::function<void(const TestOutcome&, const std::string&, const std::string&)> m_testReportFn;
    karabo::net::Connection::Pointer m_connection;
    karabo::net::Channel::Pointer m_channel;
    WriteAsyncTestsParams m_params;

    void connectHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncCli error: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "WriteAsync connection");
            if (channel) channel->close();
            return;
        }

        decltype(high_resolution_clock::now()) startTime;
        decltype(high_resolution_clock::now()) stopTime;
        decltype(startTime - stopTime) startStopInterval;

        std::clog << "[Cli] Write async client connected. Sending data ..." << std::endl;
        try {
            channel->writeAsync(m_params.dataHash, m_params.writePriority, false);
            std::clog << "[Cli]\t1. sent hash as body with copyAllData false." << std::endl;
            channel->writeAsync(m_params.dataHash, m_params.writePriority, true);
            std::clog << "[Cli]\t2. sent hash as body with copyAllData true." << std::endl;

            channel->writeAsync(m_params.dataString, m_params.writePriority);
            std::clog << "[Cli]\t3. sent string as body." << std::endl;

            channel->writeAsync(m_params.headerHash, m_params.dataHash, m_params.writePriority, false);
            std::clog << "[Cli]\t4. sent a hash for header and a hash for body with copyAllData false. " << std::endl;
            channel->writeAsync(m_params.headerHash, m_params.dataHash, m_params.writePriority, true);
            std::clog << "[Cli]\t5. sent a hash for header and a hash for body with copyAllData true. " << std::endl;

            channel->writeAsync(m_params.charArray, m_params.charArraySize, m_params.writePriority);
            std::clog << "[Cli]\t6. sent an array of char as body." << std::endl;

            channel->writeAsync(m_params.vectorCharPointer, m_params.writePriority);
            std::clog << "[Cli]\t7. sent a VectorCharPointer as body." << std::endl;

            channel->writeAsync(m_params.headerHash, m_params.vectorCharPointer, m_params.writePriority);
            std::clog << "[Cli]\t8. sent a hash for header and VectorCharPointer for body." << std::endl;

            startTime = high_resolution_clock::now();
            channel->writeAsync(m_params.dataHashNDArray, m_params.writePriority, false);
            stopTime = high_resolution_clock::now();
            startStopInterval = stopTime - startTime;
            std::clog << "[Cli]\t9. sent a hash with an NDArray as field with copyAllData false (in "
                      << duration_cast<milliseconds>(startStopInterval).count() << "."
                      << duration_cast<microseconds>(startStopInterval).count() % 1000 << " milliseconds)."
                      << std::endl;
            startTime = high_resolution_clock::now();
            channel->writeAsync(m_params.dataHashNDArray, m_params.writePriority, true);
            stopTime = high_resolution_clock::now();
            startStopInterval = stopTime - startTime;
            std::clog << "[Cli]\t10. sent a hash with an NDArray as field with copyAllData true (in "
                      << duration_cast<milliseconds>(startStopInterval).count() << "."
                      << duration_cast<microseconds>(startStopInterval).count() % 1000 << " milliseconds)."
                      << std::endl;

            channel->writeAsync(m_params.headerHash, m_params.charArray, m_params.charArraySize,
                                m_params.writePriority);
            std::clog << "[Cli]\t11. sent a hash for header and an array of char for body." << std::endl;

            channel->writeAsync(m_params.headerHash, m_params.dataString, m_params.writePriority);
            std::clog << "[Cli]\t12. sent a hash for header and a string for body" << std::endl;

            channel->writeAsync(m_params.headerHash, m_params.vectorChar, m_params.writePriority);
            std::clog << "[Cli]\t13. sent a hash for header and a vector of char for body." << std::endl;

            channel->writeAsync(m_params.vectorChar, m_params.writePriority);
            std::clog << "[Cli]\t14. sent a vector of char for body." << std::endl;

            std::clog << "[Cli] ... all test data sent by the client" << std::endl;
        } catch (karabo::util::Exception& ke) {
            std::clog << "Error during write sequence by the client: " << ke.what() << std::endl;
            std::clog << "Details: " << std::endl;
            ke.showTrace(std::clog);
            m_testReportFn(TestOutcome::FAILURE, ke.what() + std::string(": ") + ke.detailedMsg(),
                           "WriteAsync sequence");
            if (channel) channel->close();
        } catch (std::exception& e) {
            std::clog << "Error during write sequence by the client: " << e.what() << std::endl;
            m_testReportFn(TestOutcome::FAILURE, e.what(), "WriteAsync sequence");
            if (channel) channel->close();
        }

        // Keep channel alive to avoid that [Srv] gets informed about disconnection before all data has been read
        // (but do not block the thread by sleeping).
        m_channel = channel;
    }
};

//</editor-fold>


TcpNetworking_Test::TcpNetworking_Test() {
    // To switch on logging:
    // #include "karabo/log/Logger.hh"
    // karabo::log::Logger::configure(karabo::util::Hash("priority", "DEBUG"));
    // karabo::log::Logger::useOstream();
}


TcpNetworking_Test::~TcpNetworking_Test() {}


void TcpNetworking_Test::setUp() {}


void TcpNetworking_Test::tearDown() {}

void TcpNetworking_Test::testConfig() {
    karabo::util::Hash cfg("hostname", "localhost", "port", 12345u); // default is client
    CPPUNIT_ASSERT_NO_THROW(karabo::net::Connection::create(karabo::util::Hash("Tcp", cfg)));

    karabo::util::Hash cfg2("url", "tcp://localhost:12345"); // default is client
    CPPUNIT_ASSERT_NO_THROW(karabo::net::Connection::create(karabo::util::Hash("Tcp", cfg2)));

    karabo::util::Hash cfg3("url", "localhost:12345"); // do not forget the protocol
    bool failed = false;
    try {
        karabo::net::Connection::create(karabo::util::Hash("Tcp", cfg3));
    } catch (const karabo::util::NetworkException& e) {
        CPPUNIT_ASSERT(e.userFriendlyMsg().find("does not start with 'tcp'") != std::string::npos);
        failed = true;
    }
    CPPUNIT_ASSERT(failed);
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


void TcpNetworking_Test::testBufferSet() {
    using namespace karabo::util;
    using namespace karabo::net;

    auto thr = boost::thread(&EventLoop::work);

    // Create server with handler for connections
    auto serverCon = Connection::create("Tcp", Hash("type", "server"));

    Channel::Pointer serverChannel;
    std::string failureReasonServ;
    auto serverConnectHandler = [&serverChannel, &failureReasonServ](const ErrorCode& ec,
                                                                     const Channel::Pointer& channel) {
        if (ec) {
            failureReasonServ = "Server connection failed: " + toString(ec.value()) += " -- " + ec.message();
            std::clog << failureReasonServ << std::endl;
        } else {
            serverChannel = channel;
        }
    };
    const unsigned int serverPort = serverCon->startAsync(serverConnectHandler);
    CPPUNIT_ASSERT(serverPort != 0);

    // Create client, connect to server and validate connection from server side
    Connection::Pointer clientConn = Connection::create("Tcp", Hash("type", "client", "port", serverPort));
    Channel::Pointer clientChannel = clientConn->start();

    int timeout = 10000;
    while (timeout >= 0) {
        if (serverChannel) break;
        std::this_thread::sleep_for(10ms);
        timeout -= 10;
    }
    CPPUNIT_ASSERT_MESSAGE(failureReasonServ + ", timeout: " + toString(timeout), serverChannel);

    //////////////////////////////////////////////////////////////////////////
    // Prepare serialiser and data for sending
    //////////////////////////////////////////////////////////////////////////

    // Create Hash with many small NDArray: When it is sent that will create a lot of buffers, but due to an overall
    // rather small message it will likely go through the synchronous code path - and that was buggy
    // (using socket::read_some instead of asio::read) up to Karabo 2.7.0:
    Hash data1;
    const int numNDarray = 500;
    for (int i = 0; i < numNDarray; ++i) {
        data1.set(toString(i), NDArray(Dims(1ull), i));
    }
    auto serializer = karabo::io::BinarySerializer<Hash>::create("Bin");
    auto buffers = std::vector<karabo::io::BufferSet::Pointer>( // vector of two BufferSets
          {std::make_shared<karabo::io::BufferSet>(), std::make_shared<karabo::io::BufferSet>()});
    serializer->save(data1, *(buffers[0])); // save into first BufferSet
    // Add a second BufferSet with normally big NDArray
    const Hash data2("vec", std::vector<short>(100, 7), // vector of length 100 shorts
                     "arr", NDArray(10000ull, 77));     // 1D array of 10k ints
    serializer->save(data2, *(buffers[1]));             // save into second BufferSet

    //////////////////////////////////////////////////////////////////////////
    // Prepare the (always asynchronous) reading
    //////////////////////////////////////////////////////////////////////////
    auto recvProm = std::make_shared<std::promise<bool>>();
    auto recvFut = recvProm->get_future();
    std::string failureReason;
    Hash receivedHeader;
    std::vector<karabo::io::BufferSet::Pointer> receivedBuffers;
    auto onRead = [&recvProm, &failureReason, &receivedHeader, &receivedBuffers](
                        const boost::system::error_code& ec, const karabo::util::Hash& h,
                        const std::vector<karabo::io::BufferSet::Pointer>& bufs) {
        if (ec) {
            failureReason = toString(ec.value()) += " -- " + ec.message();
            recvProm->set_value(false);
        } else {
            receivedHeader = h;
            receivedBuffers = bufs;
            recvProm->set_value(true);
        }
    };

    // Register handler
    clientChannel->readAsyncHashVectorBufferSetPointer(onRead);

    //////////////////////////////////////////////////////////////////////////
    // First test synchronous writing of HashVectorBufferSetPointer
    //////////////////////////////////////////////////////////////////////////

    const Hash header("my", "header", "data", 42);
    serverChannel->write(header, buffers); // synchronous writing with header

    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, recvFut.wait_for(seconds(10)));
    CPPUNIT_ASSERT_MESSAGE("Error Sending data" + failureReason, recvFut.get());

    // Check that content is as expected - in a lambda to do the exact same checks for both sync and async writing
    auto doAsserts = [&]() {
        CPPUNIT_ASSERT_EQUAL(2ul, receivedBuffers.size());
        Hash dataRead;
        serializer->load(dataRead, *(receivedBuffers[0]));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(numNDarray), dataRead.size());
        for (int i = 0; i < numNDarray; ++i) {
            const std::string key(toString(i));
            CPPUNIT_ASSERT_MESSAGE("Miss key " + toString(key), dataRead.has(key));
            const NDArray& arr = dataRead.get<NDArray>(key);
            CPPUNIT_ASSERT_EQUAL(1ul, arr.size());
            CPPUNIT_ASSERT_EQUAL(i, arr.getData<int>()[0]);
        }
        dataRead.clear();
        serializer->load(dataRead, *(receivedBuffers[1]));
        CPPUNIT_ASSERT_EQUAL(2ul, dataRead.size());
        CPPUNIT_ASSERT_MESSAGE("Received data: " + toString(dataRead), dataRead.fullyEquals(data2));
        // Erase internals from header before comparing - should that already be done internally or should we test these
        // internals as potentially needed in cross API communication?
        // For now assert at least on their existence.
        CPPUNIT_ASSERT(receivedHeader.erase("_bufferSetLayout_"));
        CPPUNIT_ASSERT(receivedHeader.erase("nData"));
        CPPUNIT_ASSERT(receivedHeader.erase("byteSizes"));
        CPPUNIT_ASSERT_MESSAGE("Received header: " + toString(receivedHeader),
                               receivedHeader.fullyEquals(Hash("my", "header", "data", 42)));
    };
    doAsserts();

    //////////////////////////////////////////////////////////////////////////
    // Re-repare the asynchronous reading
    //////////////////////////////////////////////////////////////////////////
    recvProm = std::make_shared<std::promise<bool>>();
    recvFut = recvProm->get_future();
    failureReason.clear();
    receivedHeader.clear();
    receivedBuffers.clear();
    clientChannel->readAsyncHashVectorBufferSetPointer(onRead);

    //////////////////////////////////////////////////////////////////////////
    // Now asynchronous writing of HashVectorBufferSetPointer
    //////////////////////////////////////////////////////////////////////////
    auto sendProm = std::make_shared<std::promise<bool>>();
    auto sendFut = sendProm->get_future();
    std::string sendFailureReason;

    auto onWriteComplete = [sendProm, &sendFailureReason](const boost::system::error_code& ec) {
        if (ec) {
            sendProm->set_value(false);
            sendFailureReason = toString(ec.value()) += " -- " + ec.message();
        } else {
            sendProm->set_value(true);
        }
    };
    serverChannel->writeAsyncHashVectorBufferSetPointer(header, buffers, onWriteComplete);

    // Wait for confirmation that data is written and read
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, sendFut.wait_for(seconds(10)));
    CPPUNIT_ASSERT_MESSAGE("Error sending data: " + sendFailureReason, sendFut.get());
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, recvFut.wait_for(seconds(10)));
    CPPUNIT_ASSERT_MESSAGE("Error Sending data" + failureReason, recvFut.get());

    // Now check correctness of asynchronously written data
    doAsserts();

    EventLoop::stop();
    thr.join();
}


void TcpNetworking_Test::testConsumeBytesAfterReadUntil() {
    using namespace karabo::util;
    using namespace karabo::net;

    auto thr = boost::thread(&EventLoop::work);

    // Create server with handler for connections
    auto serverCon = Connection::create("Tcp", Hash("type", "server", "sizeofLength", 0));

    Channel::Pointer serverChannel;
    std::string failureReasonServ;
    auto serverConnectHandler = [&serverChannel, &failureReasonServ](const ErrorCode& ec,
                                                                     const Channel::Pointer& channel) {
        if (ec) {
            failureReasonServ = "Server connect failed: " + toString(ec.value()) += " -- " + ec.message();
        } else {
            serverChannel = channel;
        }
    };
    const unsigned int serverPort = serverCon->startAsync(serverConnectHandler);
    CPPUNIT_ASSERT(serverPort != 0);

    // Create client, connect to server and validate connection
    Connection::Pointer clientConn =
          Connection::create("Tcp", Hash("sizeofLength", 0, "type", "client", "port", serverPort));
    Channel::Pointer clientChannel;
    std::string failureReasonCli;
    auto clientConnectHandler = [&clientChannel, &failureReasonCli](const ErrorCode& ec,
                                                                    const Channel::Pointer& channel) {
        if (ec) {
            std::stringstream os;
            os << "\nClient connection failed: " << ec.value() << " -- " << ec.message();
            failureReasonCli = os.str();
        } else {
            clientChannel = channel;
        }
    };
    clientConn->startAsync(clientConnectHandler);

    int timeout = 10000;
    while (timeout >= 0) {
        if (clientChannel && serverChannel) break;
        std::this_thread::sleep_for(10ms);
        timeout -= 10;
    }
    CPPUNIT_ASSERT_MESSAGE(failureReasonServ + ", timeout: " + toString(timeout), serverChannel);
    CPPUNIT_ASSERT_MESSAGE(failureReasonCli + ", timeout: " + toString(timeout), clientChannel);

    // Upon successful connection, server sends 'Ready' string to client.
    // Client reads the message with consumeBytesAfterReadUntil. Both operations are done synchronously.
    // Even though consumeBytesAfterReadUntil has been created to be used in conjuction with readAsyncStringntil,
    // it can be used in standalone mode and in this unit test that capability is used.
    const std::string readyMsg("Ready");
    serverChannel->write(readyMsg);
    std::string readyMsgRead;
    CPPUNIT_ASSERT_NO_THROW(readyMsgRead = clientChannel->consumeBytesAfterReadUntil(readyMsg.size()));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Ready message differs from expected.", readyMsgRead, readyMsg);

    // Checks the interplay between readAsyncStringUntil and consumeBytesAfterReadUntil.
    const std::string untilSep("HTTP 1.1 403 Forbidden\n\n");
    const std::string afterSep("No access granted for user.");
    const std::string httpMsg(untilSep + afterSep);

    serverChannel->write(httpMsg);
    std::atomic<bool> readSeqCompleted(false);
    auto readUntilHandler = [&clientChannel, &httpMsg, &readSeqCompleted, &failureReasonCli, &untilSep, &afterSep](
                                  const ErrorCode& ec, std::string msgRead) {
        if (ec) {
            std::stringstream os;
            os << "\nreadAsyncStringUntil failed: " + ec.message();
            failureReasonCli = os.str();
        } else {
            if (msgRead != untilSep) {
                failureReasonCli =
                      "\nreadAsyncStringUntil result, '" + msgRead + "', differs from expected, '" + untilSep + "'.";
            } else {
                const std::string afterSepStr = clientChannel->consumeBytesAfterReadUntil(afterSep.size());
                if (afterSepStr != afterSep) {
                    failureReasonCli = "\nconsumeBytesAfterReadUntil result, '" + afterSepStr +
                                       "', differs from expected." + afterSep + "'.";
                } else {
                    readSeqCompleted = true;
                }
            }
        }
    };
    clientChannel->readAsyncStringUntil("\n\n", readUntilHandler);

    // Waits for the read sequence test to succeed or timeout.
    timeout = 12000;
    while (timeout >= 0) {
        if (readSeqCompleted) break;
        std::this_thread::sleep_for(10ms);
        timeout -= 10;
    }

    // The order of the asserts is important: had the timeout assert come first, failureReasonCli would never be shown.
    CPPUNIT_ASSERT(readSeqCompleted); // if this test line is skipped and we reached timeout, next line might be
                                      // concurrent to a late setting of failureReasonCli which could lead to a crash
    CPPUNIT_ASSERT_MESSAGE("Read sequence test failed" + failureReasonCli, failureReasonCli.empty());
    CPPUNIT_ASSERT_MESSAGE("ReadAsyncStringUntil - consumeBytesAfterReadUntil sequence timed out!", timeout >= 0);

    EventLoop::stop();
    thr.join();
}


void TcpNetworking_Test::testWriteAsync() {
    using namespace std;

    // Data items for the test results - to be called by either the server or the client to report test
    // results.
    TestOutcome testOutcome = TestOutcome::UNKNOWN;
    std::string testOutcomeMessage("");
    std::string failingTestCaseName("");
    decltype(high_resolution_clock::now()) startTime;
    decltype(high_resolution_clock::now()) finishTime;

    // Mutex for test results access
    std::mutex testResultsMutex;

    // Setter for test results data.
    auto testReportFn = [&](const TestOutcome& outcome, const std::string& outcomeMessage,
                            const std::string& failTestCaseName) -> void {
        std::lock_guard<std::mutex> lock(testResultsMutex);
        testOutcome = outcome;
        testOutcomeMessage = outcomeMessage;
        failingTestCaseName = failTestCaseName;
        finishTime = high_resolution_clock::now();
    };

    startTime = high_resolution_clock::now();

    WriteAsyncSrv server(testReportFn);
    WriteAsyncCli client("localhost", server.port(), testReportFn);

    karabo::net::EventLoop::run();

    std::lock_guard<std::mutex> lock(testResultsMutex);
    if (testOutcome == TestOutcome::SUCCESS) {
        auto testDuration = finishTime - startTime;
        std::clog << "Test took " << duration_cast<milliseconds>(testDuration).count() << " milliseconds." << std::endl;
        CPPUNIT_ASSERT(true);
    } else {
        CPPUNIT_ASSERT_MESSAGE(
              "Failed:\n---------------\n" + testOutcomeMessage + "\n---------------\nat test: " + failingTestCaseName,
              false);
    }
}

void TcpNetworking_Test::testAsyncWriteCompleteHandler() {
    constexpr int timeoutSec = 10;

    auto thr = boost::thread(&EventLoop::work);

    //
    // Create server, client and connect them
    //
    auto clientServerPair = createClientServer(Hash(), Hash());
    Connection::Pointer& clientConn = clientServerPair.first.first;
    Channel::Pointer& clientChannel = clientServerPair.first.second;

    //
    // Test that handler is called although channel destructed
    //
    {
        auto prom = std::make_shared<std::promise<const boost::system::error_code&>>();
        auto fut = prom->get_future();
        auto handler = [prom](const boost::system::error_code& ec) { prom->set_value(ec); };
        clientChannel->writeAsyncHash(Hash("some", "data"), handler);
        clientChannel.reset();
        clientConn.reset();

        auto ok = fut.wait_for(seconds(timeoutSec));
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, ok);
    }

    EventLoop::stop();
    thr.join();
}


void TcpNetworking_Test::testConnCloseChannelStop() {
    auto thr = boost::thread(&EventLoop::work);

    Connection::Pointer emptyConnPtr;
    {
        // server reads, but client channel closes
        auto clientServerPair = createClientServer(Hash(), Hash());
        Channel::Pointer& clientChannel = clientServerPair.first.second;
        Channel::Pointer& serverChannel = clientServerPair.second.second;
        // Test
        testConnCloseChannelStop(serverChannel, clientChannel, emptyConnPtr);
        std::clog << " Done server reads and client closes." << std::flush;
    }

    {
        // server reads, but client connection stops
        auto clientServerPair = createClientServer(Hash(), Hash());
        Channel::Pointer& clientChannel = clientServerPair.first.second;
        Connection::Pointer& clientConn = clientServerPair.first.first;
        Channel::Pointer& serverChannel = clientServerPair.second.second;
        // Test
        testConnCloseChannelStop(serverChannel, clientChannel, clientConn);
        std::clog << " Done server reads and client connection stops." << std::flush;
    }

    {
        // client reads, but server channel closes
        auto clientServerPair = createClientServer(Hash(), Hash());
        Channel::Pointer& clientChannel = clientServerPair.first.second;
        Channel::Pointer& serverChannel = clientServerPair.second.second;
        // Test
        testConnCloseChannelStop(clientChannel, serverChannel, emptyConnPtr);
        std::clog << " Done client reads and server closes." << std::flush;
    }

    {
        // client reads, but server connection stops
        auto clientServerPair = createClientServer(Hash(), Hash());
        Channel::Pointer& clientChannel = clientServerPair.first.second;
        Channel::Pointer& serverChannel = clientServerPair.second.second;
        Connection::Pointer& serverConn = clientServerPair.second.first;
        // Test
        testConnCloseChannelStop(clientChannel, serverChannel, serverConn);
        std::clog << " Done client reads and server connection stops." << std::flush;
    }

    EventLoop::stop();
    thr.join();
}

void TcpNetworking_Test::testConnCloseChannelStop(Channel::Pointer& alice, Channel::Pointer& bob,
                                                  Connection::Pointer& bobConn) {
    constexpr int timeoutSec = 10;

    // Register handlers and asynchronously write and read from bob to alice
    auto readProm = std::make_shared<std::promise<boost::system::error_code>>();
    auto readFut = readProm->get_future();
    Channel::ReadHashHandler readHandler = [readProm](const boost::system::error_code& ec, Hash&) {
        readProm->set_value(ec);
    };
    alice->readAsyncHash(readHandler);
    bob->writeAsyncHash(Hash("a", 1), [](const boost::system::error_code& ec) {});
    auto ok = readFut.wait_for(seconds(timeoutSec));
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, ok);
    boost::system::error_code ec = readFut.get();
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), 0, ec.value());

    // Register another async read handler at alice, but after bob has "touched" its TCP classes:
    // - either close bob's channel: alice's read handler will be called with failure
    // - or stop bob's connection: all stays fine, bob can still write and so alice will receive OK
    readProm = std::make_shared<std::promise<boost::system::error_code>>();
    readFut = readProm->get_future();
    readHandler = [readProm](const boost::system::error_code& ec, Hash& h) { readProm->set_value(ec); };
    bool stoppedConn = false;
    if (bobConn) {
        bobConn->stop();
        stoppedConn = true;
    } else {
        bob->close();
    }
    CPPUNIT_ASSERT(alice->isOpen());
    alice->readAsyncHash(readHandler);
    // This will fail if channel closed, but we do not care
    bob->writeAsyncHash(Hash("a", 2), [](const boost::system::error_code& ec) {});

    ok = readFut.wait_for(seconds(timeoutSec));
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, ok);

    ec = readFut.get();
    if (stoppedConn) {
        // Connection stop has no influence, reading succeeded
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), 0, ec.value());
    } else {
        // Channel closed, so other end read failed (2 means 'End of file')
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), 2, ec.value());
    }
}
