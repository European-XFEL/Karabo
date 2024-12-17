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
 * File:   Broker_Test.cc
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on February 19, 2021, 3:09 PM
 */

#include "Broker_Test.hh"

#include <karabo/net/EventLoop.hh>
#include <karabo/tests/BrokerUtils.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/StringTools.hh>
#include <stack>
#include <tuple>


constexpr uint32_t TEST_EXPIRATION_TIME_IN_SECONDS = 3;


using namespace karabo::util;
using namespace karabo::net;
using boost::system::error_code;

CPPUNIT_TEST_SUITE_REGISTRATION(Broker_Test);

Broker_Test::Broker_Test()
    : m_domain(Broker::brokerDomainFromEnv()),
      m_thread(),
      m_config(),
      m_brokersUnderTest(getBrokersFromEnv()),
      m_invalidBrokers({{"amqp", INVALID_AMQP}}) {}

Broker_Test::~Broker_Test() {}

void Broker_Test::setUp() {
    auto prom = std::promise<void>();
    auto fut = prom.get_future();
    m_thread = std::make_shared<boost::thread>([&prom]() {
        // postpone promise setting until EventLoop is activated
        EventLoop::getIOService().post([&prom]() { prom.set_value(); });
        EventLoop::work();
    });
    fut.get(); // block here until promise is set
}


void Broker_Test::tearDown() {
    EventLoop::stop();
    m_thread->join();
    m_thread.reset();
}


void Broker_Test::_loopFunction(const std::string& functionName, const std::function<void()>& testFunction) {
    if (m_brokersUnderTest.empty()) {
        std::clog << "\n\t" << functionName << " No broker specified in the environment, skipping" << std::endl;
    }
    for (Hash::const_iterator it = m_brokersUnderTest.begin(); it != m_brokersUnderTest.end(); ++it) {
        const std::vector<std::string>& brokers = it->getValue<std::vector<std::string>>();
        const std::string& protocol = it->getKey();
        Hash content;
        content.set("brokers", brokers);
        content.set("domain", m_domain);
        m_config.clear();
        m_config.set(protocol, content);
        std::clog << "\n\t" << functionName << " " << protocol << " : '" << toString(brokers) << "'" << std::endl;
        testFunction();
    }
}


void Broker_Test::testConnectDisconnect() {
    const std::string id = "alice";

    for (Hash::const_iterator it = m_brokersUnderTest.begin(); it != m_brokersUnderTest.end(); ++it) {
        const std::string& protocol = it->getKey();
        std::vector<std::string> brokers = it->getValue<std::vector<std::string>>();
        brokers.push_back(m_invalidBrokers[protocol]);
        std::clog << "\n\t" << __FUNCTION__ << " " << protocol << " : '" << toString(brokers) << "'" << std::endl;
        Hash content;
        content.set("brokers", brokers);
        content.set("domain", m_domain);
        content.set("instanceId", id);
        m_config.clear();
        m_config.set(protocol, content);
        _testConnectDisconnect();
    }
}


void Broker_Test::_testConnectDisconnect() {
    std::string classId = m_config.begin()->getKey();
    Broker::Pointer broker = Configurator<Broker>::create(m_config);
    broker->connect();

    CPPUNIT_ASSERT(broker->isConnected());
    CPPUNIT_ASSERT(broker->getBrokerType() == classId);
    CPPUNIT_ASSERT(broker->getBrokerUrl() == m_brokersUnderTest.get<std::vector<std::string>>(classId)[0]);
    CPPUNIT_ASSERT(broker->getInstanceId() == m_config.get<std::string>(classId + ".instanceId"));

    // Clone configuration and create new instance
    Broker::Pointer other = broker->clone("test2");
    CPPUNIT_ASSERT_EQUAL(std::string("test2"), other->getInstanceId());
    CPPUNIT_ASSERT_NO_THROW(other->connect());
    CPPUNIT_ASSERT(other->isConnected());
    CPPUNIT_ASSERT(other->getBrokerType() == classId);
    CPPUNIT_ASSERT(other->getBrokerUrl() == broker->getBrokerUrl());
    CPPUNIT_ASSERT(other->getDomain() == broker->getDomain());
    CPPUNIT_ASSERT(other->getInstanceId() != broker->getInstanceId());

    CPPUNIT_ASSERT_NO_THROW(broker->disconnect());
    CPPUNIT_ASSERT_NO_THROW(other->disconnect());
}


