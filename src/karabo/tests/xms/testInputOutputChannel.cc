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

#include <gtest/gtest.h>
#include <ifaddrs.h>
#include <sys/types.h>

#include <boost/regex.hpp>
#include <chrono>
#include <future>
#include <mutex>
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


class TestInputOutputChannel : public ::testing::Test {
    static bool m_calledTestAsyncUpdate;

   protected:
    TestInputOutputChannel();
    ~TestInputOutputChannel() override {}
    void SetUp() override {}
    void TearDown() override {}
};

// Static initialization
bool TestInputOutputChannel::m_calledTestAsyncUpdate = false;


TestInputOutputChannel::TestInputOutputChannel() {
    if (!m_calledTestAsyncUpdate) {
        m_calledTestAsyncUpdate = true;
        std::clog << " Settings given for TestInputOutputChannel::testAsyncUpdate<NxM> are: "
                  << "onSlowness, dataDistribution, memoryLocation, safeNDArray:" << std::endl;
    }
}


TEST_F(TestInputOutputChannel, testOutputChannelElement) {
    Schema pipeSchema;
    INT32_ELEMENT(pipeSchema).key("int32").readOnly().commit();

    Schema s;
    EXPECT_NO_THROW(
          OUTPUT_CHANNEL_ELEMENT(s).key("validkey").displayedName("Valid output").dataSchema(pipeSchema).commit());
    EXPECT_TRUE(s.has("validkey.schema.int32"));
    EXPECT_EQ("OutputSchema", s.getDisplayType("validkey.schema"));

    // The deviceId/channel delimiters ':' and (for backward compatibility) '@' are not allowed in keys.
    EXPECT_THROW(OUTPUT_CHANNEL_ELEMENT(s).key("invalid:key"), karabo::data::ParameterException);
    EXPECT_THROW(OUTPUT_CHANNEL_ELEMENT(s).key("invalid@key2"), karabo::data::ParameterException);
}


