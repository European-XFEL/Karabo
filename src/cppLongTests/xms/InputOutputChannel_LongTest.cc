/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   InputOutputChannel_LongTest.cc
 * Author: gero.flucke@xfel.eu
 *
 * Created on May 2019
 */

#include "InputOutputChannel_LongTest.hh"

#include <chrono>
#include <future>

#include "karabo/net/EventLoop.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/util/Hash.hh"
#include "karabo/xms/InputChannel.hh"
#include "karabo/xms/OutputChannel.hh"

using namespace karabo;
using namespace std::chrono;
using util::Configurator;
using util::Hash;
using xms::InputChannel;
using xms::OutputChannel;

/// A class that adds threads following the RAII principle
/// to safely (exceptions!) remove them when going out of scope.
class ThreadAdder {
   public:
    ThreadAdder(int nThreads) : m_nThreads(nThreads) {
        karabo::net::EventLoop::addThread(m_nThreads);
    }

    ~ThreadAdder() {
        karabo::net::EventLoop::removeThread(m_nThreads);
    }

   private:
    const int m_nThreads;
};


CPPUNIT_TEST_SUITE_REGISTRATION(InputOutputChannel_LongTest);


InputOutputChannel_LongTest::InputOutputChannel_LongTest() {}


void InputOutputChannel_LongTest::setUp() {
    // Event loop started in xmsTestRunner.cc's main().
}


void InputOutputChannel_LongTest::tearDown() {}


// copy case - sender is irrelevant, so keep its defaults


void InputOutputChannel_LongTest::testDisconnectWhileSending1() {
    testDisconnectWhileSending_impl("copy", "wait", "wait");
}


void InputOutputChannel_LongTest::testDisconnectWhileSending2() {
    testDisconnectWhileSending_impl("copy", "drop", "wait");
}


void InputOutputChannel_LongTest::testDisconnectWhileSending3() {
    testDisconnectWhileSending_impl("copy", "queueDrop", "wait");
}


// load-balanced shared case - onSlowness of receiver is irrelevant, so keep default "wait"


void InputOutputChannel_LongTest::testDisconnectWhileSending4() {
    testDisconnectWhileSending_impl("shared", "wait", "wait");
}


void InputOutputChannel_LongTest::testDisconnectWhileSending5() {
    testDisconnectWhileSending_impl("shared", "wait", "drop");
}


void InputOutputChannel_LongTest::testDisconnectWhileSending6() {
    testDisconnectWhileSending_impl("shared", "wait", "queueDrop");
}


// round-robin shared case - onSlowness of receiver is irrelevant, so keep default "wait"


void InputOutputChannel_LongTest::testDisconnectWhileSending7() {
    testDisconnectWhileSending_impl("shared", "wait", "wait", true); // true -> round-robin
}


void InputOutputChannel_LongTest::testDisconnectWhileSending8() {
    testDisconnectWhileSending_impl("shared", "wait", "drop"); // true -> round-robin
}


void InputOutputChannel_LongTest::testDisconnectWhileSending9() {
    testDisconnectWhileSending_impl("shared", "wait", "queueDrop"); // true -> round-robin
}


