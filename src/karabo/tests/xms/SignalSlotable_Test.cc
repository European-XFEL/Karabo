/*
 * File:   SignalSlotable_Test.cc
 * Author: heisenb
 *
 * Created on Apr 4, 2013, 1:24:22 PM
 */

#include <boost/thread/pthread/thread_data.hpp>

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
        
        //karabo::log::Logger::configure(Hash("priority", "DEBUG"));
        
        BrokerConnection::Pointer connection;

        try {

            connection = BrokerConnection::create("Jms", Hash("serializationType", "text", "hostname", "localhost"));

        } catch (const Exception& e) {
            clog << "Could not establish connection to broker, skipping SignalSlotable_Test" << endl;
            return;
        }

        SignalSlotDemo ssDemo("SignalSlotDemo", connection);

        boost::thread t(boost::bind(&SignalSlotable::runEventLoop, &ssDemo, 10, Hash(), 2));
        
        // Give thread some time to come up
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        
        ssDemo.connectN("signalA", "slotA");

        ssDemo.emit("signalA", "Hello World!");

        int reply;
        try {
            ssDemo.request("SignalSlotDemo", "slotC", 1).timeout(500).receive(reply);
        } catch (karabo::util::TimeoutException&) {
            CPPUNIT_ASSERT(false == true);
        }
        CPPUNIT_ASSERT(reply == 2);
        
        string someData("myPrivateStuff");
        ssDemo.request("SignalSlotDemo", "slotC", 1).receiveAsync<int>(boost::bind(&SignalSlotDemo::myCallBack, &ssDemo, someData, _1));
            
        ssDemo.call("SignalSlotDemo", "slotC", 1);
                
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        ssDemo.stopEventLoop();
        t.join();

        CPPUNIT_ASSERT(ssDemo.wasOk() == true);
    } catch (const karabo::util::Exception& e) {
        cout << e;
    }        
}