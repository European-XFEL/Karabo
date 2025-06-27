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
 * File:   InputOutputChannel_Test.cc
 * Author: flucke
 *
 * Created on November 8, 2016, 3:54 PM
 */

#include "InputOutputChannel_Test.hh"

#include <ifaddrs.h>
#include <sys/types.h>

#include <chrono>
#include <future>
#include <mutex>
#include <regex>
#include <thread>

#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/VectorElement.hh"
#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/NDArray.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/net/utils.hh"
#include "karabo/xms/InputChannel.hh"
#include "karabo/xms/OutputChannel.hh"

using namespace std::chrono;
using namespace std::literals::chrono_literals;
using namespace std::string_literals; // for '"abc"s'

using namespace karabo;
using data::Configurator;
using data::Epochstamp;
using data::Hash;
using data::INT32_ELEMENT;
using data::NDArray;
using data::Schema;
using data::STRING_ELEMENT;
using data::VECTOR_INT32_ELEMENT;
using xms::InputChannel;
using xms::OUTPUT_CHANNEL_ELEMENT;
using xms::OutputChannel;

// Return a vector with valid address names that can be used to create
// a NetworkInterface:
//
//   - One of the host IPs that is not a loopback address
//   - One host interface (for instance, eth0)
//   - One address range
//
std::vector<std::string> createTestAddresses() {
    struct ifaddrs* ifap{};
    if (getifaddrs(&ifap) == -1) {
        return {};
    }

    char presentation_ip[INET6_ADDRSTRLEN];
    for (auto ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        inet_ntop(AF_INET, &(reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr)->sin_addr.s_addr), presentation_ip,
                  INET6_ADDRSTRLEN);

        if (std::string{presentation_ip}.find("127.") == 0) {
            continue;
        } else {
            std::string host_ip{presentation_ip};
            size_t dot = host_ip.rfind('.');
            freeifaddrs(ifap);
            return {host_ip, ifa->ifa_name, host_ip.substr(0, dot) + ".0/24"};
        }
    }

    freeifaddrs(ifap);
    return {};
}

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

const int connectTimeoutMs = 10000; // once saw CI failure with 5000!

CPPUNIT_TEST_SUITE_REGISTRATION(InputOutputChannel_Test);

bool InputOutputChannel_Test::m_calledTestAsyncUpdate = false;

InputOutputChannel_Test::InputOutputChannel_Test() {
    if (!m_calledTestAsyncUpdate) {
        m_calledTestAsyncUpdate = true;
        std::clog << " Settings given for InputOutputChannel_Test::testAsyncUpdate<NxM> are: "
                  << "onSlowness, dataDistribution, memoryLocation, safeNDArray:" << std::endl;
    }
}


void InputOutputChannel_Test::setUp() {
    // Event loop started in testRunner.cc's main().
}


void InputOutputChannel_Test::tearDown() {}


void InputOutputChannel_Test::testOutputChannelElement() {
    Schema pipeSchema;
    INT32_ELEMENT(pipeSchema).key("int32").readOnly().commit();

    Schema s;
    CPPUNIT_ASSERT_NO_THROW(
          OUTPUT_CHANNEL_ELEMENT(s).key("validkey").displayedName("Valid output").dataSchema(pipeSchema).commit());
    CPPUNIT_ASSERT(s.has("validkey.schema.int32"));
    CPPUNIT_ASSERT_EQUAL(std::string("OutputSchema"), s.getDisplayType("validkey.schema"));

    // The deviceId/channel delimiters ':' and (for backward compatibility) '@' are not allowed in keys.
    CPPUNIT_ASSERT_THROW(OUTPUT_CHANNEL_ELEMENT(s).key("invalid:key"), karabo::data::ParameterException);
    CPPUNIT_ASSERT_THROW(OUTPUT_CHANNEL_ELEMENT(s).key("invalid@key2"), karabo::data::ParameterException);
}


