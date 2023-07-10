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
#include "MqttClient_Test.hh"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/thread.hpp>
#include <chrono>
#include <future>
#include <tuple>
#include <unordered_set>

#include "karabo/log/Logger.hh"
#include "karabo/net/Broker.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/MqttClient.hh"
#include "karabo/tests/BrokerUtils.hh"

using namespace karabo::util;
using namespace karabo::net;
using namespace karabo::log;

using boost::placeholders::_1;
using boost::placeholders::_2;
using boost::placeholders::_3;

CPPUNIT_TEST_SUITE_REGISTRATION(MqttClient_Test);
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MqttClient_Test, "MqttClient");

constexpr uint32_t TEST_EXPIRATION_TIME_IN_SECONDS = 3;

typedef boost::shared_ptr<boost::asio::io_context> ContextPtr;

template <typename MessageProducer>
class Publisher : public MessageProducer {
   public:
    Publisher(const std::string& instanceId, const PubOpts qos, const std::string& topic)
        : MessageProducer(instanceId),
          m_instanceId(instanceId),
          m_qos(qos),
          m_topic(topic),
          m_future(m_promise.get_future()),
          m_brokers(getMqttBrokerFromEnv()),
          m_domain(Broker::brokerDomainFromEnv()) {}

    void start(const std::string& classId) {
        Hash config("brokers", m_brokers, "domain", m_domain, "instanceId", m_instanceId);
        // auto publisherClient = Configurator<MqttClient>::create("MqttCppClient", config);
        m_client = MqttClient::create(classId, config);
        m_client->connectAsync(boost::bind(&Publisher::handleConnect, this, _1));
    }

    void wait() {
        auto status = m_future.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
        CPPUNIT_ASSERT(status == std::future_status::ready);
    }

   private:
    void handleConnect(const boost::system::error_code ec) {
        CPPUNIT_ASSERT(!ec);
        CPPUNIT_ASSERT(m_client->isConnected() == true);
        publishMessage();
    }
    void publishMessage() {
        auto msg = MessageProducer::produceMessage();
        if (msg) {
            m_client->publishAsync(m_topic, msg, m_qos, [this](const boost::system::error_code ec) {
                CPPUNIT_ASSERT_MESSAGE("*** Publish: Error code is not zero", !ec);
                publishMessage();
            });
        } else {
            m_client->disconnectAsync([this](const boost::system::error_code& ec) {
                CPPUNIT_ASSERT_MESSAGE("*** disconnectAsync : Error code is not zero", !ec);
                CPPUNIT_ASSERT_MESSAGE("*** disconnectAsync : client is still connected",
                                       m_client->isConnected() == false);
                m_promise.set_value();
            });
        }
    }

    MqttClient::Pointer m_client;
    const std::string m_instanceId;
    const PubOpts m_qos;
    const std::string m_topic;
    std::promise<void> m_promise;
    std::future<void> m_future;
    std::vector<std::string> m_brokers;
    std::string m_domain;
};


template <typename MessageHandler>
class Subscriber : public MessageHandler {
   public:
    Subscriber(const std::string& instanceId, const SubOpts qos, const std::string& topic)
        : m_instanceId(instanceId),
          m_qos(qos),
          m_topic(topic),
          m_future(m_promise.get_future()),
          m_subFuture(m_subPromise.get_future()),
          m_brokers(getMqttBrokerFromEnv()),
          m_domain(Broker::brokerDomainFromEnv()) {}

    void start(const std::string& classId) {
        Hash config("brokers", m_brokers, "domain", m_domain, "instanceId", m_instanceId);
        m_client = MqttClient::create(classId, config);
        m_client->connectAsync([this](const boost::system::error_code ec) {
            CPPUNIT_ASSERT(!ec);
            CPPUNIT_ASSERT(m_client->isConnected() == true);
            m_client->subscribeAsync(m_topic, m_qos, boost::bind(&Subscriber::handleRead, this, _1, _2, _3),
                                     [this](const boost::system::error_code ec) {
                                         CPPUNIT_ASSERT(!ec);
                                         CPPUNIT_ASSERT(m_client->isSubscribed(m_topic) == true);
                                         m_subPromise.set_value();
                                     });
        });
    }

    void wait() {
        auto status = m_future.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
        CPPUNIT_ASSERT(status == std::future_status::ready);
    }

    void waitForSubscription() {
        m_subFuture.wait();
    }

   private:
    void handleRead(boost::system::error_code ec, const std::string topic, const karabo::util::Hash::Pointer msg) {
        CPPUNIT_ASSERT(!ec);
        bool disconnect = MessageHandler::handleMessage(topic, msg);
        if (disconnect) {
            m_client->unsubscribeAsync(m_topic, [this](const boost::system::error_code ec) {
                CPPUNIT_ASSERT(!ec);
                CPPUNIT_ASSERT(m_client->isSubscribed(m_topic) == false);
                m_client->disconnectAsync([this](const boost::system::error_code ec) {
                    CPPUNIT_ASSERT(!ec);
                    CPPUNIT_ASSERT(m_client->isConnected() == false);
                    m_promise.set_value();
                });
            });
        }
    }
    MqttClient::Pointer m_client;
    const std::string m_instanceId;
    const SubOpts m_qos;
    const std::string m_topic;
    std::promise<void> m_promise;
    std::future<void> m_future;
    std::promise<void> m_subPromise;
    std::future<void> m_subFuture;
    std::vector<std::string> m_brokers;
    std::string m_domain;
};


MqttClient_Test::MqttClient_Test() : m_brokers(getMqttBrokerFromEnv()), m_domain(Broker::brokerDomainFromEnv()) {
    // Logger::configure(Hash("priority","ERROR"));
    // Logger::useOstream();
    std::clog << std::flush;
}


MqttClient_Test::~MqttClient_Test() {
    // Logger.reset();
}


void MqttClient_Test::testTopicHasWildcard() {
    CPPUNIT_ASSERT(!mqtttools::topicHasWildcard("foo/bar"));
    CPPUNIT_ASSERT(!mqtttools::topicHasWildcard("foo"));
    CPPUNIT_ASSERT(!mqtttools::topicHasWildcard(""));
    CPPUNIT_ASSERT(mqtttools::topicHasWildcard("foo/+"));
    CPPUNIT_ASSERT(mqtttools::topicHasWildcard("+/bar"));
    CPPUNIT_ASSERT(mqtttools::topicHasWildcard("foo/#"));
    CPPUNIT_ASSERT(mqtttools::topicHasWildcard("#"));
    CPPUNIT_ASSERT(mqtttools::topicHasWildcard("foo/+/baz"));
}


void MqttClient_Test::testTopicMatch() {
    CPPUNIT_ASSERT(!mqtttools::topicMatches("", ""));
    CPPUNIT_ASSERT(!mqtttools::topicMatches("", "foo/bar"));
    CPPUNIT_ASSERT(!mqtttools::topicMatches("foo/bar", ""));
    CPPUNIT_ASSERT(!mqtttools::topicMatches("foo/bar", "foo"));
    CPPUNIT_ASSERT(!mqtttools::topicMatches("foo/+", "foo/bar/baz"));
    CPPUNIT_ASSERT(!mqtttools::topicMatches("foo/+/baz", "foo/bar/bar"));
    CPPUNIT_ASSERT(!mqtttools::topicMatches("foo/+/#", "fo2/bar/baz"));
    CPPUNIT_ASSERT(!mqtttools::topicMatches("/#", "foo/bar"));
    CPPUNIT_ASSERT(mqtttools::topicMatches("foo/bar", "foo/bar"));
    CPPUNIT_ASSERT(mqtttools::topicMatches("foo/+", "foo/bar"));
    CPPUNIT_ASSERT(mqtttools::topicMatches("foo/+/baz", "foo/bar/baz"));
    CPPUNIT_ASSERT(mqtttools::topicMatches("foo/+/#", "foo/bar/baz"));
    CPPUNIT_ASSERT(mqtttools::topicMatches("foo/#", "foo/bar/baz"));
    CPPUNIT_ASSERT(mqtttools::topicMatches("#", "foo/bar/baz"));
    CPPUNIT_ASSERT(mqtttools::topicMatches("#", "/foo/bar"));
    CPPUNIT_ASSERT(mqtttools::topicMatches("/#", "/foo/bar"));
    CPPUNIT_ASSERT(mqtttools::topicMatches("foo//bar", "foo//bar"));
    CPPUNIT_ASSERT(mqtttools::topicMatches("foo//+", "foo//bar"));
    CPPUNIT_ASSERT(mqtttools::topicMatches("foo/+/+/baz", "foo///baz"));
    CPPUNIT_ASSERT(mqtttools::topicMatches("foo/bar/+", "foo/bar/"));
}