TEST_F(TestInputOutputChannel, testManyToOne) {
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
        EXPECT_LT(0u, outputInfo.get<unsigned int>("port"));
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
        ASSERT_EQ(std::future_status::ready, connectFuture.wait_for(milliseconds(connectTimeoutMs)))
              << "attempt for " << outputIds[i];
        EXPECT_EQ(karabo::net::ErrorCode(), // i.e. no error
                  connectFuture.get())
              << "attempt for " << outputIds[i];

        // All up to the last one are connected now
        auto connectStatusMap(input->getConnectionStatus());
        EXPECT_EQ(outputs.size(), connectStatusMap.size());
        for (size_t j = 0; j < outputs.size(); ++j) {
            EXPECT_TRUE(connectStatusMap.find(outputIds[j]) != connectStatusMap.end());
            namespace kd = karabo::data;
            namespace kn = karabo::net;
            EXPECT_EQ((j <= i ? kn::ConnectionStatus::CONNECTED : kn::ConnectionStatus::DISCONNECTED),
                      connectStatusMap[outputIds[j]])
                  << "Tested j = " << j << ", connected i = " << i;
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
        EXPECT_TRUE(registered) << "Not yet ready: output " << i;
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
    EXPECT_EQ(1u, static_cast<unsigned int>(nReceivedEos)) << "Data received:\n"
                                                           << karabo::data::toString(receivedData);

    // Proper number and order of data received from each output
    for (size_t i = 0; i < outputIds.size(); ++i) {
        const auto& data = receivedData.get<std::vector<unsigned int>>(outputIds[i]);
        EXPECT_EQ(numData, data.size()) << outputIds[i] << " lacks data, all received:\n"
                                        << karabo::data::toString(receivedData);
        for (unsigned int iData = 0; iData < data.size(); ++iData) {
            EXPECT_EQ(iData, data[iData]) << "Output " << i << ", data " << iData;
        }
    }
}


TEST_F(TestInputOutputChannel, testConnectDisconnect) {
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
    EXPECT_EQ(0u, calls);
    {
        std::lock_guard lock(handlerDataMutex);
        EXPECT_EQ(0uL, table.size());
    }

    // Connect
    Hash outputInfo(output->getInformation());
    EXPECT_LT(0u, outputInfo.get<unsigned int>("port"));
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
        EXPECT_EQ(1ul, connectStatusMap.size());
        EXPECT_EQ(outputChannelId, connectStatusMap.begin()->first);
        EXPECT_TRUE(connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::DISCONNECTED)
              << static_cast<int>(connectStatusMap[outputChannelId]);

        // initiate connect and block until done (fail test if timeout))
        input->connect(outputInfo, connectHandler);

        // Now connecting or - with very weird threading - already connected
        connectStatusMap = input->getConnectionStatus();
        EXPECT_EQ(1ul, connectStatusMap.size());
        EXPECT_EQ(outputChannelId, connectStatusMap.begin()->first);
        EXPECT_TRUE(connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::CONNECTING ||
                    connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::CONNECTED)
              << static_cast<int>(connectStatusMap[outputChannelId]);

        ASSERT_EQ(std::future_status::ready, connectFuture.wait_for(milliseconds(connectTimeoutMs)))
              << "attempt number " << i;
        EXPECT_EQ(connectFuture.get(),
                  karabo::net::ErrorCode()) << "attempt number " << i; // i.e. no error

        // We are connected - check that the status tracker received all steps
        // (rely on order of calls to connection tracker (first) and handler (second) at the end of
        // InputChannel::onConnect
        ASSERT_TRUE(trackedStatus.size() > 0ul);
        EXPECT_EQ(static_cast<int>(karabo::net::ConnectionStatus::CONNECTING), static_cast<int>(trackedStatus[0]));
        // Without waiting for tracker really being called, this test relies on order of calls to connection
        // tracker and handler given to InputChannel::connect(..) (although might succeed most times even otherwise)
        ASSERT_TRUE(trackedStatus.size() > 1ul);
        EXPECT_EQ(static_cast<int>(karabo::net::ConnectionStatus::CONNECTED), static_cast<int>(trackedStatus[1]));
        EXPECT_EQ(2ul, trackedStatus.size()); // i.e. nothing else (yet)!

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
        ASSERT_EQ(1UL, table.size());
        // ... and check the published connection information
        EXPECT_EQ(table[0].get<std::string>("remoteId"), input->getInstanceId());
        EXPECT_EQ(table[0].get<std::string>("dataDistribution"), std::string("copy"));
        EXPECT_EQ(table[0].get<std::string>("onSlowness"), std::string("drop"));
        EXPECT_EQ(table[0].get<std::string>("memoryLocation"), std::string("local"));

        // Now we are indeed connected:
        connectStatusMap = input->getConnectionStatus();
        ASSERT_EQ(1ul, connectStatusMap.size());
        EXPECT_EQ(outputChannelId, connectStatusMap.begin()->first);
        EXPECT_TRUE(connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::CONNECTED)
              << static_cast<int>(connectStatusMap[outputChannelId]);

        // Write data again (twice in one go...) - now input is connected.
        output->write(Hash("key", 43));
        output->write(Hash("key", -43));
        output->update();

        trials = 200;
        while (--trials >= 0) {
            if (2u == calls) break;
            std::this_thread::sleep_for(2ms); // time for callback
        }
        EXPECT_EQ(2u, calls);

        // Disconnect
        input->disconnect(outputChannelId);
        connectStatusMap = input->getConnectionStatus();
        EXPECT_EQ(1ul, connectStatusMap.size());
        ASSERT_EQ(outputChannelId, connectStatusMap.begin()->first);
        EXPECT_TRUE(connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::DISCONNECTED)
              << static_cast<int>(connectStatusMap[outputChannelId]);

        // Some time to travel for message
        trials = 1000; // failed with 200 in https://git.xfel.eu/Karabo/Framework/-/jobs/131075/raw
        do {
            std::this_thread::sleep_for(2ms);
            std::lock_guard lock(handlerDataMutex);
            if (table.empty() && trackedStatus.size() > 2ul) {
                break;
            }
        } while (--trials >= 0);
        EXPECT_EQ(0uL, table.size());
        // Also the tracker got informed about disconnection:
        ASSERT_TRUE(trackedStatus.size() > 2ul);
        EXPECT_EQ(static_cast<int>(karabo::net::ConnectionStatus::DISCONNECTED), static_cast<int>(trackedStatus[2]));
        EXPECT_EQ(3ul, trackedStatus.size()); // i.e. nothing else!
    }

    // Write data again - input does not anymore receive data.
    output->write(Hash("key", 44));
    output->update();
    // Extended time for callback to be really sure nothing comes.
    std::this_thread::sleep_for(100ms);
    // still 2:
    EXPECT_EQ(2u, calls);

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
        ASSERT_EQ(std::future_status::ready, connectFuture.wait_for(milliseconds(connectTimeoutMs)))
              << "Connection handler not called in time for " + toString(badOutputInfo);
        EXPECT_TRUE(connectFuture.get() != karabo::net::ErrorCode())
              << "Connection did not fail for " << toString(badOutputInfo); // not all OK (do not care which problem)
    }
}


