/* 
 * File:   Broker_Test.cc
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 * 
 * Created on February 19, 2021, 3:09 PM
 */

#include "Broker_Test.hh"
#include <karabo/util/Hash.hh>
#include <karabo/net/EventLoop.hh>


#define MQTT_BROKER "mqtt://exfldl02n0:1883"
#define JMS_BROKER "tcp://exflbkr02n0:7777"
#define INVALID_MQTT "mqtt://invalid.example.org:1883"
#define INVALID_JMS  "tcp://invalid.example.org:7777"

using namespace karabo::util;
using namespace karabo::net;
using boost::system::error_code;


CPPUNIT_TEST_SUITE_REGISTRATION(Broker_Test);


Broker_Test::Broker_Test() : m_config() {
    unsetenv("KARABO_BROKER");
    //m_brokers = getenv("KARABO_BROKER") ? getenv("KARABO_BROKER") : "tcp://exfl-broker:7777";
    m_domain = Broker::brokerDomainFromEnv();
}


Broker_Test::~Broker_Test() {
}


void Broker_Test::setUp() {
    auto prom = std::promise<void>();
    auto fut = prom.get_future();
    m_thread = boost::make_shared<boost::thread>([&prom]() {
        // postpone promise setting until EventLoop is activated
        EventLoop::getIOService().post([&prom]() {
            prom.set_value();
        });
                                                 EventLoop::work();
    });
    fut.get(); // block here until promise is set
}


void Broker_Test::tearDown() {
    EventLoop::stop();
    m_thread->join();
    m_thread.reset();
}


void Broker_Test::testConnectDisconnect() {
    std::string id = "alice";
    std::string urls = std::string(INVALID_JMS) + "," + JMS_BROKER;
    setenv("KARABO_BROKER", urls.c_str(), true);
    m_config.clear();
    m_config.set("jms.brokers", fromString<std::string, std::vector>(urls));
    m_config.set("jms.domain", m_domain);
    m_config.set<std::string>("jms.instanceId", id);
    _testConnectDisconnect();

    urls = std::string(INVALID_MQTT) + "," + MQTT_BROKER;
    setenv("KARABO_BROKER", urls.c_str(), true);
    m_config.clear();
    m_config.set("mqtt.brokers", fromString<std::string, std::vector>(urls));
    m_config.set("mqtt.domain", m_domain);
    m_config.set("mqtt.instanceId", id);
    _testConnectDisconnect();
}


void Broker_Test::_testConnectDisconnect() {
    std::string classId = m_config.begin()->getKey();
    Broker::Pointer broker = Configurator<Broker>::create(m_config);
    broker->connect();

    CPPUNIT_ASSERT(broker->isConnected());
    CPPUNIT_ASSERT(broker->getBrokerType() == classId);
    CPPUNIT_ASSERT(broker->getBrokerUrl() == JMS_BROKER || broker->getBrokerUrl() == MQTT_BROKER);
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

    std::string urls = JMS_BROKER;
    setenv("KARABO_BROKER", urls.c_str(), true);
    m_config.clear();
    m_config.set("jms.brokers", fromString<std::string, std::vector>(urls));
    m_config.set("jms.domain", m_domain);
    _testPublishSubscribe();

    urls = MQTT_BROKER;
    setenv("KARABO_BROKER", urls.c_str(), true);
    m_config.clear();
    m_config.set("mqtt.brokers", fromString<std::string, std::vector>(urls));
    m_config.set("mqtt.domain", m_domain);
    _testPublishSubscribe();
}