void MqttClient_Test::testConnectSync() {
    if (m_brokers.empty()) {
        std::clog << " No MQTT broker in environment. Skipping..." << std::flush;
        return;
    }
    testConnectSync_("MqttCppClient");
    //    testConnectSync_("MqttPahoClient");
}


void MqttClient_Test::testConnectSync_(const std::string& classId) {
    Hash config("brokers", m_brokers, "domain", m_domain, "instanceId", "testConnectSync_" + classId);

    // Connect to the default broker (environment variable)
    auto client = MqttClient::create(classId, config);
    auto ec = client->connect();
    CPPUNIT_ASSERT(ec == KARABO_ERROR_CODE_SUCCESS);
    CPPUNIT_ASSERT_MESSAGE("Not connected!", client->isConnected() == true);
    std::string default_broker = client->getBrokerUrl();
    ec = client->disconnect();
    CPPUNIT_ASSERT(ec == KARABO_ERROR_CODE_SUCCESS);
    CPPUNIT_ASSERT(client->isConnected() == false);
    client.reset();

    config.set("brokers", m_brokers);
    client = MqttClient::create(classId, config);
    // Try to connect two times, the second time should return an error
    ec = client->connect();
    CPPUNIT_ASSERT(ec == KARABO_ERROR_CODE_SUCCESS);
    CPPUNIT_ASSERT(client->isConnected() == true);
    ec = client->connect();
    CPPUNIT_ASSERT(ec == KARABO_ERROR_CODE_ALREADY_CONNECTED);
    CPPUNIT_ASSERT(client->isConnected() == true);
    ec = client->disconnect();
    CPPUNIT_ASSERT(ec == KARABO_ERROR_CODE_SUCCESS);
    CPPUNIT_ASSERT(client->isConnected() == false);
    client.reset();

    // Try to connect to an invalid broker
    config.set("brokers", fromString<std::string, std::vector>("mqtt://invalidbroker.example.com:7777"));
    client = MqttClient::create(classId, config);
    ec = client->connect();
    CPPUNIT_ASSERT(ec == boost::asio::error::host_not_found);
    CPPUNIT_ASSERT(client->isConnected() == false);
    client.reset();

    // Add an invalid broker before the default broker
    config.set("brokers", fromString<std::string, std::vector>(std::string("mqtt://invalidbroker.example.com:7777,") +
                                                               default_broker));
    client = MqttClient::create(classId, config);
    ec = client->connect();
    CPPUNIT_ASSERT(ec == KARABO_ERROR_CODE_SUCCESS);
    CPPUNIT_ASSERT(client->isConnected() == true);
    CPPUNIT_ASSERT(client->getBrokerUrl() == default_broker);
    ec = client->disconnect();
    CPPUNIT_ASSERT(ec == KARABO_ERROR_CODE_SUCCESS);
    CPPUNIT_ASSERT(client->isConnected() == false);
    client.reset();
}


void MqttClient_Test::testConnectAsync() {
    if (m_brokers.empty()) {
        std::clog << " No MQTT broker in environment. Skipping..." << std::flush;
        return;
    }
    testConnectAsync_("MqttCppClient");
    //    testConnectAsync_("MqttPahoClient");
}


void MqttClient_Test::testConnectAsync_(const std::string& classId) {
    Hash config("brokers", m_brokers, "domain", m_domain, "instanceId", "testConnectAsync_" + classId);

    std::string default_broker;
    {
        std::promise<void> p;
        auto f = p.get_future();

        auto client = MqttClient::create(classId, config);
        // Connect to the default broker
        client->connectAsync([&client, &p, &default_broker](const boost::system::error_code& ec) {
            CPPUNIT_ASSERT(ec == KARABO_ERROR_CODE_SUCCESS);
            CPPUNIT_ASSERT(client->isConnected() == true);
            default_broker = client->getBrokerUrl();
            client->disconnectAsync([&client, &p](const boost::system::error_code& ec) {
                CPPUNIT_ASSERT(ec == KARABO_ERROR_CODE_SUCCESS);
                CPPUNIT_ASSERT(client->isConnected() == false);
                p.set_value();
            });
        });
        auto status = f.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
        CPPUNIT_ASSERT(status == std::future_status::ready);
        client.reset();
    }

    // Connect to an invalid broker ...
    // ... and expect error code : Host not found (authoritative)
    //
    config.set("brokers", fromString<std::string, std::vector>("mqtt://invalidbroker.example.com:7777"));
    {
        std::promise<boost::system::error_code> p;
        auto f = p.get_future();

        auto client = MqttClient::create(classId, config);
        client->connectAsync([&client, &p](const boost::system::error_code& ec) {
            CPPUNIT_ASSERT(ec);
            CPPUNIT_ASSERT(client->isConnected() == false);
            p.set_value(ec);
        });
        boost::system::error_code ec = f.get();
        CPPUNIT_ASSERT(ec == boost::asio::error::host_not_found); // -- Host not found (authoritative)
        client.reset();
    }

    // Connect again to the invalid and then existing broker ...
    config.set("brokers", fromString<std::string, std::vector>(std::string("mqtt://invalidbroker.example.com:7777,") +
                                                               default_broker));
    {
        std::promise<void> p;
        auto f = p.get_future();

        auto client = MqttClient::create(classId, config);
        // Add an invalid broker before the default broker
        client->connectAsync([&client, &p, &default_broker](const boost::system::error_code& ec) {
            CPPUNIT_ASSERT(ec == KARABO_ERROR_CODE_SUCCESS);
            CPPUNIT_ASSERT(client->isConnected() == true);
            CPPUNIT_ASSERT(client->getBrokerUrl() == default_broker);
            client->disconnectAsync([&client, &p](const boost::system::error_code& ec) {
                CPPUNIT_ASSERT(ec == KARABO_ERROR_CODE_SUCCESS);
                CPPUNIT_ASSERT(client->isConnected() == false);
                p.set_value();
            });
        });
        auto status = f.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
        CPPUNIT_ASSERT(status == std::future_status::ready);
        client.reset();
    }
}


void MqttClient_Test::testTryingToCallOperationsWithoutBeingConnected() {
    if (m_brokers.empty()) {
        std::clog << " No MQTT broker in environment. Skipping..." << std::flush;
        return;
    }
    testTryingToCallOperationsWithoutBeingConnected_("MqttCppClient");
    //    testTryingToCallOperationsWithoutBeingConnected_("MqttPahoClient");
}


void MqttClient_Test::testTryingToCallOperationsWithoutBeingConnected_(const std::string& classId) {
    std::promise<void> promise;
    std::future<void> future(promise.get_future());

    Hash config("brokers", m_brokers, "domain", m_domain, "instanceId",
                "testTryingToCallOperationsWithoutBeingConnected");
    auto client = MqttClient::create(classId, config);
    auto ec = client->subscribe("foo", SubQos::AtMostOnce, ReadHashHandler());
    CPPUNIT_ASSERT(ec);
    CPPUNIT_ASSERT(client->isConnected() == false);
    client->subscribeAsync("foo", SubQos::AtMostOnce, ReadHashHandler(), [&](const boost::system::error_code ec) {
        CPPUNIT_ASSERT(ec);
        CPPUNIT_ASSERT(client->isConnected() == false);
        promise.set_value();
    });
    auto status = future.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
    CPPUNIT_ASSERT(status == std::future_status::ready);
}


