/* 
 * File:   JmsConnection_Test.cc
 * Author: heisenb
 * 
 * Created on July 15, 2016, 5:05 PM
 */

#include "JmsConnection_Test.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/JmsConnection.hh"
#include "karabo/net/JmsConsumer.hh"
#include "karabo/net/JmsProducer.hh"
#include "karabo/log/Logger.hh"

#include <boost/thread.hpp>

using namespace karabo::util;
using namespace karabo::net;

CPPUNIT_TEST_SUITE_REGISTRATION(JmsConnection_Test);


JmsConnection_Test::JmsConnection_Test() {
}


JmsConnection_Test::~JmsConnection_Test() {
}


unsigned int JmsConnection_Test::incrementMessageCount() {
    boost::mutex::scoped_lock lock(m_mutex);
    return ++m_messageCount;
}


unsigned int JmsConnection_Test::getMessageCount() {
    boost::mutex::scoped_lock lock(m_mutex);
    return m_messageCount;
}


void JmsConnection_Test::testConnect() {

    {
        m_connection = JmsConnection::Pointer(new JmsConnection());
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
        m_connection->connect();
        CPPUNIT_ASSERT(m_connection->isConnected() == true);
        CPPUNIT_ASSERT(m_connection->getBrokerUrl() == "tcp://exfl-broker.desy.de:7777" || m_connection->getBrokerUrl() == "tcp://exfl-broker:7777");
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
        CPPUNIT_ASSERT(m_connection->getBrokerUrl() == "tcp://exfl-broker.desy.de:7777" || m_connection->getBrokerUrl() == "tcp://exfl-broker:7777");
        m_connection->disconnect();
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
    }

    {
        char env[] = "KARABO_BROKER=tcp://exfl-broker.desy.de:7777";
        ::putenv(env);
        m_connection = JmsConnection::Pointer(new JmsConnection("tcp://someBadHost:7777"));
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
        m_connection->connect();
        CPPUNIT_ASSERT(m_connection->isConnected() == true);
        CPPUNIT_ASSERT(m_connection->getBrokerUrl() == "tcp://exfl-broker.desy.de:7777" || m_connection->getBrokerUrl() == "tcp://exfl-broker:7777");
        m_connection->disconnect();
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
        unsetenv("KARABO_BROKER");
    }
}


void JmsConnection_Test::readHandler1(karabo::net::JmsConsumer::Pointer consumer,
                                      karabo::net::JmsProducer::Pointer producer,
                                      karabo::util::Hash::Pointer header,
                                      karabo::util::Hash::Pointer body) {



    if (m_messageCount == 0) {
        m_tick = boost::posix_time::microsec_clock::local_time();
        CPPUNIT_ASSERT(header->has("header"));
        CPPUNIT_ASSERT(header->get<std::string>("header") == "some header");
        CPPUNIT_ASSERT(body->has("body"));
        CPPUNIT_ASSERT(body->get<int>("body") == 42);
    }

    if (m_messageCount == 1000) {
        boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::local_time() - m_tick;
        const float msPerMsg = diff.total_milliseconds() / 1000.;
        CPPUNIT_ASSERT(msPerMsg < 5); // Performance assert...
        return;
    }

    m_messageCount++;
    consumer->setTopic("testTopic2");
    consumer->readAsync(boost::bind(&JmsConnection_Test::readHandler1, this, consumer, producer, _1, _2));
    producer->write("testTopic2", header, body);
}


void JmsConnection_Test::testCommunication1() {

    m_messageCount = 0;

    m_connection = JmsConnection::Pointer(new JmsConnection());

    m_connection->connect();

    EventLoop::addThread();
    JmsConsumer::Pointer consumer = m_connection->createConsumer("testTopic1");
    JmsProducer::Pointer producer = m_connection->createProducer();
    
    consumer->readAsync(boost::bind(&JmsConnection_Test::readHandler1, this, consumer, producer, _1, _2));

    Hash::Pointer header(new Hash("header", "some header"));

    Hash::Pointer body(new Hash("body", 42));

    producer->write("testTopic1", header, body);

    EventLoop::run();
    EventLoop::removeThread();

    CPPUNIT_ASSERT_EQUAL(1000u, m_messageCount);
}


void JmsConnection_Test::readHandler2(karabo::net::JmsConsumer::Pointer channel,
                                      karabo::util::Hash::Pointer header,
                                      karabo::util::Hash::Pointer body) {

    incrementMessageCount();
}


void JmsConnection_Test::readHandler3(karabo::net::JmsConsumer::Pointer channel,
                                      karabo::util::Hash::Pointer header,
                                      karabo::util::Hash::Pointer body) {
    incrementMessageCount();
    
}


void JmsConnection_Test::readHandler4(karabo::net::JmsConsumer::Pointer c,
                                      karabo::util::Hash::Pointer header,
                                      karabo::util::Hash::Pointer body) {
    incrementMessageCount();
    if (header->get<std::string>("key") == "bar") return;
    c->setTopic("testTopic1");
    c->readAsync(boost::bind(&JmsConnection_Test::readHandler4, this, c, _1, _2));
}


void JmsConnection_Test::testCommunication2() {
    
    m_connection = JmsConnection::Pointer(new JmsConnection());
    
    m_connection->connect();   
    
    m_messageCount = 0;
    Hash::Pointer header1(new Hash("key", "foo"));
    Hash::Pointer header2(new Hash("key", "bar"));
    Hash::Pointer body(new Hash("body", 42));

    EventLoop::addThread(3); // One for each consumer using readAsync.
    JmsConsumer::Pointer c1 = m_connection->createConsumer("testTopic1", "key = 'foo'");
    JmsConsumer::Pointer c2 = m_connection->createConsumer("testTopic1", "key = 'bar'");
    JmsConsumer::Pointer c3 = m_connection->createConsumer("testTopic1");
    JmsProducer::Pointer p = m_connection->createProducer();

    c1->readAsync(boost::bind(&JmsConnection_Test::readHandler2, this, c1, _1, _2));
    c2->readAsync(boost::bind(&JmsConnection_Test::readHandler3, this, c2, _1, _2));
    c3->readAsync(boost::bind(&JmsConnection_Test::readHandler4, this, c3, _1, _2));

    p->write("testTopic1", header1, body);
    p->write("testTopic1", header2, body);
   
    EventLoop::run();
    EventLoop::removeThread(3);

    CPPUNIT_ASSERT_EQUAL(4u, m_messageCount);
}


void JmsConnection_Test::testPermanentRead() {

    m_connection = boost::make_shared<JmsConnection>();
    m_connection->connect();

    m_messageCount = 0;

    boost::thread t(boost::bind(&EventLoop::work));

    const std::string topic("nochEinTestTopic");
    JmsConsumer::Pointer consumer = m_connection->createConsumer(topic);
    JmsProducer::Pointer producer = m_connection->createProducer();

    consumer->startReading([this](karabo::util::Hash::Pointer p, karabo::util::Hash::Pointer q) {
        // Do not care about message content...
        incrementMessageCount();
    });


    auto header = boost::make_shared<Hash>("headerKey", "bar");
    auto body = boost::make_shared<Hash>("bodyKey", 42);

    const unsigned int numMessages = 10;
    for (unsigned int i = 0; i < numMessages; ++i) {
        producer->write(topic, header, body);
    }

    int trials = 100;
    while (--trials >= 0) {
        if (getMessageCount() == numMessages) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }

    EventLoop::stop();
    t.join();

    // After stop() and join() since otherwise they are missed in case of failure - and program does not stop...
    CPPUNIT_ASSERT_EQUAL(numMessages, m_messageCount);
}
