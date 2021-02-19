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
#include "karabo/util/Schema.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/net/EventLoop.hh"

using namespace karabo;
using xms::InputChannel;
using xms::OutputChannel;
using xms::OUTPUT_CHANNEL_ELEMENT;
using util::Configurator;
using util::Hash;
using util::Schema;
using util::INT32_ELEMENT;

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


void InputOutputChannel_Test::testOutputChannelElement() {

    Schema pipeSchema;
    INT32_ELEMENT(pipeSchema).key("int32").readOnly().commit();

    Schema s;
    CPPUNIT_ASSERT_NO_THROW(OUTPUT_CHANNEL_ELEMENT(s)
                            .key("validkey")
                            .displayedName("Valid output")
                            .dataSchema(pipeSchema)
                            .commit());
    CPPUNIT_ASSERT(s.has("validkey.schema.int32"));

    // The deviceId/channel delimiters ':' and (for backward compatibility) '@' are not allowed in keys.
    CPPUNIT_ASSERT_THROW(OUTPUT_CHANNEL_ELEMENT(s).key("invalid:key"), karabo::util::ParameterException);
    CPPUNIT_ASSERT_THROW(OUTPUT_CHANNEL_ELEMENT(s).key("invalid@key2"), karabo::util::ParameterException);
}


void InputOutputChannel_Test::testManyToOne() {
    // To switch on logging output for debugging, do e.g. the following:
    //    karabo::log::Logger::configure(Hash("priority", "DEBUG",
    //                                        // enable timestamps, with ms precision
    //                                        "ostream.pattern", "%d{%F %H:%M:%S,%l} %p  %c  : %m%n"));
    //    karabo::log::Logger::useOstream();

    const unsigned int numOutputs = 6;
    ThreadAdder extraThreads(numOutputs);

    std::array<OutputChannel::Pointer, numOutputs> outputs;
    std::vector<std::string> outputIds(outputs.size());
    for (size_t i = 0; i < outputs.size(); ++i) {
        const std::string channelId("output" + karabo::util::toString(i));
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
    input->registerDataHandler([&receivedData] (const Hash& data, const InputChannel::MetaData & meta) {
        const std::string& sourceName = meta.getSource();
        receivedData.get<std::vector<unsigned int>>(sourceName).push_back(data.get<unsigned int>("uint"));
    });

    // Handler to count endOfStream events
    std::atomic<int> nReceivedEos(0);
    input->registerEndOfStreamEventHandler([&nReceivedEos](const InputChannel::Pointer&) {
        ++nReceivedEos;
    });

    for (size_t i = 0; i < outputs.size(); ++i) {
        // Connect
        Hash outputInfo(outputs[i]->getInformation());
        CPPUNIT_ASSERT_GREATER(0u, outputInfo.get<unsigned int>("port"));
        outputInfo.set("outputChannelString", outputIds[i]);
        // Alternate scenarios to test both memory location code paths:
        outputInfo.set("memoryLocation", (i % 2 == 0 // alternate between...
                                          ? "local" // - using inner-process data shortcut via static Memory class
                                          : "remote")); // - sending data via Tcp (buggy till 2.9.X for many-to-one)

        // Setup connection handler
        std::promise<karabo::net::ErrorCode> connectErrorCode;
        auto connectFuture = connectErrorCode.get_future();
        auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode & ec) {
            connectErrorCode.set_value(ec);
        };
        // Initiate connect and block until done - fail test if timeout.
        // Being more clever and waiting only once for all connections in one go is not worth it in the test here.
        input->connect(outputInfo, connectHandler);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("attempt for " + outputIds[i],
                                     std::future_status::ready,
                                     connectFuture.wait_for(std::chrono::milliseconds(5000)));
        CPPUNIT_ASSERT_EQUAL_MESSAGE("attempt for " + outputIds[i],
                                     karabo::net::ErrorCode(), // i.e. no error
                                     connectFuture.get());

        // All up to the last one are connected now
        auto connectStatusMap(input->getConnectionStatus());
        CPPUNIT_ASSERT_EQUAL(outputs.size(), connectStatusMap.size());
        for (size_t j = 0; j < outputs.size(); ++j) {
            CPPUNIT_ASSERT(connectStatusMap.find(outputIds[j]) != connectStatusMap.end());
            namespace ku = karabo::util;
            namespace kn = karabo::net;
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Tested j = " + ku::toString(j) += ", connected i = " + ku::toString(i),
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
            // Happesn very rarely - seen 6 times in 20,000 local test runs.
                boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }
        CPPUNIT_ASSERT_MESSAGE("Not yet ready: output " + karabo::util::toString(i), registered);
    }

    // Prepare lambda to send data
    const size_t numData = 200;
    boost::function<void(unsigned int) > sending = [&outputs, numData](unsigned int outNum) {
        for (unsigned int i = 0; i < numData; ++i) {
            outputs[outNum]->write(Hash("uint", i));
            outputs[outNum]->update();
        }
        outputs[outNum]->signalEndOfStream();
    };

    // Start to send data from all outputs in parallel (we added enough threads in the beginning!).
    for (unsigned int i = 0; i < numOutputs; ++i) {
        karabo::net::EventLoop::getIOService().post(boost::bind(sending, i));
    }

    // Wait for endOfStream arrival
    int trials = 3000;
    do {
        boost::this_thread::sleep(boost::posix_time::milliseconds(3));
        if (nReceivedEos > 0) break;
    } while (--trials >= 0);

    // endOfStream received once
    // We give some time for more to arrive - but there should only be one, although each output sent it!
    boost::this_thread::sleep(boost::posix_time::milliseconds(200));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Data received:\n" + karabo::util::toString(receivedData),
                                 1u, static_cast<unsigned int> (nReceivedEos));

    // Proper number and order of data received from each output
    for (size_t i = 0; i < outputIds.size(); ++i) {
        const auto& data = receivedData.get<std::vector<unsigned int>>(outputIds[i]);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(outputIds[i] + " lacks data, all received:\n" + karabo::util::toString(receivedData),
                                     numData, data.size());
        for (unsigned int iData = 0; iData < data.size(); ++iData) {
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Output " + karabo::util::toString(i) += ", data " + karabo::util::toString(iData),
                                         iData, data[iData]);
        }
    }
}

