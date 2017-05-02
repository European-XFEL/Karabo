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
        
        KARABO_SLOT(noded_slot, int);

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
    
    void noded_slot(int number) {
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


enum class ExceptionType {

    none, timeout, remote, cast, signalslot, std, unknown
};


void waitEqual(ExceptionType target, const ExceptionType& test, int trials = 10) {
    // test is by reference since it is expected to be updated from the outside

    // trials = 10 => maximum wait for millisecondsSleep = 2 is about 2 seconds
    unsigned int millisecondsSleep = 2;
    do {
        if (target == test) {
            return;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(millisecondsSleep));
        millisecondsSleep *= 2;
    } while (--trials > 0); // trials is signed to avoid --trials to be very large for input of 0
}


CPPUNIT_TEST_SUITE_REGISTRATION(SignalSlotable_Test);


SignalSlotable_Test::SignalSlotable_Test() {
}


SignalSlotable_Test::~SignalSlotable_Test() {
}


void SignalSlotable_Test::setUp() {
    //Logger::configure(Hash("priority", "ERROR"));
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


    responder->registerSlot<std::string>([&responder](const std::string & q) {
        responder->reply(q + ", world!");
    }, "slotAnswer");

    std::string result;
    greeter->request("responder", "slotAnswer", "Hello").receiveAsync<std::string>([&result](const std::string & answer) {
        result = answer;
    });

    // Wait maximum 200 ms for message travel
    int trials = 10;
    while (--trials >= 0) {
        if (result == "Hello, world!") break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(20));
    }
    CPPUNIT_ASSERT(result == "Hello, world!");
}


