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
#include <iostream>
#include <fstream>
#include <cassert>
#include <iosfwd>

#include <karabo/util/Hash.hh>
#include <karabo/net/EventLoop.hh>
#include <karabo/net/JmsConnection.hh>
#include <karabo/net/JmsConsumer.hh>
#include <karabo/log/Logger.hh>
#include <karabo/xms/SignalSlotable.hh>

using namespace std;
using namespace karabo::util;
using namespace karabo::net;
using namespace karabo::log;
using namespace karabo::xms;

enum class BrokerType {
    OPEN_MQ,
    MQTT
};

void printHelp(const char* execName) {
    cout << "Log all messages of a given broker topic to standard output.\n" << endl;
    cout << "Usage: " << execName << " [-h] [-t topic] [-b brokerUrl] [-s selection]\n" << endl;
    cout << "  -h          : print this help and exit" << endl;
    cout << "  -t topic    : broker topic - if not specified, use environment variables" << endl;
    cout << "                KARABO_BROKER_TOPIC, LOGNAME, USER, LNAME or USERNAME in that" << endl;
    cout << "                order of precendence" << endl;
    cout << "  -b brokerUrl: URL of broker, but environment variable KARABO_BROKER has" << endl;
    cout << "                precedence - if neither option nor variable given, use" << endl;
    cout << "                tcp://exfl-broker.desy.de:7777" << endl;
    cout << "  -s selector : (OpenMQ specific) selector string, e.g. \"slotInstanceIds LIKE '%|deviceId|%'\"" << endl << endl;
}

void readHandler(const Hash::Pointer& header,
                 const Hash::Pointer& body) {

    cout << *header << endl;
    cout << *body << endl;
    cout << "-----------------------------------------------------------------------" << endl << endl;

}

void errorNotifier(consumer::Error errCode, const string& errDesc) {

    cout << ">>>> Error >>>>" << endl;
    cout << " Code: " << static_cast<int>(errCode) << endl;
    cout << " Description: " << errDesc << endl;
}

BrokerType brokerTypeFromUrl(const std::string& brokerUrl) {
    std::string urlLower(brokerUrl);
    transform(urlLower.begin(), urlLower.end(), urlLower.begin(), ::tolower);
    if (urlLower.substr(0, 6) == "tcp://") {
        return BrokerType::OPEN_MQ;
    } else if (urlLower.substr(0, 7) == "mqtt://") {
        return BrokerType::MQTT;
    } else {
        throw runtime_error(brokerUrl +
                            " does not correspond to a known broker connection type.");
    }
}

void logOpenMQ(const std::string& brokerUrl,
               const std::string& topic,
               const std::string& selector) {

    JmsConnection::Pointer connection = boost::make_shared<JmsConnection>(brokerUrl);
    connection->connect();
    string effectiveUrl = connection->getBrokerUrl(); // environment variable has precedence...

    cout << "# Starting to consume messages..." << std::endl;
    cout << "# Broker (OpenMQ): " << effectiveUrl
         << (brokerUrl == effectiveUrl ? "" : " [from environment var]" )
         << endl;
    cout << "# Topic: " << topic << endl;
    cout << "# Selector: " << selector << endl;

    // Obtain consumer
    JmsConsumer::Pointer consumer = connection->createConsumer(topic, selector);

    // Read and logs messages and errors.
    consumer->startReading(boost::bind(&readHandler, _1, _2),
                           boost::bind(&errorNotifier, _1, _2));
    EventLoop::work(); // Block forever
}

void logMqtt(const std::string& brokerUrl,
             const std::string& topic) {
    // TODO: write this once MQTT classes are available
    throw KARABO_NOT_IMPLEMENTED_EXCEPTION("MQTT support is not ready yet!");
}

int main(int argc, char** argv) {

    try {

        // Parse command line
        Hash options;
        for (int i = 1; i < argc; i += 2) {
            const std::string argv_i(argv[i]);
            if (argv_i == "-h" || argv[i - 1] == std::string("-h")
                || argc <= i + 1
                || (argv_i != "-b" && argv_i != "-t" && argv_i != "-s")) {
                printHelp(argv[0]);
                return 0;
            };
            options.set<string>(argv[i], argv[i + 1]);
        }
        string brokerUrl("tcp://exfl-broker.desy.de:7777");
        if (options.has("-b")) options.get("-b", brokerUrl);
        string topic("karabo");
        if (options.has("-t")) options.get("-t", topic);
        else {
            topic = karabo::net::Broker::brokerDomainFromEnv();
        }
        string selector;
        if (options.has("-s")) options.get("-s", selector);

        // Start Logger
        Logger::configure(Hash("priority", "ERROR"));
        Logger::useOstream();

        BrokerType brkType = brokerTypeFromUrl(brokerUrl);
        cout << "# Trying to connect to broker '" << brokerUrl << "'...\n" << endl;
        if (brkType == BrokerType::OPEN_MQ) {
            logOpenMQ(brokerUrl, topic, selector);
        } else {
            logMqtt(brokerUrl, topic);
        }

    } catch (const Exception& e) {
        cerr << e << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