void Broker_Test::testPublishSubscribe() {
    _loopFunction(__FUNCTION__, [this] { this->_testPublishSubscribe(); });
}


void Broker_Test::_testPublishSubscribe() {
    std::string classId = m_config.begin()->getKey();
    m_config.set(classId + ".instanceId", "alice");

    // Create subscriber ...
    auto alice = Configurator<Broker>::create(m_config);
    CPPUNIT_ASSERT_NO_THROW(alice->connect());
    CPPUNIT_ASSERT(alice->isConnected());
    CPPUNIT_ASSERT(alice->getBrokerType() == classId);
    CPPUNIT_ASSERT(alice->getBrokerUrl() == m_brokersUnderTest.get<std::vector<std::string>>(classId)[0]);
    CPPUNIT_ASSERT(alice->getInstanceId() == "alice");

    auto prom = std::make_shared<std::promise<bool>>();
    auto fut = prom->get_future();

    constexpr int maxLoop = 10;

    // Ensure the subscriber is receiving messages
    alice->startReading(
          [prom, &maxLoop](Hash::Pointer h, Hash::Pointer data) {
              int loop = h->get<int>("count");
              if (loop >= maxLoop) prom->set_value(true);
          },
          [prom](consumer::Error err, const std::string& msg) { prom->set_value(false); });

    error_code ec = alice->subscribeToRemoteSignal("bob", "signalFromBob");
    CPPUNIT_ASSERT(!ec);

    // Clone a producer "Bob" which uses the same broker settings...
    auto bob = alice->clone("bob");
    CPPUNIT_ASSERT_NO_THROW(bob->connect());
    CPPUNIT_ASSERT(bob->isConnected());
    CPPUNIT_ASSERT(bob->getBrokerType() == classId);
    CPPUNIT_ASSERT(bob->getInstanceId() == "bob");
    CPPUNIT_ASSERT(bob->getBrokerUrl() == alice->getBrokerUrl());
    CPPUNIT_ASSERT(bob->getDomain() == alice->getDomain());

    auto hdr = std::make_shared<Hash>("signalInstanceId", "bob", "signalFunction", "signalFromBob", "slotInstanceIds",
                                      "|alice|", "slotFunctions", "|alice:aliceSlot");
    auto body = std::make_shared<Hash>("a.b.c", 42);

    for (int i = 0; i < maxLoop; ++i) {
        hdr->set("count", i + 1);
        CPPUNIT_ASSERT_NO_THROW(bob->write(m_domain, hdr, body, 4, 0));
    }

    // Wait on future ...
    bool result = fut.get();
    CPPUNIT_ASSERT(result);

    ec = alice->unsubscribeFromRemoteSignal("bob", "signalFromBob");
    CPPUNIT_ASSERT(!ec);
    CPPUNIT_ASSERT_NO_THROW(alice->stopReading());
    CPPUNIT_ASSERT_NO_THROW(bob->disconnect());
    CPPUNIT_ASSERT_NO_THROW(alice->disconnect());
}


void Broker_Test::testPublishSubscribeAsync() {
    _loopFunction(__FUNCTION__, [this] { this->_testPublishSubscribeAsync(); });
}