std::string MqttClient_Test::qos2operationString(const int qos) {
    std::string operationStr;
    switch (qos) {
        case 0:
            operationStr = "PublishAtMostOnce";
            break;
        case 1:
            operationStr = "PublishAtLeastOnce";
            break;
        case 2:
            operationStr = "PublishExactlyOnce";
            break;
        default:
            throw std::invalid_argument("Invalid Qos");
            break;
    }
    return operationStr;
}


void MqttClient_Test::testPublishSubscribeSync(const std::string& classId, const int qos) {
    std::string operationStr = qos2operationString(qos);
    const std::string TOPIC = m_domain + "/test/" + classId + "/" + operationStr;

    // Used for synchronization
    // (http://www.modernescpp.com/index.php/thread-synchronization-with-condition-variables-or-tasks)
    std::promise<void> promise;
    auto future = promise.get_future();

    std::string instanceId = "test" + classId + operationStr;
    Hash config("brokers", m_brokers, "domain", m_domain, "instanceId", instanceId);
    auto client = MqttClient::create(classId, config);
    auto ec = client->connect();
    CPPUNIT_ASSERT(!ec);

    SubOpts subopts = SubQos::AtMostOnce;
    if (qos == 1) {
        subopts = SubQos::AtLeastOnce;
    } else if (qos == 2) {
        subopts = SubQos::ExactlyOnce;
    }

    ec = client->subscribe(
          TOPIC, subopts,
          [&](boost::system::error_code ec, const std::string topic, const karabo::util::Hash::Pointer msg) {
              CPPUNIT_ASSERT(!ec);
              CPPUNIT_ASSERT(topic == TOPIC);
              CPPUNIT_ASSERT(msg->has("header"));
              CPPUNIT_ASSERT(msg->has("body"));
              Hash& header = msg->get<Hash>("header");
              Hash& body = msg->get<Hash>("body");
              CPPUNIT_ASSERT(header.has("header"));
              CPPUNIT_ASSERT(header.get<std::string>("header") == "some header");
              CPPUNIT_ASSERT(body.has("body"));
              CPPUNIT_ASSERT(body.get<int>("body") == 42);
              promise.set_value();
          });
    CPPUNIT_ASSERT(!ec);
    CPPUNIT_ASSERT(client->isSubscribed(TOPIC) == true);

    Hash::Pointer msg(new Hash("header", Hash("header", "some header"), "body", Hash("body", 42)));
    if (qos != 0 && qos != 1 && qos != 2) {
        throw std::invalid_argument("Invalid Qos");
    }
    PubOpts pubopts = PubQos::AtMostOnce;
    if (qos == 1) {
        pubopts = PubQos::AtLeastOnce;
    } else if (qos == 2) {
        pubopts = PubQos::ExactlyOnce;
    }
    ec = client->publish(TOPIC, msg, pubopts);
    CPPUNIT_ASSERT(!ec);
    auto status = future.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
    CPPUNIT_ASSERT(status == std::future_status::ready);
    future.get();
    ec = client->unsubscribe(TOPIC);
    CPPUNIT_ASSERT(!ec);
    CPPUNIT_ASSERT(client->isSubscribed(TOPIC) == false);
    client->disconnect();
    CPPUNIT_ASSERT(client->isConnected() == false);
}


void MqttClient_Test::testPublishManySubscribeSync(const std::string& classId, const int qos) {
    std::string operationStr = qos2operationString(qos);
    const std::string TOPIC1 = m_domain + "/test1/" + classId + "/" + operationStr;
    const std::string TOPIC2 = m_domain + "/test2/" + classId + "/" + operationStr;

    // Used for synchronization
    // (http://www.modernescpp.com/index.php/thread-synchronization-with-condition-variables-or-tasks)
    std::promise<void> promise1;
    auto future1 = promise1.get_future();
    std::promise<void> promise2;
    auto future2 = promise2.get_future();

    std::string instanceId = "test" + classId + operationStr;
    Hash config("brokers", m_brokers, "domain", m_domain, "instanceId", instanceId);
    auto client = MqttClient::create(classId, config);
    auto ec = client->connect();
    CPPUNIT_ASSERT(!ec);

    auto subopts = SubQos::AtMostOnce;
    if (qos == 1) {
        subopts = SubQos::AtLeastOnce;
    } else if (qos == 2) {
        subopts = SubQos::ExactlyOnce;
    }

    ec = client->subscribe(
          TOPIC1, subopts,
          [&](boost::system::error_code ec, const std::string topic, const karabo::util::Hash::Pointer msg) {
              CPPUNIT_ASSERT(!ec);
              CPPUNIT_ASSERT(topic == TOPIC1);
              CPPUNIT_ASSERT(msg->has("header"));
              CPPUNIT_ASSERT(msg->has("body"));
              Hash& header = msg->get<Hash>("header");
              Hash& body = msg->get<Hash>("body");
              CPPUNIT_ASSERT(header.has("header"));
              CPPUNIT_ASSERT(header.get<std::string>("header") == "some header");
              CPPUNIT_ASSERT(body.has("body"));
              CPPUNIT_ASSERT(body.get<int>("body") == 42);
              promise1.set_value();
          });
    CPPUNIT_ASSERT(!ec);
    CPPUNIT_ASSERT(client->isSubscribed(TOPIC1) == true);

    ec = client->subscribe(
          TOPIC2, subopts,
          [&](boost::system::error_code ec, const std::string topic, const karabo::util::Hash::Pointer msg) {
              CPPUNIT_ASSERT(!ec);
              CPPUNIT_ASSERT(topic == TOPIC2);
              CPPUNIT_ASSERT(msg->has("header"));
              CPPUNIT_ASSERT(msg->has("body"));
              Hash& header = msg->get<Hash>("header");
              Hash& body = msg->get<Hash>("body");
              CPPUNIT_ASSERT(header.has("header"));
              CPPUNIT_ASSERT(header.get<std::string>("header") == "some header");
              CPPUNIT_ASSERT(body.has("body"));
              CPPUNIT_ASSERT(body.get<int>("body") == 42);
              promise2.set_value();
          });
    CPPUNIT_ASSERT(!ec);
    CPPUNIT_ASSERT(client->isSubscribed(TOPIC2) == true);

    Hash::Pointer msg(new Hash("header", Hash("header", "some header"), "body", Hash("body", 42)));
    if (qos != 0 && qos != 1 && qos != 2) {
        throw std::invalid_argument("Invalid Qos");
    }
    PubOpts pubopts = PubQos::AtMostOnce;
    if (qos == 1) {
        pubopts = PubQos::AtLeastOnce;
    } else if (qos == 2) {
        pubopts = PubQos::ExactlyOnce;
    }
    ec = client->publish(TOPIC1, msg, pubopts);
    CPPUNIT_ASSERT(!ec);
    ec = client->publish(TOPIC2, msg, pubopts);
    CPPUNIT_ASSERT(!ec);
    auto status1 = future1.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
    CPPUNIT_ASSERT(status1 == std::future_status::ready);
    auto status2 = future2.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
    CPPUNIT_ASSERT(status2 == std::future_status::ready);

    ec = client->unsubscribe(TOPIC1);
    CPPUNIT_ASSERT(!ec);
    CPPUNIT_ASSERT(client->isSubscribed(TOPIC1) == false);
    ec = client->unsubscribe(TOPIC2);
    CPPUNIT_ASSERT(!ec);
    CPPUNIT_ASSERT(client->isSubscribed(TOPIC2) == false);
    client->disconnect();
    CPPUNIT_ASSERT(client->isConnected() == false);
}