void InputOutputChannel_Test::testManyToOne() {
    // To switch on logging output for debugging, do e.g. the following:
    //    karabo::log::Logger::configure(Hash("priority", "DEBUG",
    //                                        // enable timestamps, with ms precision
    //                                        "ostream.pattern", "%d{%F %H:%M:%S,%l} %p  %c  : %m%n"));
    //    karabo::log::Logger::useConsole();

    const unsigned int numOutputs = 6;
    ThreadAdder extraThreads(numOutputs);

    std::array<OutputChannel::Pointer, numOutputs> outputs;
    std::vector<std::string> outputIds(outputs.size());
    for (size_t i = 0; i < outputs.size(); ++i) {
        const std::string channelId("output" + karabo::data::toString(i));
        outputs[i] = Configurator<OutputChannel>::create("OutputChannel", Hash(), 0);
        outputs[i]->setInstanceIdAndName("outputChannel", channelId);
        outputs[i]->initialize(); // needed due to additional int == 0 argument above
        outputIds[i] = outputs[i]->getInstanceId() + ":" + channelId;
    }

    // Setup input channel
    const Hash cfg("connectedOutputChannels", outputIds, "onSlowness", "wait");
    InputChannel::Pointer input = Configurator<InputChannel>::create("InputChannel", cfg);
    input->setInstanceId("inputChannel");

    // Prepare and register data handler
    Hash receivedData;
    for (size_t i = 0; i < outputIds.size(); ++i) {
        // Already add all entries in the map behind the Hash receivedData - so parallel access to items is thread safe
        receivedData.set(outputIds[i], std::vector<unsigned int>());
    }
    input->registerDataHandler([&receivedData](const Hash& data, const InputChannel::MetaData& meta) {
        const std::string& sourceName = meta.getSource();
        receivedData.get<std::vector<unsigned int>>(sourceName).push_back(data.get<unsigned int>("uint"));
    });

    // Handler to count endOfStream events
    std::atomic<int> nReceivedEos(0);
    input->registerEndOfStreamEventHandler([&nReceivedEos](const InputChannel::Pointer&) { ++nReceivedEos; });

    for (size_t i = 0; i < outputs.size(); ++i) {
        // Connect
        Hash outputInfo(outputs[i]->getInformation());
        CPPUNIT_ASSERT_GREATER(0u, outputInfo.get<unsigned int>("port"));
        outputInfo.set("outputChannelString", outputIds[i]);
        // Alternate scenarios to test both memory location code paths:
        outputInfo.set("memoryLocation",
                       (i % 2 == 0          // alternate between...
                              ? "local"     // - using inner-process data shortcut via static Memory class
                              : "remote")); // - sending data via Tcp (buggy till 2.9.X for many-to-one)

        // Setup connection handler
        std::promise<karabo::net::ErrorCode> connectErrorCode;
        auto connectFuture = connectErrorCode.get_future();
        auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode& ec) { connectErrorCode.set_value(ec); };
        // Initiate connect and block until done - fail test if timeout.
        // Being more clever and waiting only once for all connections in one go is not worth it in the test here.
        input->connect(outputInfo, connectHandler);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("attempt for " + outputIds[i], std::future_status::ready,
                                     connectFuture.wait_for(milliseconds(connectTimeoutMs)));
        CPPUNIT_ASSERT_EQUAL_MESSAGE("attempt for " + outputIds[i],
                                     karabo::net::ErrorCode(), // i.e. no error
                                     connectFuture.get());

        // All up to the last one are connected now
        auto connectStatusMap(input->getConnectionStatus());
        CPPUNIT_ASSERT_EQUAL(outputs.size(), connectStatusMap.size());
        for (size_t j = 0; j < outputs.size(); ++j) {
            CPPUNIT_ASSERT(connectStatusMap.find(outputIds[j]) != connectStatusMap.end());
            namespace kd = karabo::data;
            namespace kn = karabo::net;
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                  "Tested j = " + kd::toString(j) += ", connected i = " + kd::toString(i),
                  (j <= i ? kn::ConnectionStatus::CONNECTED : kn::ConnectionStatus::DISCONNECTED),
                  connectStatusMap[outputIds[j]]);
        }
    } // all connected

    // Did the output channels already take note of the connection, i.e. received the 'hello' message?
    // As long as not, output->update() in 'sending' function below will actually not send!
    // Instead of this check here, we could introduce a "hello-back" message from the output channel to the input
    // channel and fire the 'connected' handler of InputChannel::connect only when this is received.
    // But would require a protocol extension, i.e. change in all APIs - without real use in the field...
    for (size_t i = 0; i < outputs.size(); ++i) {
        int trials = 1000;
        bool registered = false;
        while (--trials >= 0) {
            registered = outputs[i]->hasRegisteredCopyInputChannel(input->getInstanceId());
            if (registered) break;
            // Happens very rarely - seen 6 times in 20,000 local test runs.
            std::this_thread::sleep_for(1ms);
        }
        CPPUNIT_ASSERT_MESSAGE("Not yet ready: output " + karabo::data::toString(i), registered);
    }

    // Prepare lambda to send data
    const size_t numData = 200;
    std::function<void(unsigned int)> sending = [&outputs, numData](unsigned int outNum) {
        for (unsigned int i = 0; i < numData; ++i) {
            outputs[outNum]->write(Hash("uint", i));
            outputs[outNum]->update();
        }
        outputs[outNum]->signalEndOfStream();
    };

    // Start to send data from all outputs in parallel (we added enough threads in the beginning!).
    for (unsigned int i = 0; i < numOutputs; ++i) {
        boost::asio::post(karabo::net::EventLoop::getIOService(), std::bind(sending, i));
    }

    // Wait for endOfStream arrival
    int trials = 3000;
    do {
        std::this_thread::sleep_for(3ms);
        if (nReceivedEos > 0) break;
    } while (--trials >= 0);

    // endOfStream received once
    // We give some time for more to arrive - but there should only be one, although each output sent it!
    std::this_thread::sleep_for(200ms);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Data received:\n" + karabo::data::toString(receivedData), 1u,
                                 static_cast<unsigned int>(nReceivedEos));

    // Proper number and order of data received from each output
    for (size_t i = 0; i < outputIds.size(); ++i) {
        const auto& data = receivedData.get<std::vector<unsigned int>>(outputIds[i]);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(
              outputIds[i] + " lacks data, all received:\n" + karabo::data::toString(receivedData), numData,
              data.size());
        for (unsigned int iData = 0; iData < data.size(); ++iData) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(
                  "Output " + karabo::data::toString(i) += ", data " + karabo::data::toString(iData), iData,
                  data[iData]);
        }
    }
}