void SignalSlotable_Test::testReceiveAsyncError() {
    auto greeter = boost::make_shared<SignalSlotable>("greeter");
    auto responder = boost::make_shared<SignalSlotable>("responder");
    greeter->start();
    responder->start();


    responder->registerSlot<std::string>([&responder](const std::string & q) {
        if (q == "Please, throw!") {
            throw KARABO_PARAMETER_EXCEPTION("Exception was requested");
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
                                         responder->reply(q + ", world!");
    }, "slotAnswer");

    std::string result;
    const auto successHandler = [&result](const std::string & answer) {
        result = answer;
    };
    greeter->request("responder", "slotAnswer", "Hello").timeout(50).receiveAsync<std::string>(successHandler);

    boost::this_thread::sleep(boost::posix_time::milliseconds(200)); // ensure reply could be delivered (though not in time)

    CPPUNIT_ASSERT(result == "");

    // Now the same, but test error handling, first timeout, then remote exception
    result = "some";
    ExceptionType caughtType = ExceptionType::none;
    const auto errHandler = [&caughtType]() {
        try {
            throw;
        } catch (const karabo::util::TimeoutException&) {
            karabo::util::Exception::clearTrace();
            caughtType = ExceptionType::timeout;
        } catch (const karabo::util::RemoteException&) {
            karabo::util::Exception::clearTrace();
            caughtType = ExceptionType::remote;
        } catch (const karabo::util::CastException&) {
            karabo::util::Exception::clearTrace();
            caughtType = ExceptionType::cast;
        } catch (const karabo::util::SignalSlotException&) {
            caughtType = ExceptionType::signalslot;
        } catch (const std::exception& e) {
            caughtType = ExceptionType::std;
        } catch (...) {
            caughtType = ExceptionType::unknown;
        }
    };
    greeter->request("responder", "slotAnswer", "Hello").timeout(50)
            .receiveAsync<std::string>(successHandler, errHandler);
    waitEqual(ExceptionType::timeout, caughtType);
    CPPUNIT_ASSERT_EQUAL(int(ExceptionType::timeout), int(caughtType));
    CPPUNIT_ASSERT_EQUAL(std::string("some"), result);

    caughtType = ExceptionType::none;
    caughtType = ExceptionType::none;
    greeter->request("responder", "slotAnswer", "Please, throw!").timeout(50) // short timeout: should immediately throw
            .receiveAsync<std::string>(successHandler, errHandler);
    waitEqual(ExceptionType::remote, caughtType);
    CPPUNIT_ASSERT_EQUAL(int(ExceptionType::remote), int(caughtType));

    // Trying to receive int where string comes gives SignalSlotException
    // since Slot::callRegisteredSlotFunctions converts the underlying CastException
    // (whereas in the synchronous case one gets SignalSlotException!):
    const auto badSuccessHandler1 = [](int answer) {
    };
    caughtType = ExceptionType::none;
    greeter->request("responder", "slotAnswer", "Hello").timeout(200)
            .receiveAsync<int>(badSuccessHandler1, errHandler);
    waitEqual(ExceptionType::signalslot, caughtType);
    CPPUNIT_ASSERT_EQUAL(int(ExceptionType::signalslot), int(caughtType));

    // Trying to receive more items than come gives karabo::util::SignalSlotException:
    const auto badSuccessHandler2 = [](const std::string& answer, int answer2) {
    };
    caughtType = ExceptionType::none;
    greeter->request("responder", "slotAnswer", "Hello").timeout(200)
            .receiveAsync<std::string, int>(badSuccessHandler2, errHandler);
    waitEqual(ExceptionType::signalslot, caughtType);
    CPPUNIT_ASSERT_EQUAL(int(ExceptionType::signalslot), int(caughtType));

    //    // Too many arguments to slot seems not to harm - should we make it harm?
    //    caughtType = ExceptionType::none;
    //    greeter->request("responder", "slotAnswer", "Hello", 42).timeout(200)
    //            .receiveAsync<std::string>(successHandler, errHandler);
    //    waitEqual(ExceptionType::remote, caughtType);
    //    CPPUNIT_ASSERT_EQUAL(int(ExceptionType::remote), int(caughtType));

    // Too few arguments to slot
    caughtType = ExceptionType::none;
    greeter->request("responder", "slotAnswer").timeout(200)
            .receiveAsync<std::string>(successHandler, errHandler);
    waitEqual(ExceptionType::remote, caughtType);
    CPPUNIT_ASSERT_EQUAL(int(ExceptionType::remote), int(caughtType));

    // Non existing slot of existing instanceId
    caughtType = ExceptionType::none;
    greeter->request("responder", "slot_no_answer", "Hello").timeout(200)
            .receiveAsync<std::string>(successHandler, errHandler);
    waitEqual(ExceptionType::remote, caughtType);
    CPPUNIT_ASSERT_EQUAL(int(ExceptionType::remote), int(caughtType)); // remote exception

    // Non-existing receiver instanceId will run into timeout (shortened time to have less test delay)
    caughtType = ExceptionType::none;
    greeter->request("responder_not_existing", "slotAnswer", "Hello").timeout(150)
            .receiveAsync<std::string>(successHandler, errHandler);
    waitEqual(ExceptionType::timeout, caughtType);
    CPPUNIT_ASSERT_EQUAL(int(ExceptionType::timeout), int(caughtType)); // timeout exception

}


void SignalSlotable_Test::testReceiveExceptions() {
    // Testing the different kinds of exceptions
    auto greeter = boost::make_shared<SignalSlotable>("greeter");
    auto responder = boost::make_shared<SignalSlotable>("responder");
    greeter->start();
    responder->start();

    auto replyString = [&responder](const std::string & q) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(10)); // little sleep for TimeoutException below
        responder->reply(q + ", world!");
    };
    responder->registerSlot<std::string>(replyString, "slotAnswer");

    // Trying to receive int where string comes gives CastException:
    int resultInt;
    CPPUNIT_ASSERT_THROW(greeter->request("responder", "slotAnswer", "Hello").timeout(500).receive(resultInt),
                         karabo::util::CastException);
    karabo::util::Exception::clearTrace();
    // Trying to receive more items than come gives karabo::util::SignalSlotException:
    std::string answer;
    CPPUNIT_ASSERT_THROW(greeter->request("responder", "slotAnswer", "Hello").timeout(500).receive(answer, resultInt),
                         karabo::util::SignalSlotException);
    karabo::util::Exception::clearTrace();
    // Too short timeout gives TimeoutException:
    CPPUNIT_ASSERT_THROW(greeter->request("responder", "slotAnswer", "Hello").timeout(1).receive(answer),
                         karabo::util::TimeoutException);
    karabo::util::Exception::clearTrace();
    // Wrong argument type to slot
    CPPUNIT_ASSERT_THROW(greeter->request("responder", "slotAnswer", 42).timeout(500).receive(answer),
                         karabo::util::RemoteException);
    karabo::util::Exception::clearTrace();
    // Too many arguments to slot seems not to harm - should we make it harm?
    //CPPUNIT_ASSERT_THROW(greeter->request("responder", "slotAnswer", "Hello", 42).timeout(500).receive(answer),
    //                     karabo::util::RemoteException);
    //karabo::util::Exception::clearTrace();
    // Too few arguments to slot
    CPPUNIT_ASSERT_THROW(greeter->request("responder", "slotAnswer").timeout(500).receive(answer),
                         karabo::util::RemoteException);
    karabo::util::Exception::clearTrace();
    // Non existing slot of existing instanceId
    CPPUNIT_ASSERT_THROW(greeter->request("responder", "slot_no_answer", "Hello").timeout(500).receive(answer),
                         karabo::util::RemoteException);
    karabo::util::Exception::clearTrace();
    // Non-existing receiver instanceId will run into timeout (shortened time to have less test delay)
    CPPUNIT_ASSERT_THROW(greeter->request("responder_not_existing", "slotAnswer", "Hello").timeout(150).receive(answer),
                         karabo::util::TimeoutException);
    karabo::util::Exception::clearTrace();
    // Finally no exception:
    CPPUNIT_ASSERT_NO_THROW(greeter->request("responder", "slotAnswer", "Hello").timeout(500).receive(answer));

}

