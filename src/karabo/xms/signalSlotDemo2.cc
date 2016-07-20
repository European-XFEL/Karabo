/*
 * $Id: main.cc 4640 2011-11-04 13:50:55Z heisenb@DESY.DE $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on January,2011,10:31PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <fstream>
#include <cassert>
#include <iosfwd>

#include <boost/thread.hpp>

#include <exfel/net/BrokerIOService.hh>
#include <exfel/net/BrokerConnection.hh>
#include <exfel/net/BrokerChannel.hh>

#include "SignalSlotable.hh"

using namespace std;
using namespace exfel::util;
using namespace exfel::net;
using namespace exfel::xms;


class SignalSlotDemo : public SignalSlotable {


public:


    EXFEL_CLASSINFO(SignalSlotDemo, "SignalSlotDemo", "1.0")

    SignalSlotDemo(const BrokerConnection::Pointer connection, const std::string& instanceId) : SignalSlotable(connection, instanceId) {

        SLOT1(slotGreet, std::string);

    }


    void slotGreet(const std::string& msg) {
        cout << msg << endl;
        reply("Only ", 42, ", of course!");
    }

};


int main(int argc, char** argv) {

    try {

        Hash config("Jms.hashSerialization.Bin");
        BrokerConnection::Pointer connection = BrokerConnection::create(config);

        SignalSlotDemo ssDemo(connection, "SignalSlotDemo");

        boost::thread t(boost::bind(&SignalSlotable::runEventLoop, &ssDemo, false));

        try {
            std::string answer1;
            int answer2;
            std::string answer3;

            ssDemo.request("SignalSlotDemo", "slotGreet", "Whats up?").timeout(500).receive(answer1, answer2, answer3);

            std::cout << answer1 << answer2 << answer3 << std::endl;
        } catch (const Exception& e) {
            cout << e;
        }

        ssDemo.stopEventLoop();
        t.join();

    } catch (const Exception& e) {
        cout << e;
    }
    return 1;
}