void InputOutputChannel_Test::testConnectDisconnect() {
    // To switch on logging output for debugging, do e.g. the following:
    //    karabo::log::Logger::configure(Hash("priority", "DEBUG",
    //                                        // enable timestamps, with ms precision
    //                                        "ostream.pattern", "%d{%F %H:%M:%S,%l} %p  %c  : %m%n"));
    //    karabo::log::Logger::useConsole();

    // Setup output channel
    OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel", Hash(), 0);
    output->setInstanceIdAndName("outputChannel", "output");
    output->initialize(); // needed due to int == 0 argument above

    std::vector<karabo::data::Hash> table;
    std::mutex handlerDataMutex;
    output->registerShowConnectionsHandler(
          [&table, &handlerDataMutex](const std::vector<karabo::data::Hash>& connections) {
              std::lock_guard lock(handlerDataMutex);
              table = connections;
          });

    // Setup input channel
    const std::string outputChannelId(output->getInstanceId() + ":output");
    const Hash cfg("connectedOutputChannels", std::vector<std::string>(1, outputChannelId));
    InputChannel::Pointer input = Configurator<InputChannel>::create("InputChannel", cfg);
    input->setInstanceId("inputChannel");
    unsigned int calls = 0;
    input->registerDataHandler([&calls](const Hash& data, const InputChannel::MetaData& meta) { ++calls; });
    std::vector<karabo::net::ConnectionStatus> trackedStatus;
    input->registerConnectionTracker([&trackedStatus, &handlerDataMutex, outputChannelId](
                                           const std::string& outputId, karabo::net::ConnectionStatus status) {
        if (outputId == outputChannelId) {
            std::lock_guard lock(handlerDataMutex);
            trackedStatus.push_back(status);
        }
    });

    // Write first data - nobody connected yet.
    output->write(Hash("key", 42));
    output->update();
    std::this_thread::sleep_for(20ms); // time for call back
    CPPUNIT_ASSERT_EQUAL(0u, calls);
    {
        std::lock_guard lock(handlerDataMutex);
        CPPUNIT_ASSERT_EQUAL(0uL, table.size());
    }

    // Connect
    Hash outputInfo(output->getInformation());
    CPPUNIT_ASSERT_GREATER(0u, outputInfo.get<unsigned int>("port"));
    outputInfo.set("outputChannelString", outputChannelId);
    outputInfo.set("memoryLocation", "local");
    const size_t n = 50;
    for (size_t i = 0; i < n; ++i) {
        trackedStatus.clear();
        calls = 0;
        // Setup connection handler
        std::promise<karabo::net::ErrorCode> connectErrorCode;
        auto connectFuture = connectErrorCode.get_future();
        auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode& ec) { connectErrorCode.set_value(ec); };
        // Not connected yet
        auto connectStatusMap(input->getConnectionStatus());
        CPPUNIT_ASSERT_EQUAL(1ul, connectStatusMap.size());
        CPPUNIT_ASSERT_EQUAL(outputChannelId, connectStatusMap.begin()->first);
        CPPUNIT_ASSERT_MESSAGE(karabo::data::toString(static_cast<int>(connectStatusMap[outputChannelId])),
                               connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::DISCONNECTED);

        // initiate connect and block until done (fail test if timeout))
        input->connect(outputInfo, connectHandler);

        // Now connecting or - with very weird threading - already connected
        connectStatusMap = input->getConnectionStatus();
        CPPUNIT_ASSERT_EQUAL(1ul, connectStatusMap.size());
        CPPUNIT_ASSERT_EQUAL(outputChannelId, connectStatusMap.begin()->first);
        CPPUNIT_ASSERT_MESSAGE(karabo::data::toString(static_cast<int>(connectStatusMap[outputChannelId])),
                               connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::CONNECTING ||
                                     connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::CONNECTED);

        CPPUNIT_ASSERT_EQUAL_MESSAGE("attempt number " + karabo::data::toString(i), std::future_status::ready,
                                     connectFuture.wait_for(milliseconds(connectTimeoutMs)));
        CPPUNIT_ASSERT_EQUAL_MESSAGE("attempt number " + karabo::data::toString(i), connectFuture.get(),
                                     karabo::net::ErrorCode()); // i.e. no error

        // We are connected - check that the status tracker received all steps
        // (rely on order of calls to connection tracker (first) and handler (second) at the end of
        // InputChannel::onConnect
        CPPUNIT_ASSERT(trackedStatus.size() > 0ul);
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(karabo::net::ConnectionStatus::CONNECTING),
                             static_cast<int>(trackedStatus[0]));
        // Without waiting for tracker really being called, this test relies on order of calls to connection
        // tracker and handler given to InputChannel::connect(..) (although might succeed most times even otherwise)
        CPPUNIT_ASSERT(trackedStatus.size() > 1ul);
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(karabo::net::ConnectionStatus::CONNECTED),
                             static_cast<int>(trackedStatus[1]));
        CPPUNIT_ASSERT_EQUAL(2ul, trackedStatus.size()); // i.e. nothing else (yet)!

        // Now ensure that output channel took note of input registration:
        int trials = 200;
        do {
            std::this_thread::sleep_for(2ms);
            std::lock_guard lock(handlerDataMutex);
            if (!table.empty()) {
                break;
            }
        } while (--trials >= 0);
        // No further call back, so no need to lock tableMutex here
        CPPUNIT_ASSERT_EQUAL(1UL, table.size());
        // ... and check the published connection information
        CPPUNIT_ASSERT_EQUAL(table[0].get<std::string>("remoteId"), input->getInstanceId());
        CPPUNIT_ASSERT_EQUAL(table[0].get<std::string>("dataDistribution"), std::string("copy"));
        CPPUNIT_ASSERT_EQUAL(table[0].get<std::string>("onSlowness"), std::string("drop"));
        CPPUNIT_ASSERT_EQUAL(table[0].get<std::string>("memoryLocation"), std::string("local"));

        // Now we are indeed connected:
        connectStatusMap = input->getConnectionStatus();
        CPPUNIT_ASSERT_EQUAL(1ul, connectStatusMap.size());
        CPPUNIT_ASSERT_EQUAL(outputChannelId, connectStatusMap.begin()->first);
        CPPUNIT_ASSERT_MESSAGE(karabo::data::toString(static_cast<int>(connectStatusMap[outputChannelId])),
                               connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::CONNECTED);

        // Write data again (twice in one go...) - now input is connected.
        output->write(Hash("key", 43));
        output->write(Hash("key", -43));
        output->update();

        trials = 200;
        while (--trials >= 0) {
            if (2u == calls) break;
            std::this_thread::sleep_for(2ms); // time for callback
        }
        CPPUNIT_ASSERT_EQUAL(2u, calls);

        // Disconnect
        input->disconnect(outputChannelId);
        connectStatusMap = input->getConnectionStatus();
        CPPUNIT_ASSERT_EQUAL(1ul, connectStatusMap.size());
        CPPUNIT_ASSERT_EQUAL(outputChannelId, connectStatusMap.begin()->first);
        CPPUNIT_ASSERT_MESSAGE(karabo::data::toString(static_cast<int>(connectStatusMap[outputChannelId])),
                               connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::DISCONNECTED);

        // Some time to travel for message
        trials = 1000; // failed with 200 in https://git.xfel.eu/Karabo/Framework/-/jobs/131075/raw
        do {
            std::this_thread::sleep_for(2ms);
            std::lock_guard lock(handlerDataMutex);
            if (table.empty() && trackedStatus.size() > 2ul) {
                break;
            }
        } while (--trials >= 0);
        CPPUNIT_ASSERT_EQUAL(0uL, table.size());
        // Also the tracker got informed about disconnection:
        CPPUNIT_ASSERT(trackedStatus.size() > 2ul);
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(karabo::net::ConnectionStatus::DISCONNECTED),
                             static_cast<int>(trackedStatus[2]));
        CPPUNIT_ASSERT_EQUAL(3ul, trackedStatus.size()); // i.e. nothing else!
    }

    // Write data again - input does not anymore receive data.
    output->write(Hash("key", 44));
    output->update();
    // Extended time for callback to be really sure nothing comes.
    std::this_thread::sleep_for(100ms);
    // still 2:
    CPPUNIT_ASSERT_EQUAL(2u, calls);

    //
    // Now test connection attempts that fail
    //
    std::vector<karabo::data::Hash> badOutputInfos;
    // Not supported protocol (only tcp works):
    badOutputInfos.push_back(outputInfo);
    badOutputInfos.back().set("connectionType", "udp");
    // Invalid port to connect to (client needs a specific one):
    badOutputInfos.push_back(outputInfo);
    badOutputInfos.back().set("port", 0u);
    // Non-existing host:
    badOutputInfos.push_back(outputInfo);
    badOutputInfos.back().set("hostname", "exflblablupp-not-there.desy.de");
    // Non-configured output channel:
    badOutputInfos.push_back(outputInfo);
    badOutputInfos.back().set("outputChannelString", "not_configured");
    // Missing info about memoryLocation:
    badOutputInfos.push_back(outputInfo);
    badOutputInfos.back().erase("memoryLocation");

    for (const Hash& badOutputInfo : badOutputInfos) {
        // Setup connection handler
        std::promise<karabo::net::ErrorCode> connectErrorCode;
        auto connectFuture = connectErrorCode.get_future();
        auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode& ec) { connectErrorCode.set_value(ec); };
        input->connect(badOutputInfo, connectHandler);
        // See failure in https: // git.xfel.eu/Karabo/Framework/-/jobs/290206 with 5000 ms wait_for
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Connection handler not called in time for " + toString(badOutputInfo),
                                     std::future_status::ready, connectFuture.wait_for(milliseconds(connectTimeoutMs)));
        CPPUNIT_ASSERT_MESSAGE(
              "Connection did not fail for " + toString(badOutputInfo),
              connectFuture.get() != karabo::net::ErrorCode()); // not all OK (do not care which problem)
    }
}