void InputOutputChannel_Test::testConnectDisconnect() {
    // To switch on logging output for debugging, do e.g. the following:
    //    karabo::log::Logger::configure(Hash("priority", "DEBUG",
    //                                        // enable timestamps, with ms precision
    //                                        "ostream.pattern", "%d{%F %H:%M:%S,%l} %p  %c  : %m%n"));
    //    karabo::log::Logger::useOstream();

    // Setup output channel
    OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel", Hash(), 0);
    output->setInstanceIdAndName("outputChannel", "output");
    output->initialize(); // needed due to int == 0 argument above

    std::vector<karabo::util::Hash> table;
    boost::mutex handlerDataMutex;
    output->registerShowConnectionsHandler([&table, &handlerDataMutex](const std::vector<karabo::util::Hash>& connections) {
        boost::mutex::scoped_lock lock(handlerDataMutex);
        table = connections;
    });

    // Setup input channel
    const std::string outputChannelId(output->getInstanceId() + ":output");
    const Hash cfg("connectedOutputChannels", std::vector<std::string>(1, outputChannelId));
    InputChannel::Pointer input = Configurator<InputChannel>::create("InputChannel", cfg);
    input->setInstanceId("inputChannel");
    unsigned int calls = 0;
    input->registerDataHandler([&calls](const Hash& data, const InputChannel::MetaData & meta) {
        ++calls;
    });
    std::vector<karabo::net::ConnectionStatus> trackedStatus;
    input->registerConnectionTracker([&trackedStatus, &handlerDataMutex, outputChannelId](const std::string& outputId,
                                                                                          karabo::net::ConnectionStatus status) {
        if (outputId == outputChannelId) {
            boost::mutex::scoped_lock lock(handlerDataMutex);
            trackedStatus.push_back(status);
        }
    });

    // Write first data - nobody connected yet.
    output->write(Hash("key", 42));
    output->update();
    boost::this_thread::sleep(boost::posix_time::milliseconds(20)); // time for call back
    CPPUNIT_ASSERT_EQUAL(0u, calls);
    {
        boost::mutex::scoped_lock lock(handlerDataMutex);
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
        auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode & ec) {
            connectErrorCode.set_value(ec);
        };
        // Not connected yet
        auto connectStatusMap(input->getConnectionStatus());
        CPPUNIT_ASSERT_EQUAL(1ul, connectStatusMap.size());
        CPPUNIT_ASSERT_EQUAL(outputChannelId, connectStatusMap.begin()->first);
        CPPUNIT_ASSERT_MESSAGE(karabo::util::toString(static_cast<int> (connectStatusMap[outputChannelId])),
                               connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::DISCONNECTED);

        // initiate connect and block until done (fail test if timeout))
        input->connect(outputInfo, connectHandler);

        // Now connecting or - with very weird threading - already connected
        connectStatusMap = input->getConnectionStatus();
        CPPUNIT_ASSERT_EQUAL(1ul, connectStatusMap.size());
        CPPUNIT_ASSERT_EQUAL(outputChannelId, connectStatusMap.begin()->first);
        CPPUNIT_ASSERT_MESSAGE(karabo::util::toString(static_cast<int> (connectStatusMap[outputChannelId])),
                               connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::CONNECTING
                               || connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::CONNECTED);

        CPPUNIT_ASSERT_EQUAL_MESSAGE("attempt number " + karabo::util::toString(i),
                                     std::future_status::ready, connectFuture.wait_for(std::chrono::milliseconds(5000)));
        CPPUNIT_ASSERT_EQUAL_MESSAGE("attempt number " + karabo::util::toString(i),
                                     connectFuture.get(), karabo::net::ErrorCode()); // i.e. no error

        // We are connected - check that the status tracker received all steps
        // (rely on order of calls to connection tracker (first) and handler (second) at the end of InputChannel::onConnect
        CPPUNIT_ASSERT(trackedStatus.size() > 0ul);
        CPPUNIT_ASSERT_EQUAL(static_cast<int> (karabo::net::ConnectionStatus::CONNECTING),
                             static_cast<int> (trackedStatus[0]));
        // Without waiting for tracker really being called, this test relies on order of calls to connection
        // tracker and handler given to InputChannel::connect(..) (although might succeed most times even otherwise)
        CPPUNIT_ASSERT(trackedStatus.size() > 1ul);
        CPPUNIT_ASSERT_EQUAL(static_cast<int> (karabo::net::ConnectionStatus::CONNECTED),
                             static_cast<int> (trackedStatus[1]));
        CPPUNIT_ASSERT_EQUAL(2ul, trackedStatus.size()); // i.e. nothing else (yet)!

        // Now ensure that output channel took note of input registration:
        int trials = 200;
        do {
            boost::this_thread::sleep(boost::posix_time::milliseconds(2));
            boost::mutex::scoped_lock lock(handlerDataMutex);
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
        CPPUNIT_ASSERT_MESSAGE(karabo::util::toString(static_cast<int> (connectStatusMap[outputChannelId])),
                               connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::CONNECTED);

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
        connectStatusMap = input->getConnectionStatus();
        CPPUNIT_ASSERT_EQUAL(1ul, connectStatusMap.size());
        CPPUNIT_ASSERT_EQUAL(outputChannelId, connectStatusMap.begin()->first);
        CPPUNIT_ASSERT_MESSAGE(karabo::util::toString(static_cast<int> (connectStatusMap[outputChannelId])),
                               connectStatusMap[outputChannelId] == karabo::net::ConnectionStatus::DISCONNECTED);

        // Some time to travel for message
        trials = 1000; // failed with 200 in https://git.xfel.eu/gitlab/Karabo/Framework/-/jobs/131075/raw
        do {
            boost::this_thread::sleep(boost::posix_time::milliseconds(2));
            boost::mutex::scoped_lock lock(handlerDataMutex);
            if (table.empty() && trackedStatus.size() > 2ul) {
                break;
            }
        } while (--trials >= 0);
        CPPUNIT_ASSERT_EQUAL(0uL, table.size());
        // Also the tracker got informed about disconnection:
        CPPUNIT_ASSERT(trackedStatus.size() > 2ul);
        CPPUNIT_ASSERT_EQUAL(static_cast<int> (karabo::net::ConnectionStatus::DISCONNECTED),
                             static_cast<int> (trackedStatus[2]));
        CPPUNIT_ASSERT_EQUAL(3ul, trackedStatus.size()); // i.e. nothing else!
    }

    // Write data again - input does not anymore receive data.
    output->write(Hash("key", 44));
    output->update();
    // Extended time for callback to be really sure nothing comes.
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    // still 2:
    CPPUNIT_ASSERT_EQUAL(2u, calls);

    //
    // Now test connection attempts that fail
    //
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
        auto connectHandler = [&connectErrorCode](const karabo::net::ErrorCode & ec) {
            connectErrorCode.set_value(ec);
        };
        input->connect(badOutputInfo, connectHandler);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Connection handler not called in time for " + toString(badOutputInfo),
                                     std::future_status::ready, connectFuture.wait_for(std::chrono::milliseconds(5000)));
        CPPUNIT_ASSERT_MESSAGE("Connection did not fail for " + toString(badOutputInfo),
                               connectFuture.get() != karabo::net::ErrorCode()); // not all OK (do not care which problem)
    }
}