void Broker_Test::_testPublishSubscribe() {

    std::string classId = m_config.begin()->getKey();
    m_config.set(classId + ".instanceId", "alice");

    // Create subscriber ...
    auto alice = Configurator<Broker>::create(m_config);
    CPPUNIT_ASSERT_NO_THROW(alice->connect());
    CPPUNIT_ASSERT(alice->isConnected());
    CPPUNIT_ASSERT(alice->getBrokerType() == classId);
    CPPUNIT_ASSERT(alice->getBrokerUrl() == JMS_BROKER || alice->getBrokerUrl() == MQTT_BROKER);
    CPPUNIT_ASSERT(alice->getInstanceId() == "alice");

    auto prom = std::make_shared<std::promise<bool> >();
    auto fut = prom->get_future();

    constexpr int maxLoop = 10;

    // Ensure the subscriber is receiving messages
    alice->startReading([prom, &maxLoop]
                        (Hash::Pointer h, Hash::Pointer data) {
                            int loop = h->get<int>("count");
                            if (loop >= maxLoop) prom->set_value(true);
                        },
                        [prom]
                        (consumer::Error err, const std::string & msg) {
                            prom->set_value(false);
                        });

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

    auto hdr = boost::make_shared<Hash>("signalInstanceId", "bob",
                                        "signalFunction", "signalFromBob",
                                        "slotInstanceIds", "|alice|",
                                        "slotFunctions", "|alice:aliceSlot");
    auto body = boost::make_shared<Hash>("a.b.c", 42);

    for (int i = 0; i < maxLoop; ++i) {
        hdr->set("count", i + 1);
        CPPUNIT_ASSERT_NO_THROW(bob->write(m_domain, hdr, body, 4, 0));
    }

    // Wait on future ...
    bool result = fut.get();
    CPPUNIT_ASSERT(result);

    CPPUNIT_ASSERT_NO_THROW(bob->disconnect());
    CPPUNIT_ASSERT_NO_THROW(alice->stopReading());
    ec = alice->unsubscribeFromRemoteSignal("bob", "signalFromBob");
    CPPUNIT_ASSERT(!ec);
    CPPUNIT_ASSERT_NO_THROW(alice->disconnect());
}


void Broker_Test::testPublishSubscribeAsync() {

    std::string urls = JMS_BROKER;
    setenv("KARABO_BROKER", urls.c_str(), true);
    m_config.clear();
    m_config.set("jms.brokers", fromString<std::string, std::vector>(urls));
    m_config.set("jms.domain", m_domain);
    _testPublishSubscribeAsync();

    urls = MQTT_BROKER;
    setenv("KARABO_BROKER", urls.c_str(), true);
    m_config.clear();
    m_config.set("mqtt.brokers", fromString<std::string, std::vector>(urls));
    m_config.set("mqtt.domain", m_domain);
    _testPublishSubscribeAsync();
}