void InputOutputChannel_Test::testConcurrentConnect() {
    // To switch on logging output for debugging, do e.g. the following:
    //    karabo::log::Logger::configure(Hash("priority", "DEBUG",
    //                                        // enable timestamps, with ms precision
    //                                        "ostream.pattern", "%d{%F %H:%M:%S,%l} %p  %c  : %m%n"));
    //    karabo::log::Logger::useConsole();

    int counter = 100;
    while (--counter >= 0) { // repeat test since it depends on timing!
        // Setup output channel
        OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel", Hash(), 0);
        output->setInstanceIdAndName("outputChannel", "output");
        output->initialize(); // needed due to int == 0 argument above

        // Setup input channel
        const std::string outputChannelId(output->getInstanceId() + ":output");
        const Hash cfg("connectedOutputChannels", std::vector<std::string>(1, outputChannelId));
        InputChannel::Pointer input = Configurator<InputChannel>::create("InputChannel", cfg);
        input->setInstanceId("inputChannel");

        Hash outputInfo(output->getInformation());
        CPPUNIT_ASSERT_MESSAGE("OutputChannel keeps port 0!", outputInfo.get<unsigned int>("port") > 0);

        outputInfo.set("outputChannelString", outputChannelId);
        outputInfo.set("memoryLocation", "local");

        // Setup connection handlers
        std::promise<karabo::net::ErrorCode> connectPromise1;
        std::future<karabo::net::ErrorCode> connectFuture1 = connectPromise1.get_future();
        std::function<void(const karabo::net::ErrorCode&)> connectHandler1 =
              [&connectPromise1](const karabo::net::ErrorCode& ec) { connectPromise1.set_value(ec); };
        std::promise<karabo::net::ErrorCode> connectPromise2;
        std::future<karabo::net::ErrorCode> connectFuture2 = connectPromise2.get_future();
        std::function<void(const karabo::net::ErrorCode&)> connectHandler2 =
              [&connectPromise2](const karabo::net::ErrorCode& ec) { connectPromise2.set_value(ec); };
        // Subsequent connect(..): first succeeds, second fails since already connected (less likely) or connecting
        input->connect(outputInfo, connectHandler1);
        input->connect(outputInfo, connectHandler2);
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, connectFuture1.wait_for(milliseconds(connectTimeoutMs)));
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, connectFuture2.wait_for(milliseconds(connectTimeoutMs)));

        CPPUNIT_ASSERT_EQUAL(karabo::net::ErrorCode(), connectFuture1.get());
        const karabo::net::ErrorCode ec = connectFuture2.get();
        namespace bse = boost::system::errc;
        CPPUNIT_ASSERT_MESSAGE(karabo::data::toString(ec),
                               ec == bse::make_error_code(bse::connection_already_in_progress) ||
                                     ec == bse::make_error_code(bse::already_connected));

        input->disconnect(outputInfo);

        // Ensure it is disconnected
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(karabo::net::ConnectionStatus::DISCONNECTED),
                             static_cast<int>(input->getConnectionStatus()[outputChannelId]));
        //
        // Now second scenario: disconnect in between two connect attempts:
        //
        // Setup more connection handlers
        std::promise<karabo::net::ErrorCode> connectPromise3;
        std::future<karabo::net::ErrorCode> connectFuture3 = connectPromise3.get_future();
        std::function<void(const karabo::net::ErrorCode&)> connectHandler3 =
              [&connectPromise3](const karabo::net::ErrorCode& ec) { connectPromise3.set_value(ec); };
        std::promise<karabo::net::ErrorCode> connectPromise4;
        std::future<karabo::net::ErrorCode> connectFuture4 = connectPromise4.get_future();
        std::function<void(const karabo::net::ErrorCode&)> connectHandler4 =
              [&connectPromise4](const karabo::net::ErrorCode& ec) { connectPromise4.set_value(ec); };

        input->connect(outputInfo, connectHandler3);
        input->disconnect(outputInfo);
        input->connect(outputInfo, connectHandler4);

        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, connectFuture3.wait_for(milliseconds(connectTimeoutMs)));
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, connectFuture4.wait_for(milliseconds(connectTimeoutMs)));

        // Now it is not exactly clear what to expect - depends on timing of threads:
        // - 1st fails as operation_canceled, 2nd succeeds, i.e. disconnect(..) clears from "being setup"
        // - 1st succeeds and 2nd succeeds, i.e. disconnect(..) got called (and fully succeeded!) when 1st connect(..)
        // already succeeded
        const karabo::net::ErrorCode ec1 = connectFuture3.get();
        const karabo::net::ErrorCode ec2 = connectFuture4.get();

        CPPUNIT_ASSERT_MESSAGE(
              "1: " + karabo::data::toString(ec1) += ", 2: " + karabo::data::toString(ec2),
              (ec1 == bse::make_error_code(bse::operation_canceled) && ec2 == karabo::net::ErrorCode()) ||
                    (ec1 == karabo::net::ErrorCode() && ec2 == karabo::net::ErrorCode()));
    }
}

