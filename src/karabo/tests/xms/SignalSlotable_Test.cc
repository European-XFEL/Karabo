/*
 * File:   SignalSlotable_Test.cc
 * Author: heisenb
 *
 * Created on Apr 4, 2013, 1:24:22 PM
 */

#include "SignalSlotable_Test.hh"

using namespace std;
using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::net;
using namespace karabo::xms;

CPPUNIT_TEST_SUITE_REGISTRATION(SignalSlotable_Test);


SignalSlotable_Test::SignalSlotable_Test() {
}


SignalSlotable_Test::~SignalSlotable_Test() {
}


void SignalSlotable_Test::setUp() {
}


void SignalSlotable_Test::tearDown() {
}


void SignalSlotable_Test::testMethod() {

    try {
        BrokerConnection::Pointer connection;

        try {

            connection = BrokerConnection::create("Jms", Hash("serializationType", "text"));

        } catch (const Exception& e) {
            clog << "Could not establish connection to broker, skipping SignalSlotable_Test" << endl;
            return;
        }

        SignalSlotDemo ssDemo("SignalSlotDemo", connection);

        boost::thread t(boost::bind(&SignalSlotable::runEventLoop, &ssDemo, 0, Hash()));

        ssDemo.connectN("signalA", "slotA");

        ssDemo.emit("signalA", "Hello World!");

        int reply;
        ssDemo.request("SignalSlotDemo", "slotC", 1).timeout(100).receive(reply);
        CPPUNIT_ASSERT(reply == 2);
        
        ssDemo.call("SignalSlotDemo", "slotC", 1);
            
        ssDemo.stopEventLoop();
        t.join();

        CPPUNIT_ASSERT(ssDemo.wasOk() == true);
    } catch (const karabo::util::Exception& e) {
        cout << e;
    }
}

