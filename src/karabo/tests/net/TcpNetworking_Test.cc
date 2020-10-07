/*
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Created on Oct 30, 2012, 1:33:46 PM
 */

#include "TcpNetworking_Test.hh"

#include "karabo/net/Connection.hh"
#include "karabo/net/Channel.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/Queues.hh"
#include "karabo/io/BinarySerializer.hh"
#include "karabo/io/BufferSet.hh"
#include "karabo/util/Exception.hh"
#include "karabo/net/TcpChannel.hh"

#include <boost/bind.hpp>
#include <boost/thread.hpp>

#include <iostream>
#include <fstream>
#include <cassert>
#include <iosfwd>

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
        , m_deadline(karabo::net::EventLoop::getIOService()) { // timeout repetition channel ec
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


//<editor-fold desc="Server, client and parameters for testWriteAsync">

#define CHAR_ARRAY_SIZE 4

struct WriteAsyncTestsParams {
    const karabo::util::Hash dataHash = karabo::util::Hash("Name", "DataHash", "PiField", 3.14159);
    const karabo::util::Hash dataHashNDArray =
            karabo::util::Hash("Data", karabo::util::NDArray(karabo::util::Dims(10000, 60000), 1000u));
    const std::string dataString = std::string("Sample of std::string");
    const karabo::util::Hash headerHash = karabo::util::Hash("Header", "hdr", "NumOfFields", 3, "required", true);
    const karabo::net::VectorCharPointer vectorCharPointer =
            boost::make_shared<std::vector<char>>(std::vector<char>(10, 'A'));
    const std::vector<char> vectorChar = std::vector<char>(20, 'B');
    const std::size_t charArraySize = CHAR_ARRAY_SIZE;
    const char charArray[CHAR_ARRAY_SIZE] = {'1', '2', '5', 'A'};
    const int writePriority = 4;


    bool equalsTestDataHash(const karabo::util::Hash& other) {
        return (other == dataHash &&
                other.get<std::string>("Name") == dataHash.get<std::string>("Name") &&
                other.get<double>("PiField") - dataHash.get<double>("PiField") < 1.e-14);
    }


    bool equalsTestHeaderHash(const karabo::util::Hash& other) {
        return (other == headerHash &&
                other.get<std::string>("Header") == headerHash.get<std::string>("Header") &&
                other.get<int>("NumOfFields") == headerHash.get<int>("NumOfFields") &&
                other.get<bool>("required") == headerHash.get<bool>("required"));
    }


    bool equalsTestNDArrayHash(const karabo::util::Hash& other) {
        return (other.size() == dataHashNDArray.size() &&
                other.get<karabo::util::NDArray>("Data").getShape() == dataHashNDArray.get<karabo::util::NDArray>("Data").getShape() &&
                other.get<karabo::util::NDArray>("Data").getData<unsigned int>()[0] == 1000u);
    }
};


enum class TestOutcome {
    UNKNOWN, SUCCESS, FAILURE
};

/**
 * The server part for the WriteAsync tests. Reads the data sent by the predefined sequence of writeAsyncs
 * issued by the client part. After the last data in the sequence is read the server flags that it is done reading
 * to the client, and the client closes the connection.
 */
struct WriteAsyncSrv {


    KARABO_CLASSINFO(WriteAsyncSrv, "WriteAndForgetSrv", "1.0");


    WriteAsyncSrv(boost::function<void (const TestOutcome&, const std::string&, const std::string&) > testReportFn) :
        m_port(0),
        m_testReportFn(testReportFn) {
        m_connection = karabo::net::Connection::create(karabo::util::Hash("Tcp.port", 0, "Tcp.type", "server"));
        m_port = m_connection->startAsync(boost::bind(&WriteAsyncSrv::connectHandler, this, _1, _2));
    }

    int port() const {
        return m_port;
    }

private:
    int m_port;
    boost::function<void(const TestOutcome&, const std::string&, const std::string&) > m_testReportFn;
    karabo::net::Connection::Pointer m_connection;
    WriteAsyncTestsParams m_params;


    void connectHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "WriteAsync connection");
            if (channel) channel->close();
            return;
        }
        channel->readAsyncHash(boost::bind(&WriteAsyncSrv::readAsyncHashHandlerCopyFalse, this, _1, channel, _2));
    }


    void readAsyncHashHandlerCopyFalse(const boost::system::error_code& ec,
                                       const karabo::net::Channel::Pointer& channel,
                                       karabo::util::Hash& hash) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashHandlerCopyFalse: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashHandlerCopyFalse");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 1.1. Read hash sent in body with copyAllData false." << std::endl;
        if (!m_params.equalsTestDataHash(hash)) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Hash read differs from hash written:\n") +
                           "Expected:\n" + karabo::util::toString(m_params.dataHash) + "\nActual:\n" +
                           karabo::util::toString(hash), "#1. readAsyncHashHandlerCopyFalse");
            if (channel) channel->close();
        } else {
            std::clog << "[Srv]\t 1.2. Hash checked to be OK." << std::endl;
            // Reads the next piece of data sent by WriteAsyncCli as part of the test.
            channel->readAsyncHash(boost::bind(&WriteAsyncSrv::readAsyncHashHandlerCopyTrue, this, _1, channel, _2));
        }
    }


    void readAsyncHashHandlerCopyTrue(const boost::system::error_code& ec,
                                      const karabo::net::Channel::Pointer& channel,
                                      karabo::util::Hash& hash) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashHandlerCopyTrue: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashHandlerCopyTrue");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 2.1. Read hash sent in body with copyAllData true." << std::endl;
        if (!m_params.equalsTestDataHash(hash)) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Hash read differs from hash written:\n") +
                           "Expected:\n" + karabo::util::toString(m_params.dataHash) + "\nActual:\n" +
                           karabo::util::toString(hash), "#2. readAsyncHashHandlerCopyTrue");
            if (channel) channel->close();
        } else {
            std::clog << "[Srv]\t 2.2. Hash checked to be OK." << std::endl;
            // Reads the next piece of data sent by WriteAsyncCli as part of the test.
            channel->readAsyncString(boost::bind(&WriteAsyncSrv::readAsyncStringHandler, this, _1, channel, _2));
        }
    }


    void readAsyncStringHandler(const boost::system::error_code& ec,
                                const karabo::net::Channel::Pointer& channel,
                                std::string& str) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncStringHandler: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncStringHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 3.1. Read string sent in body." << std::endl;
        if (str != m_params.dataString) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("String read differs from string written:\n") +
                                       "Expected:\n" + m_params.dataString +
                                       "\nActual:\n" + str, "#3. readAsyncStringHandler");
            if (channel) channel->close();
        } else {
            std::clog << "[Srv]\t 3.2. String checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHashHash(boost::bind(&WriteAsyncSrv::readAsyncHashHashHandlerCopyFalse, this, _1, channel, _2, _3));
        }
    }


    void readAsyncHashHashHandlerCopyFalse(const boost::system::error_code& ec,
                                           const karabo::net::Channel::Pointer& channel,
                                           karabo::util::Hash& header,
                                           karabo::util::Hash& body) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashHashHandlerCopyFalse: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashHashHandlerCopyFalse");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 4.1. Read hashes sent in header and body with copyAllData false." << std::endl;
        if (!m_params.equalsTestHeaderHash(header) || !m_params.equalsTestDataHash(body)) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Hashes read don't match the ones written:\n") +
                           "Expected header:\n" + karabo::util::toString(m_params.headerHash) +
                           "\nActual header:\n" + karabo::util::toString(header) +
                           "\nExpected body:\n" + karabo::util::toString(m_params.dataHash) +
                           "\nActual body:\n" + karabo::util::toString(body), "#4. readAsyncHashHashHandlerCopyFalse");
            if (channel) channel->close();
        } else {
            std::clog << "[Srv]\t 4.2. Hashes checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHashHash(boost::bind(&WriteAsyncSrv::readAsyncHashHashHandlerCopyTrue, this, _1, channel, _2, _3));
        }
    }


    void readAsyncHashHashHandlerCopyTrue(const boost::system::error_code& ec,
                                          const karabo::net::Channel::Pointer& channel,
                                          karabo::util::Hash& header,
                                          karabo::util::Hash& body) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashHashHandlerCopyTrue: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashHashHandlerCopyTrue");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 5.1. Read hashes sent in header and body with copyAllData true." << std::endl;
        if (!m_params.equalsTestHeaderHash(header) || !m_params.equalsTestDataHash(body)) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Hashes read don't match the ones written:\n") +
                           "Expected header:\n" + karabo::util::toString(m_params.headerHash) +
                           "\nActual header:\n" + karabo::util::toString(header) +
                           "\nExpected body:\n" + karabo::util::toString(m_params.dataHash) +
                           "\nActual body:\n" + karabo::util::toString(body), "#5. readAsyncHashHashHandlerCopyTrue");
            if (channel) channel->close();
        } else {
            std::clog << "[Srv]\t 5.2. Hashes checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncVector(boost::bind(&WriteAsyncSrv::readAsyncCharArrayHandler, this, _1, channel, _2));
        }
    }


    void readAsyncCharArrayHandler(const boost::system::error_code& ec,
                                   const karabo::net::Channel::Pointer& channel,
                                   std::vector<char>& vector) {

        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncVectorHandler: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncVectorHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 6.1. Read vector of char sent in body." << std::endl;
        if (vector != std::vector<char>(std::begin(m_params.charArray), std::end(m_params.charArray))) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Vector read doesn't match the one written."),
                           "#6. readAsyncVectorHandler");
            if (channel) channel->close();
        } else {
            std::clog << "[Srv]\t 6.2. Vector checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncVectorPointer(boost::bind(&WriteAsyncSrv::readAsyncVectorPointerHandler, this, _1, channel, _2));
        }
    }


    void readAsyncVectorPointerHandler(const boost::system::error_code& ec,
                                       const karabo::net::Channel::Pointer& channel,
                                       boost::shared_ptr<std::vector<char>>&vectorCharPointer) {

        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncVectorPointerHandler: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncVectorHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 7.1. Read VetorCharPointer sent in body." << std::endl;
        if (*vectorCharPointer != *(m_params.vectorCharPointer)) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Vector read doesn't match the one written.") +
                           "Expected vector size:" + karabo::util::toString(m_params.vectorCharPointer->size()) +
                           std::string("\nActual vector size: ") + karabo::util::toString(vectorCharPointer->size()) +
                           std::string("\nExpected first position content: ") + (*m_params.vectorCharPointer)[0] +
                           std::string("\nActual first position content: ") + (*vectorCharPointer)[0],
                           "#7. readAsyncVectorPointerHandler");
            if (channel) channel->close();
        } else {
            std::clog << "[Srv]\t 7.2. VectorCharPointer checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHashVectorPointer(boost::bind(&WriteAsyncSrv::readAsyncHashVectorPointerHandler, this, _1, channel, _2, _3));
        }
    }


    void readAsyncHashVectorPointerHandler(const boost::system::error_code& ec,
                                           const karabo::net::Channel::Pointer& channel,
                                           const karabo::util::Hash& header,
                                           const karabo::net::VectorCharPointer& data) {

        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashVectorPointerHandler: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashVectorPointerHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 8.1. Read header hash and VectorCharPointer body." << std::endl;

        if (!m_params.equalsTestHeaderHash(header) ||
            *data != *(m_params.vectorCharPointer)) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Data read doesn't match the data written:\n") +
                           "Expected header:\n" + karabo::util::toString(m_params.headerHash) +
                           "\nActual header:\n" + karabo::util::toString(header) +
                           "Expected body vector:"
                           + karabo::util::toString(std::vector<char>((*m_params.vectorCharPointer).begin(),
                                                                      (*m_params.vectorCharPointer).end()), 80)
                           +
                           "\nActual body vector: "
                           + karabo::util::toString(std::vector<char>((*data).begin(), (*data).end()), 80),
                           "#8. readAsyncHashVectorPointerHandler");
        } else {
            std::clog << "[Srv]\t 8.2. Hash header and VectorCharPointer body checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHash(boost::bind(&WriteAsyncSrv::readAsyncHashNDArrayHandlerCopyFalse, this, _1, channel, _2));
        }
    }


    void readAsyncHashNDArrayHandlerCopyFalse(const boost::system::error_code& ec,
                                              const karabo::net::Channel::Pointer& channel,
                                              const karabo::util::Hash& dataHash) {

        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashNDArrayHandlerCopyFalse: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashNDArrayHandlerCopyFalse");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 9.1. Read hash with NDArray sent in body with copyAllData false." << std::endl;
        if (!m_params.equalsTestNDArrayHash(dataHash)) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Hash with NDArray read doesn't match the hash written:\n.") +
                           "Expected hash size: " + karabo::util::toString(m_params.dataHashNDArray.size()) +
                           "\nActual hash size: " + karabo::util::toString(dataHash.size()) +
                           "\nExpected NDArray size: " + karabo::util::toString(m_params.dataHashNDArray.get<karabo::util::NDArray>("Data").size()) +
                           "\nActual NDArray size: " + karabo::util::toString(dataHash.get<karabo::util::NDArray>("Data").size()),
                           "#9. readAsyncHashNDArrayHandlerCopyFalse");
        } else {
            std::clog << "[Srv]\t 9.2. Hash with NDArray checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHash(boost::bind(&WriteAsyncSrv::readAsyncHashNDArrayHandlerCopyTrue, this, _1, channel, _2));
        }
    }


    void readAsyncHashNDArrayHandlerCopyTrue(const boost::system::error_code& ec,
                                             const karabo::net::Channel::Pointer& channel,
                                             const karabo::util::Hash& dataHash) {

        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashNDArrayHandlerCopyTrue: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashNDArrayHandlerCopyTrue");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 10.1. Read hash with NDArray sent in body with copyAllData true." << std::endl;
        if (!m_params.equalsTestNDArrayHash(dataHash)) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Hash with NDArray read doesn't match the hash written:\n.") +
                           "Expected hash size: " + karabo::util::toString(m_params.dataHashNDArray.size()) +
                           "\nActual hash size: " + karabo::util::toString(dataHash.size()) +
                           "\nExpected NDArray size: " + karabo::util::toString(m_params.dataHashNDArray.get<karabo::util::NDArray>("Data").size()) +
                           "\nActual NDArray size: " + karabo::util::toString(dataHash.get<karabo::util::NDArray>("Data").size()),
                           "#10. readAsyncHashNDArrayHandlerCopyTrue");
        } else {
            std::clog << "[Srv]\t 10.2. Hash with NDArray checked to be OK." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHashVector(boost::bind(&WriteAsyncSrv::readAsyncHashCharArrayHandler, this, _1, channel, _2, _3));
        }
    }


    void readAsyncHashCharArrayHandler(const boost::system::error_code& ec,
                                       const karabo::net::Channel::Pointer& channel,
                                       const karabo::util::Hash& headerHash,
                                       const std::vector<char>& dataVect) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAysncHashCharArrayHandler: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAysncHashCharArrayHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 11.1. Read header hash and body as a vector of chars." << std::endl;

        if (!m_params.equalsTestHeaderHash(headerHash) || dataVect.size() != m_params.charArraySize) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Data read doesn't match the data written:\n") +
                           "Expected header:\n" + karabo::util::toString(m_params.headerHash) +
                           "\nActual header:\n" + karabo::util::toString(headerHash) +
                           "\nExpected body vector size: " + karabo::util::toString(m_params.charArraySize) +
                           "\nActual body vector size: " + karabo::util::toString(dataVect.size()),
                           "#11. readAysncHashCharArrayHandler");
        } else {
            std::clog << "[Srv]\t 11.2. Header hash and array of char for body matched." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHashString(boost::bind(&WriteAsyncSrv::readAsyncHashStringHandler, this, _1, channel, _2, _3));
        }
    }


    void readAsyncHashStringHandler(const boost::system::error_code& ec,
                                    const karabo::net::Channel::Pointer& channel,
                                    const karabo::util::Hash& headerHash,
                                    const std::string& dataStr) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAsyncHashStringHandler: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashStringHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 12.1. Read header hash and body as a string." << std::endl;

        if (!m_params.equalsTestHeaderHash(headerHash) || dataStr != m_params.dataString) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Data read doesn't match the data written:\n") +
                           "Expected header:\n" + karabo::util::toString(m_params.headerHash) +
                           "\nActual header:\n" + karabo::util::toString(headerHash) +
                           "\nExpected body string: " + m_params.dataString +
                           "\nActual body string: " + dataStr,
                           "#12. readAsyncHashStringHandler");
        } else {
            std::clog << "[Srv]\t 12.2. Header hash and string for body matched." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncHashVector(boost::bind(&WriteAsyncSrv::readAsyncHashVectorHandler, this, _1, channel, _2, _3));
        }
    }


    void readAsyncHashVectorHandler(const boost::system::error_code& ec,
                                    const karabo::net::Channel::Pointer& channel,
                                    const karabo::util::Hash& headerHash,
                                    const std::vector<char>& dataVect) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAsyncHashVectorHandler: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "readAsyncHashVectorHandler");
            if (channel) channel->close();
            return;
        }
        std::clog << "[Srv]\t 13.1. Read header hash and body as a vector of char." << std::endl;


        if (!m_params.equalsTestHeaderHash(headerHash) || dataVect != m_params.vectorChar) {
            m_testReportFn(TestOutcome::FAILURE,
                           std::string("Data read doesn't match the data written:\n") +
                           "Expected header:\n" + karabo::util::toString(m_params.headerHash) +
                           "\nActual header:\n" + karabo::util::toString(headerHash) +
                           "\nExpected body size: " + karabo::util::toString(m_params.vectorChar.size()) +
                           "\nActual body size: " + karabo::util::toString(dataVect.size()),
                           "#13. readAsyncHashVectorHandler");
        } else {
            std::clog << "[Srv]\t 13.2. Header hash and vector of char for body matched." << std::endl;
            // Reads the next piece of data sent by the WriteAsyncCli as part of the test.
            channel->readAsyncVector(boost::bind(&WriteAsyncSrv::readAsyncVectorHandler, this, _1, channel, _2));
        }
    }


    void readAsyncVectorHandler(const boost::system::error_code& ec,
                                const karabo::net::Channel::Pointer& channel,
                                const std::vector<char>& dataVect) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncSrv error at readAsyncVectorHandler: " << ec.value() << " -- " << ec.message();
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

    WriteAsyncCli(const std::string& host,
                  int port,
                  boost::function<void(const TestOutcome&, const std::string&, const std::string&) > testReportFn,
                  boost::function<const TestOutcome&() > testOutcomeFn)
        : m_port(port)
        , m_testReportFn(testReportFn)
        , m_testOutcomeFn(testOutcomeFn)
        , m_connection(karabo::net::Connection::create(karabo::util::Hash("Tcp.port", m_port, "Tcp.hostname", host))) {
        m_connection->startAsync(boost::bind(&WriteAsyncCli::connectHandler, this, _1, _2));
    }

