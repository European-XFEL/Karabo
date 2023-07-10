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
 * File:   JmsConnection_Test.cc
 * Author: heisenb
 *
 * Created on July 15, 2016, 5:05 PM
 */

#include "JmsConnection_Test.hh"

#include <boost/thread.hpp>

#include "karabo/log/Logger.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/JmsConnection.hh"
#include "karabo/net/JmsConsumer.hh"
#include "karabo/net/JmsProducer.hh"
#include "karabo/tests/BrokerUtils.hh"
using namespace karabo::util;
using namespace karabo::net;

using boost::placeholders::_1;
using boost::placeholders::_2;

CPPUNIT_TEST_SUITE_REGISTRATION(JmsConnection_Test);


JmsConnection_Test::JmsConnection_Test()
    // Use the environment variable KARABO_CI_BROKERS to define the brokers to use in tests:
    // e.g. export KARABO_CI_BROKERS=tcp://a-jms-broker:7777;amqp://an-amqp-broker:5672
    : m_defaultBrokers(getJmsBrokerFromEnv()),
      m_baseTopic(
            Broker::brokerDomainFromEnv()), // parallel CIs or users must get different topics, so take from environment
      m_messageCount(0) {}


JmsConnection_Test::~JmsConnection_Test() {}


void JmsConnection_Test::testConnect() {
    if (m_defaultBrokers.empty()) {
        std::clog << " No JMS broker in environment. Skipping..." << std::flush;
        return;
    }
    { // constructor with empty vector<string> leads to exception in connect()
        JmsConnection::Pointer connection(new JmsConnection(std::vector<std::string>()));
        CPPUNIT_ASSERT(connection->isConnected() == false);
        CPPUNIT_ASSERT_THROW(connection->connect(), karabo::util::NetworkException);
        CPPUNIT_ASSERT(connection->isConnected() == false);
    }

    CPPUNIT_ASSERT_GREATEREQUAL(1ul, m_defaultBrokers.size());
    { // constructor from vector<string>
        JmsConnection::Pointer connection(new JmsConnection(m_defaultBrokers));
        CPPUNIT_ASSERT(connection->isConnected() == false);
        connection->connect();
        CPPUNIT_ASSERT(connection->isConnected() == true);
        CPPUNIT_ASSERT_EQUAL(m_defaultBrokers[0], connection->getBrokerUrl());
        connection->disconnect();
        CPPUNIT_ASSERT(connection->isConnected() == false);
        connection->connect();
        CPPUNIT_ASSERT(connection->isConnected() == true);
    }

    { // constructor from string, with more than one addresses and first bad
        JmsConnection::Pointer connection(new JmsConnection("tcp://someBadHost:7777," + m_defaultBrokers[0]));
        CPPUNIT_ASSERT(connection->isConnected() == false);
        connection->connect();
        CPPUNIT_ASSERT(connection->isConnected() == true);
        CPPUNIT_ASSERT_EQUAL(m_defaultBrokers[0], connection->getBrokerUrl());
        connection->disconnect();
        CPPUNIT_ASSERT(connection->isConnected() == false);
    }

    { // constructor from Hash
        JmsConnection::Pointer connection(new JmsConnection(Hash("brokers", m_defaultBrokers)));
        CPPUNIT_ASSERT(connection->isConnected() == false);
        connection->connect();
        CPPUNIT_ASSERT(connection->isConnected() == true);
        CPPUNIT_ASSERT_EQUAL(m_defaultBrokers[0], connection->getBrokerUrl());
        connection->disconnect();
        CPPUNIT_ASSERT(connection->isConnected() == false);
    }
}


void JmsConnection_Test::readHandler1(karabo::net::JmsConsumer::Pointer consumer,
                                      karabo::net::JmsProducer::Pointer producer, karabo::util::Hash::Pointer header,
                                      karabo::util::Hash::Pointer body) {
    const std::string topic2(m_baseTopic + "_anotherTopic");
    if (m_messageCount == 0) {
        m_tick = boost::posix_time::microsec_clock::local_time();

        if (!header->has("header")) {
            m_failures.push_back("Missing header");
        } else if (header->get<std::string>("header") != "some header") {
            m_failures.push_back(" Wrong header: " + karabo::util::toString(header->get<std::string>("header")));
        }
        if (!body->has("body")) {
            m_failures.push_back(" Missing body");
        } else if (body->get<int>("body") != 42) {
            m_failures.push_back(" Wrong body: " + karabo::util::toString(body->get<int>("body")));
        }
        consumer->stopReading();
        // We switch topic now!
        consumer->setTopic(topic2);
        consumer->startReading(boost::bind(&JmsConnection_Test::readHandler1, this, consumer, producer, _1, _2));
        boost::this_thread::sleep(
              boost::posix_time::milliseconds(50)); // Needed? Maybe switching topic does not read immediately...
    }

    if (m_messageCount < 100) {
        body->set<int>("body", m_messageCount + 1);
        producer->write(topic2, header, body);
    } else if (m_messageCount == 100) {
        boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::local_time() - m_tick;
        consumer->stopReading(); // may block for a while, therefore after calculating diff
        const float msPerMsg = diff.total_milliseconds() / 100.f;
        if (msPerMsg > 7.f) { // Performance assert...
            m_failures.push_back("Slow turnaround: " + karabo::util::toString(msPerMsg));
        }
        return;
    }

    m_messageCount++;
}