void InputOutputChannel_Test::testInputHandler() {
    // Setup output channel
    OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel", Hash(), 0);
    output->setInstanceIdAndName("outputChannel", "output");
    output->initialize(); // needed due to int == 0 argument above

    // Setup input channel
    const std::string outputChannelId(output->getInstanceId() + ":output");
    const Hash cfg("connectedOutputChannels", std::vector<std::string>(1, outputChannelId));
    InputChannel::Pointer input = Configurator<InputChannel>::create("InputChannel", cfg);
    input->setInstanceId("inputChannel");
    Hash::Pointer hashRead;
    input->registerInputHandler([&hashRead](const InputChannel::Pointer& inputPtr) { hashRead = inputPtr->read(0); });

    // Connect - since handler passed to 'connect' fires already before output proessed "hello" message,
    //           we directly wait until we know that OutputChannel has us registered
    Hash outputInfo(output->getInformation());
    outputInfo.set("outputChannelString", outputChannelId);
    outputInfo.set("memoryLocation", "local");
    input->connect(outputInfo); // connectHandler);
    int timeout = 1000;
    while (timeout > 0) {
        if (output->hasRegisteredCopyInputChannel("inputChannel")) break;
        timeout -= 2;
        std::this_thread::sleep_for(2ms);
    }
    CPPUNIT_ASSERT_GREATEREQUAL(0, timeout);

    // Send data
    output->write(Hash("data", 42));
    output->asyncUpdate();

    // Wait until inputHandler got the data and stored it - take care that the expected data is in it
    timeout = 1000;
    while (timeout > 0) {
        if (hashRead) break;
        timeout -= 2;
        std::this_thread::sleep_for(2ms);
    }
    CPPUNIT_ASSERT(hashRead->has("data"));
    CPPUNIT_ASSERT_EQUAL(42, hashRead->get<int>("data"));

    // Hijack test to check sending an empty NDArray (caused serialisation trouble in the past)
    hashRead.reset();
    const short noData[] = {};
    output->write(Hash("emptyArray", NDArray(noData, sizeof(noData) / sizeof(noData[0]))));
    output->asyncUpdate();

    timeout = 1000;
    while (timeout > 0) {
        if (hashRead) break;
        timeout -= 2;
        std::this_thread::sleep_for(2ms);
    }
    CPPUNIT_ASSERT(hashRead->has("emptyArray"));
    CPPUNIT_ASSERT_EQUAL(0ul, hashRead->get<NDArray>("emptyArray").size());
    CPPUNIT_ASSERT_EQUAL(data::Types::INT16, hashRead->get<NDArray>("emptyArray").getType());
}


void InputOutputChannel_Test::testOutputPreparation() {
    // test an OutputChannel with defaults
    {
        OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel", Hash(), 0);
        output->setInstanceIdAndName("outputChannel", "outputWithDefault");
        output->initialize();
        const std::string address = output->getInitialConfiguration().get<std::string>("address");
        CPPUNIT_ASSERT_MESSAGE(std::string("unexpected channel address: ") + address,
                               address != std::string("default"));
    }
    // test an OutputChannel with an unclear hostname. We keep allowing the users to be creative.
    std::vector<std::string> addresses = createTestAddresses();
    addresses.push_back("default");

    for (const std::string& inputAddress : addresses) {
        OutputChannel::Pointer output =
              Configurator<OutputChannel>::create("OutputChannel", Hash("hostname", inputAddress), 0);
        output->setInstanceIdAndName("outputChannel", "oddAddress");
        output->initialize();
        const std::string address = output->getInitialConfiguration().get<std::string>("address");

        if (inputAddress == "default") {
            CPPUNIT_ASSERT_EQUAL(address, boost::asio::ip::host_name());
        } else {
            // The first address returned is the actual address, the second one is the
            // interface name, and the third one is a range, which OutputChannel turns into the actual address
            CPPUNIT_ASSERT_EQUAL(address, addresses[0]);
        }
    }

    CPPUNIT_ASSERT_THROW(Configurator<OutputChannel>::create("OutputChannel", Hash("hostname", "127.0.0.1"), 0),
                         karabo::data::LogicException);

    CPPUNIT_ASSERT_THROW(Configurator<OutputChannel>::create("OutputChannel", Hash("hostname", "192.168.0.1"), 0),
                         karabo::data::LogicException);

    CPPUNIT_ASSERT_THROW(Configurator<OutputChannel>::create("OutputChannel", Hash("hostname", "256.0.0.0/8"), 0),
                         karabo::data::LogicException);

    CPPUNIT_ASSERT_THROW(Configurator<OutputChannel>::create("OutputChannel", Hash("hostname", "256.0.0.1"), 0),
                         karabo::data::LogicException);

    CPPUNIT_ASSERT_THROW(Configurator<OutputChannel>::create("OutputChannel", Hash("hostname", "pepe"), 0),
                         karabo::data::LogicException);

    {
        // get the first valid address
        // "0.0.0.0/0" contains all addresses from 0.0.0.0 to 255.255.255.255
        const std::string expectedAddress = karabo::net::getIpFromCIDRNotation("0.0.0.0/0");
        // split the ip found in 4 parts and reformat it as a network segment
        // A.B.C.D -> A.B.C.0/24
        std::regex re("(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)");
        std::smatch what;
        bool result = std::regex_search(expectedAddress, what, re);
        CPPUNIT_ASSERT_MESSAGE(std::string("Could not parse address: ") + expectedAddress, result);
        std::ostringstream oss;
        oss << what.str(1) << "." << what.str(2) << "." << what.str(3) << ".0/24";
        const std::string inputAddress = oss.str();
        OutputChannel::Pointer output =
              Configurator<OutputChannel>::create("OutputChannel", Hash("hostname", inputAddress), 0);
        output->setInstanceIdAndName("outputChannel", "networkSegment");
        output->initialize();
        const std::string address = output->getInitialConfiguration().get<std::string>("address");
        CPPUNIT_ASSERT_EQUAL(address, expectedAddress);
    }
}

void InputOutputChannel_Test::testSchemaValidation() {
    // A schema to validate against
    Schema schema;
    VECTOR_INT32_ELEMENT(schema).key("v_int32").maxSize(10).readOnly().commit();
    STRING_ELEMENT(schema).key("str").readOnly().commit();

    const std::vector<int> vec(5, 1);
    // Validate once until end of stream
    {
        // Setup output channel
        OutputChannel::Pointer output =
              Configurator<OutputChannel>::create("OutputChannel", Hash("validateSchema", "once"), 0);
        output->setInstanceIdAndName("outputChannel", "output");
        output->initialize(schema);

        // Test extra key
        CPPUNIT_ASSERT_THROW(output->write(Hash("v_int32", vec, "str", "some", "tooMuch", 0)),
                             karabo::data::ParameterException);

        // Test missing key
        CPPUNIT_ASSERT_THROW(output->write(Hash("v_int32", vec)), karabo::data::ParameterException);

        // Test wrong type
        CPPUNIT_ASSERT_THROW(output->write(Hash("v_int32", vec, "str", 42)), karabo::data::ParameterException);
        CPPUNIT_ASSERT_THROW(output->write(Hash("v_int32", vec, "str", std::vector<Schema>(1))),
                             karabo::data::Exception);

        // Test too long vector
        const std::vector<int> longVec(50, 1); // max size is 10
        CPPUNIT_ASSERT_THROW(output->write(Hash("v_int32", longVec, "str", "some", "tooMuch", 0)),
                             karabo::data::ParameterException);

        // Now once proper data - after that even bad data is accepted (by default)
        CPPUNIT_ASSERT_NO_THROW(output->write(Hash("v_int32", vec, "str", "some")));
        CPPUNIT_ASSERT_NO_THROW(output->write(Hash("v_int32", vec, "str", "some", "tooMuch", 1)));

        // For a new "stream", the first data is validated again
        output->signalEndOfStream();
        // Bad data throws
        CPPUNIT_ASSERT_THROW(output->write(Hash("v_int32", vec, "str", "some", "tooMuch", 0)),
                             karabo::data::ParameterException);
        // First good data validates successfully
        CPPUNIT_ASSERT_NO_THROW(output->write(Hash("v_int32", vec, "str", "some")));
        // Then no further validation happens
        CPPUNIT_ASSERT_NO_THROW(output->write(Hash("v_int32", vec, "str", "some", "tooMuch", 0)));
    }

    // Validate always (default)
    {
        OutputChannel::Pointer output =
              Configurator<OutputChannel>::create("OutputChannel", Hash("validateSchema", "always"), 0);
        output->setInstanceIdAndName("outputChannel", "output");
        output->initialize(schema);

        // Now, with alidate "always", bad data is discovered even if once good data was written
        CPPUNIT_ASSERT_NO_THROW(output->write(Hash("v_int32", vec, "str", "some")));
        CPPUNIT_ASSERT_THROW(output->write(Hash("v_int32", vec, "str", "some", "tooMuch", 0)),
                             karabo::data::ParameterException);

        const Schema sch(Configurator<OutputChannel>::getSchema("OutputChannel"));
        CPPUNIT_ASSERT_EQUAL(std::string("always"), sch.getDefaultValue<std::string>("validateSchema"));
    }
}