private:
    int m_port;
    boost::function<void(const TestOutcome&, const std::string&, const std::string&) > m_testReportFn;
    boost::function<const TestOutcome&() > m_testOutcomeFn;
    karabo::net::Connection::Pointer m_connection;
    WriteAsyncTestsParams m_params;

    void connectHandler(const karabo::net::ErrorCode& ec, const karabo::net::Channel::Pointer& channel) {
        if (ec) {
            KARABO_LOG_FRAMEWORK_DEBUG << "\nWriteAsyncCli error: " << ec.value() << " -- " << ec.message();
            m_testReportFn(TestOutcome::FAILURE, ec.message(), "WriteAsync connection");
            if (channel) channel->close();
            return;
        }

        decltype(boost::chrono::high_resolution_clock::now()) startTime;
        decltype(boost::chrono::high_resolution_clock::now()) stopTime;
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

            startTime = boost::chrono::high_resolution_clock::now();
            channel->writeAsync(m_params.dataHashNDArray, m_params.writePriority, false);
            stopTime = boost::chrono::high_resolution_clock::now();
            startStopInterval = stopTime - startTime;
            std::clog << "[Cli]\t9. sent a hash with an NDArray as field with copyAllData false (in "
                    << boost::chrono::duration_cast<boost::chrono::milliseconds>(startStopInterval).count()
                    << "."
                    << boost::chrono::duration_cast<boost::chrono::microseconds>(startStopInterval).count() % 1000
                    << " milliseconds)."
                    << std::endl;
            startTime = boost::chrono::high_resolution_clock::now();
            channel->writeAsync(m_params.dataHashNDArray, m_params.writePriority, true);
            stopTime = boost::chrono::high_resolution_clock::now();
            startStopInterval = stopTime - startTime;
            std::clog << "[Cli]\t10. sent a hash with an NDArray as field with copyAllData true (in "
                    << boost::chrono::duration_cast<boost::chrono::milliseconds>(startStopInterval).count()
                    << "."
                    << boost::chrono::duration_cast<boost::chrono::microseconds>(startStopInterval).count() % 1000
                    << " milliseconds)."
                    << std::endl;

            channel->writeAsync(m_params.headerHash, m_params.charArray, m_params.charArraySize, m_params.writePriority);
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
            m_testReportFn(TestOutcome::FAILURE, ke.what() + std::string(": ") + ke.detailedMsg(), "WriteAsync sequence");
            if (channel) channel->close();
        } catch (std::exception& e) {
            std::clog << "Error during write sequence by the client: " << e.what() << std::endl;
            m_testReportFn(TestOutcome::FAILURE, e.what(), "WriteAsync sequence");
            if (channel) channel->close();
        }

        // The client has done its part; now keeps waiting for the server to do its (or to fail trying).
        constexpr unsigned int testTimeoutMs = 120000;
        const unsigned int sleepIntervalMs = 100;
        constexpr unsigned int maxSleeps = testTimeoutMs / sleepIntervalMs;
        unsigned int numOfSleeps = 0;
        do {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
            numOfSleeps++;
        } while (m_testOutcomeFn() == TestOutcome::UNKNOWN && numOfSleeps < maxSleeps);

        if (channel) channel->close();

        if (numOfSleeps >= maxSleeps) {
            m_testReportFn(TestOutcome::FAILURE,
                           "Test timed-out while waiting for server reads - more than 2 minutes elapsed.",
                           "Waiting for server reads.");
        }
    }
};