void Broker_Test::_testPublishSubscribeAsync() {
    std::string classId = m_config.begin()->getKey();
    m_config.set(classId + ".instanceId", "alice");

    // Create subscriber ...
    auto alice = Configurator<Broker>::create(m_config);
    CPPUNIT_ASSERT_NO_THROW(alice->connect());
    CPPUNIT_ASSERT(alice->isConnected());
    CPPUNIT_ASSERT(alice->getBrokerType() == classId);
    CPPUNIT_ASSERT(alice->getBrokerUrl() == m_brokersUnderTest.get<std::vector<std::string>>(classId)[0]);
    CPPUNIT_ASSERT(alice->getInstanceId() == "alice");

    auto prom = std::make_shared<std::promise<bool>>();
    auto fut = prom->get_future();

    constexpr int maxLoop = 10;

    // Ensure the subscriber is receiving messages
    alice->startReading(
          [prom, &maxLoop](Hash::Pointer h, Hash::Pointer data) {
              int loop = data->get<int>("c");
              if (loop >= maxLoop) prom->set_value(true);
          },
          [prom](consumer::Error err, const std::string& msg) { prom->set_value(false); });

    {
        auto p = std::make_shared<std::promise<boost::system::error_code>>();
        auto f = p->get_future();
        alice->subscribeToRemoteSignalAsync("bob", "signalFromBob",
                                            [p](const boost::system::error_code& ec) { p->set_value(ec); });
        auto ec = f.get();
        CPPUNIT_ASSERT(!ec);
    }

    auto bob = alice->clone("bob");

    auto t = boost::thread([this, maxLoop, classId, alice, bob]() {
        using namespace std::chrono_literals;

        CPPUNIT_ASSERT_NO_THROW(bob->connect());
        CPPUNIT_ASSERT(bob->isConnected());
        CPPUNIT_ASSERT(bob->getBrokerType() == classId);
        CPPUNIT_ASSERT(bob->getInstanceId() == "bob");
        CPPUNIT_ASSERT(bob->getDomain() == alice->getDomain());

        Hash::Pointer header =
              std::make_shared<Hash>("signalInstanceId", "bob", "signalFunction", "signalFromBob", "slotInstanceIds",
                                     "|alice|", "slotFunctions", "|alice:aliceSlot|");

        Hash::Pointer data = std::make_shared<Hash>("a", std::string("free text"), "b", 3.1415F);

        for (int i = 0; i < maxLoop; ++i) {
            data->set<int>("c", i + 1);
            CPPUNIT_ASSERT_NO_THROW(bob->write(bob->getDomain(), header, data, 4, 0));
        }
    });

    // Wait on future ... when Alice reads all maxLoop messages or failure happens...
    bool result = fut.get();
    CPPUNIT_ASSERT(result);
    t.join(); // join thread ... otherwise application is terminated

    {
        auto p = std::make_shared<std::promise<boost::system::error_code>>();
        auto f = p->get_future();
        alice->unsubscribeFromRemoteSignalAsync("bob", "signalFromBob",
                                                [p](const boost::system::error_code& ec) { p->set_value(ec); });
        auto ec = f.get();
        CPPUNIT_ASSERT(!ec);
    }
    CPPUNIT_ASSERT_NO_THROW(alice->stopReading());
    CPPUNIT_ASSERT_NO_THROW(bob->disconnect());
    CPPUNIT_ASSERT_NO_THROW(alice->disconnect());
}


void Broker_Test::testReadingHeartbeats() {
    _loopFunction(__FUNCTION__, [this] { this->_testReadingHeartbeats(); });
}


