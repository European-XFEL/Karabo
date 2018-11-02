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
#include <future>

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


void SignalSlotable_Test::testValidInstanceId() {

    const auto allowedChars = "0123456789_abcdefghijklmnopqrstuvwxyz-ABCDEFGHIJKLMNOPQRSTUVWXYZ/";
    // all allowed characters should allow the instance to start
    auto s = boost::make_shared<SignalSlotable>(allowedChars);
    CPPUNIT_ASSERT_NO_THROW(s->start());

    // dot '.' is bad since id often used as key in Hash, so instance must not start
    s = boost::make_shared<SignalSlotable>("a.b");
    CPPUNIT_ASSERT_THROW(s->start(), SignalSlotException);

    // space ' ' causes problem in xml serialisation, so instance must not start
    s = boost::make_shared<SignalSlotable>("a b");
    CPPUNIT_ASSERT_THROW(s->start(), SignalSlotException);

    // colon ':' separates instanceId and pipeline channel name, so instance must not start
    s = boost::make_shared<SignalSlotable>("a:b");
    CPPUNIT_ASSERT_THROW(s->start(), SignalSlotException);
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


void SignalSlotable_Test::testReceiveAsyncNoReply() {
    auto greeter = boost::make_shared<SignalSlotable>("greeter");
    auto responder = boost::make_shared<SignalSlotable>("responder");
    greeter->start();
    responder->start();

    responder->registerSlot([&responder]() {
        // No call to reply() - reply without arguments is the default.
    }, "slotAnswer");

    bool handlerCalled = false;
    const auto normalHandler = [&handlerCalled]() {
        handlerCalled = true;
    };
    int errorCode = 0;
    const auto errHandler = [&errorCode]() {
        try {
            throw;
        } catch (const karabo::util::SignalSlotException&) {
            errorCode = 4200;
        } catch (...) {
            // Leave some breadcrumbs for the assert statement below
            errorCode = -4200;
        }
        karabo::util::Exception::clearTrace();
    };
    // All arguments match, so we expect the normalHandler to be called
    greeter->request("responder", "slotAnswer").receiveAsync(normalHandler, errHandler);

    // Wait maximum 400 ms for message travel
    int trials = 20;
    while (--trials >= 0) {
        if (handlerCalled || errorCode != 0) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(20));
    }
    CPPUNIT_ASSERT(handlerCalled);
    CPPUNIT_ASSERT_EQUAL(0, errorCode);
    //
    // Now test reply argument mismatch of automatically placed reply()
    //
    errorCode = 0;
    handlerCalled = false;
    const auto wrongArgHandler = [&handlerCalled](const karabo::util::Hash & some) {
        // We don't expect this handler to be called since its argument does not match the (empty) reply
        handlerCalled = true;
    };
    // Now wrongArgHandler expects a Hash but slotAnswer placed (automatically) an empty reply
    greeter->request("responder", "slotAnswer").receiveAsync<karabo::util::Hash>(wrongArgHandler, errHandler);

    // Wait maximum 400 ms for message travel
    trials = 20;
    while (--trials >= 0) {
        if (handlerCalled || errorCode != 0) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(20));
    }

    // Assert that the handler was not called, but the error handler with the correct exception (due to argument mismatch)
    CPPUNIT_ASSERT(!handlerCalled);
    CPPUNIT_ASSERT_EQUAL(4200, errorCode);
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


