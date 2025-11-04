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
 * File:   Broker_Test.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on February 19, 2021, 3:09 PM
 */

#include <gtest/gtest.h>

#include <chrono>
#include <stack>
#include <thread>
#include <tuple>

#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/StringTools.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/tests/BrokerUtils.hh"

using namespace std::literals::chrono_literals;

static std::string m_domain = karabo::net::Broker::brokerDomainFromEnv();
static karabo::data::Hash m_config;
// using a Karabo Hash to match the insertion order.
static karabo::data::Hash m_brokersUnderTest = getBrokersFromEnv();
static std::map<std::string, std::string> m_invalidBrokers({{"amqp", INVALID_AMQP}});
static const std::chrono::seconds m_timeout = 10s;

constexpr uint32_t TEST_EXPIRATION_TIME_IN_SECONDS = 3;

using namespace karabo::data;
using namespace karabo::net;
using boost::system::error_code;


class TestBroker : public ::testing::Test {
   protected:
    TestBroker() : m_thread(nullptr) {}

    ~TestBroker() override {}

    void SetUp() override {
        auto prom = std::promise<void>();
        auto fut = prom.get_future();
        m_thread = std::make_shared<std::jthread>([&prom]() {
            // postpone promise setting until EventLoop is activated
            boost::asio::post(EventLoop::getIOService(), [&prom]() { prom.set_value(); });
            EventLoop::work();
        });
        fut.get(); // block here until promise is set
    }

    void TearDown() override {
        EventLoop::stop();
        m_thread->join();
        m_thread.reset();
    }

    std::shared_ptr<std::jthread> m_thread;

   public:
    void _loopFunction(const std::string& functionName, const std::function<void()>& testFunction);
};