void Broker_Test::_testReadingHeartbeats() {
    //    'signalInstanceId' => bob STRING
    //    'signalFunction' => signalHeartbeat STRING
    //    'slotInstanceIds' => __none__ STRING
    //    'slotFunctions' => __none__ STRING
    //    'hostName' => exflpcx21502.desy.de STRING
    //    'userName' =>  STRING
    //
    //    'a1' => bob STRING
    //    'a2' => 1 INT32
    //    'a3' +
    //      'type' => device STRING
    //      'classId' => MqttBroker STRING
    //      'serverId' => __none__ STRING
    //      'visibility' => 4 INT32
    //      'host' => exflpcx21502 STRING
    //      'status' => ok STRING
    //      'archive' => 0 BOOL
    //      'capabilities' => 0 UINT32
    //      'heartbeatInterval' => 1 INT32
    //      'karaboVersion' => 2.11.0a10-48-g71844e8ab-dirty STRING

    std::string classId = m_config.begin()->getKey();
    m_config.set(classId + ".instanceId", "alice");

    // Create subscriber ...
    auto alice = Configurator<Broker>::create(m_config);
    CPPUNIT_ASSERT_NO_THROW(alice->connect());
    CPPUNIT_ASSERT(alice->isConnected());
    CPPUNIT_ASSERT(alice->getBrokerType() == classId);
    CPPUNIT_ASSERT(alice->getBrokerUrl() == m_brokersUnderTest.get<std::vector<std::string>>(classId)[0]);
    CPPUNIT_ASSERT(alice->getInstanceId() == "alice");

    auto prom = std::make_shared<std::promise<bool>>();
    auto fut = prom->get_future();
    auto promBeats = std::make_shared<std::promise<bool>>();
    auto futBeats = promBeats->get_future();

    constexpr int maxLoop = 10;
    int counter = 0;
    // Ensure the subscriber is receiving messages
    alice->startReading(
          [prom](Hash::Pointer h, Hash::Pointer data) {
              try {
                  CPPUNIT_ASSERT(h->get<std::string>("signalInstanceId") == "bob");
                  CPPUNIT_ASSERT(h->get<std::string>("signalFunction") == "signalFromBob");
                  CPPUNIT_ASSERT(data->get<int>("c") == 1);
              } catch (const std::exception& e) {
                  std::clog << __FILE__ << ":" << __LINE__ << " " << e.what() << std::endl;
                  prom->set_value(false);
                  return;
              }
              prom->set_value(true);
          },
          [prom](consumer::Error err, const std::string& msg) { prom->set_value(false); });

    alice->startReadingHeartbeats(
          [promBeats, maxLoop, &counter](Hash::Pointer h, Hash::Pointer d) {
              try {
                  CPPUNIT_ASSERT(h->get<std::string>("signalFunction") == "signalHeartbeat");
                  CPPUNIT_ASSERT(h->get<std::string>("signalInstanceId") == "bob");
                  CPPUNIT_ASSERT(d->has("a1"));
                  CPPUNIT_ASSERT(d->has("a2"));
                  CPPUNIT_ASSERT(d->has("a3"));
                  if (++counter == maxLoop) promBeats->set_value(true);
              } catch (const std::exception& e) {
                  std::clog << __FILE__ << ":" << __LINE__ << " " << e.what() << std::endl;
                  promBeats->set_value(false);
              }
          },
          [promBeats](karabo::net::consumer::Error ec, const std::string& message) {
              std::clog << "Heartbeat error: " << message << std::endl;
              promBeats->set_value(false);
          });

    {
        boost::system::error_code ec = alice->subscribeToRemoteSignal("bob", "signalFromBob");
        CPPUNIT_ASSERT(!ec);
    }

    auto bob = alice->clone("bob");

    auto t = boost::thread([this, maxLoop, classId, alice, bob]() {
        using namespace std::chrono_literals;

        CPPUNIT_ASSERT_NO_THROW(bob->connect());
        CPPUNIT_ASSERT(bob->isConnected());
        CPPUNIT_ASSERT(bob->getBrokerType() == classId);
        CPPUNIT_ASSERT(bob->getInstanceId() == "bob");
        CPPUNIT_ASSERT(bob->getDomain() == alice->getDomain());

        Hash::Pointer header = std::make_shared<Hash>("signalInstanceId", "bob", "signalFunction", "signalHeartbeat",
                                                      "slotInstanceIds", "__none__", "slotFunctions", "__none__");

        Hash::Pointer data = std::make_shared<Hash>(
              "a1", std::string("bob"), "a2", 1, "a3",
              Hash("type", "device", "classId", "Broker", "serverId", "__none__", "visibilty", 4, "lang", "cpp"));

        for (int i = 0; i < maxLoop; ++i) {
            // Bob sends heartbeat
            data->set<int>("c", i + 1);
            CPPUNIT_ASSERT_NO_THROW(bob->write(m_domain + "_beats", header, data, 0, 0));
        }

        Hash::Pointer h2 = std::make_shared<Hash>("signalInstanceId", "bob", "signalFunction", "signalFromBob",
                                                  "slotInstanceIds", "|alice|", "slotFunctions", "|alice:someSlot");
        Hash::Pointer d2 = std::make_shared<Hash>("c", 1);

        // Trigger the end of the test
        CPPUNIT_ASSERT_NO_THROW(bob->write(m_domain, h2, d2, 4, 0));
    });

    // Wait on future ... when Alice reads all maxLoop messages or failure happens...
    const bool resultBeats = futBeats.get();
    CPPUNIT_ASSERT(resultBeats);
    const bool result = fut.get();
    CPPUNIT_ASSERT(result);
    t.join(); // join  ... otherwise terminate() called

    CPPUNIT_ASSERT_NO_THROW(alice->stopReading()); // unsubscribeAll
    CPPUNIT_ASSERT_NO_THROW(bob->disconnect());
    CPPUNIT_ASSERT_NO_THROW(alice->disconnect());
}


