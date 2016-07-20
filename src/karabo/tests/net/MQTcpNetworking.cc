/*
 * File:   MQTcpNetworking.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on Sep 22, 2015, 3:06:16 PM
 */
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <karabo/util/Hash.hh>
#include <karabo/net/Channel.hh>
#include "MQTcpNetworking.hh"


CPPUNIT_TEST_SUITE_REGISTRATION(MQTcpNetworking);


MQTcpNetworking::MQTcpNetworking() : m_numberOfMessages(10000), m_serverCount(0), m_serverPort(0) {
    using namespace std;
    using namespace karabo::util;

    Hash tmp("a.b.c", 1, "a.b.d", vector<int>(5, 1), "a.b.e", vector<Hash > (2, Hash("a", 1)), "a.d", std::complex<double>(1.2, 4.2));
    tmp.setAttribute("a", "a1", true);
    tmp.setAttribute("a", "a2", 3.4);
    tmp.setAttribute("a.b", "b1", "3");
    tmp.setAttribute("a.b.c", "c1", 2);
    tmp.setAttribute("a.b.c", "c2", vector<string > (3, "bla"));
    m_data = tmp;

    Hash tmp2("policy", "LOSSLESS");
    m_header = tmp2;
}


MQTcpNetworking::~MQTcpNetworking() {
}


void MQTcpNetworking::createServer() {
    m_serverConnection = karabo::net::Connection::create(karabo::util::Hash("Tcp.port", 0, "Tcp.type", "server"));
    std::clog << "SERVER: connection object created. " << std::endl;
    m_serverPort = m_serverConnection->startAsync(boost::bind(&MQTcpNetworking::serverConnectHandler, this, _1));
    std::clog << "SERVER: the allocated port is " << m_serverPort << std::endl;
}


void MQTcpNetworking::serverConnectHandler(const karabo::net::Channel::Pointer& channel) {
    std::clog << "SERVER: connected" << std::endl;
    channel->setErrorHandler(boost::bind(&MQTcpNetworking::serverErrorHandler, this, channel, _1));
    channel->readAsyncHashHash(boost::bind(&MQTcpNetworking::serverReadHashHashHandler, this, channel, _1, _2));
}


void MQTcpNetworking::serverRun() {
    karabo::net::IOService::Pointer io = m_serverConnection->getIOService();
    io->run();
}


void MQTcpNetworking::serverErrorHandler(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode& ec) {
    if (ec.value() == 2) {
        std::clog << "\nSERVER: client has closed the connection!" << std::endl;
    } else {
        std::clog << "\nSERVER_ERROR: " << ec.value() << " -- " << ec.message() << std::endl;
    }
    channel->close();
}


void MQTcpNetworking::serverReadHashHashHandler(const karabo::net::Channel::Pointer& channel, karabo::util::Hash& header, karabo::util::Hash& body) {
    std::clog << "\nSERVER : Request comes...\n" << header << body << "-----------------\n";
    channel->readAsyncHashHash(boost::bind(&MQTcpNetworking::serverReadHashHashHandler, this, channel, _1, _2));

    if (body.has("START")) {
        m_numberOfMessages = body.get<int>("START");
        std::clog << "\nSERVER:  CLIENT sent START command with counter = " << m_numberOfMessages << std::endl;
        m_serverCount = 0;
        m_ts = boost::posix_time::second_clock::local_time();
        channel->writeAsync(m_header, m_data);
        m_serverCount++;
        channel->getConnection()->getIOService()->post(boost::bind(&MQTcpNetworking::serverPublish, this, channel));
    } else if (body.has("STOP")) {
        std::clog << "\nSERVER:  CLIENT requests exiting together!\n" << std::endl;
    }
}