void SignalSlotable_Test::testMethod() {

    const std::string instanceId("SignalSlotDemo");
    auto demo = boost::make_shared<SignalSlotDemo>(instanceId, "dummy");
    demo->start();

    demo->connect("signalA", "slotA");

    demo->emit("signalA", "Hello World!");

    int reply = 0;
    try {
        demo->request(instanceId, "slotC", 1).timeout(500).receive(reply);
    } catch (karabo::util::TimeoutException&) {
        CPPUNIT_ASSERT_MESSAGE("Timeout request/receive slotC", false);
    }
    CPPUNIT_ASSERT_EQUAL(2, reply);

    // The same, but request to self addressed as shortcut
    reply = 0;
    try {
        demo->request("", "slotC", 1).timeout(500).receive(reply);
    } catch (karabo::util::TimeoutException&) {
        CPPUNIT_ASSERT_MESSAGE("Timeout request/receive slotC via \"\"", false);
    }
    CPPUNIT_ASSERT_EQUAL(2, reply);

    std::string someData("myPrivateStuff");
    demo->request(instanceId, "slotC", 1).receiveAsync<int>(boost::bind(&SignalSlotDemo::myCallBack, demo, someData, _1));
    // shortcut address:
    demo->request("", "slotC", 1).receiveAsync<int>(boost::bind(&SignalSlotDemo::myCallBack, demo, someData, _1));

    demo->call(instanceId, "slotC", 1);
    // shortcut address:
    demo->call("", "slotC", 1);
    
    reply = 0;
    try {
        demo->request("", "noded.slot", 1).timeout(500).receive(reply);
    } catch (karabo::util::TimeoutException&) {
        CPPUNIT_ASSERT_MESSAGE("Timeout request/receive noded.slot", false);
    }
    CPPUNIT_ASSERT_EQUAL(2, reply);

    waitDemoOk(demo, 11);
    CPPUNIT_ASSERT(demo->wasOk(11));
    
    

    
}


void SignalSlotable_Test::testAutoConnectSignal() {

    // Give a unique name to exclude interference with other tests
    const std::string instanceId("SignalSlotDemoAutoConnectSignal");
    const std::string instanceId2(instanceId + "2");
    auto demo = boost::make_shared<SignalSlotDemo>(instanceId, instanceId2);
    demo->start();

    // Connect the other's signal to my slot - although the other is not yet there!
    demo->connect(instanceId2, "signalA", "", "slotA");
    demo->emit("signalA", "Hello World!");
    // Allow for some travel time - although nothing should travel...
    waitDemoOk(demo, 0, 6); // 6 trials: slightly more than 100 ms sleep
    CPPUNIT_ASSERT(demo->wasOk(0)); // demo is not interested in its own signals

    auto demo2 = boost::make_shared<SignalSlotDemo>(instanceId2, instanceId);
    demo2->start();

    // Give demo some time to auto-connect now that demo2 is there:
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    demo2->emit("signalA", "Hello World 2!");

    // Time for all signaling (although it is all short-cutting the broker):
    // -> slotA (of other instance) -> connect slotB to signalB -> signalB -> slotB
    waitDemoOk(demo, 2);
    CPPUNIT_ASSERT(demo->wasOk(2));
}


void SignalSlotable_Test::testAutoConnectSlot() {

    // Same as testAutoConnectSignal, but the other way round:
    // slot instance comes into game after connect was called.
    const std::string instanceId("SignalSlotDemoAutoConnectSlot");
    const std::string instanceId2(instanceId + "2");
    auto demo = boost::make_shared<SignalSlotDemo>(instanceId, instanceId2);
    demo->start();


    // Connect the other's slot to my signal - although the other is not yet there!
    demo->connect("", "signalA", instanceId2, "slotA");
    demo->emit("signalA", "Hello World 2!");
    // Allow for some travel time - although nothing should travel...
    waitDemoOk(demo, 0, 6); // 6 trials: slightly more than 100 ms sleep
    CPPUNIT_ASSERT(demo->wasOk(0)); // demo is not interested in its own signals


    auto demo2 = boost::make_shared<SignalSlotDemo>(instanceId2, instanceId);
    demo2->start();

    // Give demo some time to auto-connect now that demo2 is there:
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    demo->emit("signalA", "Hello World 2!");
    // Time for all signaling (although it is all short-cutting the broker):
    // -> slotA (of other instance) -> connect slotB to signalB -> signalB -> slotB
    waitDemoOk(demo2, 2);
    CPPUNIT_ASSERT(demo2->wasOk(2));
}


void SignalSlotable_Test::waitDemoOk(const boost::shared_ptr<SignalSlotDemo>& demo, int messageCalls,
                                     int trials) {
    // trials = 10 => maximum wait for millisecondsSleep = 2 is about 2 seconds
    unsigned int millisecondsSleep = 2;
    do {
        if (demo->wasOk(messageCalls)) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(millisecondsSleep));
        millisecondsSleep *= 2;
    } while (--trials > 0); // trials is signed to avoid --trials to be very large for input of 0
}