void Broker_Test::testReadingGlobalCalls() {
    for (Hash::const_iterator it = m_brokersUnderTest.begin(); it != m_brokersUnderTest.end(); ++it) {
        const std::vector<std::string>& brokers = it->getValue<std::vector<std::string>>();
        const std::string& protocol = it->getKey();
        std::clog << "\n\t" << __FUNCTION__ << " " << protocol << " : '" << toString(brokers) << "'" << std::endl;
        _testReadingGlobalCalls(brokers);
    }
}


void Broker_Test::_testReadingGlobalCalls(const std::vector<std::string>& brokerAddress) {
    std::string type = Broker::brokerTypeFrom(brokerAddress);

    Hash cfg("brokers", brokerAddress, "domain", m_domain, "instanceId", "listenGlobal");
    Broker::Pointer listenGlobal = Configurator<Broker>::create(type, cfg);

    cfg.set("instanceId", "notListenGlobal");
    Broker::Pointer notListenGlobal = Configurator<Broker>::create(type, cfg);
    notListenGlobal->setConsumeBroadcasts(false);

    cfg.set("instanceId", "sender");
    Broker::Pointer sender = Configurator<Broker>::create(type, cfg);

    CPPUNIT_ASSERT_NO_THROW(listenGlobal->connect());
    CPPUNIT_ASSERT_NO_THROW(notListenGlobal->connect());
    CPPUNIT_ASSERT_NO_THROW(sender->connect());

    auto promGlobal1 = std::make_shared<std::promise<std::string>>();
    auto futGlobal1 = promGlobal1->get_future();
    auto promNonGlobal1 = std::make_shared<std::promise<std::string>>();
    auto futNonGlobal1 = promNonGlobal1->get_future();

    auto readHandlerBoth1 = [promGlobal1, promNonGlobal1](Hash::Pointer hdr, Hash::Pointer body) {
        if (body->has("msg") && body->is<std::string>("msg")) {
            promNonGlobal1->set_value(body->get<std::string>("msg"));
        } else if (body->has("msgToAll") && body->is<std::string>("msgToAll")) {
            promGlobal1->set_value(body->get<std::string>("msgToAll"));
        } else {
            // unexpected - "invalidate" both
            promGlobal1->set_value(toString(body));
            promNonGlobal1->set_value(toString(body));
        }
    };
    auto errorHandlerBoth1 = [promGlobal1, promNonGlobal1](consumer::Error err, const std::string& msg) {
        // unexpected - invalidate both
        promGlobal1->set_value(msg);
        promNonGlobal1->set_value(msg);
    };

    auto promGlobal2 = std::make_shared<std::promise<std::string>>();
    auto futGlobal2 = promGlobal2->get_future();
    auto promNonGlobal2 = std::make_shared<std::promise<std::string>>();
    auto futNonGlobal2 = promNonGlobal2->get_future();

    auto readHandlerBoth2 = [promGlobal2, promNonGlobal2](Hash::Pointer hdr, Hash::Pointer body) {
        if (body->has("msg") && body->is<std::string>("msg")) {
            promNonGlobal2->set_value(body->get<std::string>("msg"));
        } else if (body->has("msgToAll") && body->is<std::string>("msgToAll")) {
            promGlobal2->set_value(body->get<std::string>("msgToAll"));
        } else {
            // unexpected - "invalidate" both
            promGlobal2->set_value(toString(body));
            promNonGlobal2->set_value(toString(body));
        }
    };
    auto errorHandlerBoth2 = [promGlobal2, promNonGlobal2](consumer::Error err, const std::string& msg) {
        // unexpected - "invalidate" both
        promGlobal2->set_value(msg);
        promNonGlobal2->set_value(msg);
    };


    listenGlobal->startReading(readHandlerBoth1, errorHandlerBoth1);
    notListenGlobal->startReading(readHandlerBoth2, errorHandlerBoth2);

    // Prepare and send global message
    auto hdr = std::make_shared<Hash>("signalInstanceId", sender->getInstanceId(), "signalFunction", "__call__",
                                      "slotInstanceIds", "|*|", "slotFunctions",
                                      "|*:aSlot|"); //
    auto bodyGlobal = std::make_shared<Hash>("msgToAll", "A global message");
    sender->write(m_domain, hdr, bodyGlobal, 4, 0);

    // Prepare and send specific messages
    hdr->erase("slotFunctions"); // Specific slot calls do not need their slot for routing, ...
    hdr->set("slotInstanceIds", "|" + listenGlobal->getInstanceId() + "|"); // ... but a specific instanceId
    auto bodyNonGlobal = std::make_shared<Hash>("msg", "A specific message");
    sender->write(m_domain, hdr, bodyNonGlobal, 4, 0);
    hdr->set("slotInstanceIds", "|" + notListenGlobal->getInstanceId() + "|");
    sender->write(m_domain, hdr, bodyNonGlobal, 4, 0);

    // Assert that both messages arrived at listenGlobal
    const std::string msg = futGlobal1.get();
    CPPUNIT_ASSERT_EQUAL(std::string("A global message"), msg);

    const std::string msg2 = futNonGlobal1.get();
    CPPUNIT_ASSERT_EQUAL(std::string("A specific message"), msg2);

    // At listNonGlobal, only the non-global message arrives
    const std::string msg3 = futNonGlobal2.get();
    CPPUNIT_ASSERT_EQUAL(std::string("A specific message"), msg3);

    auto status = futGlobal2.wait_for(std::chrono::milliseconds(50));
    CPPUNIT_ASSERT_EQUAL(std::future_status::timeout, status);

    std::clog << "OK." << std::endl;
}


