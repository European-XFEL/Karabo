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

class SignalSlotDemo : public karabo::xms::SignalSlotable {
    int m_messageCount;
    bool m_allOk;
    boost::mutex m_mutex;

public:

    KARABO_CLASSINFO(SignalSlotDemo, "SignalSlotDemo", "1.0")

    SignalSlotDemo(const std::string& instanceId, const karabo::net::BrokerConnection::Pointer connection) :
    karabo::xms::SignalSlotable(instanceId, connection), m_messageCount(0), m_allOk(true) {

        KARABO_SIGNAL("signalA", std::string);

        KARABO_SLOT(slotA, std::string);

        KARABO_SLOT(slotB, int, karabo::util::Hash);

        KARABO_SLOT(slotC, int);

    }

    void slotA(const std::string& msg) {
        boost::mutex::scoped_lock lock(m_mutex);
        // Assertions
        m_messageCount++;
        if (msg != "Hello World!") m_allOk = false;
        if (getSenderInfo("slotA")->getInstanceIdOfSender() != "SignalSlotDemo") {
            m_messageCount += 1000; // Invalidate message count will let the test fail!
        }
        SIGNAL2("signalB", int, karabo::util::Hash);
        connect("signalB", "slotB");
        emit("signalB", 42, karabo::util::Hash("Was.soll.das.bedeuten", "nix"));
    }

    void slotB(int someInteger, const karabo::util::Hash& someConfig) {
        boost::mutex::scoped_lock lock(m_mutex);
        // Assertions
        m_messageCount++;
        if (someInteger != 42) m_allOk = false;
        if (someConfig.get<std::string>("Was.soll.das.bedeuten") != "nix") m_allOk = false;
    }

    void slotC(int number) {
        boost::mutex::scoped_lock lock(m_mutex);
        // Assertions
        m_messageCount++;
        if (number != 1) m_allOk = false;
        reply(number + number);
    }

    bool wasOk() {
        std::cout << "\nwasOk : m_messageCount=" << m_messageCount << ", m_allOk=" << m_allOk << std::endl;
        return ((m_messageCount == 6) && m_allOk);
    }

    void myCallBack(const std::string& someData, int number) {
        m_messageCount++;
        std::clog << "Got called with: " << someData << " and " << number << std::endl;
    }


};


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

            //connection = BrokerConnection::create("Jms", Hash("serializationType", "text", "hostname", "localhost"));
            connection = BrokerConnection::create("Jms", Hash("serializationType", "text"));

        } catch (const Exception& e) {
            clog << "Could not establish connection to broker, skipping SignalSlotable_Test" << endl;
            return;
        }

        SignalSlotDemo ssDemo("SignalSlotDemo", connection);
        ssDemo.setNumberOfThreads(2);
        boost::thread t(boost::bind(&SignalSlotable::runEventLoop, &ssDemo, 10, Hash()));                

        // Give thread some time to come up
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));

        bool ok = ssDemo.ensureOwnInstanceIdUnique();
        if (!ok) {
            t.join(); // Blocks
            return;
        }

        ssDemo.connect("signalA", "slotA");

        ssDemo.emit("signalA", "Hello World!");

        bool timeout = false;

        int reply;
        try {
            ssDemo.request("SignalSlotDemo", "slotC", 1).timeout(500).receive(reply);
        } catch (karabo::util::TimeoutException&) {
            timeout = true;
        }

        string someData("myPrivateStuff");
        ssDemo.request("SignalSlotDemo", "slotC", 1).receiveAsync<int>(boost::bind(&SignalSlotDemo::myCallBack, &ssDemo, someData, _1));

        ssDemo.call("SignalSlotDemo", "slotC", 1);

        boost::this_thread::sleep(boost::posix_time::seconds(1));
        ssDemo.stopEventLoop();
        t.join();

        // Looks to be better to do this after joining the thread...
        CPPUNIT_ASSERT(timeout == false);
        CPPUNIT_ASSERT(reply == 2);
        // Give thread some time to receiveAsync from above
        boost::this_thread::sleep(boost::posix_time::milliseconds(250));
        CPPUNIT_ASSERT(ssDemo.wasOk() == true);
    } catch (const karabo::util::Exception& e) {
        cout << e;
    }        
}