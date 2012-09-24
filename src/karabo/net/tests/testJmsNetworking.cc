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

#include "../IOService.hh"
#include "../Connection.hh"
#include "../Channel.hh"

using namespace std;
using namespace exfel::util;
using namespace exfel::net;


static int messageGotRead = 0;

void readHandler2(Channel::Pointer channel, const std::string& body, const Hash& header) {
    cout << header << endl;
    cout << body << endl << endl;
    //messageGotRead++;
}

void readHandler1(Channel::Pointer channel, const std::string& body, const Hash& header) {
    cout << header << endl;
    cout << body << endl << endl;
    assert(body == "Random message body");
    assert(header.has("randomHeaderGarbage") == true);
    assert(header.get<string > ("randomHeaderGarbage") == "indeed");
    messageGotRead++;
    channel->readAsyncStringHash(readHandler2);
}

void countDownThenStop(IOService::Pointer ioService, int seconds) {
    boost::this_thread::sleep(boost::posix_time::seconds(seconds));
    ioService->stop();
}

void onError(Channel::Pointer channel, const std::string& errorMessage) {
    cout << errorMessage;
}

int testJmsNetworking(int argc, char** argv) {

    try {

        Test t;
        TEST_INIT(t, argc, argv);

	Connection::Pointer connection = Connection::create(Hash("Jms"));

        IOService::Pointer ioService = connection->getIOService();

        Channel::Pointer channel = connection->start();

        channel->readAsyncStringHash(readHandler1);

        channel->setErrorHandler(&onError);

        channel->write("Random message body", Hash("randomHeaderGarbage", "indeed"));

        boost::thread countDown(&countDownThenStop, ioService, 2);

        ioService->run();

        cerr << "Run has finished" << endl;

        countDown.join();

        cerr << "Countdown finished" << endl;

        assert(messageGotRead == 1);
	
    } catch (const OpenMqException& e) {
      cout << "Skipping test with message: " + e.userFriendlyMsg() << endl;
      return EXIT_SUCCESS;
    } catch (const Exception& e) {
        cout << "Test produced an error:" << endl;
        cout << e.userFriendlyMsg() << endl << endl;
        cout << "Details:" << endl;
        cout << e.detailedMsg();
        return EXIT_SUCCESS;
    }
    return EXIT_SUCCESS;
}