void Broker_Test::_testPublishSubscribeAsync() {

    std::string classId = m_config.begin()->getKey();
    m_config.set(classId + ".instanceId", "alice");

    // Create subscriber ...
    auto alice = Configurator<Broker>::create(m_config);
    CPPUNIT_ASSERT_NO_THROW(alice->connect());
    CPPUNIT_ASSERT(alice->isConnected());
    CPPUNIT_ASSERT(alice->getBrokerType() == classId);
    CPPUNIT_ASSERT(alice->getBrokerUrl() == JMS_BROKER || alice->getBrokerUrl() == MQTT_BROKER);
    CPPUNIT_ASSERT(alice->getInstanceId() == "alice");

    auto prom = std::make_shared<std::promise<bool> >();
    auto fut = prom->get_future();

    constexpr int maxLoop = 10;

    // Ensure the subscriber is receiving messages
    alice->startReading(
        [prom, &maxLoop]
        (Hash::Pointer h, Hash::Pointer data) {
            int loop = data->get<int>("c");
            if (loop >= maxLoop) prom->set_value(true);
        },
        [prom]
        (consumer::Error err, const std::string & msg) {
            prom->set_value(false);
        }
    );

    // NOTE:  MQTT: make sure that subscribing to signals is done AFTER startReading
    //        JMS:  doesn't matter
    {
        auto p = std::make_shared<std::promise<boost::system::error_code> >();
        auto f = p->get_future();
        alice->subscribeToRemoteSignalAsync(
            "bob", "signalFromBob",
            [p]
            (const boost::system::error_code& ec) {
                p->set_value(ec);
            }
        );
        auto ec = f.get();
        CPPUNIT_ASSERT(!ec);
    }

    auto bob = alice->clone("bob");

    auto t = std::thread([this, maxLoop, classId, alice, bob]() {
        using namespace std::chrono_literals;

        CPPUNIT_ASSERT_NO_THROW(bob->connect());
        CPPUNIT_ASSERT(bob->isConnected());
        CPPUNIT_ASSERT(bob->getBrokerType() == classId);
        CPPUNIT_ASSERT(bob->getInstanceId() == "bob");
        CPPUNIT_ASSERT(bob->getDomain() == alice->getDomain());

        Hash::Pointer header = boost::make_shared<Hash>(
                "signalInstanceId", "bob",
                "signalFunction", "signalFromBob",
                "slotInstanceIds", "|alice|",
                "slotFunctions", "|alice:aliceSlot|");

        Hash::Pointer data = boost::make_shared<Hash>(
                "a", std::string("free text"),
                "b", 3.1415F);

        for (int i = 0; i < maxLoop; ++i) {
            data->set<int>("c", i + 1);
            CPPUNIT_ASSERT_NO_THROW(bob->write(bob->getDomain(), header, data, 4, 0));
        }

        CPPUNIT_ASSERT_NO_THROW(bob->disconnect());
    });

    // Wait on future ... when Alice reads all maxLoop messages or failure happens...
    bool result = fut.get();
    CPPUNIT_ASSERT(result);
    t.join(); // join thread ... otherwise application is terminated

    CPPUNIT_ASSERT_NO_THROW(alice->stopReading());
    {
        auto p = std::make_shared<std::promise<boost::system::error_code> >();
        auto f = p->get_future();
        alice->unsubscribeFromRemoteSignalAsync(
            "bob", "signalFromBob",
            [p]
            (const boost::system::error_code& ec) {
                p->set_value(ec);
            }
        );
        auto ec = f.get();
        CPPUNIT_ASSERT(!ec);
    }
    CPPUNIT_ASSERT_NO_THROW(alice->disconnect());
}


void Broker_Test::testReadingHeartbeatsAndLogs() {

    std::string urls = JMS_BROKER;
    setenv("KARABO_BROKER", urls.c_str(), true);
    m_config.clear();
    m_config.set("jms.brokers", fromString<std::string, std::vector>(urls));
    m_config.set("jms.domain", m_domain);
    _testReadingHeartbeatsAndLogs();

    urls = MQTT_BROKER;
    setenv("KARABO_BROKER", urls.c_str(), true);
    m_config.clear();
    m_config.set("mqtt.brokers", fromString<std::string, std::vector>(urls));
    m_config.set("mqtt.domain", m_domain);
    _testReadingHeartbeatsAndLogs();
}