void JmsConnection_Test::testCommunication1() {
    if (m_defaultBrokers.empty()) {
        std::clog << " No JMS broker in environment. Skipping..." << std::flush;
        return;
    }
    // Here we test e.g. switching topic in consumer and producer
    m_messageCount = 0;
    m_failures.clear();

    JmsConnection::Pointer connection(new JmsConnection(m_defaultBrokers));

    connection->connect();

    JmsConsumer::Pointer consumer = connection->createConsumer(m_baseTopic);
    JmsProducer::Pointer producer = connection->createProducer();

    consumer->startReading(boost::bind(&JmsConnection_Test::readHandler1, this, consumer, producer, _1, _2));

    Hash::Pointer header(new Hash("header", "some header"));

    Hash::Pointer body(new Hash("body", 42));

    producer->write(m_baseTopic, header, body);

    boost::thread t(boost::bind(&EventLoop::work));
    int trials = 2000;
    while (--trials >= 0) {
        if (m_messageCount >= 100) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }

    EventLoop::stop();
    t.join();

    // After stop() and join() since otherwise they are missed in case of failure - and program does not stop...
    CPPUNIT_ASSERT_MESSAGE(
          karabo::util::toString(m_failures) += ", messageCount " + karabo::util::toString(m_messageCount),
          m_failures.empty());
    CPPUNIT_ASSERT_EQUAL(100u, static_cast<unsigned int>(m_messageCount));
}


void JmsConnection_Test::readHandler2(karabo::net::JmsConsumer::Pointer channel, karabo::util::Hash::Pointer header,
                                      karabo::util::Hash::Pointer body) {
    ++m_messageCount;
}


void JmsConnection_Test::testCommunication2() {
    if (m_defaultBrokers.empty()) {
        std::clog << " No JMS broker in environment. Skipping..." << std::flush;
        return;
    }
    // Here we basically test selectors for the consumer.

    JmsConnection::Pointer connection(new JmsConnection(m_defaultBrokers));

    connection->connect();

    m_messageCount = 0;
    Hash::Pointer header1(new Hash("key", "foo"));
    Hash::Pointer header2(new Hash("key", "bar"));
    Hash::Pointer body(new Hash("body", 42));

    JmsConsumer::Pointer c1 = connection->createConsumer(m_baseTopic, "key = 'foo'");
    JmsConsumer::Pointer c2 = connection->createConsumer(m_baseTopic, "key = 'bar'");
    JmsConsumer::Pointer c3 = connection->createConsumer(m_baseTopic);
    JmsProducer::Pointer p = connection->createProducer();

    c1->startReading(boost::bind(&JmsConnection_Test::readHandler2, this, c1, _1, _2));
    c2->startReading(boost::bind(&JmsConnection_Test::readHandler2, this, c2, _1, _2));
    c3->startReading(boost::bind(&JmsConnection_Test::readHandler2, this, c3, _1, _2));

    p->write(m_baseTopic, header1, body); // received by c1 and c3
    p->write(m_baseTopic, header2, body); // received by c2 and c3

    boost::thread t(boost::bind(&EventLoop::work));
    int trials = 500;
    while (--trials >= 0) {
        if (m_messageCount == 4u) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(2));
    }

    EventLoop::stop();
    t.join();

    // After stop() and join() since otherwise they are missed in case of failure - and program does not stop...
    CPPUNIT_ASSERT_EQUAL(4u, static_cast<unsigned int>(m_messageCount));
}


void JmsConnection_Test::testPermanentRead() {
    if (m_defaultBrokers.empty()) {
        std::clog << " No JMS broker in environment. Skipping..." << std::flush;
        return;
    }
    JmsConnection::Pointer connection = boost::make_shared<JmsConnection>(m_defaultBrokers);
    connection->connect();

    m_messageCount = 0;

    boost::thread t(boost::bind(&EventLoop::work));

    const std::string topic(m_baseTopic += "_oneMore");
    JmsConsumer::Pointer consumer = connection->createConsumer(topic);
    JmsProducer::Pointer producer = connection->createProducer();

    auto counters = std::make_shared<std::vector<unsigned int>>();
    // 'counters' is ptr and captured by value to prevent crash in case of test failure
    auto read = [this, counters](karabo::util::Hash::Pointer h, karabo::util::Hash::Pointer body) {
        // Collect counters to test for sequentiality
        counters->push_back(body->get<unsigned int>("counter"));
        // increment at the end since its value is in the break condition
        ++m_messageCount;
    };
    consumer->startReading(read);


    auto header = boost::make_shared<Hash>("headerKey", "bar");
    auto body = boost::make_shared<Hash>();

    const unsigned int numMessages = 500;
    for (unsigned int i = 0; i < numMessages; ++i) {
        body->set("counter", i);
        producer->write(topic, header, body);
        if (i == numMessages / 2) {
            // stop reading to resume later
            consumer->stopReading();
        }
    }
    // resume reading - no message should be lost!
    consumer->startReading(read);

    int trials = 3000;
    while (--trials >= 0) {
        if (m_messageCount == numMessages) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }

    EventLoop::stop();
    t.join();

    // After stop() and join() since otherwise they are missed in case of failure - and program does not stop...
    CPPUNIT_ASSERT_EQUAL(numMessages, static_cast<unsigned int>(m_messageCount));
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(numMessages), counters->size());
    for (unsigned i = 0; i < numMessages; ++i) {
        // Test correct ordering
        CPPUNIT_ASSERT_EQUAL(i, counters->at(i));
    }
}