void TestBroker::_loopFunction(const std::string& functionName, const std::function<void()>& testFunction) {
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


void _testConnectDisconnect() {
    std::string classId = m_config.begin()->getKey();
    Broker::Pointer broker = Configurator<Broker>::create(m_config);
    broker->connect();

    EXPECT_TRUE(broker->isConnected());
    EXPECT_TRUE(broker->getBrokerType() == classId);
    EXPECT_TRUE(broker->getBrokerUrl() == m_brokersUnderTest.get<std::vector<std::string>>(classId)[0]);
    EXPECT_TRUE(broker->getInstanceId() == m_config.get<std::string>(classId + ".instanceId"));

    // Clone configuration and create new instance
    Broker::Pointer other = broker->clone("test2");
    EXPECT_STREQ("test2", other->getInstanceId().c_str());
    EXPECT_NO_THROW(other->connect());
    EXPECT_TRUE(other->isConnected());
    EXPECT_TRUE(other->getBrokerType() == classId);
    EXPECT_TRUE(other->getBrokerUrl() == broker->getBrokerUrl());
    EXPECT_TRUE(other->getDomain() == broker->getDomain());
    EXPECT_TRUE(other->getInstanceId() != broker->getInstanceId());

    EXPECT_NO_THROW(broker->disconnect());
    EXPECT_NO_THROW(other->disconnect());
}


TEST_F(TestBroker, testConnectDisconnect) {
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


static void _testPublishSubscribe() {
    std::string classId = m_config.begin()->getKey();
    m_config.set(classId + ".instanceId", "alice");

    // Create subscriber ...
    auto alice = Configurator<Broker>::create(m_config);
    EXPECT_NO_THROW(alice->connect());
    EXPECT_TRUE(alice->isConnected());
    EXPECT_TRUE(alice->getBrokerType() == classId);
    EXPECT_TRUE(alice->getBrokerUrl() == m_brokersUnderTest.get<std::vector<std::string>>(classId)[0]);
    EXPECT_TRUE(alice->getInstanceId() == "alice");

    auto prom = std::make_shared<std::promise<bool>>();
    auto fut = prom->get_future();

    constexpr int maxLoop = 10;
    auto slotToCall = std::make_shared<std::string>();

    // Ensure the subscriber is receiving messages
    alice->startReading(
          [prom, &maxLoop, slotToCall](const std::string& slot, bool /*isBroadcast*/, Hash::Pointer h,
                                       Hash::Pointer data) {
              *slotToCall = slot;
              int loop = h->get<int>("count");
              if (loop >= maxLoop) prom->set_value(true);
          },
          [prom](consumer::Error err, const std::string& msg) { prom->set_value(false); });

    error_code ec = alice->subscribeToRemoteSignal("aliceSlot", "bob", "signalFromBob");
    EXPECT_TRUE(!ec);

    // Clone a producer "Bob" which uses the same broker settings...
    auto bob = alice->clone("bob");
    EXPECT_NO_THROW(bob->connect());
    EXPECT_TRUE(bob->isConnected());
    EXPECT_TRUE(bob->getBrokerType() == classId);
    EXPECT_TRUE(bob->getInstanceId() == "bob");
    EXPECT_TRUE(bob->getBrokerUrl() == alice->getBrokerUrl());
    EXPECT_TRUE(bob->getDomain() == alice->getDomain());

    auto hdr = std::make_shared<Hash>("signalInstanceId", "bob");
    auto body = std::make_shared<Hash>("a.b.c", 42);

    for (int i = 0; i < maxLoop; ++i) {
        hdr->set("count", i + 1);
        EXPECT_NO_THROW(bob->sendSignal("signalFromBob", hdr, body));
    }

    // Wait on future ...
    EXPECT_EQ(std::future_status::ready, fut.wait_for(m_timeout));
    bool result = fut.get();
    EXPECT_TRUE(result);
    EXPECT_STREQ("aliceSlot", (*slotToCall).c_str());

    ec = alice->unsubscribeFromRemoteSignal("aliceSlot", "bob", "signalFromBob");
    EXPECT_TRUE(!ec);
    EXPECT_NO_THROW(alice->stopReading());
    EXPECT_NO_THROW(bob->disconnect());
    EXPECT_NO_THROW(alice->disconnect());
}


TEST_F(TestBroker, testPublishSubscribe) {
    _loopFunction(__FUNCTION__, [] { ::_testPublishSubscribe(); });
}


static void _testPublishSubscribeAsync() {
    std::string classId = m_config.begin()->getKey();
    m_config.set(classId + ".instanceId", "alice");

    // Create subscriber ...
    auto alice = Configurator<Broker>::create(m_config);
    EXPECT_NO_THROW(alice->connect());
    EXPECT_TRUE(alice->isConnected());
    EXPECT_TRUE(alice->getBrokerType() == classId);
    EXPECT_TRUE(alice->getBrokerUrl() == m_brokersUnderTest.get<std::vector<std::string>>(classId)[0]);
    EXPECT_TRUE(alice->getInstanceId() == "alice");

    auto prom = std::make_shared<std::promise<bool>>();
    auto fut = prom->get_future();
    auto slotToCall = std::make_shared<std::string>();

    constexpr int maxLoop = 10;

    // Ensure the subscriber is receiving messages
    alice->startReading(
          [prom, &maxLoop, slotToCall](const std::string& slot, bool /*isBroadcast*/, Hash::Pointer h,
                                       Hash::Pointer data) {
              *slotToCall = slot;
              int loop = data->get<int>("c");
              if (loop >= maxLoop) prom->set_value(true);
          },
          [prom](consumer::Error err, const std::string& msg) { prom->set_value(false); });

    {
        auto p = std::make_shared<std::promise<boost::system::error_code>>();
        auto f = p->get_future();
        alice->subscribeToRemoteSignalAsync("aliceSlot", "bob", "signalFromBob",
                                            [p](const boost::system::error_code& ec) { p->set_value(ec); });
        EXPECT_EQ(std::future_status::ready, f.wait_for(m_timeout));
        auto ec = f.get();
        EXPECT_TRUE(!ec);
    }

    auto bob = alice->clone("bob");

    auto t = std::jthread([maxLoop, classId, alice, bob]() {
        using namespace std::chrono_literals;

        EXPECT_NO_THROW(bob->connect());
        EXPECT_TRUE(bob->isConnected());
        EXPECT_TRUE(bob->getBrokerType() == classId);
        EXPECT_TRUE(bob->getInstanceId() == "bob");
        EXPECT_TRUE(bob->getDomain() == alice->getDomain());

        Hash::Pointer header = std::make_shared<Hash>("signalInstanceId", "bob");

        Hash::Pointer data = std::make_shared<Hash>("a", std::string("free text"), "b", 3.1415F);

        for (int i = 0; i < maxLoop; ++i) {
            data->set<int>("c", i + 1);
            EXPECT_NO_THROW(bob->sendSignal("signalFromBob", header, data));
        }
    });

    // Wait on future ... when Alice reads all maxLoop messages or failure happens...
    EXPECT_EQ(std::future_status::ready, fut.wait_for(m_timeout));
    bool result = fut.get();
    EXPECT_TRUE(result);
    t.join(); // join thread ... otherwise application is terminated
    EXPECT_STREQ("aliceSlot", (*slotToCall).c_str());

    {
        auto p = std::make_shared<std::promise<boost::system::error_code>>();
        auto f = p->get_future();
        alice->unsubscribeFromRemoteSignalAsync("aliceSlot", "bob", "signalFromBob",
                                                [p](const boost::system::error_code& ec) { p->set_value(ec); });
        EXPECT_EQ(std::future_status::ready, f.wait_for(m_timeout));
        auto ec = f.get();
        EXPECT_TRUE(!ec);
    }
    EXPECT_NO_THROW(alice->stopReading());
    EXPECT_NO_THROW(bob->disconnect());
    EXPECT_NO_THROW(alice->disconnect());
}


TEST_F(TestBroker, testPublishSubscribeAsync) {
    _loopFunction(__FUNCTION__, [] { ::_testPublishSubscribeAsync(); });
}


static void _testReadingHeartbeats() {
    std::string classId = m_config.begin()->getKey();
    m_config.set(classId + ".instanceId", "alice");

    // Create subscriber ...
    auto alice = Configurator<Broker>::create(m_config);
    EXPECT_NO_THROW(alice->connect());
    EXPECT_TRUE(alice->isConnected());
    EXPECT_TRUE(alice->getBrokerType() == classId);
    EXPECT_TRUE(alice->getBrokerUrl() == m_brokersUnderTest.get<std::vector<std::string>>(classId)[0]);
    EXPECT_TRUE(alice->getInstanceId() == "alice");

    auto prom = std::make_shared<std::promise<bool>>();
    auto fut = prom->get_future();
    auto promBeats = std::make_shared<std::promise<bool>>();
    auto futBeats = promBeats->get_future();

    constexpr int maxLoop = 10;
    int counterBeats = 0;
    // Ensure the subscriber is receiving messages
    alice->startReading(
          [prom, promBeats, maxLoop, &counterBeats](const std::string& slot, bool isBroadcast, Hash::Pointer h,
                                                    Hash::Pointer data) {
              if (slot == "slotAlice") {
                  try {
                      EXPECT_STREQ("bob", h->get<std::string>("signalInstanceId").c_str());
                      EXPECT_EQ(1, data->get<int>("c"));
                      EXPECT_TRUE(!isBroadcast);
                  } catch (const std::exception& e) {
                      std::clog << __FILE__ << ":" << __LINE__ << " " << e.what() << std::endl;
                      prom->set_value(false);
                      return;
                  }
                  prom->set_value(true);
              } else if (slot == "slotHeartbeat") {
                  try {
                      Hash::Pointer d(data);
                      EXPECT_STREQ("bob", h->get<std::string>("signalInstanceId").c_str());
                      EXPECT_TRUE(d->has("a1"));
                      EXPECT_TRUE(d->has("a2"));
                      EXPECT_TRUE(!d->has("a3"));
                      EXPECT_TRUE(d->has("a2.c"));
                      EXPECT_EQ(counterBeats, d->get<int>("a2.c"));
                      EXPECT_TRUE(isBroadcast);
                      if (++counterBeats == maxLoop) promBeats->set_value(true);
                  } catch (const std::exception& e) {
                      std::clog << __FILE__ << ":" << __LINE__ << " " << e.what() << std::endl;
                      promBeats->set_value(false);
                  }
              } else {
                  std::clog << "Unknown slot received: " << slot << std::endl;
                  prom->set_value(false);
                  promBeats->set_value(false);
              }
          },
          [prom](consumer::Error err, const std::string& msg) { prom->set_value(false); });

    alice->startReadingHeartbeats();

    {
        boost::system::error_code ec = alice->subscribeToRemoteSignal("slotAlice", "bob", "signalFromBob");
        EXPECT_TRUE(!ec);
    }

    auto bob = alice->clone("bob");

    auto t = std::jthread([maxLoop, classId, alice, bob]() {
        EXPECT_NO_THROW(bob->connect());
        EXPECT_TRUE(bob->isConnected());
        EXPECT_TRUE(bob->getBrokerType() == classId);
        EXPECT_TRUE(bob->getInstanceId() == "bob");
        EXPECT_TRUE(bob->getDomain() == alice->getDomain());

        Hash::Pointer header = std::make_shared<Hash>("signalInstanceId", "bob");

        Hash::Pointer data = std::make_shared<Hash>(
              "a1", std::string("bob"), "a2",
              Hash("type", "device", "classId", "Broker", "serverId", "__none__", "lang", "cpp"));

        for (int i = 0; i < maxLoop; ++i) {
            // Bob sends heartbeat
            data->set<int>("a2.c", i);
            EXPECT_NO_THROW(bob->sendBroadcast("slotHeartbeat", header, data));
        }

        Hash::Pointer h2 = std::make_shared<Hash>("signalInstanceId", "bob");
        Hash::Pointer d2 = std::make_shared<Hash>("c", 1);

        // Trigger the end of the test
        EXPECT_NO_THROW(bob->sendSignal("signalFromBob", h2, d2));
    });

    // Wait on future ... when Alice reads all maxLoop messages or failure happens...

    EXPECT_EQ(std::future_status::ready, futBeats.wait_for(m_timeout));
    const bool resultBeats = futBeats.get();
    EXPECT_TRUE(resultBeats);
    EXPECT_EQ(std::future_status::ready, fut.wait_for(m_timeout));
    const bool result = fut.get();
    EXPECT_TRUE(result);
    t.join(); // join  ... otherwise terminate() called

    EXPECT_NO_THROW(alice->stopReading()); // unsubscribeAll
    EXPECT_NO_THROW(bob->disconnect());
    EXPECT_NO_THROW(alice->disconnect());
}


TEST_F(TestBroker, testReadingHeartbeats) {
    _loopFunction(__FUNCTION__, [] { ::_testReadingHeartbeats(); });
}


static void _testReadingGlobalCalls(const std::vector<std::string>& brokerAddress) {
    std::string type = Broker::brokerTypeFrom(brokerAddress);

    Hash cfg("brokers", brokerAddress, "domain", m_domain, "instanceId", "listenGlobal");
    Broker::Pointer listenGlobal = Configurator<Broker>::create(type, cfg);

    cfg.set("instanceId", "notListenGlobal");
    Broker::Pointer notListenGlobal = Configurator<Broker>::create(type, cfg);
    notListenGlobal->setConsumeBroadcasts(false);

    cfg.set("instanceId", "sender");
    Broker::Pointer sender = Configurator<Broker>::create(type, cfg);

    EXPECT_NO_THROW(listenGlobal->connect());
    EXPECT_NO_THROW(notListenGlobal->connect());
    EXPECT_NO_THROW(sender->connect());

    auto promGlobal1 = std::make_shared<std::promise<std::string>>();
    auto futGlobal1 = promGlobal1->get_future();
    auto promNonGlobal1 = std::make_shared<std::promise<std::string>>();
    auto futNonGlobal1 = promNonGlobal1->get_future();

    auto readHandlerBoth1 = [promGlobal1, promNonGlobal1](const std::string& slot, bool isBroadcast, Hash::Pointer hdr,
                                                          Hash::Pointer body) {
        if (body->has("msg") && body->is<std::string>("msg") && !isBroadcast) {
            promNonGlobal1->set_value(body->get<std::string>("msg"));
        } else if (body->has("msgToAll") && body->is<std::string>("msgToAll") && isBroadcast) {
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

    auto readHandlerBoth2 = [promGlobal2, promNonGlobal2](const std::string& slot, bool isBroadcast, Hash::Pointer hdr,
                                                          Hash::Pointer body) {
        if (body->has("msg") && body->is<std::string>("msg") && !isBroadcast) {
            promNonGlobal2->set_value(body->get<std::string>("msg"));
        } else if (body->has("msgToAll") && body->is<std::string>("msgToAll") && isBroadcast) {
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
    auto hdr = std::make_shared<Hash>("signalInstanceId", sender->getInstanceId());
    auto bodyGlobal = std::make_shared<Hash>("msgToAll", "A global message");
    sender->sendBroadcast("slotInstanceNew", hdr, bodyGlobal); // Note: not all slots can be broadcasted

    // Send specific messages
    auto bodyNonGlobal = std::make_shared<Hash>("msg", "A specific message");
    sender->sendOneToOne(listenGlobal->getInstanceId(), "simpleSlot", hdr, bodyNonGlobal);
    sender->sendOneToOne(notListenGlobal->getInstanceId(), "simpleSlot", hdr, bodyNonGlobal);

    // Assert that both messages arrived at listenGlobal
    EXPECT_EQ(std::future_status::ready, futGlobal1.wait_for(m_timeout));
    const std::string msg = futGlobal1.get();
    EXPECT_STREQ("A global message", msg.c_str());

    EXPECT_EQ(std::future_status::ready, futNonGlobal1.wait_for(m_timeout));
    const std::string msg2 = futNonGlobal1.get();
    EXPECT_STREQ("A specific message", msg2.c_str());

    // At listNonGlobal, only the non-global message arrives
    EXPECT_EQ(std::future_status::ready, futNonGlobal2.wait_for(m_timeout));
    const std::string msg3 = futNonGlobal2.get();
    EXPECT_EQ("A specific message", msg3);

    auto status = futGlobal2.wait_for(std::chrono::milliseconds(100));
    EXPECT_EQ(std::future_status::timeout, status);

    std::clog << "OK." << std::endl;
}


TEST_F(TestBroker, testReadingGlobalCalls) {
    for (Hash::const_iterator it = m_brokersUnderTest.begin(); it != m_brokersUnderTest.end(); ++it) {
        const std::vector<std::string>& brokers = it->getValue<std::vector<std::string>>();
        const std::string& protocol = it->getKey();
        std::clog << "\n\t" << __FUNCTION__ << " " << protocol << " : '" << toString(brokers) << "'" << std::endl;
        ::_testReadingGlobalCalls(brokers);
    }
}


static void _testProducerRestartConsumerContinues() {
    std::string classId = m_config.begin()->getKey();
    Hash aliceConfig = m_config;
    aliceConfig.set(classId + ".instanceId", "alice");

    auto prom = std::make_shared<std::promise<bool>>();
    auto fut = prom->get_future();

    std::vector<int> bottle1;
    std::vector<int> bottle2;
    std::vector<int> bottle3;

    auto alice = Configurator<Broker>::create(aliceConfig);
    EXPECT_NO_THROW(alice->connect());
    EXPECT_TRUE(alice->isConnected());

    auto errorMessage = [prom](consumer::Error err, const std::string& desc) {
        std::clog << "Alice: Error ==> " << int(err) << " -- " << desc << std::endl;
        prom->set_value(false);
    };
    auto parseMessage = [prom, &bottle1, &bottle2, &bottle3](const std::string& slot, bool /*isBroadcast*/,
                                                             Hash::Pointer h, Hash::Pointer d) {
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
    ec = alice->subscribeToRemoteSignal("aliceSlot", "bob", "signalFromBob");
    EXPECT_TRUE(!ec);

    auto t = std::jthread([]() {
        std::string classId = m_config.begin()->getKey();
        Hash bobConfig = m_config;
        bobConfig.set(classId + ".instanceId", "bob");

        auto bob = Configurator<Broker>::create(bobConfig);
        EXPECT_NO_THROW(bob->connect());
        EXPECT_TRUE(bob->isConnected());

        Hash::Pointer header = std::make_shared<Hash>("signalInstanceId", "bob");

        Hash::Pointer data = std::make_shared<Hash>("fill", "bottle1");

        for (int i = 1; i <= 16; ++i) {
            data->set("c", i);
            EXPECT_NO_THROW(bob->sendOneToOne("alice", "aliceSlot", header, data));
        }

        EXPECT_NO_THROW(bob->disconnect());
        bob.reset();

        // Bob restarts ... Alice continues ...

        bob = Configurator<Broker>::create(bobConfig); // new incarnation of Bob
        EXPECT_NO_THROW(bob->connect());
        EXPECT_TRUE(bob->isConnected());

        data->set("fill", "bottle2");

        for (int i = 1; i <= 20; ++i) {
            data->set("c", -i);
            EXPECT_NO_THROW(bob->sendOneToOne("alice", "aliceSlot", header, data));
        }

        Hash::Pointer stop(new Hash("stop", Hash()));
        EXPECT_NO_THROW(bob->sendOneToOne("alice", "aliceSlot", header, stop));
        EXPECT_NO_THROW(bob->disconnect());
    });

    EXPECT_EQ(std::future_status::ready, fut.wait_for(m_timeout));
    bool result = fut.get(); // wait until bottles are filled
    EXPECT_TRUE(result);

    t.join();

    ec = alice->unsubscribeFromRemoteSignal("aliceSlot", "bob", "signalBob");
    EXPECT_TRUE(!ec);

    EXPECT_NO_THROW(alice->disconnect());

    EXPECT_EQ(16ul, bottle1.size());
    for (int i = 1; i <= int(bottle1.size()); ++i) EXPECT_EQ(i, bottle1[i - 1]);

    EXPECT_EQ(20ul, bottle2.size());
    for (int i = 1; i <= int(bottle2.size()); ++i) EXPECT_EQ(-i, bottle2[i - 1]);

    EXPECT_EQ(0ul, bottle3.size());
}


TEST_F(TestBroker, testProducerRestartConsumerContinues) {
    _loopFunction(__FUNCTION__, [] { ::_testProducerRestartConsumerContinues(); });
}


static void _testProducerContinuesConsumerRestart() {
    std::string classId = m_config.begin()->getKey();
    Hash aliceConfig = m_config;
    aliceConfig.set(classId + ".instanceId", "alice");

    boost::system::error_code ec;

    std::vector<int> bottle;

    // Create Bob instance
    Hash bobConfig = m_config;
    bobConfig.set(classId + ".instanceId", "bob");

    auto bob = Configurator<Broker>::create(bobConfig);

    EXPECT_NO_THROW(bob->connect());
    EXPECT_TRUE(bob->isConnected());

    Hash::Pointer header = std::make_shared<Hash>("signalInstanceId", "bob");
    Hash::Pointer data = std::make_shared<Hash>(); // data container

    Broker::Pointer alice;

    alice = Configurator<Broker>::create(aliceConfig);
    EXPECT_NO_THROW(alice->connect());
    EXPECT_TRUE(alice->isConnected());

    auto p1 = std::make_shared<std::promise<bool>>();
    auto f1 = p1->get_future();

    auto error1 = [p1](consumer::Error err, const std::string& desc) {
        std::clog << "Alice: Error ==> " << int(err) << " -- " << desc << std::endl;
        p1->set_value(false);
    };

    constexpr int maxLoop1 = 6;
    int loopCount1 = maxLoop1;
    auto parse1 = [p1, &bottle, &loopCount1](const std::string& slot, bool /*isBroadcast*/, Hash::Pointer h,
                                             Hash::Pointer d) {
        int n = d->get<int>("c");
        bottle.push_back(n);
        if (--loopCount1 == 0) p1->set_value(true);
    };

    // Alice is preparing to receive messages ...
    alice->startReading(parse1, error1);
    // This subscription will use callbacks from startReading...
    ec = alice->subscribeToRemoteSignal("aliceSlot", "bob", "signalBob");
    EXPECT_TRUE(!ec);

    for (int i = 1; i <= maxLoop1; ++i) {
        data->set("c", i);
        EXPECT_NO_THROW(bob->sendSignal("signalBob", header, data));
    }

    // Alice waits here for end of step1
    EXPECT_EQ(std::future_status::ready, f1.wait_for(m_timeout));
    bool r1 = f1.get();
    EXPECT_TRUE(r1);
    EXPECT_TRUE(loopCount1 == 0);
    // check bottle...
    EXPECT_TRUE(int(bottle.size()) == maxLoop1);
    for (int i = 1; i <= int(bottle.size()); ++i) EXPECT_TRUE(i == bottle[i - 1]);

    ec = alice->unsubscribeFromRemoteSignal("aliceSlot", "bob", "signalBob");

    // FIXME: Need test that now a "signalBob" fron "bob" does not arrice at alice in "aliceSlot" anymore?
    EXPECT_TRUE(!ec);
    alice->stopReading();

    EXPECT_NO_THROW(alice->disconnect());

    bottle.clear();
    alice.reset();

    // Restart Alice ...

    auto p2 = std::make_shared<std::promise<bool>>();
    auto f2 = p2->get_future();

    alice = Configurator<Broker>::create(aliceConfig);
    EXPECT_NO_THROW(alice->connect());
    EXPECT_TRUE(alice->isConnected());

    auto error2 = [p2](consumer::Error err, const std::string& desc) { p2->set_value(false); };

    constexpr int maxLoop2 = 20;
    int loopCount2 = maxLoop2;
    auto parse2 = [p2, &bottle, &loopCount2](const std::string& slot, bool /*isBroadcast*/, Hash::Pointer h,
                                             Hash::Pointer d) {
        int n = d->get<int>("c");
        bottle.push_back(n); // fill the "bottle"
        if (--loopCount2 == 0) p2->set_value(true);
    };


    alice->startReading(parse2, error2);
    ec = alice->subscribeToRemoteSignal("aliceSlot", "bob", "signalBob");
    EXPECT_TRUE(!ec);

    // Bob continues ...
    // send negative numbers ...
    for (int i = 1; i <= maxLoop2; ++i) {
        data->set("c", -i);
        EXPECT_NO_THROW(bob->sendSignal("signalBob", header, data));
    }

    EXPECT_EQ(std::future_status::ready, f2.wait_for(m_timeout));
    auto r2 = f2.get();
    EXPECT_TRUE(r2);

    ec = alice->unsubscribeFromRemoteSignal("aliceSlot", "bob", "signalBob");
    EXPECT_TRUE(!ec);
    alice->stopReading();

    EXPECT_NO_THROW(alice->disconnect());

    EXPECT_NO_THROW(bob->disconnect());

    EXPECT_EQ(maxLoop2, int(bottle.size()));
    for (int i = 1; i <= int(bottle.size()); ++i) EXPECT_EQ(-i, bottle[i - 1]);
}


TEST_F(TestBroker, testProducerContinuesConsumerRestart) {
    _loopFunction(__FUNCTION__, [] { ::_testProducerContinuesConsumerRestart(); });
}