TEST_F(TestInputOutputChannel, testConcurrentConnect) {
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
        EXPECT_TRUE(outputInfo.get<unsigned int>("port") > 0) << "OutputChannel keeps port 0!";

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
        ASSERT_EQ(std::future_status::ready, connectFuture1.wait_for(milliseconds(connectTimeoutMs)));
        ASSERT_EQ(std::future_status::ready, connectFuture2.wait_for(milliseconds(connectTimeoutMs)));

        EXPECT_EQ(karabo::net::ErrorCode(), connectFuture1.get());
        const karabo::net::ErrorCode ec = connectFuture2.get();
        namespace bse = boost::system::errc;
        EXPECT_TRUE(ec == bse::make_error_code(bse::connection_already_in_progress) ||
                    ec == bse::make_error_code(bse::already_connected))
              << karabo::data::toString(ec);

        input->disconnect(outputInfo);

        // Ensure it is disconnected
        EXPECT_EQ(static_cast<int>(karabo::net::ConnectionStatus::DISCONNECTED),
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

        ASSERT_EQ(std::future_status::ready, connectFuture3.wait_for(milliseconds(connectTimeoutMs)));
        ASSERT_EQ(std::future_status::ready, connectFuture4.wait_for(milliseconds(connectTimeoutMs)));

        // Now it is not exactly clear what to expect - depends on timing of threads:
        // - 1st fails as operation_canceled, 2nd succeeds, i.e. disconnect(..) clears from "being setup"
        // - 1st succeeds and 2nd succeeds, i.e. disconnect(..) got called (and fully succeeded!) when 1st connect(..)
        // already succeeded
        const karabo::net::ErrorCode ec1 = connectFuture3.get();
        const karabo::net::ErrorCode ec2 = connectFuture4.get();

        EXPECT_TRUE((ec1 == bse::make_error_code(bse::operation_canceled) && ec2 == karabo::net::ErrorCode()) ||
                    (ec1 == karabo::net::ErrorCode() && ec2 == karabo::net::ErrorCode()))
              << "1: " << karabo::data::toString(ec1) << ", 2: " << karabo::data::toString(ec2);
    }
}


TEST_F(TestInputOutputChannel, testInputHandler) {
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
    EXPECT_LE(0, timeout);

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
    EXPECT_TRUE(hashRead->has("data"));
    EXPECT_EQ(42, hashRead->get<int>("data"));

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
    ASSERT_TRUE(hashRead->has("emptyArray"));
    EXPECT_EQ(0ul, hashRead->get<NDArray>("emptyArray").size());
    EXPECT_EQ(data::Types::INT16, hashRead->get<NDArray>("emptyArray").getType());
}