void MqttClient_Test::testPublishMultiSubscribeSync(const std::string& classId, const int qos) {
    std::string operationStr = qos2operationString(qos);
    const std::string TOPIC_PREFIX = m_domain + "/" + classId + "/" + operationStr + "/";
    const std::string TOPIC1 = TOPIC_PREFIX + "1";
    const std::string TOPIC2 = TOPIC_PREFIX + "2";

    // Used for synchronization
    // (http://www.modernescpp.com/index.php/thread-synchronization-with-condition-variables-or-tasks)
    std::promise<void> promise1;
    auto future1 = promise1.get_future();
    std::promise<void> promise2;
    auto future2 = promise2.get_future();

    std::string instanceId = "test" + classId + operationStr;
    Hash config("brokers", m_brokers, "domain", m_domain, "instanceId", instanceId);
    auto client = MqttClient::create(classId, config);
    auto ec = client->connect();
    CPPUNIT_ASSERT(!ec);

    if (qos != 0 && qos != 1 && qos != 2) {
        throw std::invalid_argument("Invalid Qos");
    }
    auto subopts = SubQos::AtMostOnce;
    auto pubopts = PubQos::AtMostOnce;
    if (qos == 1) {
        subopts = SubQos::AtLeastOnce;
        pubopts = PubQos::AtLeastOnce;
    } else if (qos == 2) {
        subopts = SubQos::ExactlyOnce;
        pubopts = PubQos::ExactlyOnce;
    }

    std::vector<std::tuple<std::string, SubOpts, ReadHashHandler>> topics;
    topics.push_back(std::make_tuple(
          TOPIC1, subopts,
          [&](boost::system::error_code ec, const std::string topic, const karabo::util::Hash::Pointer msg) {
              CPPUNIT_ASSERT(!ec);
              CPPUNIT_ASSERT(topic == TOPIC1);
              CPPUNIT_ASSERT(msg->has("header"));
              CPPUNIT_ASSERT(msg->has("body"));
              Hash& header = msg->get<Hash>("header");
              Hash& body = msg->get<Hash>("body");
              CPPUNIT_ASSERT(header.has("header"));
              CPPUNIT_ASSERT(header.get<std::string>("header") == "some header");
              CPPUNIT_ASSERT(body.has("body"));
              CPPUNIT_ASSERT(body.get<int>("body") == 42);
              promise1.set_value();
          }));
    topics.push_back(std::make_tuple(
          TOPIC2, subopts,
          [&](boost::system::error_code ec, const std::string topic, const karabo::util::Hash::Pointer msg) {
              CPPUNIT_ASSERT(!ec);
              CPPUNIT_ASSERT(topic == TOPIC2);
              CPPUNIT_ASSERT(msg->has("header"));
              CPPUNIT_ASSERT(msg->has("body"));
              Hash& header = msg->get<Hash>("header");
              Hash& body = msg->get<Hash>("body");
              CPPUNIT_ASSERT(header.has("header"));
              CPPUNIT_ASSERT(header.get<std::string>("header") == "some header");
              CPPUNIT_ASSERT(body.has("body"));
              CPPUNIT_ASSERT(body.get<int>("body") == 42);
              promise2.set_value();
          }));
    ec = client->subscribe(topics);
    if (ec) {
        std::cout << "Multisubscribe failed: code #" << ec.value() << " -- " << ec.message() << std::endl;
    }
    CPPUNIT_ASSERT(!ec);
    CPPUNIT_ASSERT(client->isSubscribed(TOPIC1) == true);

    Hash::Pointer msg(new Hash("header", Hash("header", "some header"), "body", Hash("body", 42)));
    ec = client->publish(TOPIC1, msg, pubopts);
    CPPUNIT_ASSERT(!ec);
    ec = client->publish(TOPIC2, msg, pubopts);
    CPPUNIT_ASSERT(!ec);
    auto status1 = future1.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
    CPPUNIT_ASSERT(status1 == std::future_status::ready);
    auto status2 = future2.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
    CPPUNIT_ASSERT(status2 == std::future_status::ready);

    std::vector<std::string> unsubscribeTopics;
    unsubscribeTopics.push_back(TOPIC1);
    unsubscribeTopics.push_back(TOPIC2);
    ec = client->unsubscribe(unsubscribeTopics);
    CPPUNIT_ASSERT(!ec);
    CPPUNIT_ASSERT(client->isSubscribed(TOPIC1) == false);
    CPPUNIT_ASSERT(client->isSubscribed(TOPIC2) == false);
    client->disconnect();
    CPPUNIT_ASSERT(client->isConnected() == false);
}


void MqttClient_Test::testPublishSubscribeAtMostOnceSync() {
    if (m_brokers.empty()) {
        std::clog << " No MQTT broker in environment. Skipping..." << std::flush;
        return;
    }
    testPublishSubscribeSync("MqttCppClient", 0);
    testPublishManySubscribeSync("MqttCppClient", 0);
    testPublishMultiSubscribeSync("MqttCppClient", 0);
    //    testPublishSubscribeSync("MqttPahoClient",0);
    //    testPublishManySubscribeSync("MqttPahoClient",0);
    //    testPublishMultiSubscribeSync("MqttPahoClient",0);
}


void MqttClient_Test::testPublishSubscribeAtLeastOnceSync() {
    if (m_brokers.empty()) {
        std::clog << " No MQTT broker in environment. Skipping..." << std::flush;
        return;
    }
    testPublishSubscribeSync("MqttCppClient", 1);
    testPublishManySubscribeSync("MqttCppClient", 1);
    testPublishMultiSubscribeSync("MqttCppClient", 1);
    //    testPublishSubscribeSync("MqttPahoClient",1);
    //    testPublishManySubscribeSync("MqttPahoClient",1);
    //    testPublishMultiSubscribeSync("MqttPahoClient",1);
}


void MqttClient_Test::testPublishSubscribeExactlyOnceSync() {
    if (m_brokers.empty()) {
        std::clog << " No MQTT broker in environment. Skipping..." << std::flush;
        return;
    }
    testPublishSubscribeSync("MqttCppClient", 2);
    testPublishManySubscribeSync("MqttCppClient", 2);
    testPublishMultiSubscribeSync("MqttCppClient", 2);
    //    testPublishSubscribeSync("MqttPahoClient",2);
    //    testPublishManySubscribeSync("MqttPahoClient",2);
    //    testPublishMultiSubscribeSync("MqttPahoClient",2);
}


