/* 
 * File:   ReadAsyncStringUntil_Test.cc
 * Author: giovanet
 * 
 * Created on May 16, 2018, 1:46 PM
 */

#include "ReadAsyncStringUntil_Test.hh"

#include "karabo/net/Connection.hh"
#include "karabo/net/Channel.hh"
#include "karabo/net/EventLoop.hh"

#include <karabo/karabo.hpp>

#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>

#include <iostream>
#include <fstream>
#include <cassert>
#include <iosfwd>


using boost::asio::ip::tcp;
using namespace std;


CPPUNIT_TEST_SUITE_REGISTRATION(ReadAsyncStringUntil_Test);


#define MAX_DATA_LEN 1024

struct EchoServer {
    EchoServer(unsigned short port) : m_port(port) {
   
    }
    
    void loopback(tcp::socket sock)
    {
      try
      {
        while(1) {
          char data[MAX_DATA_LEN];

          boost::system::error_code error;
          size_t length = sock.read_some(boost::asio::buffer(data), error);
          if (error == boost::asio::error::eof){
            break; // Connection closed cleanly by peer.
          }
          else if (error)
            throw boost::system::system_error(error); // Some other error.

          //boost::this_thread::sleep(boost::posix_time::milliseconds(100));      
          boost::asio::write(sock, boost::asio::buffer(data, length));
        }
      }
      catch (std::exception& e)
      {
        std::cerr << "Exception in thread: " << e.what() << "\n";
      }
    }

    void listen() {
      boost::asio::io_context io_context;

      tcp::acceptor a(io_context, tcp::endpoint(tcp::v4(), m_port));

      loopback(a.accept());
    }

    void start() {
        m_listen_thr_pnt = new std::thread(&EchoServer::listen, this);
    }
    
    void join_all() {
        m_listen_thr_pnt->join();
    }
    
    private:
        unsigned short m_port;
        std::thread* m_listen_thr_pnt;
};

struct TestClient {

    KARABO_CLASSINFO(TestClient, "TestClient", "1.0");

    TestClient(const std::string& host, int port, karabo::util::Hash cfg)
        : m_port(port)
        , m_connection(karabo::net::Connection::create("Tcp", cfg))
        , m_deadline(karabo::net::EventLoop::getIOService())
        , m_terminator("\r\n")
        , m_expected("") { //                                                           timeout repetition channel ec 
        m_connection->startAsync(boost::bind(&TestClient::connectHandler, this, _1, 1000, 3, _2));
    }

    virtual ~TestClient() {
    }

    void connectHandler(const karabo::net::ErrorCode& ec, int timeout, int repetition, const karabo::net::Channel::Pointer& channel) {
        if (ec) {
            CPPUNIT_FAIL("Error connecting");
            return;
        }
        
        m_channel = channel;

        const std::string& tmp = "When the going gets tough... the tough get going\r\n";   
        m_terminator = {"..."};
        m_expected = {"When the going gets tough..."};

        boost::shared_ptr<vector <char>> datap(new vector <char>());
        datap->assign(tmp.begin(), tmp.end());

        m_channel->writeAsyncVectorPointer(datap, boost::bind(&TestClient::writeCompleteHandler, this, _1));
    }

    void writeCompleteHandler(const karabo::net::ErrorCode& errorCode) {
        if(errorCode) {
            CPPUNIT_FAIL("error on write");
            m_channel->close();
        }

        m_repetition = 1;
        m_channel->readAsyncStringUntil(m_terminator, boost::bind(&TestClient::readStringHandler, this, _1, _2));

    }

    void readStringHandler(const karabo::net::ErrorCode& ec, std::string& read_str) {
        if (ec) {
            CPPUNIT_FAIL("Error reading");
            m_channel->close();
            return;
        }

        CPPUNIT_ASSERT_EQUAL(read_str,  m_expected);
        
        if(m_repetition--) {

            const std::string& tmp = "Another test string\r\n";   
            m_terminator = {"\r\n"};
            m_expected = {"Another test string\r\n"};
            boost::shared_ptr<vector <char>> datap(new vector <char>());
            datap->assign(tmp.begin(), tmp.end());
            m_channel->writeAsyncVectorPointer(datap, boost::bind(&TestClient::writeCompleteHandler, this, _1));

        }
        
        if(m_repetition <= 0) {
            m_channel->close();
            return;
        }
    }

private:
    int m_port;
    karabo::net::Connection::Pointer m_connection;
    karabo::net::Channel::Pointer m_channel;
    boost::asio::deadline_timer m_deadline;
    int m_timeout;
    int m_repetition;
    std::string m_terminator;
    std::string m_expected;
};


ReadAsyncStringUntil_Test::ReadAsyncStringUntil_Test() {
}

ReadAsyncStringUntil_Test::~ReadAsyncStringUntil_Test() {
}

void ReadAsyncStringUntil_Test::setUp() {
}

void ReadAsyncStringUntil_Test::tearDown() {
}

void ReadAsyncStringUntil_Test::runTest() {
    using namespace std;
    const unsigned short test_port = 55555;

    int nThreads = karabo::net::EventLoop::getNumberOfThreads();
    CPPUNIT_ASSERT(nThreads == 0);

    karabo::util::Hash connectionCfg;
    connectionCfg.set("hostname", "localhost");
    connectionCfg.set("port", test_port);
    connectionCfg.set("type", "client");
    connectionCfg.set("sizeofLength", 0); //default is 4

    EchoServer srv(test_port);
    srv.start();
    
    TestClient client("localhost", test_port, connectionCfg);
    
    nThreads = karabo::net::EventLoop::getNumberOfThreads();
    CPPUNIT_ASSERT(nThreads == 0);
    
    karabo::net::EventLoop::run();

    nThreads = karabo::net::EventLoop::getNumberOfThreads();
    CPPUNIT_ASSERT(nThreads == 0);
    srv.join_all();
}