TEST_F(TestInputOutputChannel, testOutputPreparation) {
    // test an OutputChannel with defaults
    {
        OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel", Hash(), 0);
        output->setInstanceIdAndName("outputChannel", "outputWithDefault");
        output->initialize();
        const std::string address = output->getInitialConfiguration().get<std::string>("address");
        EXPECT_TRUE(address != std::string("default")) << "unexpected channel address: " << address;
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
            EXPECT_EQ(address, boost::asio::ip::host_name());
        } else {
            // The first address returned is the actual address, the second one is the
            // interface name, and the third one is a range, which OutputChannel turns into the actual address
            EXPECT_EQ(address, addresses[0]);
        }
    }

    EXPECT_THROW(Configurator<OutputChannel>::create("OutputChannel", Hash("hostname", "127.0.0.1"), 0),
                 karabo::data::LogicException);

    EXPECT_THROW(Configurator<OutputChannel>::create("OutputChannel", Hash("hostname", "192.168.0.1"), 0),
                 karabo::data::LogicException);

    EXPECT_THROW(Configurator<OutputChannel>::create("OutputChannel", Hash("hostname", "256.0.0.0/8"), 0),
                 karabo::data::LogicException);

    EXPECT_THROW(Configurator<OutputChannel>::create("OutputChannel", Hash("hostname", "256.0.0.1"), 0),
                 karabo::data::LogicException);

    EXPECT_THROW(Configurator<OutputChannel>::create("OutputChannel", Hash("hostname", "pepe"), 0),
                 karabo::data::LogicException);

    {
        // get the first valid address
        // "0.0.0.0/0" contains all addresses from 0.0.0.0 to 255.255.255.255
        const std::string expectedAddress = karabo::net::getIpFromCIDRNotation("0.0.0.0/0");
        // split the ip found in 4 parts and reformat it as a network segment
        // A.B.C.D -> A.B.C.0/24
        boost::regex re("(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)");
        boost::smatch what;
        bool result = boost::regex_search(expectedAddress, what, re);
        EXPECT_TRUE(result) << "Could not parse address: " << expectedAddress;
        std::ostringstream oss;
        oss << what.str(1) << "." << what.str(2) << "." << what.str(3) << ".0/24";
        const std::string inputAddress = oss.str();
        OutputChannel::Pointer output =
              Configurator<OutputChannel>::create("OutputChannel", Hash("hostname", inputAddress), 0);
        output->setInstanceIdAndName("outputChannel", "networkSegment");
        output->initialize();
        const std::string address = output->getInitialConfiguration().get<std::string>("address");
        EXPECT_EQ(address, expectedAddress);
    }
}