void MqttClient_Test::testPublishSubscribeAsync(const std::string& classId, const int qos) {
    SubOpts subopts = SubQos::AtMostOnce;
    PubOpts pubopts = PubQos::AtMostOnce;
    if (qos == 1) {
        subopts = SubQos::AtLeastOnce;
        pubopts = PubQos::AtLeastOnce;
    } else if (qos == 2) {
        subopts = SubQos::ExactlyOnce;
        pubopts = PubQos::ExactlyOnce;
    }
    std::string operationStr = qos2operationString(qos);
    const std::string TOPIC = m_domain + "/" + classId + "/testPublishSubscribeAsync_" + operationStr + "_Async/0";
    std::string instanceId = m_domain + "_" + classId + "_PublishSubscribe_" + operationStr + "_Async";
    Hash config("brokers", m_brokers, "domain", m_domain, "instanceId", instanceId);
    std::promise<void> promRecv, promDisc;
    auto futRecv = promRecv.get_future();
    auto futDisc = promDisc.get_future();

    auto client = MqttClient::create(classId, config);
    auto read_handler = [&](const boost::system::error_code& ec, const std::string& topic,
                            const karabo::util::Hash::Pointer& msg) {
        CPPUNIT_ASSERT(!ec);
        CPPUNIT_ASSERT(topic == TOPIC);
        CPPUNIT_ASSERT(msg->has("header"));
        CPPUNIT_ASSERT(msg->has("body"));
        Hash& header = msg->get<Hash>("header");
        Hash& body = msg->get<Hash>("body");
        CPPUNIT_ASSERT(header.has("header"));
        CPPUNIT_ASSERT(header.get<std::string>("header") == "some header");
        CPPUNIT_ASSERT(body.has("body"));
        CPPUNIT_ASSERT(body.get<int>("body") == 42);
        promRecv.set_value();
    };

    auto disconnectHandler = [&](const boost::system::error_code& ec) {
        CPPUNIT_ASSERT(!ec);
        CPPUNIT_ASSERT(client->isConnected() == false);
        promDisc.set_value();
    };

    auto unsubHandler = [&](const boost::system::error_code& ec) {
        CPPUNIT_ASSERT(!ec);
        CPPUNIT_ASSERT(client->isSubscribed(TOPIC) == false);
        client->disconnectAsync(disconnectHandler);
    };

    auto publishHandler = [&](const boost::system::error_code& ec) { CPPUNIT_ASSERT(!ec); };

    auto subHandler = [&](const boost::system::error_code& ec) {
        CPPUNIT_ASSERT(!ec);
        CPPUNIT_ASSERT(client->isSubscribed(TOPIC) == true);
        Hash::Pointer msg(new Hash("header", Hash("header", "some header"), "body", Hash("body", 42)));
        client->publishAsync(TOPIC, msg, pubopts, publishHandler);
    };

    auto connectHandler = [&](const boost::system::error_code ec) {
        CPPUNIT_ASSERT(!ec);
        CPPUNIT_ASSERT(client->isConnected() == true);
        client->subscribeAsync(TOPIC, subopts, read_handler, subHandler);
    };

    client->connectAsync(connectHandler);

    auto status = futRecv.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
    CPPUNIT_ASSERT(status == std::future_status::ready);

    futRecv.get();
    client->unsubscribeAsync(TOPIC, unsubHandler);

    status = futDisc.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
    CPPUNIT_ASSERT(status == std::future_status::ready);
    futDisc.get();
}


void MqttClient_Test::testPublishManySubscribeAsync(const std::string& classId, const int qos) {
    SubOpts subopts = SubQos::AtMostOnce;
    PubOpts pubopts = PubQos::AtMostOnce;
    if (qos == 1) {
        subopts = SubQos::AtLeastOnce;
        pubopts = PubQos::AtLeastOnce;
    } else if (qos == 2) {
        subopts = SubQos::ExactlyOnce;
        pubopts = PubQos::ExactlyOnce;
    }
    std::string operationStr = qos2operationString(qos);
    // NOTE: Before we have tried to support and test overlapped subscriptions which is insane idea.
    // Now we came to conclusion: we should avoid overlapped subscriptions
    // like below ...
    const std::string TOPIC = m_domain + "/" + classId + "/testPublishManySubscribeAsync_" + operationStr + "_Async/";
    const std::string TOPIC1 = TOPIC + "1";
    const std::string TOPIC2 = TOPIC + "2";
    const std::string SPEC =
          m_domain + "/" + classId + "/testPublishManySubscribeAsync_SPECIAL_" + operationStr + "_Async/";
    const std::string TOPIC3 = SPEC + "3";
    std::string instanceId = m_domain + "_" + classId + "_PublishManySubscribeAsync_" + operationStr + "_Async";
    Hash config("brokers", m_brokers, "domain", m_domain, "instanceId", instanceId);
    std::promise<void> promMsgReceived1, promMsgReceived2, promMsgReceived3, promDisconnected;
    auto futMsgReceived1 = promMsgReceived1.get_future();
    auto futMsgReceived2 = promMsgReceived2.get_future();
    auto futMsgReceived3 = promMsgReceived3.get_future();
    auto futDisconnected = promDisconnected.get_future();

    auto client = MqttClient::create(classId, config);

    auto read_handler1 = [&](const boost::system::error_code& ec, const std::string topic,
                             const karabo::util::Hash::Pointer msg) {
        CPPUNIT_ASSERT(!ec);
        CPPUNIT_ASSERT(topic == TOPIC1);
        CPPUNIT_ASSERT(msg->has("header"));
        CPPUNIT_ASSERT(msg->has("body"));
        Hash& header = msg->get<Hash>("header");
        Hash& body = msg->get<Hash>("body");
        CPPUNIT_ASSERT(header.has("header"));
        CPPUNIT_ASSERT(header.get<std::string>("header") == "some header");
        CPPUNIT_ASSERT(body.has("body"));
        CPPUNIT_ASSERT(body.get<int>("body") == 77);
        promMsgReceived1.set_value();
    };

    auto read_handler2 = [&](const boost::system::error_code& ec, const std::string topic,
                             const karabo::util::Hash::Pointer msg) {
        CPPUNIT_ASSERT(!ec);
        CPPUNIT_ASSERT(topic == TOPIC2);
        CPPUNIT_ASSERT(msg->has("header"));
        CPPUNIT_ASSERT(msg->has("body"));
        Hash& header = msg->get<Hash>("header");
        Hash& body = msg->get<Hash>("body");
        CPPUNIT_ASSERT(header.has("header"));
        CPPUNIT_ASSERT(header.get<std::string>("header") == "some header");
        CPPUNIT_ASSERT(body.has("body"));
        CPPUNIT_ASSERT(body.get<int>("body") == 77);
        promMsgReceived2.set_value();
    };

    auto read_handler3 = [&](const boost::system::error_code& ec, const std::string topic,
                             const karabo::util::Hash::Pointer msg) {
        CPPUNIT_ASSERT(!ec);
        CPPUNIT_ASSERT(topic == TOPIC3);
        CPPUNIT_ASSERT(msg->has("header"));
        CPPUNIT_ASSERT(msg->has("body"));
        Hash& header = msg->get<Hash>("header");
        Hash& body = msg->get<Hash>("body");
        CPPUNIT_ASSERT(header.has("header"));
        CPPUNIT_ASSERT(header.get<std::string>("header") == "some header");
        CPPUNIT_ASSERT(body.has("body"));
        CPPUNIT_ASSERT(body.get<int>("body") == 77);
        promMsgReceived3.set_value();
    };

    client->connectAsync([&](const boost::system::error_code ec) {
        CPPUNIT_ASSERT(!ec);
        CPPUNIT_ASSERT(client->isConnected() == true);
        std::vector<std::tuple<std::string, SubOpts, ReadHashHandler>> topics;
        topics.push_back(std::make_tuple(TOPIC1, subopts, read_handler1));
        topics.push_back(std::make_tuple(TOPIC2, subopts, read_handler2));
        topics.push_back(std::make_tuple(TOPIC3, subopts, read_handler3));
        client->subscribeAsync(topics, [&](const boost::system::error_code& ec) {
            CPPUNIT_ASSERT(!ec);
            CPPUNIT_ASSERT(client->isSubscribed(TOPIC1) == true);
            CPPUNIT_ASSERT(client->isSubscribed(TOPIC2) == true);
            CPPUNIT_ASSERT(client->isSubscribed(TOPIC3) == true);
            Hash::Pointer msg(new Hash("header", Hash("header", "some header"), "body", Hash("body", 77)));

            auto publishHandler = [&](const boost::system::error_code& ec) { CPPUNIT_ASSERT(!ec); };
            client->publishAsync(TOPIC1, msg, pubopts, publishHandler);
            client->publishAsync(TOPIC2, msg, pubopts, publishHandler);
            client->publishAsync(TOPIC3, msg, pubopts, publishHandler);
        });
    });

    {
        auto status = futMsgReceived1.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
        CPPUNIT_ASSERT(status == std::future_status::ready);
    }
    {
        auto status = futMsgReceived2.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
        CPPUNIT_ASSERT(status == std::future_status::ready);
    }
    {
        auto status = futMsgReceived3.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
        CPPUNIT_ASSERT(status == std::future_status::ready);
    }

    std::vector<std::string> topics{TOPIC1, TOPIC2, TOPIC3};
    client->unsubscribeAsync(topics, [&](const boost::system::error_code& ec) {
        CPPUNIT_ASSERT(!ec);
        CPPUNIT_ASSERT(client->isSubscribed(TOPIC1) == false);
        CPPUNIT_ASSERT(client->isSubscribed(TOPIC2) == false);
        CPPUNIT_ASSERT(client->isSubscribed(TOPIC3) == false);
        client->disconnectAsync([&](const boost::system::error_code& ec) {
            CPPUNIT_ASSERT(!ec);
            CPPUNIT_ASSERT(client->isConnected() == false);
            promDisconnected.set_value();
        });
    });

    auto status = futDisconnected.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
    CPPUNIT_ASSERT(status == std::future_status::ready);
}


