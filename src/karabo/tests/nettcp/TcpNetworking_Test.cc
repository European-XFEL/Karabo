/*
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Created on Oct 30, 2012, 1:33:46 PM
 */

#include "TcpNetworking_Test.hh"

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


namespace karabo {
    namespace tcpclientserver {


        TcpClientServer::TcpClientServer(const string& rhost, unsigned short rport, unsigned short lport) :
        m_remoteCount(0), m_localCount(0), m_remoteHost(rhost), m_remotePort(rport), m_localPort(lport) {
        }


        TcpClientServer::~TcpClientServer() {
        }


        void TcpClientServer::run() {
            IOService::Pointer io(new IOService);
            Hash sconf("Tcp.port", m_localPort, "Tcp.type", "server", "Tcp.IOService", io);
            sconf.setFromPath("Tcp.sizeofLength", 4);
            sconf.setFromPath("Tcp.messageTagIsText", false);
            m_serverConnection = Connection::create(sconf);
            m_serverConnection->startAsync(boost::bind(&TcpClientServer::serverConnectHandler, this, _1));
            Hash cconf("Tcp.port", m_remotePort, "Tcp.hostname", m_remoteHost, "Tcp.IOService", io);
            cconf.setFromPath("Tcp.sizeofLength", 4);
            cconf.setFromPath("Tcp.messageTagIsText", false);
            m_clientConnection = Connection::create(cconf);
            m_clientConnection->setErrorHandler(boost::bind(&TcpClientServer::clientConnectionErrorHandler, this, _1, _2));
            m_clientConnection->startAsync(boost::bind(&TcpClientServer::clientConnectHandler, this, _1));
            io->run();
        }


        void TcpClientServer::serverConnectHandler(Channel::Pointer channel) {
            channel->setErrorHandler(boost::bind(&TcpClientServer::serverErrorHandler, this, _1, _2));
            channel->readAsyncHash(boost::bind(&TcpClientServer::serverReadHashHandler, this, _1, _2));
        }


        void TcpClientServer::clientConnectHandler(Channel::Pointer channel) {
            channel->setErrorHandler(boost::bind(&TcpClientServer::clientErrorHandler, this, _1, _2));
            Hash data("a.b", "?", "a.c", 42.22f, "a.d", 12);
            channel->writeAsyncHash(data, boost::bind(&TcpClientServer::clientWriteCompleteHandler, this, _1));
        }


        void TcpClientServer::clientConnectionErrorHandler(karabo::net::Channel::Pointer channel, const std::string& errmsg) {
            //cout << "CLIENT_ERROR: Failed to connect to remote server. Sleep and try again\n";
            boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
            channel->close();
            m_clientConnection->startAsync(boost::bind(&TcpClientServer::clientConnectHandler, this, _1));
        }


        void TcpClientServer::serverErrorHandler(Channel::Pointer channel, const string& errmsg) {
            CPPUNIT_ASSERT(errmsg == "End of file and transferred 0 bytes.");
            channel->close();
        }


        void TcpClientServer::clientErrorHandler(Channel::Pointer channel, const string& errmsg) {
            channel->close();
            CPPUNIT_FAIL(errmsg);
        }


        void TcpClientServer::serverReadHashHandler(Channel::Pointer channel, const Hash& hash) {
            m_localCount++;
            //cout << "SERVER_INFO: count " << m_localCount << "\n" << hash << "-----------------\n";
            CPPUNIT_ASSERT(hash.hasFromPath("a.d") == true);
            CPPUNIT_ASSERT(hash.isFromPath("a.c", Types::FLOAT) == true);
            CPPUNIT_ASSERT(hash.isFromPath<float>("a.c") == true);
            string john = hash.getFromPath<string>("a.b");
            CPPUNIT_ASSERT(john == "John Doe" || john == "?");
            //cout << "SERVER_INFO: hash.getFromPath<string>(\"a.b\") = '" << john << "'" << endl;
            Hash data(hash);
            if (data.empty())
                data.setFromPath("a.e", "server data");
            else {
                if (data.has("a") && data.getFromPath<string > ("a.b") == "?")
                    data.setFromPath("a.b", "server reply");
                else
                    data.setFromPath("a.b", "counter " + String::toString(m_localCount));
            }
            channel->writeAsyncHash(data, boost::bind(&TcpClientServer::serverWriteCompleteHandler, this, _1));
        }


        void TcpClientServer::clientReadHashHandler(karabo::net::Channel::Pointer channel, const karabo::util::Hash& data) {
            m_remoteCount++;
            if (m_remoteCount > 5) {
                channel->close();
                CPPUNIT_ASSERT(m_remoteCount == 6);
                return;
            }
            channel->waitAsync(200, boost::bind(&TcpClientServer::timerHandler, this, channel));
        }


        void TcpClientServer::serverWriteCompleteHandler(karabo::net::Channel::Pointer channel) {
            channel->readAsyncHash(boost::bind(&TcpClientServer::serverReadHashHandler, this, _1, _2));
        }


        void TcpClientServer::clientWriteCompleteHandler(karabo::net::Channel::Pointer channel) {
            channel->readAsyncHash(boost::bind(&TcpClientServer::clientReadHashHandler, this, _1, _2));
        }


        void TcpClientServer::timerHandler(karabo::net::Channel::Pointer channel) {
            Hash data;
            data.setFromPath("a.b", "John Doe");
            data.setFromPath("a.c", 1.0f * (static_cast<unsigned> (::rand()) % 1000));
            data.setFromPath("a.d", static_cast<int> (::rand()) % 100);
            vector<unsigned char> pixels;
            data.setFromPath("a.v", pixels);
            vector<unsigned char>& x = data.getFromPath<vector<unsigned char> >("a.v");
            for (int i = 1; i <= 20; i++) x.push_back(static_cast<unsigned char> (i % 256));
            channel->writeAsyncHash(data, boost::bind(&TcpClientServer::clientWriteCompleteHandler, this, _1));
        }
    }
}


void TcpNetworking_Test::testMethod() {
    try {
        std::string host("localhost");
        unsigned short rport = 11111;
        unsigned short lport = 11111;
        karabo::tcpclientserver::TcpClientServer cserv(host, rport, lport);
        cserv.run();
    } catch (const Exception& e) {
        RETHROW
    }
}