void InputOutputChannel_Test::testConnectHandler() {
    // Test that handler for InputChannel::onConnect is called even if InputChannel is destructed shortly afterwards

    // Setup output channel
    OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel", Hash(), 0);
    output->setInstanceIdAndName("outputChannel", "output");
    output->initialize(); // needed due to int == 0 argument above

    // Parts of setup of input channel
    const std::string outputChannelId(output->getInstanceId() + ":output");
    const Hash cfg("connectedOutputChannels", std::vector<std::string>(1, outputChannelId));
    Hash outputInfo(output->getInformation());
    outputInfo.set("outputChannelString", outputChannelId);
    outputInfo.set("memoryLocation", "local");

    // Stress test many times due to different code paths for different thread timing
    int count = 250;
    while (--count >= 0) {
        InputChannel::Pointer input = Configurator<InputChannel>::create("InputChannel", cfg);
        input->setInstanceId("inputChannel");

        auto connectPromise = std::make_shared<std::promise<karabo::net::ErrorCode>>();
        std::future<karabo::net::ErrorCode> connectFuture = connectPromise->get_future();
        auto connectHandler = [&connectPromise](const karabo::net::ErrorCode& ec) { connectPromise->set_value(ec); };
        input->connect(outputInfo, connectHandler);
        const int sleepMs = count % 4; // i.e. test 0 to 3 ms delay before destruction of InputChannel
        std::this_thread::sleep_for(milliseconds(sleepMs));
        input.reset();
        // Now ensure that handler is called
        CPPUNIT_ASSERT_EQUAL_MESSAGE("attempt for " + karabo::data::toString(count), std::future_status::ready,
                                     connectFuture.wait_for(milliseconds(connectTimeoutMs)));
    }
}

