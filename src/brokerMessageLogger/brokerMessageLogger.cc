/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 6, 2011
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <algorithm>
#include <array>
#include <boost/algorithm/string.hpp>
#include <cassert>
#include <fstream>
#include <iosfwd>
#include <karabo/log/Logger.hh>
#include <karabo/net/AmqpClient.hh>
#include <karabo/net/Broker.hh>
#include <karabo/net/EventLoop.hh>
#include <karabo/net/JmsConnection.hh>
#include <karabo/net/JmsConsumer.hh>
#include <karabo/net/MqttClient.hh>
#include <karabo/net/RedisClient.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/StringTools.hh>
#include <karabo/xms/SignalSlotable.hh>
#include <vector>

using namespace std;
using namespace karabo::util;
using namespace karabo::net;
using namespace karabo::log;
using namespace karabo::xms;
using namespace boost::placeholders;


void printHelp(const char* execName) {
    cout << "Log all messages of a given broker topic to standard output.\n" << endl;
    cout << "Usage: " << execName << " [-h] [-t topic] [-b brokerUrl] [-s selection]\n" << endl;
    cout << "  -h          : print this help and exit" << endl;
    cout << "  -t topic    : broker topic - if not specified, use environment variables" << endl;
    cout << "                KARABO_BROKER_TOPIC, LOGNAME, USER, LNAME or USERNAME in that" << endl;
    cout << "                order of precedence" << endl;
    cout << "  -b brokerUrl: URL(s) of broker" << endl;
    cout << "                if not specified, use environment variable KARABO_BROKER" << endl;
    cout << "  -s selector : Broker type specific selection of messages" << endl;
    cout << "                OpenMQ: selector string, e.g. \"slotInstanceIds LIKE '%|deviceId|%'\"" << endl;
    cout << "                MQTT:   comma separated list of MQTT subtopics," << endl;
    cout << "                        e.g. \"signals/+/signalChanged,global_slots,slots/INSTANCE|1\"" << endl;
    cout << "                REDIS:  comma separated list of REDIS subtopics, " << endl;
    cout << "                        possibly using glob style patterns: '*', '?', '[list]'" << endl;
    cout << "                        e.g. \"signals/*/signalChanged,global_slots,slots/INSTANCE|1\"" << endl;
    cout << "                AMQP:   Selection criteria involves 2 values: exchange and binding key " << endl;
    cout << "                        separated by colon sign (:) and such pairs are comma separated. " << endl;
    cout << "                        e.g. signals:*.signalChanged,global_slots:,slots:INSTANCE/1\"" << endl << endl;
}

void jmsReadHandler(const Hash::Pointer& header, const Hash::Pointer& body) {
    cout << *header << endl;
    cout << *body << endl;
    cout << "-----------------------------------------------------------------------" << endl << endl;
}


void jmsErrorNotifier(consumer::Error errCode, const string& errDesc) {
    cout << ">>>> Error >>>>" << endl;
    cout << " Code: " << static_cast<int>(errCode) << endl;
    cout << " Description: " << errDesc << endl;
    cout << "-----------------------------------------------------------------------" << endl << endl;
}


void mqttOrRedisMessageHandler(const boost::system::error_code ec, const std::string& topic,
                               const Hash::Pointer& message) {
    if (ec) {
        cout << "Error " << ec.value() << ": " << ec.message() << endl;
    }
    cout << "Topic: " << topic << endl;

    if (message) {
        cout << *message << endl;
    } else {
        cout << "MISSING MESSAGE" << endl; // Shouldn't happen - but print an indicator
    }

    cout << "-----------------------------------------------------------------------" << endl << endl;
}


void logOpenMQ(const std::vector<std::string>& brokerUrls, const std::string& topic, const std::string& selector) {
    JmsConnection::Pointer connection = boost::make_shared<JmsConnection>(brokerUrls);
    connection->connect();
    const string effectiveUrl = connection->getBrokerUrl(); // one of the potentially several

    cout << "# Starting to consume messages..." << std::endl;
    cout << "# Broker (OpenMQ): " << effectiveUrl << endl;
    cout << "# Topic: " << topic << endl;
    cout << "# Selector: " << selector << endl << endl;

    // Obtain consumer
    JmsConsumer::Pointer consumer = connection->createConsumer(topic, selector);

    // Read and logs messages and errors.
    consumer->startReading(boost::bind(&jmsReadHandler, _1, _2), boost::bind(&jmsErrorNotifier, _1, _2));
    EventLoop::work(); // Block forever
}

