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


struct WriteAsyncTestsParams {
    const karabo::util::Hash dataHash = karabo::util::Hash("Name", "DataHash", "PiField", 3.14159);
    const karabo::util::Hash dataHashNDArray =
            karabo::util::Hash("Data", karabo::util::NDArray(karabo::util::Dims(10000, 40000), 1000u));
    const std::string dataString = std::string("Sample of std::string");
    const karabo::util::Hash headerHash = karabo::util::Hash("Header", "hdr", "NumOfFields", 3, "required", true);
    const karabo::net::VectorCharPointer vectorCharPointer =
            boost::make_shared<std::vector<char>>(std::vector<char>(10, 'A'));
    const std::vector<char> vectorChar = std::vector<char>(20, 'B');
    const char* charArray = "An array of char";
    const int writePriority = 4;
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
        if (hash != m_params.dataHash) {
            m_testReportFn(TestOutcome::FAILURE, "Hash read differs from hash written.", "readAsyncHashHandlerCopyFalse");
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
        if (hash != m_params.dataHash) {
            m_testReportFn(TestOutcome::FAILURE, "Hash read differs from hash written.", "readAsyncHashHandlerCopyTrue");
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
            m_testReportFn(TestOutcome::FAILURE, "String read differs from string written.", "readAsyncStringHandler");
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
        if (header != m_params.headerHash || body != m_params.dataHash) {
            m_testReportFn(TestOutcome::FAILURE, "Hashes read don't match the ones written.", "readAsyncHashHashHandlerCopyFalse");
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
        if (header != m_params.headerHash || body != m_params.dataHash) {
            m_testReportFn(TestOutcome::FAILURE, "Hashes read don't match the ones written.", "readAsyncHashHashHandlerCopyTrue");
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
        if (vector.size() != strlen(m_params.charArray) || vector[0] != m_params.charArray[0]) {
            m_testReportFn(TestOutcome::FAILURE, "Vector read doesn't match the one written.", "readAsyncVectorHandler");
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
        if (vectorCharPointer->size() != m_params.vectorCharPointer->size() ||
            (*vectorCharPointer)[0] != (*m_params.vectorCharPointer)[0]) {
            m_testReportFn(TestOutcome::FAILURE, "Vector read doesn't match the one written.", "readAsyncVectorPointerHandler");
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
        if (header != m_params.headerHash ||
            data->size() != m_params.vectorCharPointer->size()) {
            m_testReportFn(TestOutcome::FAILURE, "Data read doesn't match the data written.", "readAsyncHashVectorPointerHandler");
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
        if (dataHash.size() != m_params.dataHashNDArray.size() ||
            dataHash.get<karabo::util::NDArray>("Data").size() != m_params.dataHashNDArray.get<karabo::util::NDArray>("Data").size()) {
            m_testReportFn(TestOutcome::FAILURE, "Hash read doesn't match the hash written.", "readAsyncHashNDArrayHandlerCopyFalse");
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
        if (dataHash.size() != m_params.dataHashNDArray.size() ||
            dataHash.get<karabo::util::NDArray>("Data").size() != m_params.dataHashNDArray.get<karabo::util::NDArray>("Data").size()) {
            m_testReportFn(TestOutcome::FAILURE, "Hash read doesn't match the hash written.", "readAsyncHashNDArrayHandlerCopyTrue");
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

        if (headerHash != m_params.headerHash || dataVect.size() != strlen(m_params.charArray)) {
            m_testReportFn(TestOutcome::FAILURE, "Data read doesn't match the data written.", "readAysncHashCharArrayHandler");
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

        if (headerHash != m_params.headerHash || dataStr != m_params.dataString) {
            m_testReportFn(TestOutcome::FAILURE, "Data read doesn't match the data written.", "readAsyncHashStringHandler");
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

        if (headerHash != m_params.headerHash || dataVect.size() != m_params.vectorChar.size()) {
            m_testReportFn(TestOutcome::FAILURE, "Data read doesn't match the data written.", "readAsyncHashVectorHandler");
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

        if (dataVect.size() != m_params.vectorChar.size()) {
            m_testReportFn(TestOutcome::FAILURE, "Data read doesn't match the data written.", "readAsyncVectorHandler");
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

            channel->writeAsync(m_params.charArray, strlen(m_params.charArray), m_params.writePriority);
            std::clog << "[Cli]\t6. sent an array of char as body." << std::endl;

            channel->writeAsync(m_params.vectorCharPointer, m_params.writePriority);
            std::clog << "[Cli]\t7. sent a VectorCharPointer as body." << std::endl;

            channel->writeAsync(m_params.headerHash, m_params.vectorCharPointer, m_params.writePriority);
            std::clog << "[Cli]\t8. sent a hash for header and VectorCharPointer for body." << std::endl;

            channel->writeAsync(m_params.dataHashNDArray, m_params.writePriority, false);
            std::clog << "[Cli]\t9. sent a hash with an NDArray as field with copyAllData false." << std::endl;
            channel->writeAsync(m_params.dataHashNDArray, m_params.writePriority, true);
            std::clog << "[Cli]\t10. sent a hash with an NDArray as field with copyAllData true." << std::endl;

            channel->writeAsync(m_params.headerHash, m_params.charArray, strlen(m_params.charArray), m_params.writePriority);
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
        boost::mutex::scoped_lock(testResultsMutex);
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
        CPPUNIT_ASSERT_MESSAGE("Failed: " + testOutcomeMessage + " at " + failingTestCaseName, false);
    }

}