void InputOutputChannel_Test::testWriteUpdateFlags() {
    // This test checks the behaviour of the raw data pointer behind an NDArray for the different values of the
    // safeNDArray flag that can be passed to OutputChannel::update(..).
    // Since input and output are local here, we can check when data is copied to ensure data consistency (i.e. new
    // pointer) and when not to improve speed (same pointer for local memoryLocation).

    using data::toString;

    // To switch on logging output for debugging, do e.g. the following:
    //    karabo::log::Logger::configure(Hash("priority", "DEBUG",
    //                                        // enable timestamps, with ms precision
    //                                        "ostream.pattern", "%d{%F %H:%M:%S,%l} %p  %c  : %m%n"));
    //    karabo::log::Logger::useConsole();

    for (const std::string& dataDistribution : {"copy"s, "shared"s}) {
        for (const std::string& onSlowness : {"wait"s, "queueDrop"s}) {
            // Setup output channel
            Hash cfgOut;
            std::vector<bool> registerSharedSelectors(1, false);
            if (dataDistribution == "shared") {
                // shared case: onSlowness on input channel side is ignored, but needed here for output
                cfgOut.set("noInputShared", onSlowness);
                // now also registration of sharedInputSelector matters
                registerSharedSelectors.push_back(true);
            }
            for (const bool registerSelector : registerSharedSelectors) {
                const std::string test(dataDistribution + " " + onSlowness + " " +
                                       std::string(registerSelector ? "sharedSelector" : ""));
                OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel", cfgOut, 0);
                output->setInstanceIdAndName("outputChannel", "output");
                if (registerSelector) {
                    output->registerSharedInputSelector([](const std::vector<std::string>& vec) {
                        if (vec.empty()) return std::string();
                        else return vec.front();
                    });
                }
                output->initialize(); // needed due to int == 0 argument above

                // Check both data transport ways: local via shared Memory or remote, i.e. tcp
                for (const std::string& memoryLocation : {"local"s, "remote"s}) {
                    // Setup input channel
                    const std::string outputChannelId(output->getInstanceId() + ":output");
                    const Hash cfg("connectedOutputChannels", std::vector<std::string>(1, outputChannelId),
                                   "dataDistribution", dataDistribution, "onSlowness", onSlowness,
                                   // No drop for queueDrop, please (otherwise does not matter):
                                   "maxQueueLength", 1000u); // InputChannel::DEFAULT_MAX_QUEUE_LENGTH is 2
                    InputChannel::Pointer input = Configurator<InputChannel>::create("InputChannel", cfg);
                    const std::string inputId("inputChannel"s + dataDistribution + onSlowness +
                                              (registerSelector ? "sharedSelector" : "") + memoryLocation);
                    input->setInstanceId(inputId);

                    // Connect preparations
                    Hash outputInfo(output->getInformation());
                    CPPUNIT_ASSERT_GREATER(0u, outputInfo.get<unsigned int>("port"));
                    outputInfo.set("outputChannelString", outputChannelId);
                    outputInfo.set("memoryLocation", memoryLocation);
                    std::promise<karabo::net::ErrorCode> connectErrorCode;
                    auto connectFuture = connectErrorCode.get_future();
                    auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode& ec) {
                        connectErrorCode.set_value(ec);
                    };

                    // Call connect and block until connection established
                    input->connect(outputInfo, connectHandler);
                    CPPUNIT_ASSERT_EQUAL(std::future_status::ready,
                                         connectFuture.wait_for(milliseconds(connectTimeoutMs)));
                    CPPUNIT_ASSERT_EQUAL(connectFuture.get(), karabo::net::ErrorCode()); // i.e. no error

                    // Create data with NDArray and get hands on its pointer
                    karabo::data::Hash data("array", karabo::data::NDArray(karabo::data::Dims(10), 4));
                    auto byteArr = data.get<karabo::data::ByteArray>("array.data");
                    const char* ptrSent = byteArr.first.get();

                    // Prepare to loop over all safeNDArray values for OutputChannel::update.
                    // If true, data is not copied - though if memoryLocation not (identified to be) "local", it will go
                    // via tcp and loopback interface will not preserve the pointer.
                    const std::vector<std::tuple<bool, bool>> vec_safeNDArray_shouldPtrBeEqual = {
                          {false, false}, // safeNDArray => data always copied and pointers differ
                          // safeNDArray true => data not copied, but loopback interface doesn't preserve pointer
                          {true, (memoryLocation == "local")}};
                    for (const auto& tup : vec_safeNDArray_shouldPtrBeEqual) {
                        // Data handler
                        const size_t nData = 5; // > 2, otherwise there may be no queue due to the two pots
                        std::vector<const char*> ptrsReceived;
                        std::promise<void> ptrPromise;
                        auto ptrFuture = ptrPromise.get_future();
                        input->registerDataHandler([&ptrPromise, &ptrsReceived, nData](
                                                         const Hash& data, const InputChannel::MetaData& meta) {
                            auto byteArr = data.get<karabo::data::ByteArray>("array.data");
                            ptrsReceived.push_back(byteArr.first.get());
                            if (ptrsReceived.size() == nData) ptrPromise.set_value();
                            // some sleep to enforce queue
                            std::this_thread::sleep_for(9ms);
                        });
                        bool safeNDArray, shouldPtrBeEqual;
                        std::tie(safeNDArray, shouldPtrBeEqual) = tup;
                        const std::string testFlags(test + " " + memoryLocation + " " + toString(safeNDArray));

                        // Currently (2.19.0a6), the fact that the input channel is connected (as checked above) only
                        // means that tcp is established. But the output channel needs to receive and process the
                        // "hello" message to register the channel. Only then it will send data to the input channel.
                        int timeout = 1000;
                        while (timeout > 0) {
                            if (dataDistribution == "shared") {
                                if (output->hasRegisteredSharedInputChannel(inputId)) break;
                            } else { // i.e. if (dataDistribution == "copy") {
                                if (output->hasRegisteredCopyInputChannel(inputId)) break;
                            }
                            timeout -= 2;
                            std::this_thread::sleep_for(2ms);
                        }
                        CPPUNIT_ASSERT_GREATEREQUAL(0, timeout);

                        for (size_t i = 0; i < nData; ++i) {
                            output->write(data);
                            output->update(safeNDArray);
                        }

                        // Receive data and check
                        CPPUNIT_ASSERT_EQUAL_MESSAGE(
                              testFlags, std::future_status::ready,
                              ptrFuture.wait_for(milliseconds(
                                    9 * nData * 50))); // * 50 as robustness margin - failed with 20 in one CI
                        ptrFuture.get();

                        for (size_t i = 0; i < nData; ++i) {
                            const std::string testI(testFlags + " " + toString(i));

                            if (shouldPtrBeEqual) {
                                CPPUNIT_ASSERT_EQUAL_MESSAGE(testI, reinterpret_cast<long long>(ptrSent),
                                                             reinterpret_cast<long long>(ptrsReceived[i]));
                            } else {
                                CPPUNIT_ASSERT_MESSAGE(testI + " " + toString(safeNDArray), ptrSent != ptrsReceived[i]);
                            }
                        }
                        // When debugging, this may help:  std::clog << "\n" << testFlags << ": OK.";
                    }
                }
            }
        }
    }
}

void InputOutputChannel_Test::testAsyncUpdate1a1() {
    testAsyncUpdate("drop", "copy", "local", false);
}

void InputOutputChannel_Test::testAsyncUpdate1a2() {
    testAsyncUpdate("drop", "copy", "local", true);
}

void InputOutputChannel_Test::testAsyncUpdate1b0() {
    testAsyncUpdate("drop", "copy", "remote", false); // safeNDArray does not matter for 'drop'  && 'remote'
}

void InputOutputChannel_Test::testAsyncUpdate2a1() {
    testAsyncUpdate("queueDrop", "copy", "local", false);
}

void InputOutputChannel_Test::testAsyncUpdate2a2() {
    testAsyncUpdate("queueDrop", "copy", "local", true);
}

void InputOutputChannel_Test::testAsyncUpdate2b1() {
    testAsyncUpdate("queueDrop", "copy", "remote", false);
}

void InputOutputChannel_Test::testAsyncUpdate2b2() {
    testAsyncUpdate("queueDrop", "copy", "remote", true);
}

void InputOutputChannel_Test::testAsyncUpdate3a1() {
    testAsyncUpdate("wait", "copy", "local", false);
}

void InputOutputChannel_Test::testAsyncUpdate3a2() {
    testAsyncUpdate("wait", "copy", "local", true);
}

void InputOutputChannel_Test::testAsyncUpdate3b0() {
    testAsyncUpdate("wait", "copy", "remote", false); // safeNDArray does not matter for 'wait' and 'remote'
}

void InputOutputChannel_Test::testAsyncUpdate4a1() {
    testAsyncUpdate("drop", "shared", "local", false);
}

void InputOutputChannel_Test::testAsyncUpdate4a2() {
    testAsyncUpdate("drop", "shared", "local", true);
}

void InputOutputChannel_Test::testAsyncUpdate4b0() {
    testAsyncUpdate("drop", "shared", "remote", false); // safeNDArray does not matter for onSlowness = 'drop'
}

void InputOutputChannel_Test::testAsyncUpdate5a1() {
    testAsyncUpdate("queueDrop", "shared", "local", false);
}

void InputOutputChannel_Test::testAsyncUpdate5a2() {
    testAsyncUpdate("queueDrop", "shared", "local", true);
}

void InputOutputChannel_Test::testAsyncUpdate5b1() {
    testAsyncUpdate("queueDrop", "shared", "remote", false);
}

void InputOutputChannel_Test::testAsyncUpdate5b2() {
    testAsyncUpdate("queueDrop", "shared", "remote", true);
}

void InputOutputChannel_Test::testAsyncUpdate6a1() {
    testAsyncUpdate("wait", "shared", "local", false);
}

