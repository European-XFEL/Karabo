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
        outputs[i] = Configurator<OutputChannel>::create("OutputChannel", Hash());
        outputs[i]->setInstanceIdAndName("outputChannel", channelId);
        outputIds[i] = outputs[i]->getInstanceId() + ":" + channelId;
    }

    // Setup input channel
    const Hash cfg("connectedOutputChannels", outputIds);
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
                                     connectFuture.wait_for(std::chrono::milliseconds(500)));
        CPPUNIT_ASSERT_EQUAL_MESSAGE("attempt for " + outputIds[i],
                                     connectFuture.get(), karabo::net::ErrorCode()); // i.e. no error
    } // all connected


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
    int trials = 1000;
    do {
        boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        if (nReceivedEos > 0) break;
    } while (--trials >= 0);

    // endOfStream received once
    // We give some time for more to arrive - but there should only be one, although each output sent it!
    boost::this_thread::sleep(boost::posix_time::milliseconds(200));
    CPPUNIT_ASSERT_EQUAL(1u, static_cast<unsigned int> (nReceivedEos));

    // Proper number and order of data received from each output
    for (size_t i = 0; i < outputIds.size(); ++i) {
        const auto& data = receivedData.get<std::vector<unsigned int>>(outputIds[i]);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(outputIds[i], numData, data.size());
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
    OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel", Hash());
    output->setInstanceIdAndName("outputChannel", "output");
    std::vector<karabo::util::Hash> table;
    boost::mutex tableMutex;
    output->registerShowConnectionsHandler([&table, &tableMutex](const std::vector<karabo::util::Hash>& connections) {
        boost::mutex::scoped_lock lock(tableMutex);
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

    // Write first data - nobody connected yet.
    output->write(Hash("key", 42));
    output->update();
    boost::this_thread::sleep(boost::posix_time::milliseconds(20)); // time for call back
    CPPUNIT_ASSERT_EQUAL(0u, calls);
    {
        boost::mutex::scoped_lock lock(tableMutex);
        CPPUNIT_ASSERT_EQUAL(0uL, table.size());
    }

    // Connect
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
        CPPUNIT_ASSERT_EQUAL_MESSAGE("attempt number " + karabo::util::toString(i),
                                     std::future_status::ready, connectFuture.wait_for(std::chrono::milliseconds(500)));
        CPPUNIT_ASSERT_EQUAL_MESSAGE("attempt number " + karabo::util::toString(i),
                                     connectFuture.get(), karabo::net::ErrorCode()); // i.e. no error

        // Now ensure that output channel took note of input registration:
        int trials = 200;
        do {
            boost::this_thread::sleep(boost::posix_time::milliseconds(2));
            boost::mutex::scoped_lock lock(tableMutex);
            if (!table.empty()) {
                break;
            }
        } while (--trials >= 0);
        // No further call back, so no need to lock tableMutex here
        CPPUNIT_ASSERT_EQUAL(1UL, table.size());
        // ... and check the published connection information
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
        trials = 1000; // failed with 200 in https://git.xfel.eu/gitlab/Karabo/Framework/-/jobs/131075/raw
        do {
            boost::this_thread::sleep(boost::posix_time::milliseconds(2));
            boost::mutex::scoped_lock lock(tableMutex);
            if (table.empty()) {
                break;
            }
        } while (--trials >= 0);
        CPPUNIT_ASSERT_EQUAL(0uL, table.size());
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
                                     std::future_status::ready, connectFuture.wait_for(std::chrono::milliseconds(1000)));
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

    // Setup output channel
    OutputChannel::Pointer output = Configurator<OutputChannel>::create("OutputChannel", Hash());
    output->setInstanceIdAndName("outputChannel", "output");

    // Setup input channel
    const std::string outputChannelId(output->getInstanceId() + ":output");
    const Hash cfg("connectedOutputChannels", std::vector<std::string>(1, outputChannelId));
    InputChannel::Pointer input = Configurator<InputChannel>::create("InputChannel", cfg);
    input->setInstanceId("inputChannel");

    // Hack taken from InputOutputChannel_LongTest.cc:
    // Wait a little bit until OutputChannel has properly initialised, i.e. a proper port is attached
    // (see its constructor...) FIXME :-(
    Hash outputInfo;
    int trials = 500;
    while (--trials >= 0) {
        outputInfo = output->getInformation();
        if (outputInfo.get<unsigned int>("port") > 0) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(5));
    }
    CPPUNIT_ASSERT_MESSAGE("OutputChannel keeps port 0!", outputInfo.get<unsigned int>("port") > 0);

    outputInfo.set("outputChannelString", outputChannelId);
    outputInfo.set("memoryLocation", "local");

    // Setup connection handlers
    const unsigned int numConnect = 100; // > 5, see below
    std::vector<std::promise<karabo::net::ErrorCode>> connectErrorCodes(numConnect);
    std::vector<std::future<karabo::net::ErrorCode>> connectFutures;
    std::vector<std::function<void(const karabo::net::ErrorCode&)>> connectHandlers;
    for (auto& promise : connectErrorCodes) {
        connectFutures.push_back(promise.get_future());
        connectHandlers.push_back([&promise](const karabo::net::ErrorCode & ec) {
            promise.set_value(ec);
        });
    }

    // Connect all in one go - add threads to increase likelihood of concurrency
    ThreadAdder extraThreads(5);
    unsigned int counter = 0;
    for (auto& handler : connectHandlers) {
        Hash h(outputInfo);
        // Introduce some failure reasons
        if (counter == 0) {
            h.erase("memoryLocation");
        } else if (counter == 1) {
            h.erase("outputChannelString");
        } else if (counter == 2) {
            h.set("connectionType", "udp");
        } else if (counter == 3) {
            h.set("outputChannelString", "no_configured_string");
        }
        // Skip test of invalid port - it seems to be unreliable - maybe just something else accepts the connection?
        //  } else if (counter == 4) {
        //      h.set("port", 0u); // invalid port
        karabo::net::EventLoop::getIOService().post(karabo::util::bind_weak(&InputChannel::connect, input.get(),
                                                                            h, handler));
        ++counter;
    }
    // Wait for handler completion
    counter = 0;
    for (auto& future : connectFutures) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Callback for connection attempt number " + karabo::util::toString(counter++)
                                     += " not called in time",
                                     std::future_status::ready, future.wait_for(std::chrono::milliseconds(1000)));
    }

    // ... check results
    counter = 0;
    for (auto& future : connectFutures) {
        namespace bse = boost::system::errc;
        // The first four are mis-configured and will fail for different reasons.
        // The next should succeed, the following may either be declared as succeeded as well (because they "jumped on"
        // the previous) or fail as "already connected".
        if (counter == 0) { // missing "memoryLocation"
            CPPUNIT_ASSERT_EQUAL(bse::make_error_code(bse::argument_out_of_domain), future.get());
        } else if (counter == 1) { // missing "outputChannelString"
            CPPUNIT_ASSERT_EQUAL(bse::make_error_code(bse::invalid_argument), future.get());
        } else if (counter == 2) { // invalid "connectionType" ("udp")
            CPPUNIT_ASSERT_EQUAL(bse::make_error_code(bse::protocol_not_supported), future.get());
        } else if (counter == 3) { // non-configured "outputChannelString"
            CPPUNIT_ASSERT_EQUAL(bse::make_error_code(bse::argument_out_of_domain), future.get());
        } else if (counter == 4) { // first success (If 'invalid port' is added back above, treat it here!)
            CPPUNIT_ASSERT_EQUAL(karabo::net::ErrorCode(), future.get());
        } else {
            const karabo::net::ErrorCode ec = future.get();
            CPPUNIT_ASSERT_MESSAGE("attempt number " + karabo::util::toString(counter) + ": " + karabo::util::toString(ec),
                                   ec == karabo::net::ErrorCode() || ec == bse::make_error_code(bse::already_connected));
        }
        ++counter;
    }

    // Check that connection actually OK and data goes through
    unsigned int calls = 0;
    input->registerDataHandler([&calls](const Hash& data, const InputChannel::MetaData & meta) {
        ++calls;
    });
    output->write(Hash());
    output->update();

    trials = 500;
    while (--trials >= 0) {
        if (1u == calls) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(2)); // time for callback
    }
    CPPUNIT_ASSERT_EQUAL(1u, calls);

    //
    // Now check disconnect while connecting
    //
    input->disconnect(outputChannelId);
    const unsigned numConnect2 = 10;
    // Re-setup connection handlers
    connectErrorCodes.clear();
    connectErrorCodes.resize(numConnect2);
    connectFutures.clear();
    connectHandlers.clear();
    for (auto& promise : connectErrorCodes) {
        connectFutures.push_back(promise.get_future());
        connectHandlers.push_back([&promise](const karabo::net::ErrorCode & ec) {
            promise.set_value(ec);
        });
    }

    // Launch all the connects and then disconnect
    for (auto& handler : connectHandlers) {
        // Call one after another now to be sure that connect part is done,
        // concurrency will be on the callback onConnect
        input->connect(outputInfo, handler);
    }
    input->disconnect(outputChannelId);

    // Wait for handler completion
    counter = 0;
    for (auto& future : connectFutures) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE("attempt number " + karabo::util::toString(counter++),
                                     std::future_status::ready, future.wait_for(std::chrono::milliseconds(1000)));
    }

    // The actual handler result is hard to predict: Can be
    // - 'operation_canceled' for those that disconnect found as 'being setup'
    // - success for those connect(..) that are actually executed after disconnect
    // - I also saw 'connection_refused'
    // We cannot test either whether now we are really disconnected
    // ... and thus the following test is not predictive either and is thus removed, see e.g.
    //     https://git.xfel.eu/gitlab/Karabo/Framework/-/jobs/82010
    // FIXME for next round (2.8.X)!
    return;

    // So after having checked that all handlers are called we skip their result test,
    // but take care that the connection is gone.
    output->write(Hash());
    output->update();

    // Give some time for data to travel in case connection alive (although it should not).
    boost::this_thread::sleep(boost::posix_time::milliseconds(200));
    // Still 1 as from above
    CPPUNIT_ASSERT_EQUAL(1u, calls);
}
