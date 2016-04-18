/*
 * $Id: signalSlotDemo1.cc 6931 2012-08-03 10:46:27Z heisenb $
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

        SIGNAL1("signalHello", string);

        SLOT1(slotGreet, string);

        SLOT2(slotAnswer, int, Hash);

    }


    void slotGreet(const std::string& msg) {
        cout << msg << endl;
        SIGNAL2("signalAnswer", int, Hash);
        connect("signalAnswer", "slotAnswer");
        emit("signalAnswer", 42, Hash("Was.soll.das.bedeuten", "nix"));
    }


    void slotAnswer(const int& someInteger, const Hash& someConfig) {
        cout << someInteger << endl;
        cout << someConfig << endl;
    }


};


int main(int argc, char** argv) {

    try {

        Hash config("Jms.hashSerialization.Bin");
        BrokerConnection::Pointer connection = BrokerConnection::create(config);

        SignalSlotDemo ssDemo(connection, "SignalSlotDemo");

        boost::thread t(boost::bind(&SignalSlotable::runEventLoop, &ssDemo, false));

        ssDemo.connect("signalHello", "slotGreet");

        ssDemo.emit("signalHello", (string) "Hello World!");

        ssDemo.stopEventLoop();

        t.join();

    } catch (const Exception& e) {
        cout << e;
    }
    return 1;
}

