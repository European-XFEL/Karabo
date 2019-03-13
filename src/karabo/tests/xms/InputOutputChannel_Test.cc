/* 
 * File:   InputOutputChannel_Test.cc
 * Author: flucke
 * 
 * Created on November 8, 2016, 3:54 PM
 */

#include <future>

#include "InputOutputChannel_Test.hh"

#include "karabo/xms/InputChannel.hh"
#include "karabo/xms/OutputChannel.hh"
#include "karabo/util/Configurator.hh"
#include "karabo/util/Hash.hh"
#include "karabo/net/EventLoop.hh"

using namespace karabo;
using xms::InputChannel;
using xms::OutputChannel;
using util::Configurator;
using util::Hash;

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


CPPUNIT_TEST_SUITE_REGISTRATION(InputOutputChannel_Test);


InputOutputChannel_Test::InputOutputChannel_Test() {
}


void InputOutputChannel_Test::setUp() {
    // Event loop started in xmsTestRunner.cc's main().
}


void InputOutputChannel_Test::tearDown() {
}


void InputOutputChannel_Test::testConnectDisconnect() {

    // Setup input channel
    InputChannel::Pointer input = Configurator<InputChannel>::create("InputChannel", Hash());
    input->setInstanceId("inputChannel");
    unsigned int calls = 0;
    input->registerDataHandler([&calls](const Hash& data, const InputChannel::MetaData & meta) {
        ++calls;
    });

    // Setup output channel
    OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel", Hash());
    output->setInstanceIdAndName("outputChannel", "output");
    std::vector<karabo::util::Hash> table;
    output->registerShowConnectionsHandler([&table](const std::vector<karabo::util::Hash>& connections) {
        table = connections;
    });

    // Write first data - nobody connected yet.
    output->write(Hash("key", 42));
    output->update();
    boost::this_thread::sleep(boost::posix_time::milliseconds(20)); // time for call back
    CPPUNIT_ASSERT_EQUAL(0u, calls);
    CPPUNIT_ASSERT_EQUAL(0uL, table.size());

    // Connect
    const std::string outputChannelId("outputChannelString");
    Hash outputInfo(output->getInformation());
    outputInfo.set("outputChannelString", outputChannelId);
    outputInfo.set("memoryLocation", "local");
    const size_t n = 50;
    for (size_t i = 0; i < n; ++i) {
        calls = 0;
        // Setup connection handler
        std::promise<karabo::net::ErrorCode> connectErrorCode;
        auto connectFuture = connectErrorCode.get_future();
        auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode & ec) {
            connectErrorCode.set_value(ec);
        };
        // initiate connect and block until done (fail test if timeout))
        input->connect(outputInfo, connectHandler);
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, connectFuture.wait_for(std::chrono::milliseconds(500)));
        CPPUNIT_ASSERT_EQUAL(connectFuture.get(), karabo::net::ErrorCode()); // i.e. no error

        // Now ensure that output channel took note of input registration:
        bool connected = false;
        int trials = 200;
        do {
            // By default, InputChannel is configured to receive a "copy" of all data and not to share with others
            if (output->hasRegisteredCopyInputChannel(input->getInstanceId())) {
              connected = true;
              break;
            }
            boost::this_thread::sleep(boost::posix_time::milliseconds(2));
        } while (--trials >= 0);
        CPPUNIT_ASSERT(connected);

        CPPUNIT_ASSERT_EQUAL(1UL, table.size());

        CPPUNIT_ASSERT_EQUAL(table[0].get<std::string>("remoteId"), input->getInstanceId());
        CPPUNIT_ASSERT_EQUAL(table[0].get<std::string>("dataDistribution"), std::string("copy"));
        CPPUNIT_ASSERT_EQUAL(table[0].get<std::string>("onSlowness"), std::string("wait"));
        CPPUNIT_ASSERT_EQUAL(table[0].get<std::string>("memoryLocation"), std::string("local"));

        // Write data again (twice in one go...) - now input is connected.
        output->write(Hash("key", 43));
        output->write(Hash("key", -43));
        output->update();

        trials = 200;
        while (--trials >= 0) {
            if (2u == calls) break;
            boost::this_thread::sleep(boost::posix_time::milliseconds(2)); // time for callback
        }
        CPPUNIT_ASSERT_EQUAL(2u, calls);

        // Disconnect
        input->disconnect(outputChannelId);
        // Some time to travel for message
        trials = 200;
        connected = true;
        do {
            if (!output->hasRegisteredCopyInputChannel(input->getInstanceId())) {
                connected = false;
                break;
            }
            boost::this_thread::sleep(boost::posix_time::milliseconds(2));
        } while (--trials >= 0);
        CPPUNIT_ASSERT(!connected);
        CPPUNIT_ASSERT_EQUAL(0uL, table.size());
    }

    // Write data again - input does not anymore receive data.
    output->write(Hash("key", 44));
    output->update();
    // Extended time for callback to be really sure nothing comes.
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    // still 2:
    CPPUNIT_ASSERT_EQUAL(2u, calls);

    // Now test connection attempts that fail
    std::vector<karabo::util::Hash> badOutputInfos;
    // Not supported protocol (only tcp works):
    badOutputInfos.push_back(outputInfo);
    badOutputInfos.back().set("connectionType", "udp");
    // Invalid port to connect to (client needs a specific one):
    badOutputInfos.push_back(outputInfo);
    badOutputInfos.back().set("port", 0u);
    // Non-existing host:
    badOutputInfos.push_back(outputInfo);
    badOutputInfos.back().set("hostname", "exflblablupp-not-there.desy.de");

    for (const Hash& badOutputInfo : badOutputInfos) {
        // Setup connection handler
        std::promise<karabo::net::ErrorCode> connectErrorCode;
        auto connectFuture = connectErrorCode.get_future();
        auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode & ec) {
            connectErrorCode.set_value(ec);
        };
        input->connect(badOutputInfo, connectHandler);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Connection handler not called in time for " + toString(badOutputInfo),
                                     std::future_status::ready, connectFuture.wait_for(std::chrono::milliseconds(1000)));
        CPPUNIT_ASSERT_MESSAGE("Connection did not fail for " + toString(badOutputInfo),
                               connectFuture.get() != karabo::net::ErrorCode()); // not all OK (do not care which problem)
    }
}