void SignalSlotable_Test::testConnectAsync() {

    auto signaler = boost::make_shared<SignalSlotable>("signalInstance");
    signaler->registerSignal<int>("signal");
    signaler->start();

    auto slotter = boost::make_shared<SignalSlotable>("slotInstance");
    bool slotCalled = false;
    int inSlot = -10;
    auto slotFunc = [&slotCalled, &inSlot] (int i) {
        inSlot += i;
        slotCalled = true;
    };
    slotter->registerSlot<int>(slotFunc, "slot");
    slotter->start();

    ///////////////////////////////////////////////////////////////////////////
    // First test successful connectAsync
    bool connected = false;
    auto connectedHandler = [&connected]() {
        connected = true;
    };
    signaler->asyncConnect("signalInstance", "signal", "slotInstance", "slot", connectedHandler);
    // Give some time to connect
    for (int i = 0; i < 100; ++i) {
        if (connected) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    };
    CPPUNIT_ASSERT(connected);

    signaler->emit("signal", 52);

    // Give signal some time to travel
    for (int i = 0; i < 100; ++i) {
        if (slotCalled) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    };
    CPPUNIT_ASSERT(slotCalled);
    CPPUNIT_ASSERT_EQUAL(42, inSlot);

    ///////////////////////////////////////////////////////////////////////////
    // Now test failureHandler - non-existing signal gives specific exception type and message
    bool connectFailed = false;
    bool connectTimeout = false;
    std::string connectFailedMsg;
    auto connectFailedHandler = [&connectFailed, &connectTimeout, &connectFailedMsg] () {
        connectFailed = true;
        try {
            throw;
        } catch (const karabo::util::TimeoutException& e) {
            connectTimeout = true;
        } catch (const karabo::util::SignalSlotException& e) {
            connectFailedMsg = e.what();
        } catch (...) { // Avoid that an exception leaks out and crashes the test program.
        }
    };
    auto dummyHandler = [] () {
    };
    signaler->asyncConnect("signalInstance", "NOT_A_signal", "slotInstance", "slot",
                           dummyHandler, connectFailedHandler);

    // Give some time to find out that signal is not there
    for (int i = 0; i < 100; ++i) {
        if (connectFailed) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    };
    CPPUNIT_ASSERT(connectFailed);
    CPPUNIT_ASSERT(!connectTimeout);
    // connectFailedMsg is the full, formatted exception info ("Exception =====> {\n ... \n Message....")
    // check that the original message is part of it
    CPPUNIT_ASSERT(connectFailedMsg.find("signalInstance has no signal 'NOT_A_signal'.") != std::string::npos);

    ///////////////////////////////////////////////////////////////////////////
    // Test failureHandler again - now non-existing slot gives same exception type, but other message
    connectFailed = false;
    connectTimeout = false;
    connectFailedMsg = "";
    signaler->asyncConnect("signalInstance", "signal", "slotInstance", "NOT_A_slot",
                           dummyHandler, connectFailedHandler);

    // Give some time to find out that slot is not there.
    for (int i = 0; i < 100; ++i) {
        if (connectFailed) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    };
    CPPUNIT_ASSERT(connectFailed);
    CPPUNIT_ASSERT(!connectTimeout);
    CPPUNIT_ASSERT(connectFailedMsg.find("slotInstance has no slot 'NOT_A_slot'.") != std::string::npos);

    ///////////////////////////////////////////////////////////////////////////
    // Another test for failureHandler - non-existing signalInstanceId gives TimeoutException
    connectFailed = false;
    connectTimeout = false;
    connectFailedMsg = "";
    signaler->asyncConnect("NOT_A_signalInstance", "signal", "slotInstance", "slot",
                           dummyHandler, connectFailedHandler,
                           50); // Timeout allows 25 ms per message travel
    // The first request will succeed, the second (to "NOT_A_sig...") not - so wait 4 * 25 ms plus margin to be sure
    boost::this_thread::sleep(boost::posix_time::milliseconds(105));
    CPPUNIT_ASSERT(connectFailed);
    CPPUNIT_ASSERT(connectTimeout);
    CPPUNIT_ASSERT(connectFailedMsg.empty());

    ///////////////////////////////////////////////////////////////////////////
    // Final test for failureHandler - non-existing slotInstanceId gives again TimeoutException
    connectFailed = false;
    connectTimeout = false;
    connectFailedMsg = "";
    signaler->asyncConnect("signalInstance", "signal", "NOT_A_slotInstance", "slot",
                           dummyHandler, connectFailedHandler,
                           50); // Timeout allows 25 ms per message travel
    // The first request (to "NOT_A_slot...") will fail, so allow for 2 * 25 ms plus margin to be sure
    boost::this_thread::sleep(boost::posix_time::milliseconds(55));
    CPPUNIT_ASSERT(connectFailed);
    CPPUNIT_ASSERT(connectTimeout);
    CPPUNIT_ASSERT(connectFailedMsg.empty());
}