void logMqtt(const std::vector<std::string>& brokerUrls, const std::string& domain, const std::string& subtopics) {
    // Create client, but don't care which type, just take the first (vector::at(0) will throw if empty).
    const string clientName = Configurator<MqttClient>::getRegisteredClasses().at(0);
    const Hash config("brokers", brokerUrls, "domain", domain);
    MqttClient::Pointer client = MqttClient::create(clientName, config);

    // Connect and subscribe
    auto ec = client->connect();
    if (ec) {
        throw KARABO_NETWORK_EXCEPTION("Failed to connect to MQTT broker at " + toString(brokerUrls));
    }

    TopicSubOptions subscriptions;
    if (subtopics.empty()) {
        subscriptions.emplace_back(domain + "/#", SubQos::AtMostOnce,
                                   boost::bind(&mqttOrRedisMessageHandler, _1, _2, _3));
    } else {
        for (const string& t : fromString<string, vector>(subtopics)) {
            subscriptions.emplace_back(domain + "/" + t, SubQos::AtMostOnce,
                                       boost::bind(&mqttOrRedisMessageHandler, _1, _2, _3));
        }
    }
    cout << "# Starting to consume messages..." << std::endl;
    cout << "# Broker (MQTT): " << client->getBrokerUrl() << endl;
    cout << "# Domain: " << domain << endl;
    if (!subtopics.empty()) {
        cout << "# Subtopics: " << subtopics << endl;
    }
    cout << endl;
    ec = client->subscribe(subscriptions);
    if (ec) {
        throw KARABO_NETWORK_EXCEPTION("Failed to subscribe to MQTT broker");
    }

    // As of Karabo 2.11.0, MqttClient does not need the Karabo event loop (might change later).
    // But here we need something to block for ever...
    EventLoop::work(); // block for ever
}

void logRedis(const std::vector<std::string>& brokerUrls, const std::string& domain, const std::string& subtopics) {
    const Hash config("brokers", brokerUrls, "domain", domain);
    RedisClient::Pointer client = Configurator<RedisClient>::create("RedisClient", config);

    // Connect and subscribe
    auto ec = client->connect();
    if (ec) {
        throw KARABO_NETWORK_EXCEPTION("Failed to connect to Redis broker at " + toString(brokerUrls));
    }

    RedisTopicSubOptions subscriptions;
    if (subtopics.empty()) {
        subscriptions.emplace_back(domain + "/*", boost::bind(&mqttOrRedisMessageHandler, _1, _2, _3));
    } else {
        for (const string& t : fromString<string, vector>(subtopics)) {
            subscriptions.emplace_back(domain + "/" + t, boost::bind(&mqttOrRedisMessageHandler, _1, _2, _3));
        }
    }

    cout << "# Starting to consume messages..." << std::endl;
    cout << "# Broker (REDIS): " << client->getBrokerUrl() << endl;
    cout << "# Domain: " << domain << endl;
    if (!subtopics.empty()) {
        cout << "# Subtopics: " << subtopics << endl;
    }
    cout << endl;
    ec = client->subscribe(subscriptions);
    if (ec) {
        throw KARABO_NETWORK_EXCEPTION("Failed to subscribe to REDIS broker");
    }

    EventLoop::work(); // block for ever
}