void InputOutputChannel_Test::testConcurrentConnect() {
    // To switch on logging output for debugging, do e.g. the following:
    //    karabo::log::Logger::configure(Hash("priority", "DEBUG",
    //                                        // enable timestamps, with ms precision
    //                                        "ostream.pattern", "%d{%F %H:%M:%S,%l} %p  %c  : %m%n"));
    //    karabo::log::Logger::useOstream();

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
        boost::function<void(const karabo::net::ErrorCode&) > connectHandler1 = [&connectPromise1](const karabo::net::ErrorCode & ec) {
            connectPromise1.set_value(ec);
        };
        std::promise<karabo::net::ErrorCode> connectPromise2;
        std::future<karabo::net::ErrorCode> connectFuture2 = connectPromise2.get_future();
        boost::function<void(const karabo::net::ErrorCode&) > connectHandler2 = [&connectPromise2](const karabo::net::ErrorCode & ec) {
            connectPromise2.set_value(ec);
        };
        // Subsequent connect(..): first succeeds, second fails since already connected (less likely) or connecting
        input->connect(outputInfo, connectHandler1);
        input->connect(outputInfo, connectHandler2);
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, connectFuture1.wait_for(std::chrono::milliseconds(5000)));
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, connectFuture2.wait_for(std::chrono::milliseconds(5000)));

        CPPUNIT_ASSERT_EQUAL(karabo::net::ErrorCode(), connectFuture1.get());
        const karabo::net::ErrorCode ec = connectFuture2.get();
        namespace bse = boost::system::errc;
        CPPUNIT_ASSERT_MESSAGE(karabo::util::toString(ec),
                               ec == bse::make_error_code(bse::connection_already_in_progress) || ec == bse::make_error_code(bse::already_connected));

        input->disconnect(outputInfo);

        // Ensure it is disconnected
        CPPUNIT_ASSERT_EQUAL(static_cast<int> (karabo::net::ConnectionStatus::DISCONNECTED),
                             static_cast<int> (input->getConnectionStatus()[outputChannelId]));
        //
        // Now second scenario: disconnect in between two connect attempts:
        //
        // Setup more connection handlers
        std::promise<karabo::net::ErrorCode> connectPromise3;
        std::future<karabo::net::ErrorCode> connectFuture3 = connectPromise3.get_future();
        boost::function<void(const karabo::net::ErrorCode&) > connectHandler3 = [&connectPromise3](const karabo::net::ErrorCode & ec) {
            connectPromise3.set_value(ec);
        };
        std::promise<karabo::net::ErrorCode> connectPromise4;
        std::future<karabo::net::ErrorCode> connectFuture4 = connectPromise4.get_future();
        boost::function<void(const karabo::net::ErrorCode&) > connectHandler4 = [&connectPromise4](const karabo::net::ErrorCode & ec) {
            connectPromise4.set_value(ec);
        };

        input->connect(outputInfo, connectHandler3);
        input->disconnect(outputInfo);
        input->connect(outputInfo, connectHandler4);

        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, connectFuture3.wait_for(std::chrono::milliseconds(5000)));
        CPPUNIT_ASSERT_EQUAL(std::future_status::ready, connectFuture4.wait_for(std::chrono::milliseconds(5000)));

        // Now it is not exactly clear what to expect - depends on timing of threads:
        // - 1st fails as operation_canceled, 2nd succeeds, i.e. disconnect(..) clears from "being setup"
        // - 1st succeeds and 2nd succeeds, i.e. disconnect(..) got called (and fully succeeded!) when 1st connect(..) already succeeded
        const karabo::net::ErrorCode ec1 = connectFuture3.get();
        const karabo::net::ErrorCode ec2 = connectFuture4.get();

        CPPUNIT_ASSERT_MESSAGE("1: " + karabo::util::toString(ec1) += ", 2: " + karabo::util::toString(ec2),
                               (ec1 == bse::make_error_code(bse::operation_canceled) && ec2 == karabo::net::ErrorCode())
                               || (ec1 == karabo::net::ErrorCode() && ec2 == karabo::net::ErrorCode()));
    }
}