void InputOutputChannel_LongTest::testDisconnectWhileSending_impl(const std::string& sender_dataDistribution,
                                                                  const std::string& sender_onSlowness,
                                                                  const std::string& receiver_noInputShared,
                                                                  bool registerRoundRobinSelector) {
    std::clog << "\ntestDisconnectWhileSending: sender " << sender_dataDistribution << "/" << sender_onSlowness
              << ", receiver (for shared sender) " << receiver_noInputShared
              << (registerRoundRobinSelector ? " (round-robin)" : "") << std::endl;

    const unsigned int numData = 100'000; // overall number of data items the output channel sends
    const int processTime = 1;            // ms that receiving input channel will block on data
    const int writeIntervall = 0;         // ms in between output channel writing data
    const int reconnectCycle = 4;         // ms between dis- and reconnect trials
    const int maxDuration = 300;          // s of maximum test duration

    ThreadAdder extraThreads(3); // 3 for sender, receiver's callback, dis-/reconnection

    // Setup output channel - configure only for "shared" InputChannel
    OutputChannel::Pointer output =
          Configurator<OutputChannel>::create("OutputChannel", Hash("noInputShared", receiver_noInputShared), 0);
    output->setInstanceIdAndName("outputChannel", "output");
    if (registerRoundRobinSelector) {
        // Force a round robin selection
        auto counter = std::make_shared<size_t>(0);
        auto selector = [counter{std::move(counter)}](const std::vector<std::string>& inputs) mutable {
            const size_t iSize = inputs.size();
            if (iSize > 0) {
                const size_t index = (*counter)++ % iSize;
                return inputs[index];
            } else {
                return std::string();
            }
        };
        output->registerSharedInputSelector(selector);
    }
    output->initialize(); // required due to additional '0' argument passed to create(..) above

    // Setup input channel
    const std::string outputChannelId(output->getInstanceId() + ":output");
    const Hash cfg("connectedOutputChannels", std::vector<std::string>(1, outputChannelId), "dataDistribution",
                   sender_dataDistribution, "onSlowness", sender_onSlowness);

    InputChannel::Pointer input = Configurator<InputChannel>::create("InputChannel", cfg);
    input->setInstanceId("inputChannel");
    auto calls = std::make_shared<int>(
          0); // shared_ptr to capture by value in lambda avoids trouble if handler called after test done
    input->registerDataHandler([calls, processTime](const Hash& data, const InputChannel::MetaData& meta) {
        ++(*calls);
        std::this_thread::sleep_for(milliseconds(processTime));
    });

    Hash outputInfo = output->getInformation();
    CPPUNIT_ASSERT_MESSAGE("OutputChannel keeps port 0!", outputInfo.get<unsigned int>("port") > 0);

    //
    // Connect
    //
    outputInfo.set("outputChannelString", outputChannelId);
    // Tests with "local" fail with chunk leaks, see OutputChannel::copyLocal and ::distributeLocal:
    outputInfo.set("memoryLocation", "remote");
    // Setup connection handler
    std::promise<karabo::net::ErrorCode> connectErrorCode;
    auto connectFuture = connectErrorCode.get_future();
    auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode& ec) { connectErrorCode.set_value(ec); };
    // initiate connect and block until done (fail test if timeout)
    karabo::net::ErrorCode ec;
    input->connect(outputInfo, connectHandler);
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, connectFuture.wait_for(std::chrono::milliseconds(5000)));
    ec = connectFuture.get();                                                 // Can get() only once...
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), karabo::net::ErrorCode(), ec); // i.e. no error

    //
    // Prepare handlers for sending data and in parallel dis-/reconnecting:
    //
    std::promise<unsigned int> numSent;
    auto numSentFuture = numSent.get_future();
    std::string exceptionText;
    bool doGoOn = true;
    auto send = [output, numData, &numSent, &exceptionText, &doGoOn, writeIntervall]() {
        unsigned int num = 0;
        while (num < numData && doGoOn) {
            try {
                output->write(Hash("a", 42));
            } catch (const std::exception& e) {
                (exceptionText += "output->write(..): ") += e.what();
                break;
            }
            const bool async = (num % 2 == 1);
            try {
                if (async) {
                    output->asyncUpdate();
                } else {
                    output->update();
                }
            } catch (const std::exception& e) {
                (exceptionText += (async ? "output->asyncUpdate(): " : "output->update(): ")) += e.what();
                break;
            }
            std::this_thread::sleep_for(milliseconds(writeIntervall));
            ++num;
        }
        numSent.set_value(num);
        doGoOn = false; // inform disReconnect to stop since test is done
    };

    std::string disreconnectFailure;
    std::promise<void> disReconnectDone;
    auto disReconnectFuture = disReconnectDone.get_future();
    unsigned int disconCounter = 0;
    auto disReconnect = [input, output, outputInfo, &disreconnectFailure, &disReconnectDone, &doGoOn, &disconCounter,
                         reconnectCycle]() {
        while (doGoOn) {
            input->disconnect(outputInfo.get<std::string>("outputChannelString"));
            ++disconCounter;
            std::promise<karabo::net::ErrorCode> connectErrorCode;
            auto connectFuture = connectErrorCode.get_future();
            auto connectHandler = [&connectErrorCode, &doGoOn](const karabo::net::ErrorCode& ec) {
                if (doGoOn) connectErrorCode.set_value(ec); // protect since connectErrorCode could be gone!
            };
            // initiate connect and block until done
            input->connect(outputInfo, connectHandler);
            int trials = 200;
            while (--trials >= 0 && doGoOn) {
                // NOTE:
                // After about 14100 to 14200 reconnection cycles, 'trials' every (second?) time goes down to exactly
                // 102 (with 10 ms sleep below) on my local tests - and then connect succeeds....
                // Maybe some tcp internal port cleaning done about every second only?
                if (std::future_status::timeout != connectFuture.wait_for(std::chrono::milliseconds(10))) {
                    break;
                }
            }
            if (!doGoOn) {
                break;
            }
            if (trials < 0) {
                disreconnectFailure = "Failed to reconnect within 2 s -- " + karabo::util::toString(disconCounter);
                doGoOn = false;
                break;
            }
            const karabo::net::ErrorCode ec = connectFuture.get(); // Can get only once...
            if (ec != karabo::net::ErrorCode()) {
                disreconnectFailure = "Failed reconnection: " + (ec.message() += " -- ") +=
                      karabo::util::toString(disconCounter);
                doGoOn = false;
                break;
            }
            std::this_thread::sleep_for(milliseconds(reconnectCycle));
        }
        disReconnectDone.set_value();
    };

    //
    // Actually start sending data and parallel disconnections:
    //
    karabo::net::EventLoop::getIOService().post(send);
    karabo::net::EventLoop::getIOService().post(disReconnect);

    //
    // Take care that both posted methods are done, so they cannot access local variables anymore
    // (worst case: doing that when the test method is left due to a failure)
    //
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, numSentFuture.wait_for(std::chrono::seconds(maxDuration)));
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, disReconnectFuture.wait_for(std::chrono::seconds(2)));


    const unsigned int totalSent = numSentFuture.get();
    std::clog << "DONE: output sent " << totalSent << " data items out of which only " << *calls << " reached input "
              << "since input disconnected " << disconCounter << " times" << std::endl;
    // Finally do all the necessary asserts:
    CPPUNIT_ASSERT_MESSAGE(exceptionText, exceptionText.empty());
    CPPUNIT_ASSERT_EQUAL(numData, totalSent);

    CPPUNIT_ASSERT_MESSAGE(disreconnectFailure, disreconnectFailure.empty());

    CPPUNIT_ASSERT(*calls > 0);
}