TEST_F(TestInputOutputChannel, testSchemaValidation) {
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
        EXPECT_THROW(output->write(Hash("v_int32", vec, "str", "some", "tooMuch", 0)),
                     karabo::data::ParameterException);

        // Test missing key
        EXPECT_THROW(output->write(Hash("v_int32", vec)), karabo::data::ParameterException);

        // Test wrong type
        EXPECT_THROW(output->write(Hash("v_int32", vec, "str", 42)), karabo::data::ParameterException);
        EXPECT_THROW(output->write(Hash("v_int32", vec, "str", std::vector<Schema>(1))), karabo::data::Exception);

        // Test too long vector
        const std::vector<int> longVec(50, 1); // max size is 10
        EXPECT_THROW(output->write(Hash("v_int32", longVec, "str", "some", "tooMuch", 0)),
                     karabo::data::ParameterException);

        // Now once proper data - after that even bad data is accepted (by default)
        EXPECT_NO_THROW(output->write(Hash("v_int32", vec, "str", "some")));
        EXPECT_NO_THROW(output->write(Hash("v_int32", vec, "str", "some", "tooMuch", 1)));

        // For a new "stream", the first data is validated again
        output->signalEndOfStream();
        // Bad data throws
        EXPECT_THROW(output->write(Hash("v_int32", vec, "str", "some", "tooMuch", 0)),
                     karabo::data::ParameterException);
        // First good data validates successfully
        EXPECT_NO_THROW(output->write(Hash("v_int32", vec, "str", "some")));
        // Then no further validation happens
        EXPECT_NO_THROW(output->write(Hash("v_int32", vec, "str", "some", "tooMuch", 0)));
    }

    // Validate always (default)
    {
        OutputChannel::Pointer output =
              Configurator<OutputChannel>::create("OutputChannel", Hash("validateSchema", "always"), 0);
        output->setInstanceIdAndName("outputChannel", "output");
        output->initialize(schema);

        // Now, with alidate "always", bad data is discovered even if once good data was written
        EXPECT_NO_THROW(output->write(Hash("v_int32", vec, "str", "some")));
        EXPECT_THROW(output->write(Hash("v_int32", vec, "str", "some", "tooMuch", 0)),
                     karabo::data::ParameterException);

        const Schema sch(Configurator<OutputChannel>::getSchema("OutputChannel"));
        EXPECT_EQ("always", sch.getDefaultValue<std::string>("validateSchema"));
    }
}


TEST_F(TestInputOutputChannel, testConnectHandler) {
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
        EXPECT_EQ(std::future_status::ready, connectFuture.wait_for(milliseconds(connectTimeoutMs)))
              << "attempt for " << count;
    }
}


TEST_F(TestInputOutputChannel, testWriteUpdateFlags) {
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
                    EXPECT_LT(0u, outputInfo.get<unsigned int>("port"));
                    outputInfo.set("outputChannelString", outputChannelId);
                    outputInfo.set("memoryLocation", memoryLocation);
                    std::promise<karabo::net::ErrorCode> connectErrorCode;
                    auto connectFuture = connectErrorCode.get_future();
                    auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode& ec) {
                        connectErrorCode.set_value(ec);
                    };

                    // Call connect and block until connection established
                    input->connect(outputInfo, connectHandler);
                    ASSERT_EQ(std::future_status::ready, connectFuture.wait_for(milliseconds(connectTimeoutMs)));
                    EXPECT_EQ(connectFuture.get(), karabo::net::ErrorCode()); // i.e. no error

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
                        EXPECT_LE(0, timeout);

                        for (size_t i = 0; i < nData; ++i) {
                            output->write(data);
                            output->update(safeNDArray);
                        }

                        // Receive data and check
                        ASSERT_EQ(std::future_status::ready,
                                  ptrFuture.wait_for(milliseconds(9 * nData * 50)))
                              << testFlags; // * 50 as robustness margin - failed with 20 in one CI
                        ptrFuture.get();

                        for (size_t i = 0; i < nData; ++i) {
                            const std::string testI(testFlags + " " + toString(i));

                            if (shouldPtrBeEqual) {
                                EXPECT_EQ(reinterpret_cast<long long>(ptrSent),
                                          reinterpret_cast<long long>(ptrsReceived[i]))
                                      << testI;
                            } else {
                                EXPECT_TRUE(ptrSent != ptrsReceived[i]) << testI << " " << toString(safeNDArray);
                            }
                        }
                        // When debugging, this may help:  std::clog << "\n" << testFlags << ": OK.";
                    }
                }
            }
        }
    }
}


