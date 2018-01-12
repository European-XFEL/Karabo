/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 6, 2011
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <karabo/util/Hash.hh>
#include <karabo/net/JmsConnection.hh>
#include <karabo/net/JmsConsumer.hh>
#include <karabo/net/EventLoop.hh>
#include <karabo/log/Logger.hh>
#include <iostream>
#include <fstream>
#include <cassert>
#include <iosfwd>

using namespace std;
using namespace karabo::util;
using namespace karabo::net;
using namespace karabo::log;


void printHelp(const char* execName) {
    cout << "Log all messages of a given broker topic to standard output.\n" << endl;
    cout << "Usage: " << execName << " [-h] [-t topic] [-b brokerUrl] [-s selection]\n" << endl;
    cout << "  -h          : print this help and exit" << endl;
    cout << "  -t topic    : broker topic - if not specified, use environment variables" << endl;
    cout << "                KARABO_BROKER_TOPIC or USER in that order of precendence" << endl;
    cout << "  -b brokerUrl: URL of broker, but environment variable KARABO_BROKER has" << endl;
    cout << "                precedence - if neither option nor variable given, use" << endl;
    cout << "                tcp://exfl-broker.desy.de:7777" << endl;
    cout << "  -s selector : selector string, e.g. \"slotInstanceIds LIKE '%|deviceId|%'\"" << endl << endl;
}

void readHandler(const Hash::Pointer& header,
                 const Hash::Pointer& body) {

    cout << *header << endl;
    cout << *body << endl;
    cout << "-----------------------------------------------------------------------" << endl << endl;

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
            char* env = getenv("USER");
            if (env != 0) topic = string(env);
            env = getenv("KARABO_BROKER_TOPIC");
            if (env != 0) topic = string(env);
        }
        string selector;
        if (options.has("-s")) options.get("-s", selector);

        // Start Logger
        Logger::configure(Hash("priority", "ERROR"));
        Logger::useOstream();

        cout << "# Trying to connect to broker '" << brokerUrl << "'...\n" << endl;

        // Create connection object
        JmsConnection::Pointer connection = boost::make_shared<JmsConnection>(brokerUrl);

        // Connect
        connection->connect();
        brokerUrl = connection->getBrokerUrl(); // environment variable has precedence...

        cout << "# Starting to consume messages..." << std::endl;
        cout << "# Broker: " << brokerUrl << endl;
        cout << "# Topic: " << topic << endl;
        cout << "# Selector: " << selector << endl << endl;

        // Obtain consumer
        JmsConsumer::Pointer consumer = connection->createConsumer(topic, selector);

        // Read messages
        consumer->startReading(boost::bind(&readHandler, _1, _2));

        EventLoop::work(); // Block forever

    } catch (const Exception& e) {
        cerr << e << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