// copy case - sender is irrelevant, so keep its defaults
void InputOutputChannel_Test::testDisconnectWhileSending1() {
    testDisconnectWhileSending_impl("copy", "wait", "load-balanced", "wait");
}


void InputOutputChannel_Test::testDisconnectWhileSending2() {
    testDisconnectWhileSending_impl("copy", "drop", "load-balanced", "wait");
}


void InputOutputChannel_Test::testDisconnectWhileSending3() {
    testDisconnectWhileSending_impl("copy", "queue", "load-balanced", "wait");
}


// load-balanced shared case - onSlowness of receiver is irrelevant, so keep default "wait"
void InputOutputChannel_Test::testDisconnectWhileSending4() {
    testDisconnectWhileSending_impl("shared", "wait", "load-balanced", "wait");
}


void InputOutputChannel_Test::testDisconnectWhileSending5() {
    testDisconnectWhileSending_impl("shared", "wait", "load-balanced", "drop");
}


void InputOutputChannel_Test::testDisconnectWhileSending6() {
    testDisconnectWhileSending_impl("shared", "wait", "load-balanced", "queue");
}


// round-robin shared case - onSlowness of receiver is irrelevant, so keep default "wait"
void InputOutputChannel_Test::testDisconnectWhileSending7() {
    testDisconnectWhileSending_impl("shared", "wait", "round-robin", "wait");
}


void InputOutputChannel_Test::testDisconnectWhileSending8() {
    testDisconnectWhileSending_impl("shared", "wait", "round-robin", "drop");
}


void InputOutputChannel_Test::testDisconnectWhileSending9() {
    testDisconnectWhileSending_impl("shared", "wait", "round-robin", "queue");
}


