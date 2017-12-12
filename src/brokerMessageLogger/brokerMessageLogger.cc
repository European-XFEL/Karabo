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
        for (int i = 1; i + 1 < argc; i+=2) {
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

        cout << "# Starting to consume messages..." << std::endl;
        cout << "# Broker: " << brokerUrl << endl;
        cout << "# Topic: " << topic << endl;
        cout << "# Selector: " << selector << endl << endl;

        // Create connection object
        JmsConnection::Pointer connection = boost::make_shared<JmsConnection>(brokerUrl);

        // Connect
        connection->connect();

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