void Broker_Test::testProducerRestartConsumerContinues() {
    _loopFunction(__FUNCTION__, [this] { this->_testProducerRestartConsumerContinues(); });
}


void Broker_Test::_testProducerRestartConsumerContinues() {
    std::string classId = m_config.begin()->getKey();
    Hash aliceConfig = m_config;
    aliceConfig.set(classId + ".instanceId", "alice");

    auto prom = std::make_shared<std::promise<bool>>();
    auto fut = prom->get_future();

    std::vector<int> bottle1;
    std::vector<int> bottle2;
    std::vector<int> bottle3;

    auto alice = Configurator<Broker>::create(aliceConfig);
    CPPUNIT_ASSERT_NO_THROW(alice->connect());
    CPPUNIT_ASSERT(alice->isConnected());

    auto errorMessage = [prom](consumer::Error err, const std::string& desc) {
        std::clog << "Alice: Error ==> " << int(err) << " -- " << desc << std::endl;
        prom->set_value(false);
    };
    auto parseMessage = [prom, &bottle1, &bottle2, &bottle3](Hash::Pointer h, Hash::Pointer d) {
        try {
            if (d->has("stop")) {
                prom->set_value(true);
                return;
            }
            if (!d->has("fill")) return;
            int n = d->get<int>("c");
            const std::string& fill = d->get<std::string>("fill");
            if (fill == "bottle1") {
                bottle1.push_back(n);
            } else if (fill == "bottle2") {
                bottle2.push_back(n);
            } else {
                bottle3.push_back(n);
            }
        } catch (const std::exception& e) {
            std::clog << "Exception in Alice lambda: " << e.what() << std::endl;
            prom->set_value(false);
        }
    };

    boost::system::error_code ec;
    alice->startReading(parseMessage, errorMessage);
    ec = alice->subscribeToRemoteSignal("bob", "signalFromBob");
    CPPUNIT_ASSERT(!ec);

    auto t = boost::thread([this]() {
        std::string classId = m_config.begin()->getKey();
        Hash bobConfig = m_config;
        bobConfig.set(classId + ".instanceId", "bob");

        auto bob = Configurator<Broker>::create(bobConfig);
        CPPUNIT_ASSERT_NO_THROW(bob->connect());
        CPPUNIT_ASSERT(bob->isConnected());

        Hash::Pointer header = std::make_shared<Hash>("signalInstanceId", "bob", "signalFunction", "signalFromBob");
        header->set("slotInstanceIds", "|alice|");
        header->set("slotFunctions", "|alice:aliceSlot|");

        Hash::Pointer data = std::make_shared<Hash>("fill", "bottle1");

        for (int i = 1; i <= 16; ++i) {
            data->set("c", i);
            CPPUNIT_ASSERT_NO_THROW(bob->write(m_domain, header, data, 4, 0));
        }

        CPPUNIT_ASSERT_NO_THROW(bob->disconnect());
        bob.reset();

        // Bob restarts ... Alice continues ...

        bob = Configurator<Broker>::create(bobConfig); // new incarnation of Bob
        CPPUNIT_ASSERT_NO_THROW(bob->connect());
        CPPUNIT_ASSERT(bob->isConnected());

        data->set("fill", "bottle2");

        for (int i = 1; i <= 20; ++i) {
            data->set("c", -i);
            CPPUNIT_ASSERT_NO_THROW(bob->write(m_domain, header, data, 4, 0));
        }

        Hash::Pointer stop(new Hash("stop", Hash()));
        CPPUNIT_ASSERT_NO_THROW(bob->write(m_domain, header, stop, 4, 0));
        CPPUNIT_ASSERT_NO_THROW(bob->disconnect());
    });

    bool result = fut.get(); // wait until bottles are filled
    CPPUNIT_ASSERT(result);

    t.join();

    ec = alice->unsubscribeFromRemoteSignal("bob", "signalBob");
    CPPUNIT_ASSERT(!ec);

    CPPUNIT_ASSERT_NO_THROW(alice->disconnect());

    CPPUNIT_ASSERT_EQUAL(16ul, bottle1.size());
    for (int i = 1; i <= int(bottle1.size()); ++i) CPPUNIT_ASSERT_EQUAL(i, bottle1[i - 1]);

    CPPUNIT_ASSERT_EQUAL(20ul, bottle2.size());
    for (int i = 1; i <= int(bottle2.size()); ++i) CPPUNIT_ASSERT_EQUAL(-i, bottle2[i - 1]);

    CPPUNIT_ASSERT_EQUAL(0ul, bottle3.size());
}


