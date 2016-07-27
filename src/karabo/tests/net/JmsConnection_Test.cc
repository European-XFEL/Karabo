/* 
 * File:   JmsConnection_Test.cc
 * Author: heisenb
 * 
 * Created on July 15, 2016, 5:05 PM
 */

#include "JmsConnection_Test.hh"
#include "karabo/net/JmsChannel.hh"
#include "karabo/net/IOService.hh"
#include "karabo/log/Logger.hh"
#include <karabo/net/JmsConnection.hh>
#include <boost/parameter/aux_/parameter_requirements.hpp>

using namespace karabo::util;
using namespace karabo::net;

CPPUNIT_TEST_SUITE_REGISTRATION(JmsConnection_Test);


JmsConnection_Test::JmsConnection_Test() {

    karabo::log::Logger::configure();

}


JmsConnection_Test::~JmsConnection_Test() {
}


void JmsConnection_Test::testConnect() {

    {
        m_connection = JmsConnection::Pointer(new JmsConnection());
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
        m_connection->connect();
        CPPUNIT_ASSERT(m_connection->isConnected() == true);
        m_connection->disconnect();
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
        m_connection->connect();
        CPPUNIT_ASSERT(m_connection->isConnected() == true);
    }

    {
        m_connection = JmsConnection::Pointer(new JmsConnection("tcp://someBadHost:7777,tcp://exfl-broker.desy.de:7777"));
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
        m_connection->connect();
        CPPUNIT_ASSERT(m_connection->isConnected() == true);
        CPPUNIT_ASSERT(m_connection->getBrokerUrl() == "tcp://exfl-broker.desy.de:7777");
        m_connection->disconnect();
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
    }

    {
        putenv("KARABO_BROKER=tcp://exfl-broker.desy.de:7777");
        m_connection = JmsConnection::Pointer(new JmsConnection("tcp://someBadHost:7777"));
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
        m_connection->connect();
        CPPUNIT_ASSERT(m_connection->isConnected() == true);
        CPPUNIT_ASSERT(m_connection->getBrokerUrl() == "tcp://exfl-broker.desy.de:7777");
        m_connection->disconnect();
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
        unsetenv("KARABO_BROKER");
    }
}


void JmsConnection_Test::readHandler(karabo::net::JmsChannel::Pointer channel,
                                     karabo::util::Hash::Pointer header,
                                     karabo::util::Hash::Pointer body) {


    m_counter++;
    if (m_counter == 1) tick = boost::posix_time::microsec_clock::local_time();
    if (m_counter == 1001) {
        boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::local_time() - tick;
        clog << diff.total_milliseconds() << std::endl;
        throw KARABO_PARAMETER_EXCEPTION("test");
    }
    channel->readAsync(boost::bind(&JmsConnection_Test::readHandler, this, channel, _1, _2), "heisen");
    channel->write("heisen", header, body);
}


void JmsConnection_Test::testChannel() {

    boost::shared_ptr<boost::asio::io_service> ioService(new boost::asio::io_service());
    boost::asio::io_service::work work(*ioService);
    boost::thread t(boost::bind(&boost::asio::io_service::run, ioService));

    m_connection = JmsConnection::Pointer(new JmsConnection());
    m_connection->connect();

    JmsChannel::Pointer channel = m_connection->createChannel(ioService);

    channel->readAsync(boost::bind(&JmsConnection_Test::readHandler, this, channel, _1, _2), "heisen");

    boost::this_thread::sleep(boost::posix_time::millisec(100));

    Hash::Pointer header(new Hash("header", "shit"));

    Hash::Pointer body(new Hash("body", std::vector<char>(10000, 't')));

    m_counter = 0;
    channel->write("heisen", header, body);    


    t.join();
}

