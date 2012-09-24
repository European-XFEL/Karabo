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


static int messagesRead = 0;

void readHandler4(BrokerChannel::Pointer channel, const Hash& data, const Hash& header) {
    cout << header << endl;
    cout << data << endl << endl;
    messagesRead++;
}

void readHandler3(BrokerChannel::Pointer channel, const char* body, const size_t& bodySize, const Hash& header) {
    assert(bodySize == 10000);
    cout << header;
    for (size_t i = 0; i < 10; ++i) {
        cout << body[i];
    }
    cout << " ... [another 10000 follwing]" << endl << endl;
    messagesRead++;
}

void readHandler2(BrokerChannel::Pointer channel, const std::string& body, const Hash& header) {
    cout << header << endl;
    cout << body << endl << endl;
    assert(body == "Random message body");
    assert(header.has("anotherHeader") == true);
    assert(header.get<string>("anotherHeader") == "Yes");
    messagesRead++;
}

void readHandler1(BrokerChannel::Pointer channel, const std::string& body, const Hash& header) {
    
    cout << header << endl;
    cout << body << endl << endl;
    assert(body == "Random message body");
    assert(header.has("randomHeaderGarbage") == true);
    assert(header.get<string > ("randomHeaderGarbage") == "indeed");
    messagesRead++;
    // Register another handler
    channel->readAsyncStringHash(readHandler2);
    // Send another message
    channel->write(body,Hash("anotherHeader", "Yes"));
}

void readHandlerDebugBinary(BrokerChannel::Pointer channel, const char* data, const size_t& size, const Hash& header) {
    cout << " #### DEBUG HANDLER ####" << endl;
    cout << header << endl;
    cout << string(data,size) << endl;
    cout << " #######################" << endl;
}

void waitHandler1(BrokerChannel::Pointer channel) {
    std::cout << "Finished Waiting" << endl;
}

void waitHandler(BrokerChannel::Pointer channel) {
    std::cout << "Finished waiting, will stop IOService" << endl;
    channel->getConnection()->getIOService()->stop();
}

void onError(BrokerChannel::Pointer channel, const std::string& errorMessage) {
    cout << errorMessage;
}

int main(int argc, char** argv) {
    
    try {

        // Testing read/write

        {
            Hash config("Jms.destinationName", "functionalTestJms");
            BrokerConnection::Pointer connection = BrokerConnection::create(config);
            BrokerIOService::Pointer ioService = connection->getIOService();

            {
                BrokerChannel::Pointer channel = connection->start();

                channel->readAsyncStringHash(readHandler1);

                channel->setErrorHandler(&onError);

            } // Even if the channel goes out of scope the handling still works (connection keeps the channel alive)

            BrokerChannel::Pointer channel = connection->createChannel();
            // The message will still be received as the broker will keep it for some time
            channel->write("Random message body", Hash("randomHeaderGarbage", "indeed"));

            ioService->run();

            assert(messagesRead == 2);
            cout << "ioService->run() has finished" << endl;
            
            // Setting up another connection using the same IOService
            config.clear();
            config.setFromPath("Jms.destinationName", "functionalTestJms");
            config.setFromPath("Jms.IOService", ioService);
            //config.setFromPath("Jms.destinationName", "net-test");
            config.setFromPath("Jms.hashSerialization", Hash("Xml"));
            BrokerConnection::Pointer anotherConnection = BrokerConnection::create(config);
            
            BrokerChannel::Pointer anotherChannel = anotherConnection->start();
            
            anotherChannel->readAsyncRawHash(readHandler3);
            anotherChannel->write(vector<char>(10000, 'a'), Hash());
            anotherChannel->write(vector<char>(10000, 'b'), Hash());
            anotherChannel->write(vector<char>(10000, 'c'), Hash());
            
            anotherChannel->waitAsync(1000, waitHandler);
           
            ioService->work(); // Will block until stop is called
           
            anotherChannel->close();
            assert(messagesRead == 5);
            
            BrokerChannel::Pointer channelForHash = connection->createChannel();
            channelForHash->readAsyncHashHash(readHandler4);
            
            BrokerChannel::Pointer debugChannelBinary = anotherConnection->createChannel();
            debugChannelBinary->readAsyncRawHash(readHandlerDebugBinary);
            
            BrokerChannel::Pointer yetAnotherChannel = anotherConnection->createChannel();
            
            vector<string> v(2);
            v[0] = "foo";
            v[1] = "bar";
            yetAnotherChannel->write(Hash("BodyArray", v), Hash("HeaderBla", 42));
            yetAnotherChannel->waitAsync(1000, waitHandler1);
            ioService->run(); // Will block until stop is called
            assert(messagesRead == 6);
        }
        

    } catch (const Exception& e) {
        cout << "Test produced an error:" << endl;
        cout << e.userFriendlyMsg() << endl;
        cout << "Details:" << endl;
        cout << e.detailedMsg();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