static void testAsyncUpdate(const std::string& onSlowness, const std::string& dataDistribution,
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
    EXPECT_TRUE(outputInfo.get<unsigned int>("port") > 0) << "OutputChannel keeps port 0!";

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
    ASSERT_EQ(std::future_status::ready, connectFuture.wait_for(5000ms));
    ec = connectFuture.get();                                // Can get() only once...
    EXPECT_EQ(karabo::net::ErrorCode(), ec) << ec.message(); // i.e. no error

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
    EXPECT_LE(0, timeout);

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
    ASSERT_EQ(std::future_status::ready, eosSentFuture.wait_for(5000ms));
    EXPECT_NO_THROW(eosSentFuture.get());
    ASSERT_EQ(std::future_status::ready, eosReadFuture.wait_for(5000ms))
          << (receivedData.empty() ? std::string("nothing received") : karabo::data::toString(receivedData.back()));
    EXPECT_NO_THROW(eosReadFuture.get());

    // Now investigate data received
    std::clog << ": Sent " << numToSend << " in " << durationSend << " s, received last of " << receivedData.size()
              << " items " << static_cast<double>(Epochstamp() - sentStamp) << " s later.";
    EXPECT_TRUE(!receivedData.empty());
    const bool noLoss = (onSlowness == "wait");
    if (noLoss) {
        std::string msg("str's received: ");
        if (numToSend != receivedData.size()) { // assemble debug info
            for (const Hash& h : receivedData) {
                (msg += h.get<std::string>("str")) += ",";
            }
        }
        EXPECT_EQ(numToSend, receivedData.size()) << msg;
    }
    for (size_t iSend = 1ul; iSend < receivedData.size(); ++iSend) {
        const auto previous = receivedData[iSend - 1ul].get<std::vector<long long>>("vec").front();
        const auto current = receivedData[iSend].get<std::vector<long long>>("vec").front();
        if (noLoss) {
            EXPECT_EQ(previous + 1, current);
        } else {
            EXPECT_LT(previous, current);
        }
    }
}


TEST_F(TestInputOutputChannel, testAsyncUpdate1a1) {
    testAsyncUpdate("drop", "copy", "local", false);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate1a2) {
    testAsyncUpdate("drop", "copy", "local", true);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate1b0) {
    testAsyncUpdate("drop", "copy", "remote", false); // safeNDArray does not matter for 'drop'  && 'remote'
}


TEST_F(TestInputOutputChannel, testAsyncUpdate2a1) {
    testAsyncUpdate("queueDrop", "copy", "local", false);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate2a2) {
    testAsyncUpdate("queueDrop", "copy", "local", true);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate2b1) {
    testAsyncUpdate("queueDrop", "copy", "remote", false);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate2b2) {
    testAsyncUpdate("queueDrop", "copy", "remote", true);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate3a1) {
    testAsyncUpdate("wait", "copy", "local", false);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate3a2) {
    testAsyncUpdate("wait", "copy", "local", true);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate3b0) {
    testAsyncUpdate("wait", "copy", "remote", false); // safeNDArray does not matter for 'wait' and 'remote'
}


TEST_F(TestInputOutputChannel, testAsyncUpdate4a1) {
    testAsyncUpdate("drop", "shared", "local", false);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate4a2) {
    testAsyncUpdate("drop", "shared", "local", true);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate4b0) {
    testAsyncUpdate("drop", "shared", "remote", false); // safeNDArray does not matter for onSlowness = 'drop'
}


TEST_F(TestInputOutputChannel, testAsyncUpdate5a1) {
    testAsyncUpdate("queueDrop", "shared", "local", false);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate5a2) {
    testAsyncUpdate("queueDrop", "shared", "local", true);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate5b1) {
    testAsyncUpdate("queueDrop", "shared", "remote", false);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate5b2) {
    testAsyncUpdate("queueDrop", "shared", "remote", true);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate6a1) {
    testAsyncUpdate("wait", "shared", "local", false);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate6a2) {
    testAsyncUpdate("wait", "shared", "local", true);
}


TEST_F(TestInputOutputChannel, testAsyncUpdate6b0) {
    testAsyncUpdate("wait", "shared", "remote", false); // safeNDArray does not matter for 'wait' && 'remote'
}