//</editor-fold>


TcpNetworking_Test::TcpNetworking_Test() {
    // To switch on logging:
    // #include "karabo/log/Logger.hh"
    // karabo::log::Logger::configure(karabo::util::Hash("priority", "DEBUG"));
    // karabo::log::Logger::useOstream();
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


void TcpNetworking_Test::testBufferSet() {
    using namespace karabo::util;
    using namespace karabo::net;

    auto thr = boost::thread(&EventLoop::work);

        // Create server with handler for connections
        auto serverCon = Connection::create("Tcp", Hash("type", "server"));

        Channel::Pointer serverChannel;
        std::string failureReasonServ;
            auto serverConnectHandler = [&serverChannel, &failureReasonServ] (const ErrorCode& ec, const Channel::Pointer& channel) {
                if (ec) {
                    failureReasonServ = "Server connection failed: " + toString(ec.value()) += " -- " + ec.message();
                    std::clog << failureReasonServ << std::endl;
                } else {
                    serverChannel = channel;
                }
            };
        const unsigned int serverPort = serverCon->startAsync(serverConnectHandler);
        CPPUNIT_ASSERT(serverPort != 0);

        // Create client, connect to server and validate connection
        Connection::Pointer clientConn = Connection::create("Tcp", Hash("type", "client",
                                                                        "port", serverPort));
        Channel::Pointer clientChannel;
        std::string failureReasonCli;
        auto clientConnectHandler = [&clientChannel, &failureReasonCli](const ErrorCode& ec, const Channel::Pointer & channel) {
            if (ec) {
                std::stringstream os;
                os << "\nClient connection failed: " << ec.value() << " -- " << ec.message();
                failureReasonCli = os.str();
                std::clog << failureReasonCli << std::endl;
            } else {
                clientChannel = channel;
            }
                };
        clientConn->startAsync(clientConnectHandler);

        int timeout = 10000;
        while (timeout >= 0) {
            if (clientChannel && serverChannel) break;
            boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
            timeout -= 10;
        }
        CPPUNIT_ASSERT_MESSAGE(failureReasonServ + ", timeout: " + toString(timeout), serverChannel);
        CPPUNIT_ASSERT_MESSAGE(failureReasonCli + ", timeout: " + toString(timeout), clientChannel);

        // Create Hash with many small NDArray: When it is sent that will create a lot of buffers, but due to an overall
        // rather small message it will likely go through the synchronous code path - and that was buggy
        // (using socket::read_some instead of asio::read) up to Karabo 2.7.0:
        Hash data;
        const int numNDarray = 500;
        for (int i = 0; i < numNDarray; ++i) {
            data.set(toString(i), NDArray(Dims(1ull), i));
        }
        auto serializer = karabo::io::BinarySerializer<Hash>::create("Bin");
        auto buffers = std::vector<karabo::io::BufferSet::Pointer>(1, boost::make_shared<karabo::io::BufferSet>());
        serializer->save(data, *(buffers[0])); // save into first BufferSet

        bool received = false;
        std::string failureReason;
        std::vector<karabo::io::BufferSet::Pointer> receivedBuffers;
        auto onRead = [&received, &failureReason, &receivedBuffers]
                (const boost::system::error_code& ec, const karabo::util::Hash& h,
                 const std::vector<karabo::io::BufferSet::Pointer>& bufs) {
            if (ec) {
                failureReason = toString(ec.value()) += " -- " + ec.message();
            } else {
                receivedBuffers = bufs;
            }
            received = true;
        };

        // Register handler, send data and wait until it arrived
        clientChannel->readAsyncHashVectorBufferSetPointer(onRead);
        serverChannel->write(Hash(), buffers); // synchronously with (empty) header

        timeout = 10000;
        while (timeout >= 0) {
            if (received) break;
            boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
            timeout -= 10;
        }

        CPPUNIT_ASSERT_MESSAGE("Failed to receive data, timeout " + toString(timeout), received);

        // Check that no failure and that content is as expected
        CPPUNIT_ASSERT_MESSAGE(failureReason, failureReason.empty());
        CPPUNIT_ASSERT_EQUAL(1ul, receivedBuffers.size());
        data.clear();
        serializer->load(data, *(receivedBuffers[0]));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(numNDarray), data.size());
        for (int i = 0; i < numNDarray; ++i) {
            const std::string key(toString(i));
            CPPUNIT_ASSERT_MESSAGE("Miss key " + toString(key), data.has(key));
            const NDArray& arr = data.get<NDArray>(key);
            CPPUNIT_ASSERT_EQUAL(1ul, arr.size());
            CPPUNIT_ASSERT_EQUAL(i, arr.getData<int>()[0]);
        }

        EventLoop::stop();
        thr.join();
}


