/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 6, 2011
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <fstream>
#include <cassert>
#include <iosfwd>

#include "BrokerIOService.hh"
#include "BrokerConnection.hh"
#include "BrokerChannel.hh"

using namespace std;
using namespace exfel::util;
using namespace exfel::net;

void readHandler(BrokerChannel::Pointer channel, const char* body, const size_t& bodySize, const Hash& header) {
    string messageBody(body, bodySize);
    cout << header << endl;
    cout << messageBody << endl;
    cout << "-----------------------------------------------------------------------" << endl << endl;
}

void onError(BrokerChannel::Pointer channel, const std::string& errorMessage) {
    cout << errorMessage << endl;
}

void onException(const Exception& e) {
    cout << e << endl;
}

int main(int argc, char** argv) {

    try {

        // Create Jms connection
        Hash config("Jms");
        BrokerConnection::Pointer connection = BrokerConnection::create(config);
        
        // Get a IOService object (for async reading later)
        BrokerIOService::Pointer ioService = connection->getIOService();
        
        // Start connection and obtain a channel
        BrokerChannel::Pointer channel = connection->start();
        
        // Register async reader
        if (argc <= 1) {
            channel->setFilter("signalFunction <> 'signalHeartbeat'");
        }
        channel->readAsyncRawHash(readHandler);
        
        // Block forever
        ioService->work();
        
    } catch (const Exception& e) {
        cout << e << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