void InputOutputChannel_Test::testDisconnectWhileSending_impl(const std::string& sender_dataDistribution,
                                                              const std::string& sender_onSlowness,
                                                              const std::string& receiver_distributionMode,
                                                              const std::string& receiver_noInputShared) {
    std::clog << "\ntestDisconnectWhileSending: sender " << sender_dataDistribution << "/" << sender_onSlowness
            << ", receiver (for shared sender) "
            << receiver_distributionMode << "/" << receiver_noInputShared << std::endl; // FIXME: covert to flush;

    const unsigned int numData = 100000; // overall number of data items the output channel sends
    const int processTime = 5; // ms that receiving input channel will block on data
    const int writeIntervall = 1; // ms in between output channel writing data
    const int reconnectCycle = 2; // ms between dis- and reconnect trials
    const int maxDuration = 600; // s of maximum test duration

    ThreadAdder extraThreads(3); // 3 for sender, receiver's callback, dis-/reconnection

    // Setup input channel
    InputChannel::Pointer input = Configurator<InputChannel>::create("InputChannel",
                                                                     Hash("dataDistribution", sender_dataDistribution,
                                                                          "onSlowness", sender_onSlowness));
    input->setInstanceId("inputChannel");
    unsigned int calls = 0;
    input->registerDataHandler([&calls, processTime](const Hash& data, const InputChannel::MetaData & meta) {
        ++calls;
                               boost::this_thread::sleep(boost::posix_time::milliseconds(processTime));
    });

    // Setup output channel - configure only for "shared" InputChannel
    OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel",
                                                                        Hash("distributionMode", receiver_distributionMode,
                                                                             "noInputShared", receiver_noInputShared));
    output->setInstanceIdAndName("outputChannel", "output");
    // Wait a little bit until OutputChannel has properly initialised, i.e. a proper port is attached
    // (see its constructor...) FIXME :-(
    Hash outputInfo;
    int trials = 500;
    while (--trials >= 0) {
        outputInfo = output->getInformation();
        if (outputInfo.get<unsigned int>("port") > 0) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(20));
    }
    CPPUNIT_ASSERT_MESSAGE("OutputChannel keeps port 0!", outputInfo.get<unsigned int>("port") > 0);

    //
    // Connect
    //
    const std::string outputChannelId("outputChannelString");
    //Hash outputInfo(output->getInformation());
    outputInfo.set("outputChannelString", outputChannelId);
    outputInfo.set("memoryLocation", "local");
    // Setup connection handler
    std::promise<karabo::net::ErrorCode> connectErrorCode;
    auto connectFuture = connectErrorCode.get_future();
    auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode& ec) {
        connectErrorCode.set_value(ec);
    };
    // initiate connect and block until done (fail test if timeout)
    //    int trials = 10;
    karabo::net::ErrorCode ec;
    //    while (--trials >= 0) {
        input->connect(outputInfo, connectHandler);
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, connectFuture.wait_for(std::chrono::milliseconds(1000)));
        ec = connectFuture.get(); // Can get() only once...
    //        if (!ec) break;
    //        std::clog << "Another try to connect " << trials << " " << outputInfo << std::endl;
    //        connectErrorCode = std::promise<karabo::net::ErrorCode>();
    //        connectFuture = connectErrorCode.get_future();
    //    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), karabo::net::ErrorCode(), ec); // i.e. no error

    //
    // Prepare handlers for sending data and in parallel dis-/reconnecting:
    //
    std::promise<unsigned int> numSent;
    auto numSentFuture = numSent.get_future();
    std::string exceptionText;
    std::atomic<bool> doGoOn(true); // atomic really needed ? Had a crash which might point to that...
    auto send = [output, numData, &numSent, &exceptionText, &doGoOn, writeIntervall]() {
        unsigned int num = 0;
        while (++num <= numData && doGoOn) {
            try {
                output->write(Hash("a", 42));
            } catch (const std::exception& e) {
                (exceptionText += "output->write(..): ") += e.what();
                num += 1; // since below we subtract 1 from while expression getting false...
                break;
            }
            try {
                output->update();
            } catch (const std::exception& e) {
                (exceptionText += "output->update(): ") += e.what();
                num += 1; // since below we subtract 1 from while expression getting false...
                break;
            }
            if (num % 2000 == 0) std::clog << "updated successfully " << num << std::endl;
            boost::this_thread::sleep(boost::posix_time::milliseconds(writeIntervall));
        }
        numSent.set_value(num - 1); // subtract the final ++ when the while got false
        doGoOn = false; // inform disReconnect to stop since test is done
    };

    std::string disreconnectFailure;
    auto disReconnect = [input, output, outputInfo, &disreconnectFailure, &doGoOn, reconnectCycle]() {
        unsigned int counter = 0;
        while (doGoOn) {
            input->disconnect(outputInfo.get<std::string>("outputChannelString"));

            boost::this_thread::sleep(boost::posix_time::milliseconds(reconnectCycle));
            std::promise<karabo::net::ErrorCode> connectErrorCode;
            auto connectFuture = connectErrorCode.get_future();
            auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode & ec) {
                connectErrorCode.set_value(ec);
            };
            if ((++counter) % 1000 == 0) std::clog << "Disconnected successfully " << counter << std::endl;
            // initiate connect and block until done
            input->connect(outputInfo, connectHandler);
            if (std::future_status::ready != connectFuture.wait_for(std::chrono::seconds(1000))) {
                disreconnectFailure = "Failed to reconnect within 1 s -- " + karabo::util::toString(counter);
                doGoOn = false;
                break;
            }
            const karabo::net::ErrorCode ec = connectFuture.get(); // Can get only once...
            if (ec != karabo::net::ErrorCode()) {
                disreconnectFailure = "Failed reconnection: " + (ec.message() += " -- ") += karabo::util::toString(counter);
                doGoOn = false;
                break;
            }
            boost::this_thread::sleep(boost::posix_time::milliseconds(reconnectCycle));
        }
    };

    //
    // Actually start sending data and parallel disconnections:
    //
    karabo::net::EventLoop::getIOService().post(send);
    karabo::net::EventLoop::getIOService().post(disReconnect);

    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, numSentFuture.wait_for(std::chrono::seconds(maxDuration)));
    std::clog << "Managed " << calls << " calls!" << std::endl;
    if (!exceptionText.empty()) { // FIXME remove
        std::clog << exceptionText << std::endl;
    }
    CPPUNIT_ASSERT_MESSAGE(exceptionText, exceptionText.empty());
    CPPUNIT_ASSERT_EQUAL(numSentFuture.get(), numData);

    if (!exceptionText.empty()) { // FIXME remove
        std::clog << disreconnectFailure << std::endl;
    }
    CPPUNIT_ASSERT_MESSAGE(disreconnectFailure, disreconnectFailure.empty());

    CPPUNIT_ASSERT(calls > 0);

    std::clog << "testDisconnectWhileSending " << calls << " calls -- OK!" << std::endl; // FIXME: remove testDisconnectWhileSending
    // boost::this_thread::sleep(boost::posix_time::milliseconds(500)); // time for clean-up???????????
}