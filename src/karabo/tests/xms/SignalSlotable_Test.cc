/*
 * File:   SignalSlotable_Test.cc
 * Author: heisenb
 *
 * Created on Apr 4, 2013, 1:24:22 PM
 */

#include <cppunit/TestAssert.h>

#include "SignalSlotable_Test.hh"

#include "karabo/xms/SignalSlotable.hh"
#include "karabo/util/Hash.hh"

#include "boost/thread/mutex.hpp"
#include "boost/shared_ptr.hpp"

#include <string>

using namespace karabo::util;
using namespace karabo::xms;


class SignalSlotDemo : public karabo::xms::SignalSlotable {

    const std::string m_othersId;
    int m_messageCount;
    bool m_allOk;
    boost::mutex m_mutex;

public:


    KARABO_CLASSINFO(SignalSlotDemo, "SignalSlotDemo", "1.0")

    SignalSlotDemo(const std::string& instanceId, const std::string& othersId) :
        karabo::xms::SignalSlotable(instanceId), m_othersId(othersId), m_messageCount(0), m_allOk(true) {

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
        if (sender == getInstanceId()) {
            if (msg != "Hello World!") m_allOk = false;
        } else if (sender == m_othersId) {
            if (msg != "Hello World 2!") m_allOk = false;
        } else {
            m_messageCount += 1000; // Invalidate message count will let the test fail!
        }
        KARABO_SIGNAL("signalB", int, karabo::util::Hash);
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
        return ((m_messageCount == messageCalls) && m_allOk);
    }


    void myCallBack(const std::string& someData, int number) {
        boost::mutex::scoped_lock lock(m_mutex);
        m_messageCount++;
    }


};


CPPUNIT_TEST_SUITE_REGISTRATION(SignalSlotable_Test);


SignalSlotable_Test::SignalSlotable_Test() {
}


SignalSlotable_Test::~SignalSlotable_Test() {
}


void SignalSlotable_Test::setUp() {
    //Logger::configure(Hash("priority", "DEBUG"));
    //Logger::useOstream();
    // Event loop is started in xmsTestRunner.cc's main()
}


void SignalSlotable_Test::tearDown() {
}


void SignalSlotable_Test::testUniqueInstanceId() {

    auto one = boost::make_shared<SignalSlotable>("one");
    auto two = boost::make_shared<SignalSlotable>("two");
    auto one_again = boost::make_shared<SignalSlotable>("one");

    one->start();
    two->start();
    CPPUNIT_ASSERT_THROW(one_again->start(), SignalSlotException);
}


void SignalSlotable_Test::testReceiveAsync() {
    auto greeter = boost::make_shared<SignalSlotable>("greeter");
    auto responder = boost::make_shared<SignalSlotable>("responder");
    greeter->start();
    responder->start();


    responder->registerSlot<std::string>([&responder](const std::string& q) {
        responder->reply(q + ", world!");
    }, "slotAnswer");

    std::string result;
    greeter->request("responder", "slotAnswer", "Hello").receiveAsync<std::string>([&result](const std::string& answer) {
        result = answer;
    });

    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    CPPUNIT_ASSERT(result == "Hello, world!");
}