void SignalSlotable_Test::testConnectAsyncMulti() {

    // One instance with and signalA and slotB...
    auto signalerA = boost::make_shared<SignalSlotable>("signalA_slotB");
    signalerA->registerSignal<int>("signalA");
    bool slotCalledB = false;
    int inSlotB = -10;
    auto slotFuncB = [&slotCalledB, &inSlotB] (int i) {
        inSlotB += i;
        slotCalledB = true;
    };
    signalerA->registerSlot<int>(slotFuncB, "slotB");
    signalerA->start();

    // .. and one with and signalB and slotA...
    auto signalerB = boost::make_shared<SignalSlotable>("signalB_slotA");
    signalerB->registerSignal<int>("signalB");
    bool slotCalledA = false;
    int inSlotA = -10;
    auto slotFuncA = [&slotCalledA, &inSlotA] (int i) {
        inSlotA += i;
        slotCalledA = true;
    };
    signalerB->registerSlot<int>(slotFuncA, "slotA");
    signalerB->start();

    ///////////////////////////////////////////////////////////////////////////
    // First test successful connectAsync
    bool connected = false;
    auto connectedHandler = [&connected]() {
        connected = true;
    };
    bool connectFailed = false;
    auto failureHandler = [&connectFailed]() {
        connectFailed = true;
    };
    // Cross connect the instances
    typedef SignalSlotable::SignalSlotConnection SignalSlotConnection;
    const std::vector<SignalSlotConnection> connections{SignalSlotConnection("signalA_slotB", "signalA", "signalB_slotA", "slotA"),
                                                        SignalSlotConnection("signalB_slotA", "signalB", "signalA_slotB", "slotB")};
    signalerA->asyncConnect(connections, connectedHandler, failureHandler);
    // Give some time to connect
    for (int i = 0; i < 200; ++i) {
        if (connected) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    };
    CPPUNIT_ASSERT(connected);
    CPPUNIT_ASSERT(!connectFailed);

    signalerA->emit("signalA", 52);
    signalerB->emit("signalB", -32);

    // Give signal some time to travel
    for (int i = 0; i < 200; ++i) {
        if (slotCalledA && slotCalledB) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    };
    CPPUNIT_ASSERT(slotCalledA);
    CPPUNIT_ASSERT(slotCalledB);
    CPPUNIT_ASSERT_EQUAL(42, inSlotA);
    CPPUNIT_ASSERT_EQUAL(-42, inSlotB);

    // Clean up established connections (synchronously)
    for (const SignalSlotConnection& con : connections) {
        CPPUNIT_ASSERT(signalerA->disconnect(con.signalInstanceId, con.signal, con.slotInstanceId, con.slot));
    }

    ///////////////////////////////////////////////////////////////////////////
    // Now test failureHandler - non-existing signal gives specific exception type and message

    // Re-use connectedHandler, but reset its flag to check that it is never called
    connected = false;

    connectFailed = false; // re-used variable
    bool connectTimeout = false;
    std::string connectFailedMsg;
    auto connectFailedHandler = [&connectFailed, &connectTimeout, &connectFailedMsg] () {
        connectFailed = true;
        try {
            throw;
        } catch (const karabo::util::TimeoutException& e) {
            connectTimeout = true;
        } catch (const karabo::util::SignalSlotException& e) {
            connectFailedMsg = e.what();
        } catch (...) { // Avoid that an exception leaks out and crashes the test program.
        }
    };

    std::vector<SignalSlotConnection> badConnections(connections);
    badConnections.push_back(SignalSlotConnection("signalA_slotB", "NOT_A_signal", "signalB_slotA", "slotA"));

    signalerA->asyncConnect(badConnections, connectedHandler, connectFailedHandler);

    // Give some time to find out that signal is not there
    for (int i = 0; i < 100; ++i) {
        if (connected || connectFailed) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    };
    CPPUNIT_ASSERT(connectFailed);
    CPPUNIT_ASSERT(!connected);
    CPPUNIT_ASSERT(!connectTimeout);
    // connectFailedMsg is the full, formatted exception info ("Exception =====> {\n ... \n Message....")
    // check that the original message is part of it
    CPPUNIT_ASSERT(connectFailedMsg.find("signalA_slotB has no signal 'NOT_A_signal'.") != std::string::npos);

    // Clean up established connections (synchronously)
    for (const SignalSlotConnection& con : connections) {
        // Do not test return value - the correct connections should be connected, but maybe not yet...
        signalerA->disconnect(con.signalInstanceId, con.signal, con.slotInstanceId, con.slot);
    }
    ///////////////////////////////////////////////////////////////////////////
    // Test failureHandler again - now non-existing slot gives same exception type, but other message
    connected = false;
    connectFailed = false;
    connectTimeout = false;
    connectFailedMsg = "";
    // replace bad connection by another bad one...
    badConnections.back() = SignalSlotConnection("signalA_slotB", "signalA", "signalB_slotA", "NOT_A_slot");
    signalerA->asyncConnect(badConnections, connectedHandler, connectFailedHandler);

    // Give some time to find out that slot is not there
    for (int i = 0; i < 100; ++i) {
        if (connected || connectFailed) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    };
    CPPUNIT_ASSERT(connectFailed);
    CPPUNIT_ASSERT(!connected);
    CPPUNIT_ASSERT(!connectTimeout);
    // connectFailedMsg is the full, formatted exception info ("Exception =====> {\n ... \n Message....")
    // check that the original message is part of it
    CPPUNIT_ASSERT(connectFailedMsg.find("signalB_slotA has no slot 'NOT_A_slot'.") != std::string::npos);

    // Clean up established connections (synchronously)
    for (const SignalSlotConnection& con : connections) {
        // Do not test return value - the correct connections should be connected, but maybe not yet...
        signalerA->disconnect(con.signalInstanceId, con.signal, con.slotInstanceId, con.slot);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Another test for failureHandler - non-existing signalInstanceId gives TimeoutException
    connected = false;
    connectFailed = false;
    connectTimeout = false;
    connectFailedMsg = "";
    // replace bad connection by another bad one...
    badConnections.back() = SignalSlotConnection("NOT_A_signalInstance", "signalA", "signalB_slotA", "slotA");

    // Timeout of 50 allows 25 ms per message travel (without setting this, we would have to wait for ages)
    signalerA->asyncConnect(badConnections, connectedHandler, connectFailedHandler, 50);

    // The first request will succeed, the second (to "NOT_A_sig...") not - so wait 4 * 25 ms plus margin to be sure
    boost::this_thread::sleep(boost::posix_time::milliseconds(105));

    CPPUNIT_ASSERT(connectFailed);
    CPPUNIT_ASSERT(!connected);
    CPPUNIT_ASSERT(connectTimeout);
    CPPUNIT_ASSERT(connectFailedMsg.empty());

    // Clean up established connections (synchronously)
    for (const SignalSlotConnection& con : connections) {
        // Do not test return value - the correct connections should be connected, but maybe not yet...
        signalerA->disconnect(con.signalInstanceId, con.signal, con.slotInstanceId, con.slot);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Final test for failureHandler - non-existing slotInstanceId gives again TimeoutException
    connected = false;
    connectFailed = false;
    connectTimeout = false;
    connectFailedMsg = "";
    // replace bad connection by another bad one...
    badConnections.back() = SignalSlotConnection("signalA_slotB", "signalA", "NOT_A_slotInstance", "slotA");
    // Timeout of 50 allows 25 ms per message travel
    signalerA->asyncConnect(badConnections, connectedHandler, connectFailedHandler, 50);
    // The first request (to "NOT_A_slot...") will fail, so allow for 2 * 25 ms plus margin to be sure
    boost::this_thread::sleep(boost::posix_time::milliseconds(55));

    CPPUNIT_ASSERT(connectFailed);
    CPPUNIT_ASSERT(!connected);
    CPPUNIT_ASSERT(connectTimeout);
    CPPUNIT_ASSERT(connectFailedMsg.empty());
}


void SignalSlotable_Test::testDisconnectAsync() {

    auto signaler = boost::make_shared<SignalSlotable>("signalInstance");
    signaler->registerSignal<int>("signal");
    signaler->start();

    auto slotter = boost::make_shared<SignalSlotable>("slotInstance");
    bool slotCalled = false;
    auto slotFunc = [&slotCalled] () {
        slotCalled = true;
    };
    slotter->registerSlot(slotFunc, "slot");
    slotter->start();

    ///////////////////////////////////////////////////////////////////////////
    // First test successful asyncDisconnect
    CPPUNIT_ASSERT(signaler->connect("signalInstance", "signal", "slotInstance", "slot"));

    bool disconnectSuccess = false;
    auto disconnectSuccessHandler = [&disconnectSuccess] () {
        disconnectSuccess = true;
    };
    bool disconnectFailed = false;
    bool disconnectTimeout = false;
    std::string disconnectFailedMsg;
    auto disconnectFailedHandler = [&disconnectFailed, &disconnectTimeout, &disconnectFailedMsg] () {
        disconnectFailed = true;
        try {
            throw;
        } catch (const karabo::util::TimeoutException& e) {
            disconnectTimeout = true;
        } catch (const karabo::util::SignalSlotException& e) {
            disconnectFailedMsg = e.what();
        } catch (...) { // Avoid that an exception leaks out and crashes the test program.
        }
    };
    signaler->asyncDisconnect("signalInstance", "signal", "slotInstance", "slot",
                              disconnectSuccessHandler, disconnectFailedHandler);
    // Give disconnection call some time to travel
    for (int i = 0; i < 100; ++i) {
        if (disconnectSuccess) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    };
    CPPUNIT_ASSERT(disconnectSuccess);
    CPPUNIT_ASSERT(!disconnectFailed);

    signaler->emit("signal");
    // Give signal some time to travel - but it won't, since disconnected!
    boost::this_thread::sleep(boost::posix_time::milliseconds(200));
    CPPUNIT_ASSERT(!slotCalled);

    ///////////////////////////////////////////////////////////////////////////
    // Now test failureHandler - we are not connected, so fail
    disconnectSuccess = false;
    disconnectFailed = false;
    signaler->asyncDisconnect("signalInstance", "signal", "slotInstance", "slot",
                              disconnectSuccessHandler, disconnectFailedHandler);
    // Give some time to find out that signal is not there
    for (int i = 0; i < 100; ++i) {
        if (disconnectFailed) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    };
    CPPUNIT_ASSERT(disconnectFailed);
    CPPUNIT_ASSERT(!disconnectSuccess);
    // connectFailedMsg is the full, formatted exception info ("Exception =====> {\n ... \n Message....")
    // check that the original message is part of it (in two steps)
    CPPUNIT_ASSERT(disconnectFailedMsg.find("failed to disconnect slot") != std::string::npos);
    CPPUNIT_ASSERT(disconnectFailedMsg.find("was not connected") != std::string::npos);

    ///////////////////////////////////////////////////////////////////////////
    // Another test of failure (disconnect from non-existing signal), but exactly the same symptom
    disconnectSuccess = false;
    disconnectFailed = false;
    disconnectFailedMsg.clear();
    signaler->asyncDisconnect("signalInstance", "NOT_A_signal", "slotInstance", "slot",
                              disconnectSuccessHandler, disconnectFailedHandler);
    // Give some time to find out that signal is not there
    for (int i = 0; i < 100; ++i) {
        if (disconnectFailed) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    };
    CPPUNIT_ASSERT(disconnectFailed);
    CPPUNIT_ASSERT(!disconnectSuccess);
    // connectFailedMsg is the full, formatted exception info ("Exception =====> {\n ... \n Message....")
    // check that the original message is part of it
    CPPUNIT_ASSERT(disconnectFailedMsg.find("failed to disconnect slot") != std::string::npos);
    CPPUNIT_ASSERT(disconnectFailedMsg.find("was not connected") != std::string::npos);

    ///////////////////////////////////////////////////////////////////////////
    // One more test of failure (disconnect from non-existing signalInstance): timeout
    disconnectSuccess = false;
    disconnectFailed = false;
    disconnectFailedMsg.clear();
    signaler->asyncDisconnect("NOT_A_signalInstance", "signal", "slotInstance", "slot",
                              disconnectSuccessHandler, disconnectFailedHandler, 150); // timeout of 150 ms
    // Give some time to find out that signal is not there
    for (int i = 0; i < 200; ++i) {
        if (disconnectFailed) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    };
    CPPUNIT_ASSERT(disconnectFailed);
    CPPUNIT_ASSERT(disconnectTimeout);
    CPPUNIT_ASSERT(!disconnectSuccess);

    ///////////////////////////////////////////////////////////////////////////
    // No need to test non-existing slot - it is exactly the same as a non-connected slot
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


void SignalSlotable_Test::testAsyncReply() {


    class SignalSlotableAsyncReply : public karabo::xms::SignalSlotable {


    public:


        KARABO_CLASSINFO(SignalSlotableAsyncReply, "SignalSlotAsyncReply", "1.0")

        SignalSlotableAsyncReply(const std::string& instanceId)
            : karabo::xms::SignalSlotable(instanceId)
            , m_slotCallEnded(false)
            , m_asynReplyHandlerCalled(false)
            , m_slotCallEnded_error(false)
            , m_asynReplyHandlerCalled_error(false)
            , m_timer(karabo::net::EventLoop::getIOService()) {

            KARABO_SLOT(slotAsyncReply, int);
            KARABO_SLOT(slotAsyncErrorReply);
        }


        void slotAsyncReply(int i) {
            const AsyncReply reply(this);

            // Let's stop this function call soon, but nevertheless send reply in 100 ms (likely in another thread!)
            m_timer.expires_from_now(boost::posix_time::milliseconds(100));
            m_timer.async_wait([reply, i, this](const boost::system::error_code & ec) {
                if (ec) return;
                reply(2 * i + 1);
                this->m_asynReplyHandlerCalled = true;
            });
            m_slotCallEnded = true;
        }


        void slotAsyncErrorReply() {
            const AsyncReply reply(this);

            // Let's stop this function call soon, but nevertheless send error reply in 100 ms (likely in another thread!)
            m_timer.expires_from_now(boost::posix_time::milliseconds(100));
            m_timer.async_wait([reply, this](const boost::system::error_code & ec) {
                if (ec) return;
                reply.error("Something nasty to be expected!");
                this->m_asynReplyHandlerCalled_error = true;
            });
            m_slotCallEnded_error = true;
        }


        bool m_slotCallEnded;
        bool m_asynReplyHandlerCalled;

        bool m_slotCallEnded_error;
        bool m_asynReplyHandlerCalled_error;
    private:
        boost::asio::deadline_timer m_timer;

    };

    auto slotter = boost::make_shared<SignalSlotableAsyncReply>("slotter");
    auto sender = boost::make_shared<SignalSlotable>("sender");
    sender->start();
    slotter->start();

    bool received = false;
    bool resultIs7 = false;
    auto successHandler = [&received, &resultIs7] (int j) {
        received = true;
        if (j == 7) resultIs7 = true;
    };
    bool errorHappened = false;
    auto errorHandler = [&errorHappened] () {
        errorHappened = true;
    };
    sender->request("slotter", "slotAsyncReply", 3).receiveAsync<int>(successHandler, errorHandler);
    // Some time for message travel and execution until slot call is finished...
    int counter = 100;
    while (--counter >= 0) {
        if (slotter->m_slotCallEnded) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }
    CPPUNIT_ASSERT(slotter->m_slotCallEnded);
    // ...but reply is not yet received
    CPPUNIT_ASSERT(!received);

    // Now wait until reply is received
    counter = 100;
    while (--counter >= 0) {
        if (received) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }
    // Assert and also check result:
    CPPUNIT_ASSERT(slotter->m_asynReplyHandlerCalled);
    CPPUNIT_ASSERT(received);
    CPPUNIT_ASSERT(resultIs7);
    CPPUNIT_ASSERT(!errorHappened);

    //
    // Now check AsyncReply::error
    //
    bool receivedSuccess = false;
    auto successHandler2 = [&receivedSuccess] () {
        receivedSuccess = true;
    };
    bool receivedError = false;
    bool remoteError = false;
    std::string errorText;
    auto errorHandler2 = [&receivedError, &remoteError, &errorText] () {
        receivedError = true;
        try {
            throw;
        } catch (const karabo::util::RemoteException& re) {
            remoteError = true;
            errorText = re.what();
        } catch (const std::exception& e) {
            errorText = e.what();
        }
    };

    sender->request("slotter", "slotAsyncErrorReply").receiveAsync(successHandler2, errorHandler2);
    // Some time for message travel and execution until slot call is finished...
    counter = 100;
    while (--counter >= 0) {
        if (slotter->m_slotCallEnded_error) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }
    CPPUNIT_ASSERT(slotter->m_slotCallEnded_error);
    // ...but reply is not yet received
    CPPUNIT_ASSERT(!receivedSuccess && !receivedError);

    // Now wait until reply is received
    counter = 100;
    while (--counter >= 0) {
        if (receivedSuccess || receivedError) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }
    // Assert and also check result:
    CPPUNIT_ASSERT(slotter->m_asynReplyHandlerCalled_error);
    CPPUNIT_ASSERT(receivedError);
    CPPUNIT_ASSERT(!receivedSuccess);
    // The text we have is part of the full exception message that e.g. also contains the time stamp
    CPPUNIT_ASSERT(errorText.find("Something nasty to be expected!") != std::string::npos);
    CPPUNIT_ASSERT(remoteError);

    //
    // Check also calling synchronously to a slot that answers an error via AsyncReply
    //
    CPPUNIT_ASSERT_THROW(sender->request("slotter", "slotAsyncErrorReply").timeout(500).receive(),
                         karabo::util::RemoteException);

    //
    // Now check that we can call a slot with an async reply directly (although the reply does not matter)
    //
    slotter->m_slotCallEnded = false;
    slotter->m_asynReplyHandlerCalled = false;
    CPPUNIT_ASSERT_NO_THROW(slotter->slotAsyncReply(3));
    CPPUNIT_ASSERT(slotter->m_slotCallEnded);

    counter = 100;
    while (--counter >= 0) {
        if (slotter->m_asynReplyHandlerCalled) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }
    CPPUNIT_ASSERT(slotter->m_asynReplyHandlerCalled);
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


void SignalSlotable_Test::testRegisterSlotTwice() {
    // Registering two function of the same signature for the same slot means that both are executed
    // when the slot is called.
    auto instance = boost::make_shared<SignalSlotable>("instance");
    instance->start();

    bool firstIsCalled = false;
    auto first = [&firstIsCalled]() {
        firstIsCalled = true;
    };
    instance->registerSlot(first, "slot");

    bool secondIsCalled = false;
    auto second = [&secondIsCalled]() {
        secondIsCalled = true;
    };
    // Adding second with same signature is fine in contrast to third below:
    CPPUNIT_ASSERT_NO_THROW(instance->registerSlot(second, "slot"));

    auto tester = boost::make_shared<SignalSlotable>("tester");
    tester->start();
    // Synchronous request to avoid sleeps in test - assert that no timeout happens.
    // Our slot functions do not place any answers, so an empty one will be added.
    CPPUNIT_ASSERT_NO_THROW(tester->request("instance", "slot").timeout(500).receive());

    CPPUNIT_ASSERT(firstIsCalled);
    CPPUNIT_ASSERT(secondIsCalled);

    // Trying to register a further method with another signature raises an exception:
    auto third = [](int arg) {
    };
    CPPUNIT_ASSERT_THROW(instance->registerSlot<int>(third, "slot"), karabo::util::SignalSlotException);
    karabo::util::Exception::clearTrace();
}


void SignalSlotable_Test::testAsyncConnectInputChannel() {
    using karabo::util::Hash;

    // Setup sender
    auto sender = boost::make_shared<SignalSlotable>("sender");
    OutputChannel::Pointer outputChannel = sender->createOutputChannel("output", // default config
                                                                       Hash("output", karabo::util::Hash()));
    sender->start();

    // Setup receiver
    auto receiver = boost::make_shared<SignalSlotable>("receiver");
    karabo::util::Hash inputCfg("connectedOutputChannels", std::vector<std::string>(1, "sender:output"));
    InputChannel::Pointer inputChannel = receiver->createInputChannel("input", Hash("input", inputCfg));
    receiver->start();

    // Setup handler for asyncConnectInputChannel
    std::promise<bool> successPromise;
    std::promise<std::string> exceptMsgPromise;
    auto successFuture = successPromise.get_future();
    auto exceptMsgFuture = exceptMsgPromise.get_future();
    auto handler = [&successPromise, &exceptMsgPromise](bool success) {
        successPromise.set_value(success);
        if (success) {
            exceptMsgPromise.set_value(std::string());
        } else {
            try {
                throw;
            } catch (const std::exception& e) {
                exceptMsgPromise.set_value(e.what());
            }
        }
    };
    // First test: successful connection
    receiver->asyncConnectInputChannel(inputChannel, handler);
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, exceptMsgFuture.wait_for(std::chrono::milliseconds(500)));
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, successFuture.wait_for(std::chrono::milliseconds(500)));
    CPPUNIT_ASSERT(successFuture.get());
    CPPUNIT_ASSERT_EQUAL(std::string(), exceptMsgFuture.get());

    // Reset handler
    successPromise = std::promise<bool>();
    exceptMsgPromise = std::promise<std::string>();
    successFuture = successPromise.get_future();
    exceptMsgFuture = exceptMsgPromise.get_future();

    // Second test: one output is missing (but output instance exists), so we get failure
    CPPUNIT_ASSERT(receiver->removeInputChannel("input")); // first clear
    inputCfg.get<std::vector < std::string >> ("connectedOutputChannels").push_back("sender:not_an_output");
    inputChannel = receiver->createInputChannel("input", Hash("input", inputCfg));
    receiver->asyncConnectInputChannel(inputChannel, handler);
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, exceptMsgFuture.wait_for(std::chrono::milliseconds(500)));
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, successFuture.wait_for(std::chrono::milliseconds(500)));
    CPPUNIT_ASSERT(!successFuture.get());
    std::string fullExceptMsg(exceptMsgFuture.get());
    CPPUNIT_ASSERT(fullExceptMsg.find("SignalSlot Exception") != std::string::npos);
    CPPUNIT_ASSERT(fullExceptMsg.find("Failed to create 1 out of 2 connections of an InputChannel") != std::string::npos);

    // Reset handler again
    successPromise = std::promise<bool>();
    exceptMsgPromise = std::promise<std::string>();
    successFuture = successPromise.get_future();
    exceptMsgFuture = exceptMsgPromise.get_future();

    // Third test: one output is missing (because instance does not exist), so we get failure
    CPPUNIT_ASSERT(receiver->removeInputChannel("input")); // clear again
    inputCfg.get<std::vector < std::string >> ("connectedOutputChannels").back() = "not_a_sender:output";
    inputChannel = receiver->createInputChannel("input", Hash("input", inputCfg));
    receiver->asyncConnectInputChannel(inputChannel, handler);
    // Larger timeout here: In SignalSlotable::connectInputToOutputChannel it is 1000 ms to receive the reply from
    // slotGetOutputChannelInformation of the (in this case not existing) instance of the output
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, exceptMsgFuture.wait_for(std::chrono::milliseconds(1500)));
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, successFuture.wait_for(std::chrono::milliseconds(500)));
    CPPUNIT_ASSERT(!successFuture.get());
    fullExceptMsg = exceptMsgFuture.get();
    CPPUNIT_ASSERT(fullExceptMsg.find("SignalSlot Exception") != std::string::npos);
    CPPUNIT_ASSERT(fullExceptMsg.find("Failed to create 1 out of 2 connections of an InputChannel") != std::string::npos);

    // Reset handler once more
    successPromise = std::promise<bool>();
    exceptMsgPromise = std::promise<std::string>();
    successFuture = successPromise.get_future();
    exceptMsgFuture = exceptMsgPromise.get_future();

    // Forth test: no output configured at all which means success
    CPPUNIT_ASSERT(receiver->removeInputChannel("input")); // clear once more
    inputCfg.get<std::vector < std::string >> ("connectedOutputChannels").clear();
    inputChannel = receiver->createInputChannel("input", Hash("input", inputCfg));
    receiver->asyncConnectInputChannel(inputChannel, handler);
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, exceptMsgFuture.wait_for(std::chrono::milliseconds(500)));
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, successFuture.wait_for(std::chrono::milliseconds(500)));
    CPPUNIT_ASSERT(successFuture.get());
    CPPUNIT_ASSERT_EQUAL(std::string(), exceptMsgFuture.get());
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