void logAmqp(const std::vector<std::string>& brokerUrls, const std::string& domain, const std::string& selector) {
    // AMQP: 'selector' string is sequence of pairs of exchange and binding key
    //   separated by comma where each pair is separated by colon:
    //     "exchange1:bindingKey1,exchange2:bindingKey2,..."

    const Hash config("brokers", brokerUrls, "domain", domain);
    AmqpClient::Pointer client = Configurator<AmqpClient>::create("AmqpClient", config);

    // Connect and subscribe
    auto ec = client->connect();
    if (ec) {
        throw KARABO_NETWORK_EXCEPTION("Failed to connect to AMQP (RabbitMQ) broker at " + toString(brokerUrls));
    }

    // Define read handler for message visualization...
    auto readHandler = [](const boost::system::error_code& ec, const Hash::Pointer& msg) {
        if (ec) {
            std::cout << "Error " << ec.value() << ": " << ec.message() << std::endl;
            return;
        }

        if (msg) {
            std::cout << *msg << std::endl;
        } else {
            std::cout << "MISSING MESSAGE!" << std::endl; // Shouldn't happen - but print an indicator
        }

        std::cout << "-----------------------------------------------------------------------\n" << std::endl;
    };

    client->registerConsumerHandler(readHandler);

    std::cout << "# Starting to consume messages..." << std::endl;
    std::cout << "# Broker (AMQP): " << client->getBrokerUrl() << std::endl;
    std::cout << "# Domain: " << domain << std::endl;

    if (selector.empty()) {
        // Bind to all possible messages ...
        const std::vector<std::array<std::string, 2> >& defaultTable = {
              {"karaboGuiDebug", ""},         // always empty routing key
              {domain + ".signals", "*.*"},   // any INSTANCE, any SIGNAL
              {domain + ".slots", "#"},       // any INSTANCE ID ('*' may work as well)
              {domain + ".global_slots", ""}, // always empty routing key
              {domain + ".log", ""}           // always empty routing key
        };

        boost::system::error_code ec;
        for (const auto& a : defaultTable) {
            std::cout << "# Exchange: \"" << a[0] << "\" and binding key: \"" << a[1] << "\"" << std::endl;
            ec = client->subscribe(a[0], a[1]);
            if (ec) {
                std::cout << "ERROR: Failed to subscribe to AMQP broker: #" << ec.value() << " -- " << ec.message()
                          << std::endl;
                throw KARABO_NETWORK_EXCEPTION("Failed to subscribe to AMQP broker");
            }
        }
    } else {
        for (const string& exchbind : fromString<string, vector>(selector)) {
            std::vector<std::string> v;
            boost::split(v, exchbind, boost::is_any_of(":"));
            if (v.size() != 2) {
                throw KARABO_PARAMETER_EXCEPTION("Malformed input argument: " + exchbind);
            }
            std::cout << "# Exchange: \"" << v[0] << "\" and binding key: \"" << v[1] << "\"\n";
            ec = client->subscribe(v[0], v[1]);
            if (ec) {
                throw KARABO_NETWORK_EXCEPTION("Failed to subscribe to AMQP broker");
            }
        }
    }
    std::cout << std::endl;

    EventLoop::work(); // block for ever
}


int main(int argc, char** argv) {
    try {
        // Parse command line
        Hash options;
        for (int i = 1; i < argc; i += 2) {
            const std::string argv_i(argv[i]);
            if (argv_i == "-h" || argv[i - 1] == std::string("-h") || argc <= i + 1 ||
                (argv_i != "-b" && argv_i != "-t" && argv_i != "-s")) {
                printHelp(argv[0]);
                return 0;
            };
            options.set<string>(argv[i], argv[i + 1]);
        }
        vector<string> brokerUrls;
        if (options.has("-b")) {
            brokerUrls = fromString<string, vector>(options.get<string>("-b"));
        } else {
            brokerUrls = Broker::brokersFromEnv();
        }
        string topic;
        if (options.has("-t")) options.get("-t", topic);
        else {
            topic = Broker::brokerDomainFromEnv();
        }
        string selector;
        if (options.has("-s")) options.get("-s", selector);

        // Start Logger
        Logger::configure(Hash("priority", "ERROR"));
        Logger::useOstream();

        const std::string brkType = Broker::brokerTypeFrom(brokerUrls);
        cout << "# Trying to connect to broker '" << toString(brokerUrls) << "'...\n" << endl;
        if (brkType == "jms") {
            logOpenMQ(brokerUrls, topic, selector);
        } else if (brkType == "mqtt") {
            logMqtt(brokerUrls, topic, selector);
        } else if (brkType == "amqp") {
            logAmqp(brokerUrls, topic, selector);
        } else if (brkType == "redis") {
            logRedis(brokerUrls, topic, selector);
        } else {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION(brkType + " not supported!");
        }

    } catch (const Exception& e) {
        cerr << e << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