void InputOutputChannel_Test::testAsyncUpdate6a2() {
    testAsyncUpdate("wait", "shared", "local", true);
}

void InputOutputChannel_Test::testAsyncUpdate6b0() {
    testAsyncUpdate("wait", "shared", "remote", false); // safeNDArray does not matter for 'wait' && 'remote'
}

void InputOutputChannel_Test::testAsyncUpdate(const std::string& onSlowness, const std::string& dataDistribution,
                                              const std::string& memoryLocation, bool safeNDArray) {
    // Format a bit the output to get a good overview
    std::clog << " '" << onSlowness << "', ";
    for (size_t i = 0; i < 9ul - onSlowness.size() && i < 50ul; ++i) std::clog << ' '; // 9: len("queueDrop")
    std::clog << "'" << dataDistribution << "', ";
    for (size_t i = 0; i < 6ul - dataDistribution.size() && i < 50ul; ++i) std::clog << ' '; // 6: len("shared")
    std::clog << "'" << memoryLocation << "', ";
    for (size_t i = 0; i < 6ul - memoryLocation.size() && i < 50ul; ++i) std::clog << ' '; // 6: len("remote")
    std::clog << safeNDArray << std::flush;

    constexpr size_t numToSend = 500;

    ThreadAdder threads(2);
    // Setup output channel
    // "noInputShared" does not matter if "dataDistribution" of InputChannel is "copy"
    const Hash outputCfg("noInputShared", onSlowness);
    OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel", outputCfg, 0);
    output->setInstanceIdAndName("outputChannel", "output");
    output->initialize(); // required due to additional '0' argument passed to create(..) above
    Hash outputInfo = output->getInformation();
    CPPUNIT_ASSERT_MESSAGE("OutputChannel keeps port 0!", outputInfo.get<unsigned int>("port") > 0);

    // Setup input channel
    const std::string outputChannelId(output->getInstanceId() + ":output");
    const Hash inputCfg("connectedOutputChannels", std::vector<std::string>(1, outputChannelId), //
                        "dataDistribution", dataDistribution,                                    //
                        "onSlowness", onSlowness, // onSlowness does not matter if dataDistribution is "shared"
                        // Max. queue length larger than default (2), but small enough so that something is dropped.
                        // But only relevant if onSlowness is "copy"
                        "maxQueueLength", static_cast<unsigned int>(numToSend / 5));

    InputChannel::Pointer input = Configurator<InputChannel>::create("InputChannel", inputCfg);
    input->setInstanceId("inputChannel");
    std::vector<Hash> receivedData;
    input->registerDataHandler(
          [&receivedData](const Hash& data, const InputChannel::MetaData& meta) { receivedData.push_back(data); });
    std::promise<void> eosReadPromise;
    auto eosReadFuture = eosReadPromise.get_future();
    input->registerEndOfStreamEventHandler(
          [&eosReadPromise](const InputChannel::Pointer&) { eosReadPromise.set_value(); });

    //
    // Connect
    //
    outputInfo.set("outputChannelString", outputChannelId);
    outputInfo.set("memoryLocation", memoryLocation);
    // Setup connection handler
    std::promise<karabo::net::ErrorCode> connectErrorCode;
    auto connectFuture = connectErrorCode.get_future();
    auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode& ec) { connectErrorCode.set_value(ec); };
    // initiate connect and block until done (fail test if timeout)
    karabo::net::ErrorCode ec;
    input->connect(outputInfo, connectHandler); // this is async!
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, connectFuture.wait_for(5000ms));
    ec = connectFuture.get();                                                 // Can get() only once...
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ec.message(), karabo::net::ErrorCode(), ec); // i.e. no error

    // Currently (2.19.0rc3), the fact that the input channel is connected (as checked above) only
    // means that tcp is established. But the output channel needs to receive and process the
    // "hello" message to register the channel. Only then it will send data to the input channel.
    int timeout = 1000;
    while (timeout > 0) {
        if (dataDistribution == "shared") {
            if (output->hasRegisteredSharedInputChannel(input->getInstanceId())) break;
        } else { // i.e. if (dataDistribution == "copy") {
            if (output->hasRegisteredCopyInputChannel(input->getInstanceId())) break;
        }
        timeout -= 1;
        std::this_thread::sleep_for(1ms);
    }
    CPPUNIT_ASSERT_GREATEREQUAL(0, timeout);

    // Send data many times using asyncUpdate
    receivedData.reserve(numToSend);
    Epochstamp startStamp;
    for (size_t iSend = 0; iSend < numToSend; ++iSend) {
        output->write(Hash("str", karabo::data::toString(iSend), "vec", std::vector<long long>(300, iSend), "arr",
                           karabo::data::NDArray(1000, static_cast<unsigned long long>(iSend))));
        // safeNDArray matters when queuing or local receiver
        output->asyncUpdate(safeNDArray); // We do not care about handler when writing is done
    }
    Epochstamp sentStamp;
    const double durationSend = static_cast<double>(sentStamp - startStamp);

    // Signal end of stream
    std::promise<void> eosSentPromise;
    auto eosSentFuture = eosSentPromise.get_future();
    output->asyncSignalEndOfStream([&eosSentPromise]() { eosSentPromise.set_value(); });
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, eosSentFuture.wait_for(5000ms));
    CPPUNIT_ASSERT_NO_THROW(eosSentFuture.get());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
          (receivedData.empty() ? std::string("nothing received") : karabo::data::toString(receivedData.back())),
          std::future_status::ready, eosReadFuture.wait_for(5000ms));
    CPPUNIT_ASSERT_NO_THROW(eosReadFuture.get());

    // Now investigate data received
    std::clog << ": Sent " << numToSend << " in " << durationSend << " s, received last of " << receivedData.size()
              << " items " << static_cast<double>(Epochstamp() - sentStamp) << " s later.";
    CPPUNIT_ASSERT(!receivedData.empty());
    const bool noLoss = (onSlowness == "wait");
    if (noLoss) {
        std::string msg("str's received: ");
        if (numToSend != receivedData.size()) { // assemble debug info
            for (const Hash& h : receivedData) {
                (msg += h.get<std::string>("str")) += ",";
            }
        }
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, numToSend, receivedData.size());
    }
    for (size_t iSend = 1ul; iSend < receivedData.size(); ++iSend) {
        const auto previous = receivedData[iSend - 1ul].get<std::vector<long long>>("vec").front();
        const auto current = receivedData[iSend].get<std::vector<long long>>("vec").front();
        if (noLoss) {
            CPPUNIT_ASSERT_EQUAL(previous + 1, current);
        } else {
            CPPUNIT_ASSERT_GREATER(previous, current);
        }
    }
}