void MqttClient_Test::testPublishSubscribeAtMostOnceAsync() {
    if (m_brokers.empty()) {
        std::clog << " No MQTT broker in environment. Skipping..." << std::flush;
        return;
    }
    testPublishSubscribeAsync("MqttCppClient", 0);
    testPublishManySubscribeAsync("MqttCppClient", 0);
    //    testPublishSubscribeAsync("MqttPahoClient",0);
    //    testPublishManySubscribeAsync("MqttPahoClient",0);
}


void MqttClient_Test::testPublishSubscribeAtLeastOnceAsync() {
    if (m_brokers.empty()) {
        std::clog << " No MQTT broker in environment. Skipping..." << std::flush;
        return;
    }
    testPublishSubscribeAsync("MqttCppClient", 1);
    testPublishManySubscribeAsync("MqttCppClient", 1);
    //    testPublishSubscribeAsync("MqttPahoClient",1);
    //    testPublishManySubscribeAsync("MqttPahoClient",1);
}

void MqttClient_Test::testPublishSubscribeExactlyOnceAsync() {
    if (m_brokers.empty()) {
        std::clog << " No MQTT broker in environment. Skipping..." << std::flush;
        return;
    }
    testPublishSubscribeAsync("MqttCppClient", 2);
    testPublishManySubscribeAsync("MqttCppClient", 2);
    //    testPublishSubscribeAsync("MqttPahoClient",2);
    //    testPublishManySubscribeAsync("MqttPahoClient",2);
}


void MqttClient_Test::testMultipleSubscribersToTheSameTopic() {
    if (m_brokers.empty()) {
        std::clog << " No MQTT broker in environment. Skipping..." << std::flush;
        return;
    }
    testMultipleSubscribersToTheSameTopic_("MqttCppClient");
    //    testMultipleSubscribersToTheSameTopic_("MqttPahoClient");
}

void MqttClient_Test::testMultipleSubscribersToTheSameTopic_(const std::string& classId) {
    constexpr uint32_t NUM_SUBSCRIBERS = 5;
    constexpr uint32_t NUM_PUBLISHERS = 1;
    constexpr uint32_t NUM_MSGS_PER_PUBLISHER = 10;
    constexpr const char* TOPIC = "testMultipleSubscribersToTheSameTopic";

    class MessageHandler {
       public:
        bool handleMessage(const std::string& topic, const karabo::util::Hash::Pointer msg) {
            constexpr uint32_t NUM_MESSAGES = NUM_PUBLISHERS * NUM_MSGS_PER_PUBLISHER;

            CPPUNIT_ASSERT(msg->has("header"));
            CPPUNIT_ASSERT(msg->has("body"));
            Hash& header = msg->get<Hash>("header");
            Hash& body = msg->get<Hash>("body");
            CPPUNIT_ASSERT(header.has("instanceId"));
            std::string instanceId = header.get<std::string>("instanceId");
            CPPUNIT_ASSERT(boost::starts_with(instanceId, "publisher"));
            CPPUNIT_ASSERT(body.has("messageId"));
            uint32_t messageId = body.get<uint32_t>("messageId");
            std::string uniqueMsgId = instanceId + std::to_string(messageId);
            CPPUNIT_ASSERT(m_uniqueMessagesReceivedSet.count(uniqueMsgId) == 0);
            m_uniqueMessagesReceivedSet.emplace(uniqueMsgId);

            return ++m_msgCount == NUM_MESSAGES;
        }

       private:
        uint32_t m_msgCount = 0;
        std::unordered_set<std::string> m_uniqueMessagesReceivedSet;
    };

    class MessageProducer {
       public:
        MessageProducer(const std::string& instanceId) : m_instanceId(instanceId) {}

        karabo::util::Hash::Pointer produceMessage() {
            karabo::util::Hash::Pointer msg;
            if (m_numMsgsSent < NUM_MSGS_PER_PUBLISHER) {
                msg = boost::make_shared<Hash>("header", Hash("instanceId", m_instanceId), "body",
                                               Hash("messageId", ++m_numMsgsSent));
            }
            return msg;
        }

       private:
        uint32_t m_numMsgsSent = 0;
        std::string m_instanceId;
    };

    std::vector<std::shared_ptr<Subscriber<MessageHandler>>> subscribers;
    for (uint32_t i = 0; i < NUM_SUBSCRIBERS; ++i) {
        std::string instanceId = "subscriber" + std::to_string(i);
        subscribers.emplace_back(std::make_shared<Subscriber<MessageHandler>>(instanceId, SubQos::ExactlyOnce, TOPIC));
        subscribers.back()->start(classId);
    }

    for (uint32_t i = 0; i < NUM_SUBSCRIBERS; ++i) {
        subscribers[i]->waitForSubscription();
    }

    std::vector<std::shared_ptr<Publisher<MessageProducer>>> publishers;
    for (uint32_t i = 0; i < NUM_PUBLISHERS; ++i) {
        std::string instanceId = "publisher" + std::to_string(i);
        publishers.emplace_back(std::make_shared<Publisher<MessageProducer>>(instanceId, PubQos::ExactlyOnce, TOPIC));
        publishers.back()->start(classId);
    }

    for (uint32_t i = 0; i < NUM_SUBSCRIBERS; ++i) {
        subscribers[i]->wait();
    }

    for (uint32_t i = 0; i < NUM_PUBLISHERS; ++i) {
        publishers[i]->wait();
    }
}

void MqttClient_Test::testMultipleSubscriptionsToTopicsWithAndWithoutWildcards() {
    if (m_brokers.empty()) {
        std::clog << " No MQTT broker in environment. Skipping..." << std::flush;
        return;
    }
    testMultipleSubscriptionsToTopicsWithAndWithoutWildcards_("MqttCppClient");
    //    testMultipleSubscriptionsToTopicsWithAndWithoutWildcards_("MqttPahoClient");
}

