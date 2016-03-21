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
        m_messageCount++;
        const std::string sender = getSenderInfo("slotA")->getInstanceIdOfSender();
        // Assertions
        if (sender == "SignalSlotDemo") {
            if (msg != "Hello World!") m_allOk = false;
        } else if (sender == "SignalSlotDemo2") {
            if (msg != "Hello World - now from 2!") m_allOk = false;
        } else {
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

    bool wasOk(int messageCalls) {
        std::cout << "\nwasOk : m_messageCount=" << m_messageCount << ", m_allOk=" << m_allOk << std::endl;
        return ((m_messageCount == messageCalls) && m_allOk);
    }

    void myCallBack(const std::string& someData, int number) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_messageCount++;
        std::cout << "Got called with: " << someData << " and " << number << std::endl;
    }


};


CPPUNIT_TEST_SUITE_REGISTRATION(SignalSlotable_Test);

SignalSlotable_Test::SignalSlotable_Test() {
}


SignalSlotable_Test::~SignalSlotable_Test() { }

void SignalSlotable_Test::setUp() {
    std::pair<SignalSlotDemo::Pointer, boost::shared_ptr<boost::thread> > demo = this->createDemo("SignalSlotDemo");
    m_demo = demo.first;
    m_demoThread = demo.second;
}


std::pair<SignalSlotDemo::Pointer, boost::shared_ptr<boost::thread> >
SignalSlotable_Test::createDemo(const std::string& instanceId) const {

    std::pair<SignalSlotDemo::Pointer, boost::shared_ptr<boost::thread> > result;

    BrokerConnection::Pointer connection;
    try {
        connection = BrokerConnection::create("Jms", Hash("serializationType", "text"));
    } catch (const Exception& e) {
        return result;
    }

    SignalSlotDemo::Pointer demo(new SignalSlotDemo(instanceId, connection));
    demo->setNumberOfThreads(2);
    boost::shared_ptr<boost::thread> demoThread(new boost::thread(boost::bind(&SignalSlotable::runEventLoop, demo, 10, Hash())));

    // Give thread some time to come up
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    bool ok = demo->ensureOwnInstanceIdUnique();
    if (!ok) {
        demoThread->join(); // Blocks
    } else {
        result.first = demo;
        result.second = demoThread;
    }

    return result;
}

void SignalSlotable_Test::tearDown() {
    m_demo.reset();
    m_demoThread.reset();
}


void SignalSlotable_Test::testMethod() {
    CPPUNIT_ASSERT(m_demo);

    m_demo->connect("signalA", "slotA");

    m_demo->emit("signalA", "Hello World!");

    bool timeout = false;

    int reply;
    try {
        m_demo->request("SignalSlotDemo", "slotC", 1).timeout(500).receive(reply);
    } catch (karabo::util::TimeoutException&) {
        timeout = true;
    }

    string someData("myPrivateStuff");
    m_demo->request("SignalSlotDemo", "slotC", 1).receiveAsync<int>(boost::bind(&SignalSlotDemo::myCallBack, m_demo, someData, _1));

    m_demo->call("SignalSlotDemo", "slotC", 1);

    boost::this_thread::sleep(boost::posix_time::seconds(1));
    m_demo->stopEventLoop();
    m_demoThread->join();

    // Assert after joining the thread - otherwise dangling threads in case of failures...
    CPPUNIT_ASSERT(timeout == false);
    CPPUNIT_ASSERT(reply == 2);
    CPPUNIT_ASSERT(m_demo->wasOk(6) == true);
}

void SignalSlotable_Test::testAutoConnect() {
    CPPUNIT_ASSERT(m_demo);

    // Connect the other's signal to my slot - although the other is not yet there!
    m_demo->connect("SignalSlotDemo2", "signalA", "", "slotA");
    m_demo->emit("signalA", "Hello World!");
    // Allow for some travel time - although nothing should travel...
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    const bool ok1 = m_demo->wasOk(0); // m_demo is not interested in its own signals

    std::pair<SignalSlotDemo::Pointer, boost::shared_ptr<boost::thread> > demo2Pair = this->createDemo("SignalSlotDemo2");
    SignalSlotDemo::Pointer demo2 = demo2Pair.first;
    boost::shared_ptr<boost::thread> demo2Thread = demo2Pair.second;
    const bool demo2Fine = demo2;

    // Give m_demo some time to auto-connect now that SignalSlotDemo2 is there:
    boost::this_thread::sleep(boost::posix_time::milliseconds(200));

    demo2->emit("signalA", "Hello World - now from 2!");
    // More time for all signaling (although it is all short-cutting the broker):
    // -> slotA (of other instance) -> connect slotB to signalB -> signalB -> slotB
    boost::this_thread::sleep(boost::posix_time::milliseconds(200));
    const bool ok2 = m_demo->wasOk(2);

    demo2->stopEventLoop();
    m_demo->stopEventLoop();
    demo2Thread->join();
    m_demoThread->join();

    // Asserts after all threads are joined:
    CPPUNIT_ASSERT(ok1);
    CPPUNIT_ASSERT(demo2Fine);
    CPPUNIT_ASSERT(ok2);
}