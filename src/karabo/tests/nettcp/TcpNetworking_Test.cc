/*
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Created on Oct 30, 2012, 1:33:46 PM
 */

#include "TcpNetworking_Test.hh"
#include <boost/assert.hpp>

using namespace std;
using namespace karabo::util;
using namespace karabo::net;

CPPUNIT_TEST_SUITE_REGISTRATION(TcpNetworking_Test);

TcpNetworking_Test::TcpNetworking_Test() {
}

TcpNetworking_Test::~TcpNetworking_Test() {
}

void TcpNetworking_Test::setUp() {
}

void TcpNetworking_Test::tearDown() {
}

//////////////  TcpServer implementation  /////////////////////////

namespace karabo {
    namespace nettcp {
        
        TcpServer::TcpServer() : m_count(0), m_port(0) {
            m_connection = Connection::create(Hash("Tcp.port", 0, "Tcp.type", "server"));
            m_port = m_connection->startAsync(boost::bind(&TcpServer::connectHandler, this, _1));
        }
        
        TcpServer::~TcpServer() {}

        void TcpServer::run() {
            IOService::Pointer io = m_connection->getIOService();
            io->run();
        }

        void TcpServer::errorHandler(const Channel::Pointer& channel, const ErrorCode& ec) {
            if (ec.value() == 2) {
                cout << "SERVER: client has closed the connection!" << endl;
            } else {
                cout << "SERVER_ERROR: " << ec.value() << " -- " << ec.message() << endl;
            }
            channel->close();
        }

        void TcpServer::connectHandler(const Channel::Pointer& channel) {
            channel->setErrorHandler(boost::bind(&TcpServer::errorHandler, this, channel, _1));
            channel->readAsyncHashHash(boost::bind(&TcpServer::readHashHashHandler, this, channel, _1, _2));
        }

        void TcpServer::readHashHashHandler(const Channel::Pointer& channel, Hash& header, Hash& body) {
            m_count++;
            cout << "SERVER_INFO: count " << m_count << "\n" << header << body << "-----------------\n";

            //CPPUNIT_ASSERT(header.get<string>("headline") == "*** CLIENT ***");
            //assert(header.get<string>("headline") == "*** CLIENT ***", std::string("get(headline) gives \"") + header.get<string>("headline") + "\"");
            BOOST_ASSERT(header.get<string>("headline") == "*** CLIENT ***");
            
            header.set("headline", "----- SERVER -----");
            
            body.set("a.e", "server data");
            
            if (body.has("a") && body.get<string > ("a.b") == "?")
                body.set("a.b", "server reply");
            else
                body.set("a.b", "counter " + toString(m_count));
            

            channel->writeAsyncHashHash(header, body, boost::bind(&TcpServer::writeCompleteHandler, this, channel, "some string"));
        }


        void TcpServer::writeCompleteHandler(const karabo::net::Channel::Pointer& channel, const std::string& id) {
            //CPPUNIT_ASSERT(id == "some string");
            //assert(id == "some string", std::string("assert failure: id = \"") + id + "\"");
            BOOST_ASSERT(id == "some string");
            channel->readAsyncHashHash(boost::bind(&TcpServer::readHashHashHandler, this, channel, _1, _2));
        }
        

//////////////////  TcpClient implementation  ///////////////////////////////

        TcpClient::TcpClient(const std::string& host, int port) : m_count(0), m_host(host), m_port(port) {}
        
        TcpClient::~TcpClient() {}
        
        void TcpClient::run() {
            m_connection = Connection::create(Hash("Tcp.port", m_port, "Tcp.hostname", m_host));
            //m_connection->setErrorHandler(boost::bind(&TcpClient::connectionErrorHandler, this, m_connection, _1));
            m_connection->startAsync(boost::bind(&TcpClient::connectHandler, this, _1));

            IOService::Pointer io = m_connection->getIOService();
            io->run();
        }


        void TcpClient::connectHandler(const Channel::Pointer& channel) {
            channel->setErrorHandler(boost::bind(&TcpClient::errorHandler, this, channel, _1));
            Hash header("headline", "*** CLIENT ***");
            Hash data("a.b", "?", "a.c", 42.22f, "a.d", 12);
            
            // first sending
            channel->writeAsyncHashHash(header, data, boost::bind(&TcpClient::writeCompleteHandler, this, channel, 42));
        }


        void TcpClient::connectionErrorHandler(const karabo::net::Connection::Pointer& connection, const ErrorCode& ec) {
            cout << "CLIENT_ERROR: Failed to connect to remote server. Stop...\n";
            boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
            connection->stop();
        }


        void TcpClient::errorHandler(const Channel::Pointer& channel, const ErrorCode& ec) {
            // Check if it is End-Of-File
            if (ec.value() == 2) {
                cout << "CLIENT: server has closed the connection!" << endl;
            } else {
                cout << "CLIENT_ERROR: " << ec.value() << " -- " << ec.message() << endl;
            }
            channel->close();
        }


        void TcpClient::readHashHashHandler(const karabo::net::Channel::Pointer& channel, karabo::util::Hash& header, karabo::util::Hash& body) {
            // inspect here the server reply.... just count
            m_count++;
            if (m_count >= 3) { // stop after 15 attempts
                channel->close();
                return;
            }

            BOOST_ASSERT(header.get<string>("headline") == "----- SERVER -----");
            if (body.has("a.e")) {
                BOOST_ASSERT(body.get<string>("a.e") == "server data");
                body.erase("a.e");
            }
            
            header.set("headline", "*** CLIENT ***");
            
            // Prepare new data
            body.set("a.b", "John Doe");
            body.set("a.c", 1.0f * (static_cast<unsigned int> (::rand()) % 1000));
            body.set("a.d", static_cast<int> (::rand()) % 100);
            vector<unsigned char> pixels;
            body.set("a.v", pixels);
            vector<unsigned char>& x = body.get<vector<unsigned char> >("a.v");
            for (int i = 1; i <= 20; i++) x.push_back(static_cast<unsigned char> (i % 256));

            // send client data asynchronous: define "write" completion handler
            channel->writeAsyncHashHash(header, body, boost::bind(&TcpClient::writeCompleteHandler, this, channel, 42));
        }


        void TcpClient::writeCompleteHandler(const karabo::net::Channel::Pointer& channel, int id) {
            BOOST_ASSERT(id == 42);
            // data was sent successfully! Prepare to read a reply asynchronous from a server: placeholder _1 is a Hash
            channel->readAsyncHashHash(boost::bind(&TcpClient::readHashHashHandler, this, channel, _1, _2));
        }
        
    }
}

void TcpNetworking_Test::testMethod() {
    try {
        int port = 0;
        string host = "localhost";

        karabo::nettcp::TcpServer server;
        port = server.getPort();
        boost::thread server_thread(boost::bind(&karabo::nettcp::TcpServer::run, &server));
        boost::this_thread::sleep(boost::posix_time::milliseconds(1500));
        
        cout << "testMethod: port is " << port << endl;
        karabo::nettcp::TcpClient client(host,port);
        client.run();
        
        server_thread.join();
    } catch (const std::exception& e) {
        std::cerr << "\n\ntestMethod:   ERROR: " << e.what() << std::endl;
        CPPUNIT_FAIL(e.what());
    } catch (...) {
        KARABO_RETHROW
    }
}