void MqttClient_Test::testMultipleSubscriptionsToTopicsWithAndWithoutWildcards_(const std::string& classId) {
    const std::string TOPIC_PREFIX = m_domain + "/testSubscriptionToTopicsWithAndWithoutWildcards/";
    constexpr uint32_t NUM_MSGS_PER_PUBLISHER = 10;
    constexpr uint32_t NUM_PUBLISHERS = 2;
    constexpr uint32_t NUM_SUBSCRIBERS = 2;

    class MessageHandler {
       public:
        void setNumMessages(const uint32_t numMessages) {
            m_numMessages = numMessages;
        }

        bool handleMessage(const std::string& topic, const karabo::util::Hash::Pointer msg) {
            CPPUNIT_ASSERT(msg->has("header"));
            CPPUNIT_ASSERT(msg->has("body"));
            Hash& header = msg->get<Hash>("header");
            Hash& body = msg->get<Hash>("body");
            CPPUNIT_ASSERT(header.has("instanceId"));
            std::string instanceId = header.get<std::string>("instanceId");
            CPPUNIT_ASSERT(boost::starts_with(instanceId, "publisher"));
            CPPUNIT_ASSERT(body.has("messageId"));
            uint32_t messageId = body.get<uint32_t>("messageId");
            std::string uniqueMsgId = instanceId + "_" + std::to_string(messageId);
            CPPUNIT_ASSERT(m_uniqueMessagesReceivedSet.count(uniqueMsgId) == 0);
            m_uniqueMessagesReceivedSet.emplace(uniqueMsgId);

            return ++m_msgCount == m_numMessages;
        }

       private:
        uint32_t m_msgCount = 0;
        uint32_t m_numMessages = 0;
        std::unordered_set<std::string> m_uniqueMessagesReceivedSet;
    };

    class MessageProducer {
       public:
        MessageProducer(const std::string& instanceId) : m_instanceId(instanceId) {}

        karabo::util::Hash::Pointer produceMessage() {
            karabo::util::Hash::Pointer msg;
            if (m_numMsgsSent < NUM_MSGS_PER_PUBLISHER) {
                msg = boost::make_shared<Hash>("header", Hash("instanceId", m_instanceId), "body",
                                               Hash("messageId", ++m_numMsgsSent));
            }
            return msg;
        }

       private:
        uint32_t m_numMsgsSent = 0;
        std::string m_instanceId;
    };

    std::vector<std::shared_ptr<Subscriber<MessageHandler>>> subscribers;
    for (uint32_t i = 0; i < NUM_SUBSCRIBERS; ++i) {
        std::string instanceId = "subscriber" + std::to_string(i);
        std::string topic(TOPIC_PREFIX);
        constexpr SubOpts subopts = SubQos::ExactlyOnce;
        if (i % 2 == 0) {
            topic.append("+");
            subscribers.emplace_back(std::make_shared<Subscriber<MessageHandler>>(instanceId, subopts, topic));
            subscribers.back()->setNumMessages(NUM_MSGS_PER_PUBLISHER * NUM_PUBLISHERS);
        } else {
            topic.append("publisher0");
            subscribers.emplace_back(std::make_shared<Subscriber<MessageHandler>>(instanceId, subopts, topic));
            subscribers.back()->setNumMessages(NUM_MSGS_PER_PUBLISHER);
        }
        subscribers.back()->start(classId);
    }

    for (uint32_t i = 0; i < subscribers.size(); ++i) {
        subscribers[i]->waitForSubscription();
    }

    std::vector<std::shared_ptr<Publisher<MessageProducer>>> publishers;
    constexpr PubOpts pubopts = PubQos::ExactlyOnce;
    for (uint32_t i = 0; i < NUM_PUBLISHERS; ++i) {
        std::string instanceId = "publisher" + std::to_string(i);
        std::string topic = std::string(TOPIC_PREFIX) + instanceId;
        publishers.emplace_back(std::make_shared<Publisher<MessageProducer>>(instanceId, pubopts, topic));
        publishers.back()->start(classId);
    }

    for (uint32_t i = 0; i < NUM_SUBSCRIBERS; ++i) {
        subscribers[i]->wait();
    }

    for (uint32_t i = 0; i < NUM_PUBLISHERS; ++i) {
        publishers[i]->wait();
    }
    //    stopEventLoop();
}

void MqttClient_Test::testMultipleSubscriptionsToTopicWithWildcardsAndSubtopics() {
    if (m_brokers.empty()) {
        std::clog << " No MQTT broker in environment. Skipping..." << std::flush;
        return;
    }
    testMultipleSubscriptionsToTopicWithWildcardsAndSubtopics_("MqttCppClient");
    //    testMultipleSubscriptionsToTopicWithWildcardsAndSubtopics_("MqttPahoClient");
}

/**
 * Demonstrate how overlapped subscriptions work resulting in receiving duplicated
 * messages
 */
void MqttClient_Test::testMultipleSubscriptionsToTopicWithWildcardsAndSubtopics_(const std::string& classId) {
    static std::string TOPIC_PREFIX = m_domain + "/testMultipleSubscriptions/";
    constexpr uint32_t NUM_MSGS_PER_PUBLISHER = 10;
    constexpr uint32_t NUM_PUBLISHERS = 2;

    class Consumer {
       public:
        Consumer(const std::string& instanceId)
            : m_instanceId(instanceId),
              m_future(m_promise.get_future()),
              m_brokers(getMqttBrokerFromEnv()),
              m_domain(Broker::brokerDomainFromEnv()) {}

        void start(const std::string& classId) {
            Hash config("brokers", m_brokers, "domain", m_domain, "instanceId", m_instanceId);
            m_client = MqttClient::create(classId, config);
            m_client->connectAsync([this](const boost::system::error_code ec) {
                CPPUNIT_ASSERT(!ec);
                CPPUNIT_ASSERT(m_client->isConnected() == true);
                std::string topic(TOPIC_PREFIX);
                topic += "#";
                constexpr SubOpts subopts = SubQos::AtMostOnce;
                m_client->subscribeAsync(topic, subopts, boost::bind(&Consumer::handleRead, this, _1, _2, _3),
                                         [this, topic](const boost::system::error_code ec) {
                                             CPPUNIT_ASSERT(!ec);
                                             CPPUNIT_ASSERT(m_client->isSubscribed(topic) == true);
                                             m_promise.set_value();
                                         });
            });
        }

        void wait() {
            auto status = m_future.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
            CPPUNIT_ASSERT(status == std::future_status::ready);
        }

        boost::system::error_code subscribe(const TopicSubOptions& topics) {
            return m_client->subscribe(topics);
        }

        void unsubscribeAll() {
            m_client->unsubscribeAll();
        }

        void disconnect() {
            m_client->disconnectForced();
        }

       private:
        void handleRead(boost::system::error_code ec, const std::string& topic, const karabo::util::Hash::Pointer msg) {
            throw KARABO_LOGIC_EXCEPTION("Wildcard handler should NOT be called! Topic: \"" + topic + "\"");
        }

        MqttClient::Pointer m_client;
        const std::string m_instanceId;
        std::promise<void> m_promise;
        std::future<void> m_future;
        std::vector<std::string> m_brokers;
        std::string m_domain;
    };

    class MessageProducer {
       public:
        MessageProducer(const std::string& instanceId) : m_instanceId(instanceId) {}

        karabo::util::Hash::Pointer produceMessage() {
            karabo::util::Hash::Pointer msg;
            if (m_numMsgsSent < NUM_MSGS_PER_PUBLISHER) {
                msg = boost::make_shared<Hash>("header", Hash("instanceId", m_instanceId), "body",
                                               Hash("messageId", ++m_numMsgsSent));
            }
            return msg;
        }

       private:
        uint32_t m_numMsgsSent = 0;
        std::string m_instanceId;
    };

    auto consumer = std::make_shared<Consumer>("consumer77");

    std::uint32_t n1 = 0;
    std::promise<void> promise1;
    std::future<void> future1 = promise1.get_future();
    std::uint32_t n2 = 0;
    std::promise<void> promise2;
    std::future<void> future2 = promise2.get_future();

    auto readConsume1 = [&](const boost::system::error_code& rc, const std::string& topic,
                            const karabo::util::Hash::Pointer& hash) {
        if (rc) return;
        ++n1;
        if (n1 == NUM_MSGS_PER_PUBLISHER) promise1.set_value();
    };

    auto readConsume2 = [&](const boost::system::error_code& rc, const std::string& topic,
                            const karabo::util::Hash::Pointer& hash) {
        if (rc) return;
        ++n2;
        if (n2 == NUM_MSGS_PER_PUBLISHER) promise2.set_value();
    };

    consumer->start(classId);
    consumer->wait();

    std::vector<std::tuple<std::string, SubOpts, ReadHashHandler>> topics;
    for (uint32_t i = 0; i < NUM_PUBLISHERS; ++i) {
        std::string subtopic = std::string(TOPIC_PREFIX) + "publisher" + std::to_string(i);
        constexpr SubOpts subopts = SubQos::AtMostOnce;
        if (i % 2 == 0) {
            topics.push_back(std::make_tuple(subtopic, subopts, readConsume1));
        } else {
            topics.push_back(std::make_tuple(subtopic, subopts, readConsume2));
        }
    }
    consumer->subscribe(topics);

    std::vector<std::shared_ptr<Publisher<MessageProducer>>> publishers;
    for (uint32_t i = 0; i < NUM_PUBLISHERS; ++i) {
        std::string instanceId = "publisher" + std::to_string(i);
        std::string topic = std::string(TOPIC_PREFIX) + instanceId;
        constexpr PubOpts pubopts = PubQos::AtMostOnce;
        auto publisher = std::make_shared<Publisher<MessageProducer>>(instanceId, pubopts, topic);
        publishers.emplace_back(publisher);
        publisher->start(classId);
    }

    for (uint32_t i = 0; i < NUM_PUBLISHERS; ++i) {
        publishers[i]->wait();
    }

    future1.get();
    future2.get();

    consumer->unsubscribeAll();
    consumer->disconnect();

    // Factor 2 is due overlapped subscriptions to specific topics and wildcard topic
    CPPUNIT_ASSERT(n1 == 2 * NUM_MSGS_PER_PUBLISHER);
    CPPUNIT_ASSERT(n2 == 2 * NUM_MSGS_PER_PUBLISHER);
}