void Broker_Test::_testReadingHeartbeatsAndLogs() {

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
    CPPUNIT_ASSERT(alice->getBrokerUrl() == JMS_BROKER || alice->getBrokerUrl() == MQTT_BROKER);
    CPPUNIT_ASSERT(alice->getInstanceId() == "alice");

    auto prom = std::make_shared<std::promise<bool> >();
    auto fut = prom->get_future();

    constexpr int maxLoop = 10;

    // Ensure the subscriber is receiving messages
    alice->startReading(
        [prom]
        (Hash::Pointer h, Hash::Pointer data) {
            try {
                CPPUNIT_ASSERT(h->get<std::string>("signalInstanceId") == "bob");
                CPPUNIT_ASSERT(h->get<std::string>("signalFunction") == "signalFromBob");
                CPPUNIT_ASSERT(data->get<int>("c") == 1);
            } catch(const std::exception& e) {
                std::clog << __FILE__ << ":" << __LINE__ << " " << e.what() << std::endl;
                prom->set_value(false);
                return;
            }
            prom->set_value(true);
        },
        [prom]
        (consumer::Error err, const std::string & msg) {
            prom->set_value(false);
        }
    );

    alice->startReadingHeartbeats(
        [prom]
        (Hash::Pointer h, Hash::Pointer d) {
            try {
                CPPUNIT_ASSERT(h->get<std::string>("signalFunction") == "signalHeartbeat");
                CPPUNIT_ASSERT(h->get<std::string>("signalInstanceId") == "bob");
                CPPUNIT_ASSERT(d->has("a1"));
                CPPUNIT_ASSERT(d->has("a2"));
                CPPUNIT_ASSERT(d->has("a3"));
            } catch(const std::exception& e) {
                std::clog << __FILE__ << ":" << __LINE__ << " " << e.what() << std::endl;
                prom->set_value(false);
            }
        },
        [prom]
        (karabo::net::consumer::Error ec, const std::string& message) {
            std::clog << "Heartbeat error: " << message << std::endl;
            prom->set_value(false);
        }
    );

    alice->startReadingLogs(
        [prom]
        (Hash::Pointer h, Hash::Pointer d) {
            try {
                CPPUNIT_ASSERT(h->get<std::string>("target") == "log");
                CPPUNIT_ASSERT(d->has("message"));
                const std::string& cache = d->get<std::string>("message");
                CPPUNIT_ASSERT(cache.find("test message") != std::string::npos);
            } catch(const std::exception& e) {
                std::clog << __FILE__ << ":" << __LINE__ << "  " << e.what() << std::endl;
                prom->set_value(false);
            }
        },
        [prom]
        (karabo::net::consumer::Error ec, const std::string& message) {
            std::clog << "LogError: " << message << std::endl;
            prom->set_value(false);
        }
    );

    {
        boost::system::error_code ec = alice->subscribeToRemoteSignal("bob", "signalFromBob");
        CPPUNIT_ASSERT(!ec);
    }

    auto bob = alice->clone("bob");

    auto t = std::thread([this, maxLoop, classId, alice, bob]() {
        using namespace std::chrono_literals;

        CPPUNIT_ASSERT_NO_THROW(bob->connect());
        CPPUNIT_ASSERT(bob->isConnected());
        CPPUNIT_ASSERT(bob->getBrokerType() == classId);
        CPPUNIT_ASSERT(bob->getInstanceId() == "bob");
        CPPUNIT_ASSERT(bob->getDomain() == alice->getDomain());

        Hash::Pointer header = boost::make_shared<Hash>(
                "signalInstanceId", "bob",
                "signalFunction", "signalHeartbeat",
                "slotInstanceIds", "__none__",
                "slotFunctions", "__none__");

        Hash::Pointer data = boost::make_shared<Hash>(
                "a1", std::string("bob"),
                "a2", 1,
                "a3", Hash(
                    "type", "device",
                    "classId", "Broker",
                    "serverId", "__none__",
                    "visibilty", 4,
                    "lang", "cpp"
                ));

        for (int i = 0; i < maxLoop; ++i) {
            // Bob sends heartbeat
            data->set<int>("c", i + 1);
            CPPUNIT_ASSERT_NO_THROW(bob->write(m_domain + "_beats", header, data, 0, 0));
            // write 'log' message
            std::ostringstream oss;
            oss << "test message " << (i+1);
            auto h1 = boost::make_shared<Hash>("target","log");
            auto d1 = boost::make_shared<Hash>("message", oss.str());
            CPPUNIT_ASSERT_NO_THROW(bob->write(m_domain, h1, d1, 0, 0));
        }

        Hash::Pointer h2 = boost::make_shared<Hash>(
                "signalInstanceId", "bob",
                "signalFunction", "signalFromBob",
                "slotInstanceIds", "|alice|",
                "slotFunctions", "|alice:someSlot");
        Hash::Pointer d2 = boost::make_shared<Hash>(
                "c", 1);

        // Trigger the end of the test
        CPPUNIT_ASSERT_NO_THROW(bob->write(m_domain, h2, d2, 4, 0));
        CPPUNIT_ASSERT_NO_THROW(bob->disconnect());
    });

    // Wait on future ... when Alice reads all maxLoop messages or failure happens...
    bool result = fut.get();
    CPPUNIT_ASSERT(result);
    t.join();        // join  ... otherwise terminate() called

    CPPUNIT_ASSERT_NO_THROW(alice->stopReading()); // unsubscribeAll
    CPPUNIT_ASSERT_NO_THROW(alice->disconnect());
}