void TcpNetworking_Test::testConsumeBytesAfterReadUntil() {
    using namespace karabo::util;
    using namespace karabo::net;

    auto thr = boost::thread(&EventLoop::work);

    // Create server with handler for connections
    auto serverCon = Connection::create("Tcp", Hash("type", "server",
                                                    "sizeofLength", 0));

    Channel::Pointer serverChannel;
    std::string failureReasonServ;
    auto serverConnectHandler =
            [&serverChannel, &failureReasonServ] (const ErrorCode& ec, const Channel::Pointer & channel) {
                if (ec) {
                    failureReasonServ = "Server connect failed: " + toString(ec.value()) += " -- " + ec.message();
                } else {
                    serverChannel = channel;
                }
            };
    const unsigned int serverPort = serverCon->startAsync(serverConnectHandler);
    CPPUNIT_ASSERT(serverPort != 0);

    // Create client, connect to server and validate connection
    Connection::Pointer clientConn = Connection::create("Tcp", Hash("sizeofLength", 0,
                                                                    "type", "client",
                                                                    "port", serverPort));
    Channel::Pointer clientChannel;
    std::string failureReasonCli;
    auto clientConnectHandler =
            [&clientChannel, &failureReasonCli](const ErrorCode& ec, const Channel::Pointer & channel) {
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
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
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
    auto readUntilHandler =
            [&clientChannel, &httpMsg, &readSeqCompleted, &failureReasonCli,
            &untilSep, &afterSep](const ErrorCode& ec, std::string msgRead) {
                if (ec) {
                    std::stringstream os;
                    os << "\nreadAsyncStringUntil failed: " + ec.message();
                    failureReasonCli = os.str();
                } else {
                    if (msgRead != untilSep) {
                        failureReasonCli =
                                "\nreadAsyncStringUntil result, '" + msgRead + "', differs from expected, '"
                                + untilSep + "'.";
                    } else {

                        const std::string afterSepStr = clientChannel->consumeBytesAfterReadUntil(afterSep.size());
                        if (afterSepStr != afterSep) {
                            failureReasonCli =
                                    "\nconsumeBytesAfterReadUntil result, '" + afterSepStr + "', differs from expected."
                                    + afterSep + "'.";
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
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        timeout -= 10;
    }

    // The order of the asserts is important: had the timeout assert come first, failureReasonCli would never be shown.
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
    decltype(boost::chrono::high_resolution_clock::now()) startTime;
    decltype(boost::chrono::high_resolution_clock::now()) finishTime;

    // Mutex for test results access
    boost::mutex testResultsMutex;

    // Setter for test results data.
    auto testReportFn = [&](const TestOutcome& outcome, const std::string& outcomeMessage,
                            const std::string & failTestCaseName) -> void {
                               boost::mutex::scoped_lock lock(testResultsMutex);
                               testOutcome = outcome;
                               testOutcomeMessage = outcomeMessage;
                               failingTestCaseName = failTestCaseName;
                               finishTime = boost::chrono::high_resolution_clock::now();
                           };

    // Getter for the current TestOutcome value.
    auto testOutcomeFn = [&]() -> const TestOutcome& {
        boost::mutex::scoped_lock lock(testResultsMutex);
        return testOutcome;
    };

    startTime = boost::chrono::high_resolution_clock::now();

    WriteAsyncSrv server(testReportFn);
    WriteAsyncCli client("localhost", server.port(), testReportFn, testOutcomeFn);

    karabo::net::EventLoop::addThread(1);
    karabo::net::EventLoop::run();
    karabo::net::EventLoop::removeThread(1);

    if (testOutcomeFn() == TestOutcome::SUCCESS) {
        auto testDuration = finishTime - startTime;
        std::clog << "Test took "
                << boost::chrono::duration_cast<boost::chrono::milliseconds>(testDuration).count()
                << " milliseconds." << std::endl;
        CPPUNIT_ASSERT(true);
    } else {
        CPPUNIT_ASSERT_MESSAGE("Failed:\n---------------\n" + testOutcomeMessage +
                               "\n---------------\nat test: " + failingTestCaseName, false);
    }

}
