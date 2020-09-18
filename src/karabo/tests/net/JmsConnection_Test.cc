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


// uncomment next line if performing tests against local broker
// #define BROKER_ADDRESS   "tcp://localhost:7777"

#define BROKER_ADDRESS_SHORT   "tcp://exfl-broker:7777"
// don't modify or remove next lines. This is the default
#ifndef BROKER_ADDRESS
#define BROKER_ADDRESS  "tcp://exfl-broker.desy.de:7777"
#endif


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
    const char* broker_env = getenv("KARABO_BROKER");
    {
        setenv("KARABO_BROKER", BROKER_ADDRESS, true);
        m_connection = JmsConnection::Pointer(new JmsConnection());
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
        m_connection->connect();
        CPPUNIT_ASSERT(m_connection->isConnected() == true);
        CPPUNIT_ASSERT(m_connection->getBrokerUrl() == BROKER_ADDRESS || m_connection->getBrokerUrl() == BROKER_ADDRESS_SHORT);
        m_connection->disconnect();
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
        m_connection->connect();
        CPPUNIT_ASSERT(m_connection->isConnected() == true);
        unsetenv("KARABO_BROKER");
    }

    {
        m_connection = JmsConnection::Pointer(new JmsConnection("tcp://someBadHost:7777," BROKER_ADDRESS));
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
        m_connection->connect();
        CPPUNIT_ASSERT(m_connection->isConnected() == true);
        CPPUNIT_ASSERT(m_connection->getBrokerUrl() == BROKER_ADDRESS || m_connection->getBrokerUrl() == BROKER_ADDRESS_SHORT);
        m_connection->disconnect();
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
    }

    {
        // this line should not be commented out. It is required for this test!
        setenv("KARABO_BROKER", BROKER_ADDRESS, true);
        m_connection = JmsConnection::Pointer(new JmsConnection("tcp://someBadHost:7777"));
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
        m_connection->connect();
        CPPUNIT_ASSERT(m_connection->isConnected() == true);
        CPPUNIT_ASSERT(m_connection->getBrokerUrl() == BROKER_ADDRESS || m_connection->getBrokerUrl() == BROKER_ADDRESS_SHORT);
        m_connection->disconnect();
        CPPUNIT_ASSERT(m_connection->isConnected() == false);
        unsetenv("KARABO_BROKER");
    }
    if (broker_env) {
        setenv("KARABO_BROKER", broker_env, true);
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
        consumer->stopReading();
        // We switch topic now!
        consumer->setTopic("testTopic2");
        consumer->startReading(boost::bind(&JmsConnection_Test::readHandler1, this, consumer, producer, _1, _2));
    }

    if (m_messageCount < 100) {
        body->set<int>("body", m_messageCount + 1);
        producer->write("testTopic2", header, body);
    } else if (m_messageCount == 100) {
        consumer->stopReading();
        boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::local_time() - m_tick;
        const float msPerMsg = diff.total_milliseconds() / 100.;
        CPPUNIT_ASSERT(msPerMsg < 5); // Performance assert...
        return;
    }

    m_messageCount++;
}


void JmsConnection_Test::testCommunication1() {
    setenv("KARABO_BROKER", BROKER_ADDRESS, true);

    // Here we test e.g. switching topic in consumer and producer
    m_messageCount = 0;

    m_connection = JmsConnection::Pointer(new JmsConnection());

    m_connection->connect();

    JmsConsumer::Pointer consumer = m_connection->createConsumer("testTopic1");
    JmsProducer::Pointer producer = m_connection->createProducer();

    consumer->startReading(boost::bind(&JmsConnection_Test::readHandler1, this, consumer, producer, _1, _2));

    Hash::Pointer header(new Hash("header", "some header"));

    Hash::Pointer body(new Hash("body", 42));

    producer->write("testTopic1", header, body);

    boost::thread t(boost::bind(&EventLoop::work));
    int trials = 1000;
    while (--trials >= 0) {
        if (getMessageCount() >= 100) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }

    EventLoop::stop();
    t.join();

    // After stop() and join() since otherwise they are missed in case of failure - and program does not stop...
    CPPUNIT_ASSERT_EQUAL(100u, m_messageCount);
}


void JmsConnection_Test::readHandler2(karabo::net::JmsConsumer::Pointer channel,
                                      karabo::util::Hash::Pointer header,
                                      karabo::util::Hash::Pointer body) {
    incrementMessageCount();
}


void JmsConnection_Test::testCommunication2() {
    setenv("KARABO_BROKER", BROKER_ADDRESS, true);

    // Here we basically test selectors for the consumer.

    m_connection = JmsConnection::Pointer(new JmsConnection());
    
    m_connection->connect();   
    
    m_messageCount = 0;
    Hash::Pointer header1(new Hash("key", "foo"));
    Hash::Pointer header2(new Hash("key", "bar"));
    Hash::Pointer body(new Hash("body", 42));

    JmsConsumer::Pointer c1 = m_connection->createConsumer("testTopic1", "key = 'foo'");
    JmsConsumer::Pointer c2 = m_connection->createConsumer("testTopic1", "key = 'bar'");
    JmsConsumer::Pointer c3 = m_connection->createConsumer("testTopic1");
    JmsProducer::Pointer p = m_connection->createProducer();

    c1->startReading(boost::bind(&JmsConnection_Test::readHandler2, this, c1, _1, _2));
    c2->startReading(boost::bind(&JmsConnection_Test::readHandler2, this, c2, _1, _2));
    c3->startReading(boost::bind(&JmsConnection_Test::readHandler2, this, c3, _1, _2));

    p->write("testTopic1", header1, body); // received by c1 and c3
    p->write("testTopic1", header2, body); // received by c2 and c3

    boost::thread t(boost::bind(&EventLoop::work));
    int trials = 100;
    while (--trials >= 0) {
        if (getMessageCount() == 4u) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }

    EventLoop::stop();
    t.join();

    // After stop() and join() since otherwise they are missed in case of failure - and program does not stop...
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

    std::vector<unsigned int> counters;
    consumer->startReading([this, &counters](karabo::util::Hash::Pointer h, karabo::util::Hash::Pointer body) {
        incrementMessageCount();
        // Collect counters to test for sequentiality
        counters.push_back(body->get<unsigned int>("counter"));
    });


    auto header = boost::make_shared<Hash>("headerKey", "bar");
    auto body = boost::make_shared<Hash>();

    const unsigned int numMessages = 10;
    for (unsigned int i = 0; i < numMessages; ++i) {
        body->set("counter", i);
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
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (numMessages), counters.size());
    for (unsigned i = 0; i < numMessages; ++i) {
        // Test correct ordering
        CPPUNIT_ASSERT_EQUAL(i, counters[i]);
    }
}