void Broker_Test::testProducerContinuesConsumerRestart() {
    _loopFunction(__FUNCTION__, [this] { this->_testProducerContinuesConsumerRestart(); });
}


void Broker_Test::_testProducerContinuesConsumerRestart() {
    std::string classId = m_config.begin()->getKey();
    Hash aliceConfig = m_config;
    aliceConfig.set(classId + ".instanceId", "alice");

    boost::system::error_code ec;

    std::vector<int> bottle;

    // Create Bob instance
    Hash bobConfig = m_config;
    bobConfig.set(classId + ".instanceId", "bob");

    auto bob = Configurator<Broker>::create(bobConfig);

    CPPUNIT_ASSERT_NO_THROW(bob->connect());
    CPPUNIT_ASSERT(bob->isConnected());

    Hash::Pointer header = std::make_shared<Hash>("signalInstanceId", "bob", "signalFunction", "signalBob");
    header->set("slotInstanceIds", "|alice|");
    header->set("slotFunctions", "|alice:aliceSlot|");
    Hash::Pointer data = std::make_shared<Hash>(); // data container

    Broker::Pointer alice;

    alice = Configurator<Broker>::create(aliceConfig);
    CPPUNIT_ASSERT_NO_THROW(alice->connect());
    CPPUNIT_ASSERT(alice->isConnected());

    auto p1 = std::make_shared<std::promise<bool>>();
    auto f1 = p1->get_future();

    auto error1 = [p1](consumer::Error err, const std::string& desc) {
        std::clog << "Alice: Error ==> " << int(err) << " -- " << desc << std::endl;
        p1->set_value(false);
    };

    constexpr int maxLoop1 = 6;
    int loopCount1 = maxLoop1;
    auto parse1 = [p1, &bottle, &loopCount1](Hash::Pointer h, Hash::Pointer d) {
        int n = d->get<int>("c");
        bottle.push_back(n);
        if (--loopCount1 == 0) p1->set_value(true);
    };

    // Alice is preparing to receive messages ...
    alice->startReading(parse1, error1);
    // This subscription will use callbacks from startReading...
    ec = alice->subscribeToRemoteSignal("bob", "signalBob");
    CPPUNIT_ASSERT(!ec);

    for (int i = 1; i <= maxLoop1; ++i) {
        data->set("c", i);
        CPPUNIT_ASSERT_NO_THROW(bob->write(m_domain, header, data, 4, 0));
    }

    // Alice waits here for end of step1
    bool r1 = f1.get();
    CPPUNIT_ASSERT(r1);
    CPPUNIT_ASSERT(loopCount1 == 0);
    // check bottle...
    CPPUNIT_ASSERT(int(bottle.size()) == maxLoop1);
    for (int i = 1; i <= int(bottle.size()); ++i) CPPUNIT_ASSERT(i == bottle[i - 1]);

    ec = alice->unsubscribeFromRemoteSignal("bob", "signalBob");
    CPPUNIT_ASSERT(!ec);
    alice->stopReading();

    CPPUNIT_ASSERT_NO_THROW(alice->disconnect());

    bottle.clear();
    alice.reset();

    // Restart Alice ...

    auto p2 = std::make_shared<std::promise<bool>>();
    auto f2 = p2->get_future();

    alice = Configurator<Broker>::create(aliceConfig);
    CPPUNIT_ASSERT_NO_THROW(alice->connect());
    CPPUNIT_ASSERT(alice->isConnected());

    auto error2 = [p2](consumer::Error err, const std::string& desc) { p2->set_value(false); };

    constexpr int maxLoop2 = 20;
    int loopCount2 = maxLoop2;
    auto parse2 = [p2, &bottle, &loopCount2](Hash::Pointer h, Hash::Pointer d) {
        int n = d->get<int>("c");
        bottle.push_back(n); // fill the "bottle"
        if (--loopCount2 == 0) p2->set_value(true);
    };


    alice->startReading(parse2, error2);
    ec = alice->subscribeToRemoteSignal("bob", "signalBob");
    CPPUNIT_ASSERT(!ec);

    // Bob continues ...
    // send negative numbers ...
    for (int i = 1; i <= maxLoop2; ++i) {
        data->set("c", -i);
        CPPUNIT_ASSERT_NO_THROW(bob->write(m_domain, header, data, 4, 0));
    }

    auto r2 = f2.get();
    CPPUNIT_ASSERT(r2);

    ec = alice->unsubscribeFromRemoteSignal("bob", "signalBob");
    CPPUNIT_ASSERT(!ec);
    alice->stopReading();

    CPPUNIT_ASSERT_NO_THROW(alice->disconnect());

    CPPUNIT_ASSERT_NO_THROW(bob->disconnect());

    CPPUNIT_ASSERT_EQUAL(maxLoop2, int(bottle.size()));
    for (int i = 1; i <= int(bottle.size()); ++i) CPPUNIT_ASSERT_EQUAL(-i, bottle[i - 1]);
}