void SignalSlotable_Test::testMethod() {

    const std::string instanceId("SignalSlotDemo");
    auto demo = boost::make_shared<SignalSlotDemo>(instanceId, "dummy");
    demo->start();

    CPPUNIT_ASSERT(demo);

    demo->connect("signalA", "slotA");

    demo->emit("signalA", "Hello World!");

    bool timeout = false;
    int reply = 0;
    try {
        demo->request(instanceId, "slotC", 1).timeout(500).receive(reply);
    } catch (karabo::util::TimeoutException&) {
        timeout = true;
    }

    // The same, but request to self addressed as shortcut
    bool timeout2 = false;
    int reply2 = 0;
    try {
        demo->request("", "slotC", 1).timeout(500).receive(reply2);
    } catch (karabo::util::TimeoutException&) {
        timeout2 = true;
    }

    std::string someData("myPrivateStuff");
    demo->request(instanceId, "slotC", 1).receiveAsync<int>(boost::bind(&SignalSlotDemo::myCallBack, demo, someData, _1));
    // shortcut address:
    demo->request("", "slotC", 1).receiveAsync<int>(boost::bind(&SignalSlotDemo::myCallBack, demo, someData, _1));

    demo->call(instanceId, "slotC", 1);
    // shortcut address:
    demo->call("", "slotC", 1);

    boost::this_thread::sleep(boost::posix_time::seconds(1));

    // Assert after joining the thread - otherwise dangling threads in case of failures...
    CPPUNIT_ASSERT(timeout == false);
    CPPUNIT_ASSERT(reply == 2);
    CPPUNIT_ASSERT(timeout2 == false);
    CPPUNIT_ASSERT(reply2 == 2);
    CPPUNIT_ASSERT(demo->wasOk(10) == true);
}


void SignalSlotable_Test::testAutoConnectSignal() {

    const std::string instanceId("SignalSlotDemoAutoConnectSignal");
    const std::string instanceId2(instanceId + "2");
    auto demo = boost::make_shared<SignalSlotDemo>(instanceId, instanceId2);
    demo->start();

    CPPUNIT_ASSERT(demo);

    // Connect the other's signal to my slot - although the other is not yet there!
    demo->connect(instanceId2, "signalA", "", "slotA");
    demo->emit("signalA", "Hello World!");
    // Allow for some travel time - although nothing should travel...
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    const bool ok1 = demo->wasOk(0); // m_demo is not interested in its own signals

    auto demo2 = boost::make_shared<SignalSlotDemo>(instanceId2, instanceId);
    demo2->start();

    const bool demo2Fine = demo2 != 0;

    // Give m_demo some time to auto-connect now that SignalSlotDemo2 is there:
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    demo2->emit("signalA", "Hello World 2!");

    // More time for all signaling (although it is all short-cutting the broker):
    // -> slotA (of other instance) -> connect slotB to signalB -> signalB -> slotB
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    const bool ok2 = demo->wasOk(2);

    // Asserts after all threads are joined:
    CPPUNIT_ASSERT(ok1);
    CPPUNIT_ASSERT(demo2Fine);
    CPPUNIT_ASSERT(ok2);
}


void SignalSlotable_Test::testAutoConnectSlot() {

    const std::string instanceId("SignalSlotDemoAutoConnectSlot");
    const std::string instanceId2(instanceId + "2");
    auto demo = boost::make_shared<SignalSlotDemo>(instanceId, instanceId2);
    demo->start();

    // Same as testAutoConnectSignal, but the other way round:
    // slot instance comes into game after connect was called.
    CPPUNIT_ASSERT(demo);

    // Connect the other's slot to my signal - although the other is not yet there!
    demo->connect("", "signalA", instanceId2, "slotA");
    demo->emit("signalA", "Hello World 2!");
    // Allow for some travel time - although nothing should travel...
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    const bool ok1 = demo->wasOk(0); // m_demo is not interested in its own signals

    auto demo2 = boost::make_shared<SignalSlotDemo>(instanceId2, instanceId);
    demo2->start();

    const bool demo2Fine = demo2 != 0;

    // Give m_demo some time to auto-connect now that SignalSlotDemo2 is there:
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    demo->emit("signalA", "Hello World 2!");
    // More time for all signaling (although it is all short-cutting the broker):
    // -> slotA (of other instance) -> connect slotB to signalB -> signalB -> slotB
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    const bool ok2 = demo2->wasOk(2);

    // Asserts after all threads are joined:
    CPPUNIT_ASSERT(ok1);
    CPPUNIT_ASSERT(demo2Fine);
    CPPUNIT_ASSERT(ok2);
}