void MqttClient_Test::testTopicsSubscriptionsInArbitraryOrder() {
    if (m_brokers.empty()) {
        std::clog << " No MQTT broker in environment. Skipping..." << std::flush;
        return;
    }
    testTopicsSubscriptionsInArbitraryOrder_("MqttCppClient");
    //    testTopicsSubscriptionsInArbitraryOrder_("MqttPahoClient");
}


/**
 * The similar test as before... just the order of subscriptions is different
 * This test is candidate for removing...
 */
void MqttClient_Test::testTopicsSubscriptionsInArbitraryOrder_(const std::string& classId) {
    static std::string TOPIC_PREFIX = m_domain + "/testMultipleSubscriptionsAny/";
    constexpr uint32_t NUM_MSGS_PER_PUBLISHER = 10;
    constexpr uint32_t NUM_PUBLISHERS = 2;

    class Consumer {
       public:
        Consumer(const std::string& instanceId)
            : m_instanceId(instanceId),
              m_topic0(TOPIC_PREFIX + "#"),
              m_topic1(TOPIC_PREFIX + "publisher0"),
              m_topic2(TOPIC_PREFIX + "publisher1"),
              m_future(m_promise.get_future()),
              m_n1(0),
              m_future1(m_promise1.get_future()),
              m_n2(0),
              m_future2(m_promise2.get_future()),
              m_brokers(getMqttBrokerFromEnv()),
              m_domain(Broker::brokerDomainFromEnv()) {}

        void start(const std::string& classId) {
            Hash config("brokers", m_brokers, "domain", m_domain, "instanceId", m_instanceId);
            m_client = MqttClient::create(classId, config);
            m_client->connectAsync([this](const boost::system::error_code ec) {
                CPPUNIT_ASSERT(!ec);
                CPPUNIT_ASSERT(m_client->isConnected() == true);
                constexpr SubOpts subopts = SubQos::AtMostOnce;
                TopicSubOptions params;
                params.push_back(
                      std::make_tuple(m_topic1, subopts, boost::bind(&Consumer::handleRead1, this, _1, _2, _3)));
                params.push_back(
                      std::make_tuple(m_topic0, subopts, boost::bind(&Consumer::handleRead0, this, _1, _2, _3)));
                params.push_back(
                      std::make_tuple(m_topic2, subopts, boost::bind(&Consumer::handleRead2, this, _1, _2, _3)));
                m_client->subscribeAsync(params, [this](const boost::system::error_code ec) {
                    CPPUNIT_ASSERT(!ec);
                    CPPUNIT_ASSERT(m_client->isSubscribed(m_topic0) == true);
                    CPPUNIT_ASSERT(m_client->isSubscribed(m_topic1) == true);
                    CPPUNIT_ASSERT(m_client->isSubscribed(m_topic2) == true);
                    m_promise.set_value();
                });
            });
        }

        void wait() {
            auto status = m_future.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
            CPPUNIT_ASSERT(status == std::future_status::ready);
        }

        boost::system::error_code subscribe(
              const std::vector<std::tuple<std::string, SubOpts, ReadHashHandler>>& topics) {
            return m_client->subscribe(topics);
        }

        void disconnect() {
            {
                auto status = m_future1.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
                CPPUNIT_ASSERT(status == std::future_status::ready);
            }
            {
                auto status = m_future2.wait_for(std::chrono::seconds(TEST_EXPIRATION_TIME_IN_SECONDS));
                CPPUNIT_ASSERT(status == std::future_status::ready);
            }
            m_client->unsubscribeAll();
            m_client->disconnectForced();
            // Overlapped subscriptions results in factor 2 number of messages
            CPPUNIT_ASSERT(m_n1 == 2 * NUM_MSGS_PER_PUBLISHER);
            CPPUNIT_ASSERT(m_n2 == 2 * NUM_MSGS_PER_PUBLISHER);
        }

       private:
        void handleRead0(boost::system::error_code ec, const std::string& topic,
                         const karabo::util::Hash::Pointer msg) {
            throw KARABO_LOGIC_EXCEPTION("Wildcard handler should NOT be called! Topic: \"" + topic + "\"");
        }

        void handleRead1(boost::system::error_code ec, const std::string& topic,
                         const karabo::util::Hash::Pointer msg) {
            if (ec) return;
            ++m_n1;
            if (m_n1 == NUM_MSGS_PER_PUBLISHER) m_promise1.set_value();
        }

        void handleRead2(boost::system::error_code ec, const std::string& topic,
                         const karabo::util::Hash::Pointer msg) {
            if (ec) return;
            ++m_n2;
            if (m_n2 == NUM_MSGS_PER_PUBLISHER) m_promise2.set_value();
        }

        MqttClient::Pointer m_client;
        const std::string m_instanceId;
        std::string m_topic0;
        std::string m_topic1;
        std::string m_topic2;
        std::promise<void> m_promise;
        std::future<void> m_future;
        std::uint32_t m_n1;
        std::promise<void> m_promise1;
        std::future<void> m_future1;
        std::uint32_t m_n2;
        std::promise<void> m_promise2;
        std::future<void> m_future2;
        std::vector<std::string> m_brokers;
        std::string m_domain;
    };

    class MessageProducer {
       public:
        MessageProducer(const std::string& instanceId) : m_instanceId(instanceId) {}

        karabo::util::Hash::Pointer produceMessage() {
            karabo::util::Hash::Pointer msg;
            if (m_numMsgsSent < NUM_MSGS_PER_PUBLISHER) {
                msg = boost::make_shared<Hash>("header", Hash("instanceId", m_instanceId), "body",
                                               Hash("messageId", ++m_numMsgsSent));
            }
            return msg;
        }

       private:
        uint32_t m_numMsgsSent = 0;
        std::string m_instanceId;
    };

    auto consumer = std::make_shared<Consumer>("consumer99");
    consumer->start(classId);
    consumer->wait();

    std::vector<std::shared_ptr<Publisher<MessageProducer>>> publishers;
    for (uint32_t i = 0; i < NUM_PUBLISHERS; ++i) {
        std::string instanceId = "publisher" + std::to_string(i);
        std::string topic = TOPIC_PREFIX + instanceId;
        constexpr PubOpts pubopts = PubQos::AtLeastOnce;
        auto publisher = std::make_shared<Publisher<MessageProducer>>(instanceId, pubopts, topic);
        publishers.emplace_back(publisher);
        publisher->start(classId);
    }

    for (uint32_t i = 0; i < NUM_PUBLISHERS; ++i) {
        publishers[i]->wait();
    }

    consumer->disconnect();
}