void Broker_Test::testReadingGlobalCalls() {

    // TODO: JmsConnection gives precedence to environment variable, even if something else is configured!
    unsetenv("KARABO_BROKER");

    _testReadingGlobalCalls(JMS_BROKER);
    _testReadingGlobalCalls(MQTT_BROKER);
}


void Broker_Test::_testReadingGlobalCalls(const std::string& brokerAddress) {

    const size_t endProtocol = brokerAddress.find(':');
    std::string type = brokerAddress.substr(0, endProtocol);
    if (type == "tcp") {// tcp address means jms protocol
        type = "jms";
    }
    std::clog << "_testReadingGlobalCalls " << type << " (" << brokerAddress << "): " << std::flush;

    Hash cfg("brokers", std::vector<std::string>({brokerAddress}),
             "domain", m_domain,
             "instanceId", "listenGlobal");
    Broker::Pointer listenGlobal = Configurator<Broker>::create(type, cfg);

    cfg.set("instanceId", "notListenGlobal");
    Broker::Pointer notListenGlobal = Configurator<Broker>::create(type, cfg);
    notListenGlobal->setConsumeBroadcasts(false);

    cfg.set("instanceId", "sender");
    Broker::Pointer sender = Configurator<Broker>::create(type, cfg);

    CPPUNIT_ASSERT_NO_THROW(listenGlobal->connect());
    CPPUNIT_ASSERT_NO_THROW(notListenGlobal->connect());
    CPPUNIT_ASSERT_NO_THROW(sender->connect());

    auto promGlobal1 = std::make_shared<std::promise < std::string >> ();
    auto futGlobal1 = promGlobal1->get_future();
    auto promNonGlobal1 = std::make_shared<std::promise < std::string >> ();
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
    auto errorHandlerBoth1 = [promGlobal1, promNonGlobal1] (consumer::Error err, const std::string & msg) {
        // unexpected - invalidate both
        promGlobal1->set_value(msg);
        promNonGlobal1->set_value(msg);
    };

    auto promGlobal2 = std::make_shared<std::promise<std::string> >();
    auto futGlobal2 = promGlobal2->get_future();
    auto promNonGlobal2 = std::make_shared<std::promise<std::string> >();
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
    auto errorHandlerBoth2 = [promGlobal2, promNonGlobal2] (consumer::Error err, const std::string & msg) {
        // unexpected - "invalidate" both
        promGlobal2->set_value(msg);
        promNonGlobal2->set_value(msg);
    };


    listenGlobal->startReading(readHandlerBoth1, errorHandlerBoth1);
    notListenGlobal->startReading(readHandlerBoth2, errorHandlerBoth2);

    // Prepare and send global message
    auto hdr = boost::make_shared<Hash>("signalInstanceId", sender->getInstanceId(),
                                        "signalFunction", "__call__",
                                        "slotInstanceIds", "|*|",
                                        "slotFunctions", "|*:aSlot|"); // MQTT global message needs to know the slot
    auto bodyGlobal = boost::make_shared<Hash>("msgToAll", "A global message");
    sender->write(m_domain, hdr, bodyGlobal, 4, 0);

    // Prepare and send specific messages
    hdr->erase("slotFunctions"); // Specific slot calls do not need their slot for routing, ...
    hdr->set("slotInstanceIds", "|" + listenGlobal->getInstanceId() + "|"); // ... but a specific instanceId
    auto bodyNonGlobal = boost::make_shared<Hash>("msg", "A specific message");
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