void MQTcpNetworking::serverPublish(const karabo::net::Channel::Pointer& channel) {
    channel->writeAsync(m_header, m_data);
    m_serverCount++;
    if (m_serverCount < m_numberOfMessages)
        channel->getConnection()->getIOService()->post(boost::bind(&MQTcpNetworking::serverPublish, this, channel));
    else {
        boost::posix_time::ptime t = boost::posix_time::second_clock::local_time();
        boost::posix_time::time_duration diff = t - m_ts;
        std::clog << "SERVER : " << diff.total_milliseconds() << " ms,  publishing rate = "
                << double(m_serverCount) / diff.total_milliseconds() << " per ms" << std::endl;
    }
}


void MQTcpNetworking::testClientServerMethod() {
    m_connection = karabo::net::Connection::create(karabo::util::Hash("Tcp.port", m_serverPort, "Tcp.hostname", "localhost"));
    m_connection->setErrorHandler(boost::bind(&MQTcpNetworking::clientConnectionErrorHandler, this, m_connection, _1));
    m_connection->startAsync(boost::bind(&MQTcpNetworking::onClientConnected, this, _1));
    karabo::net::IOService::Pointer io = m_connection->getIOService();
    io->run();
}


void MQTcpNetworking::setUp() {
    try {
        createServer();
    } catch (const std::exception& e) {
        std::clog << "SETUP exception: " << e.what() << std::endl;
        return;
    }
    m_serverThread = boost::thread(boost::bind(&MQTcpNetworking::serverRun, this));
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    //boost::this_thread::yield();
}


void MQTcpNetworking::tearDown() {
    m_serverThread.join();
    std::clog << "CLIENT: server joined." << std::endl;
}


void MQTcpNetworking::clientConnectionErrorHandler(const karabo::net::Connection::Pointer& connection, const karabo::net::ErrorCode& ec) {
    std::clog << "\nCLIENT : Failed to connect to remote server. Stop...\n";
    boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
    connection->stop();
}


void MQTcpNetworking::onClientConnected(const karabo::net::Channel::Pointer& channel) {
    channel->setErrorHandler(boost::bind(&MQTcpNetworking::clientChannelErrorHandler, this, channel, _1));
    karabo::util::Hash header("headline", "*** CLIENT ***");
    karabo::util::Hash data("START", 10000);

    // first sending
    channel->writeAsync(header, data);
    m_clientCount = 0;
    m_clientTimestamp = boost::posix_time::second_clock::local_time();
    channel->readAsyncHashHash(boost::bind(&MQTcpNetworking::clientReadHashHashHandler, this, channel, _1, _2));
}


void MQTcpNetworking::clientChannelErrorHandler(const karabo::net::Channel::Pointer& channel, const karabo::net::ErrorCode& ec) {
    // Check if it is End-Of-File
    if (ec.value() == 2) {
        //std::clog << "\nCLIENT: server has closed the connection!" << std::endl;
    } else {
        std::clog << "\nCLIENT ERROR: " << ec.value() << " -- " << ec.message() << std::endl;
    }
    channel->close();
}


void MQTcpNetworking::clientReadHashHashHandler(const karabo::net::Channel::Pointer& channel, karabo::util::Hash& header, karabo::util::Hash& body) {
    // inspect here the server reply.... just count
    m_clientCount++;
    if (m_clientCount < m_numberOfMessages) {

        channel->readAsyncHashHash(boost::bind(&MQTcpNetworking::clientReadHashHashHandler, this, channel, _1, _2));
    } else {
        karabo::util::Hash header("headline", "*** CLIENT ***");
        karabo::util::Hash data("Stop", karabo::util::Hash());
        channel->writeAsync(header, data);
        channel->getConnection()->getIOService()->post(boost::bind(&MQTcpNetworking::onClientEnd, this, channel));
    }

}


void MQTcpNetworking::onClientEnd(const karabo::net::Channel::Pointer& channel) {
    boost::posix_time::ptime t = boost::posix_time::second_clock::local_time();
    boost::posix_time::time_duration diff = t - m_clientTimestamp;
    double rate = double(m_clientCount) / diff.total_milliseconds();
    std::clog << "CLIENT Summary : " << diff.total_milliseconds() << " ms, rate = " << rate << " 1/ms" << std::endl;
    channel->close();
}
