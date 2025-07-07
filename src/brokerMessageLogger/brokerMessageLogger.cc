/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 6, 2011
 *
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
#include <algorithm>
#include <array>
#include <boost/algorithm/string.hpp>
#include <cassert>
#include <fstream>
#include <iosfwd>
#include <karabo/log/Logger.hh>
#include <karabo/net/AmqpConnection.hh>
#include <karabo/net/AmqpHashClient.hh>
#include <karabo/net/Broker.hh>
#include <karabo/net/EventLoop.hh>
#include <karabo/xms/SignalSlotable.hh>
#include <vector>

#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/StringTools.hh"

using namespace std;
using namespace karabo::data;
using namespace karabo::net;
using namespace karabo::log;
using namespace karabo::xms;
using namespace std::placeholders;


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
    cout << "                AMQP:   Selection criteria involves 2 values: exchange and binding key " << endl;
    cout << "                        separated by colon sign (:) and such pairs are comma separated. " << endl;
    cout << "                        e.g. signals:*.signalChanged,global_slots:,slots:INSTANCE/1\"" << endl << endl;
}

void readHandler(const Hash::Pointer& header, const Hash::Pointer& body, const std::string& exchange,
                 const std::string& routingKey) {
    cout << "Message to exchange '" << exchange << "' with routingKey '" << routingKey << "':\n\n";
    cout << *header << "\n";
    cout << *body << endl;
    cout << "-----------------------------------------------------------------------" << endl << endl;
}


void logAmqp(const std::vector<std::string>& brokerUrls, const std::string& domain, const std::string& selector) {
    // AMQP: 'selector' string is sequence of pairs of exchange and binding key
    //   separated by comma where each pair is separated by colon:
    //     "exchange1:bindingKey1,exchange2:bindingKey2,..."

    AmqpConnection::Pointer connection(std::make_shared<AmqpConnection>(brokerUrls));

    const Hash config("brokers", brokerUrls, "domain", domain);
    AMQP::Table queueArgs;
    queueArgs
          .set("x-max-length", 10'000)    // Queue limit
          .set("x-overflow", "drop-head") // drop oldest if limit reached
          .set("x-message-ttl", 30'000);  // message time-to-live in ms
    std::ostringstream idStr;
    idStr << domain << ".messageLogger/" << karabo::net::bareHostName() << "/" << getpid();
    AmqpHashClient::Pointer client =
          AmqpHashClient::create(connection, idStr.str(), queueArgs, readHandler, [](const std::string& msg) {
              std::cout << "Error reading message: " << msg
                        << "\n-----------------------------------------------------------------------\n"
                        << std::endl;
          });

    // Wait until connection established and thus connection->getCurrentUrl() shows proper url
    std::promise<boost::system::error_code> isConnected;
    auto futConnected = isConnected.get_future();
    connection->asyncConnect([&isConnected](const boost::system::error_code& ec) { isConnected.set_value(ec); });
    const boost::system::error_code ec = futConnected.get();
    if (ec) {
        throw KARABO_NETWORK_EXCEPTION("Broker connection failed: " + ec.message());
    }

    std::cout << "# Starting to consume messages..." << std::endl;
    std::cout << "# Broker (AMQP): " << connection->getCurrentUrl()
              << std::endl; // might not be the one we actually connect...
    std::cout << "# Domain: " << domain << std::endl;

    // Lambda to initiate subscription, returns future to wait for:
    auto subscribe = [](AmqpHashClient::Pointer& client, const std::string& exchange, const std::string& bindingKey) {
        std::cout << "# Exchange: '" << exchange << "' and binding key: '" << bindingKey << "'" << std::endl;

        auto done = std::make_shared<std::promise<boost::system::error_code>>();
        std::future<boost::system::error_code> fut = done->get_future();
        client->asyncSubscribe(exchange, bindingKey,
                               [done](const boost::system::error_code& ec) { done->set_value(ec); });
        return fut;
    };
    std::vector<std::future<boost::system::error_code>> futures;
    if (selector.empty()) {
        // Bind to all possible messages ...
        const std::vector<std::array<std::string, 2>>& defaultTable = {
              {domain + ".Signals", "#"},     // any INSTANCE, any SIGNAL
              {domain + ".Slots", "#"},       // any INSTANCE, any direct slot call
              {domain + ".Global_Slots", "#"} // any INSTANCE, any broadcast slot
        };
        for (const auto& a : defaultTable) {
            futures.push_back(subscribe(client, a[0], a[1]));
        }
    } else {
        for (const string& exchbind : fromString<string, vector>(selector)) {
            std::vector<std::string> v;
            boost::split(v, exchbind, boost::is_any_of(":"));
            if (v.size() != 2) {
                throw KARABO_PARAMETER_EXCEPTION("Malformed input argument: " + exchbind);
            }
            futures.push_back(subscribe(client, v[0], v[1]));
        }
    }
    for (auto& fut : futures) {
        const boost::system::error_code ec = fut.get();
        if (ec) {
            throw KARABO_NETWORK_EXCEPTION(std::string("Failed to subscribe to AMQP broker: ") += ec.message());
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
        Logger::configure(Hash("level", "ERROR"));
        Logger::useConsole();

        const std::string brkType = Broker::brokerTypeFrom(brokerUrls);
        cout << "# Trying to connect to broker '" << toString(brokerUrls) << "'...\n" << endl;
        if (brkType == "amqp") {
            logAmqp(brokerUrls, topic, selector);
        } else {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION(brkType + " not supported!");
        }

    } catch (const Exception& e) {
        cerr << e << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
