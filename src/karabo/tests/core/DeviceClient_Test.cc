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
 * File:   DevcieClient_Test.cc
 * Author: flucke
 *
 * Created on August 24, 2017, 9:49 AM
 */

#include "DeviceClient_Test.hh"

#include <boost/algorithm/string/case_conv.hpp>
#include <chrono>
#include <future>
#include <karabo/core/DeviceClient.hh>
#include <string>
#include <thread>
#include <tuple>

#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/NDArray.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/xms/ImageData.hh"
#include "karabo/xms/InputChannel.hh"
#include "karabo/xms/SignalSlotable.hh"

#define KRB_TEST_MAX_TIMEOUT 10 // not aware of a failure with 5 here, but in other tests

CPPUNIT_TEST_SUITE_REGISTRATION(DeviceClient_Test);

using namespace karabo::core;
using namespace karabo::data;
using karabo::xms::ImageData;
using karabo::xms::InputChannel;
using karabo::xms::SignalSlotable;


const int maxIterWait = 2000;
const int sleepPerIter = 5;

template <class Container>
void assertIgnoringOrder(const Container& expected, const Container& actual, const std::string& which) {
    CPPUNIT_ASSERT_EQUAL_MESSAGE(which, expected.size(), actual.size());

    for (auto itExpected = expected.cbegin(); itExpected != expected.cend(); ++itExpected) {
        CPPUNIT_ASSERT_MESSAGE(which + "." + *itExpected,
                               std::find(actual.cbegin(), actual.cend(), *itExpected) != actual.cend());
    }
}

DeviceClient_Test::DeviceClient_Test() {}


DeviceClient_Test::~DeviceClient_Test() {}


void DeviceClient_Test::setUp() {
    // uncomment this if ever testing against a local broker
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);
    // Event loop is started in coreTestRunner.cc's main()

    const Hash config("serverId", "testServerDeviceClient", "log.level", "FATAL");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();

    // Create client
    m_deviceClient = std::shared_ptr<DeviceClient>(new DeviceClient());
}


void DeviceClient_Test::tearDown() {
    m_deviceClient.reset();
    m_deviceServer.reset();
}


void DeviceClient_Test::testAll() {
    // A single test to reduce setup/teardown time
    testConcurrentInitTopology();
    // testGet() and testSet() in that order - to avoid the need to instantiate again
    testGet();
    testSet();
    testProperServerSignalsSent();
    testMonitorChannel();
    testGetSchema();
    testGetSchemaNoWait();
    testConnectionHandling();
    testCurrentlyExecutableCommands();
    testSlotsWithArgs();
}


void DeviceClient_Test::testConcurrentInitTopology() {
    std::clog << "testConcurrentInitTopology:" << std::flush;
    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          "testServerDeviceClient", "PropertyTest", Hash("deviceId", "TestedDevice"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Calls DeviceClient::getDevices and returns the elapsed time, in milliseconds, for
    // the call to complete, a vector with the device names and the id of the thread
    // that executed the call.
    auto getDeviceWorker = [this]() -> std::tuple<unsigned, std::vector<std::string>, std::jthread::id> {
        const auto startTimePoint = std::chrono::high_resolution_clock::now();

        const std::vector<std::string> devices = this->m_deviceClient->getDevices();

        const auto finishTimePoint = std::chrono::high_resolution_clock::now();
        const auto duration = finishTimePoint - startTimePoint;

        unsigned elapsedTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

        return make_tuple(elapsedTimeMs, devices, std::this_thread::get_id());
    };

    // Dispatches two calls to getDeviceWorker, each in a different thread and then asserts on the results.
    auto getDev1 = std::async(std::launch::async, getDeviceWorker);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    auto getDev2 = std::async(std::launch::async, getDeviceWorker);

    const auto getDev1Result = getDev1.get();
    const auto getDev2Result = getDev2.get();

    // Checks that the first getDevices execution has taken around 2 secs (the length of the sleep interval
    // in SignalSlotable::getAvailableInstances for gathering slotPing replies) and ...
    const unsigned getDev1Time = std::get<0>(getDev1Result);
    CPPUNIT_ASSERT(getDev1Time > 1600u && getDev1Time <= 2400u);

    // ... the second getDevices execution has taken around 1 sec (time between the second getDevices call is made and
    // the conclusion of the first call) and ...
    const unsigned getDev2Time = std::get<0>(getDev2Result);
    CPPUNIT_ASSERT(getDev2Time > 600u && getDev2Time <= 1400u);

    // ... should have been executed in different threads and ...
    const std::jthread::id getDev1ThreadId = std::get<2>(getDev1Result);
    const std::jthread::id getDev2ThreadId = std::get<2>(getDev2Result);
    CPPUNIT_ASSERT(getDev1ThreadId != getDev2ThreadId);

    // ... should return the same list of devices.
    const std::vector<std::string> getDev1Devices = std::get<1>(getDev1Result);
    const std::vector<std::string> getDev2Devices = std::get<1>(getDev2Result);
    bool resultsEqual = (getDev1Devices.size() == getDev2Devices.size());
    if (resultsEqual) {
        for (std::size_t i = 0; i < getDev1Devices.size(); i++) {
            if (getDev1Devices[i] != getDev2Devices[i]) {
                resultsEqual = false;
                break;
            }
        }
    }
    CPPUNIT_ASSERT(resultsEqual);

    success = m_deviceClient->killDevice("TestedDevice", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    std::clog << " OK" << std::endl;
}


void DeviceClient_Test::testGet() {
    std::clog << "testGet:" << std::flush;
    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          "testServerDeviceClient", "PropertyTest", Hash("deviceId", "TestedDevice"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // int as normal type - test both get, i.e. return by reference argument or return by value
    int intProperty = 0;
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get<int>("TestedDevice", "int32Property", intProperty));
    CPPUNIT_ASSERT_EQUAL(32000000, intProperty);
    intProperty = 0;
    CPPUNIT_ASSERT_NO_THROW(intProperty = m_deviceClient->get<int>("TestedDevice", "int32Property"));
    CPPUNIT_ASSERT_EQUAL(32000000, intProperty);

    // State is specially treated: Internally it is a string, but that should stay internal!
    State state(State::UNKNOWN);
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get<State>("TestedDevice", "state", state));
    CPPUNIT_ASSERT(state != State::UNKNOWN); // still INIT or very likely NORMAL
    state = State::UNKNOWN;
    CPPUNIT_ASSERT_NO_THROW(state = m_deviceClient->get<State>("TestedDevice", "state"));
    CPPUNIT_ASSERT(state != State::UNKNOWN);
    //
    std::string dummy;
    CPPUNIT_ASSERT_THROW(m_deviceClient->get<std::string>("TestedDevice", "state", dummy), ParameterException);
    CPPUNIT_ASSERT_THROW(dummy = m_deviceClient->get<std::string>("TestedDevice", "state"), ParameterException);

    // The same for AlarmConditionState
    AlarmCondition alarm(AlarmCondition::ALARM);
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get<AlarmCondition>("TestedDevice", "alarmCondition", alarm));
    CPPUNIT_ASSERT(alarm == AlarmCondition::NONE); // no alarm on device!
    alarm = AlarmCondition::ALARM;
    CPPUNIT_ASSERT_NO_THROW(alarm = m_deviceClient->get<AlarmCondition>("TestedDevice", "alarmCondition"));
    CPPUNIT_ASSERT(alarm == AlarmCondition::NONE); // no alarm on device!
    //
    CPPUNIT_ASSERT_THROW(m_deviceClient->get<std::string>("TestedDevice", "alarmCondition", dummy), ParameterException);
    CPPUNIT_ASSERT_THROW(dummy = m_deviceClient->get<std::string>("TestedDevice", "alarmCondition"),
                         ParameterException);

    // No shutdown - done in following testSet
    //    success = m_deviceClient->killDevice("TestedDevice", KRB_TEST_MAX_TIMEOUT);
    //    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    std::clog << " OK" << std::endl;
}


void DeviceClient_Test::testSet() {
    std::clog << "testGet:" << std::flush;
    // CPPUNIT_ASSERT_EQUAL(true, m_deviceClient->get<bool>("TestedDevice", "archive"));
    // Cannot reconfigure non-reconfigurable parameters - here caught already by client
    CPPUNIT_ASSERT_THROW(m_deviceClient->set("TestedDevice", "archive", false), karabo::data::ParameterException);

    std::pair<bool, std::string> success = m_deviceClient->killDevice("TestedDevice", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    std::clog << " OK" << std::endl;
}

void DeviceClient_Test::testProperServerSignalsSent() {
    auto client = std::shared_ptr<DeviceClient>(new DeviceClient("device_client-plugin_tests"));

    auto plugins_promise = std::make_shared<std::promise<bool>>();
    std::future<bool> plugins_future = plugins_promise->get_future();

    client->registerInstanceNewMonitor([plugins_promise](const Hash& hash) -> void {
        std::vector<std::string> plugins =
              hash.getAttribute<std::vector<std::string>>("server.device_server-plugin_tests", "deviceClasses");
        plugins_promise->set_value(!plugins.empty());
    });

    auto instance_update_called = std::make_shared<std::atomic<bool>>(false);
    client->registerInstanceUpdatedMonitor([instance_update_called](const Hash& entry) {
        std::clog << "Unexpected entry received in instanceUpdateMonitor: " << entry << std::endl;
        instance_update_called->exchange(true);
    });

    const Hash config("serverId", "device_server-plugin_tests", "log.level", "FATAL");
    auto server = DeviceServer::create("DeviceServer", config);
    server->finalizeInternalInitialization();

    auto result = plugins_future.wait_for(std::chrono::seconds{KRB_TEST_MAX_TIMEOUT});
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, result);
    bool plugins_loaded = plugins_future.get();
    CPPUNIT_ASSERT(plugins_loaded);

    // This wait gives enough time for the instanceUpdate signal to be called,
    // in case it is actually called (which is what we have to make sure it does
    // not happen)
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    CPPUNIT_ASSERT(!(*instance_update_called));
}

void DeviceClient_Test::testMonitorChannel() {
    std::clog << "testMonitorChannel:" << std::flush;
    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          "testServerDeviceClient", "PropertyTest", Hash("deviceId", "TestedDevice2"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Cannot unregister if nothing is registered
    CPPUNIT_ASSERT(!m_deviceClient->unregisterChannelMonitor("TestedDevice2", "output")); // existing channel
    CPPUNIT_ASSERT(
          !m_deviceClient->unregisterChannelMonitor("TestedDevice2", "notExistingoutput"));   // non-existing channel
    CPPUNIT_ASSERT(!m_deviceClient->unregisterChannelMonitor("nonExistingDevice", "output")); // non-existing device

    // register data handler
    int int32inChannel = -1;
    std::string strInChannel("not a number");
    std::vector<long long> vecInt64inChannel;
    Dims ndArrayDims;
    float ndArrayEntry = -1.f;
    Dims imageDims;
    unsigned short imageEntry = 0;
    auto dataHandler = [&int32inChannel, &strInChannel, &vecInt64inChannel, &ndArrayDims, &ndArrayEntry, &imageDims,
                        &imageEntry](const Hash& data, const InputChannel::MetaData& metaData) {
        data.get("node.int32", int32inChannel);
        data.get("node.string", strInChannel);
        data.get("node.vecInt64", vecInt64inChannel);
        const NDArray& ndArray = data.get<NDArray>("node.ndarray");
        ndArrayDims = ndArray.getShape();
        if (ndArray.size() > 0) {
            ndArrayEntry = ndArray.getData<float>()[0];
        }
        const ImageData& image = data.get<ImageData>("node.image");
        imageDims = image.getDimensions();
        imageEntry = image.getData().getData<unsigned short>()[0];
    };

    std::promise<karabo::net::ConnectionStatus> trackerPromise;
    auto trackerFuture = trackerPromise.get_future();
    auto connectionTracker = [&trackerPromise](karabo::net::ConnectionStatus status) {
        // Should first receive CONNECTING and then CONNECTED - ignore the first (tested in InputOutputChannel_Test).
        if (status != karabo::net::ConnectionStatus::CONNECTING) {
            trackerPromise.set_value(status);
        }
    };

    karabo::core::DeviceClient::InputChannelHandlers handlers;
    handlers.dataHandler = dataHandler;
    handlers.statusTracker = connectionTracker;
    CPPUNIT_ASSERT(m_deviceClient->registerChannelMonitor("TestedDevice2:output", handlers));
    // not allowed to register again for same channel
    CPPUNIT_ASSERT(!m_deviceClient->registerChannelMonitor("TestedDevice2:output", dataHandler));

    // Check that we are connected:
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, trackerFuture.wait_for(std::chrono::seconds(KRB_TEST_MAX_TIMEOUT)));
    CPPUNIT_ASSERT_EQUAL(static_cast<int>(karabo::net::ConnectionStatus::CONNECTED),
                         static_cast<int>(trackerFuture.get()));

    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestedDevice2", "writeOutput", KRB_TEST_MAX_TIMEOUT));

    int counter = 0;
    while (counter++ < maxIterWait) { // failed with 100 in https://git.xfel.eu/Karabo/Framework/-/jobs/144940
        if (imageEntry == 1) break;   // Check the last variable assigned in dataHandler
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepPerIter));
    }
    // Now check all data arrived as it should:
    CPPUNIT_ASSERT_EQUAL(1, int32inChannel);
    CPPUNIT_ASSERT_EQUAL(std::string("1"), strInChannel);
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(100u), vecInt64inChannel.size());
    CPPUNIT_ASSERT_EQUAL(1ll, vecInt64inChannel[0]);
    CPPUNIT_ASSERT(ndArrayDims == Dims(100ull, 200ull));
    // Float comparison can fail, see e.g. https://git.xfel.eu/Karabo/Framework/-/jobs/26996
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1., ndArrayEntry, 1.e-7);
    CPPUNIT_ASSERT(imageDims == Dims(400ull, 500ull));
    CPPUNIT_ASSERT_EQUAL(static_cast<unsigned short>(1), imageEntry);

    // unregister and trigger channel again
    CPPUNIT_ASSERT(m_deviceClient->unregisterChannelMonitor("TestedDevice2", "output"));
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestedDevice2", "writeOutput", KRB_TEST_MAX_TIMEOUT));
    // Give some time to any data that would travel - though there is none... Any way around this sleep?
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // Since writing output is not monitored, int32inChannel stays as it is, i.e. we miss the '2'
    CPPUNIT_ASSERT_EQUAL(1, int32inChannel);

    // Register again and trigger channel once more
    // To be sure to go on when connection established, first reset promise/future of connectionTracker
    // (which is a member of 'handlers')
    trackerPromise = std::promise<karabo::net::ConnectionStatus>();
    trackerFuture = trackerPromise.get_future();
    CPPUNIT_ASSERT(m_deviceClient->registerChannelMonitor("TestedDevice2:output", handlers));
    // Check that we are connected (CI failed once with just waiting 50 ms):
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, trackerFuture.wait_for(std::chrono::seconds(KRB_TEST_MAX_TIMEOUT)));
    CPPUNIT_ASSERT_EQUAL(static_cast<int>(karabo::net::ConnectionStatus::CONNECTED),
                         static_cast<int>(trackerFuture.get()));

    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestedDevice2", "writeOutput", KRB_TEST_MAX_TIMEOUT));

    // Now should get the next number, i.e. 3
    counter = 0;
    while (counter++ < maxIterWait) {
        if (int32inChannel == 3) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepPerIter));
    }
    CPPUNIT_ASSERT_EQUAL(3, int32inChannel);

    // Now kill and re-instantiate sender device - should automatically reconnect.
    success = m_deviceClient->killDevice("TestedDevice2", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    success = m_deviceClient->instantiate("testServerDeviceClient", "PropertyTest", Hash("deviceId", "TestedDevice2"),
                                          KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    int32inChannel = -1;
    counter = 0;
    while (counter++ < maxIterWait) {
        if (int32inChannel > 0) break; // see comment below
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestedDevice2", "writeOutput", KRB_TEST_MAX_TIMEOUT));
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepPerIter));
    }
    // Do not care about the exact value of int32inChannel since auto-reconnect might take time
    CPPUNIT_ASSERT(int32inChannel > 0);
    CPPUNIT_ASSERT(int32inChannel <= counter);

    // Test InputHandler
    success = m_deviceClient->instantiate("testServerDeviceClient", "PropertyTest", Hash("deviceId", "TestedDevice3"),
                                          KRB_TEST_MAX_TIMEOUT);

    // Re-use the above InputChannelHandlers object ('handlers'), now with input instead of data handler
    handlers.dataHandler = nullptr;
    int sizeMsg = 0;
    int dataCounter = 0;
    handlers.inputHandler = [&dataCounter, &sizeMsg](const InputChannel::Pointer& channel) {
        sizeMsg = channel->size();
        const Hash::Pointer& data = channel->read(0);
        data->get("node.int32", dataCounter);
    };
    // We re-use the handlers.statusTracker, but have to reset promise and future
    trackerPromise = std::promise<karabo::net::ConnectionStatus>();
    trackerFuture = trackerPromise.get_future();
    CPPUNIT_ASSERT(m_deviceClient->registerChannelMonitor(
          "TestedDevice3:output", handlers,
          Hash("onSlowness", "wait"))); // "wait" excludes data loss (default is "drop")

    // Take care that connection is established before sending data
    CPPUNIT_ASSERT_EQUAL(std::future_status::ready, trackerFuture.wait_for(std::chrono::seconds(KRB_TEST_MAX_TIMEOUT)));
    CPPUNIT_ASSERT_EQUAL(static_cast<int>(karabo::net::ConnectionStatus::CONNECTED),
                         static_cast<int>(trackerFuture.get()));

    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestedDevice3", "writeOutput", KRB_TEST_MAX_TIMEOUT));
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestedDevice3", "writeOutput", KRB_TEST_MAX_TIMEOUT));
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestedDevice3", "writeOutput", KRB_TEST_MAX_TIMEOUT));

    counter = 0;
    while (counter++ < maxIterWait) {
        if (dataCounter == 3) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepPerIter));
    }
    CPPUNIT_ASSERT_EQUAL(3, dataCounter);
    CPPUNIT_ASSERT_EQUAL(1, sizeMsg);

    // Final clean-up
    CPPUNIT_ASSERT(m_deviceClient->unregisterChannelMonitor("TestedDevice3:output"));
    success = m_deviceClient->killDevice("TestedDevice2", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    success = m_deviceClient->killDevice("TestedDevice3", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    std::clog << " OK" << std::endl;
}


void DeviceClient_Test::testGetSchema() {
    std::clog << "testGetSchema:" << std::flush;
    // NOTE:
    // The deviceId needs to be another one than in the other tests, otherwise the test might succeed
    // even if the DeviceClient does not trigger to connect to schema updates: The registration that is
    // triggered by DeviceClient::get in 'testGet()' could still be valid.
    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          "testServerDeviceClient", "PropertyTest", Hash("deviceId", "TestedDevice3"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Check initial maxSize of one exemplary vector
    Schema schema(m_deviceClient->getDeviceSchema("TestedDevice3"));
    CPPUNIT_ASSERT(schema.has("vectors.floatProperty"));
    CPPUNIT_ASSERT(schema.hasMaxSize("vectors.floatProperty"));
    CPPUNIT_ASSERT_EQUAL(10u, schema.getMaxSize("vectors.floatProperty"));

    // Now update maxSize - this should trigger the signaling of an updated Schema and the client
    // should be informed since it should be "connected".
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestedDevice3", "slotUpdateSchema", KRB_TEST_MAX_TIMEOUT));

    schema = m_deviceClient->getDeviceSchema("TestedDevice3");
    CPPUNIT_ASSERT(schema.has("vectors.floatProperty"));
    CPPUNIT_ASSERT(schema.hasMaxSize("vectors.floatProperty"));
    CPPUNIT_ASSERT_EQUAL(20u, schema.getMaxSize("vectors.floatProperty"));

    // Final clean-up
    success = m_deviceClient->killDevice("TestedDevice3", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    std::clog << " OK" << std::endl;
}


void DeviceClient_Test::testCurrentlyExecutableCommands() {
    std::clog << "testCurrentlyExecutableCommands:" << std::flush;
    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          "testServerDeviceClient", "PropertyTest", Hash("deviceId", "TestedDevice3_5"), KRB_TEST_MAX_TIMEOUT);

    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    //  Check if the parameter hierarchy can be correctly traversed or it throws a KeyError
    std::vector<std::string> commands;
    CPPUNIT_ASSERT_NO_THROW(commands = m_deviceClient->getCurrentlyExecutableCommands("TestedDevice3_5"));
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(11), commands.size());
    CPPUNIT_ASSERT_EQUAL(std::string("slotClearLock"), commands[0]);

    // Final clean-up
    success = m_deviceClient->killDevice("TestedDevice3_5", KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    std::clog << " OK" << std::endl;
}


void DeviceClient_Test::testSlotsWithArgs() {
    std::clog << "testSlotsWithArgs:" << std::flush;
    const std::string deviceId{"ArgSlotPropTest"};

    // Register slots with args dynamically in a SignalSlotable instance used
    // only by this test case.
    SignalSlotable::Pointer ss = std::make_shared<SignalSlotable>(deviceId);
    auto weakSs = SignalSlotable::WeakPointer(ss);

    std::function<void(int, int)> argSlotMultiply = [weakSs](int num1, int num2) {
        auto shrSs = weakSs.lock();
        if (shrSs != nullptr) {
            shrSs->reply(num1 * num2);
        }
    };
    ss->registerSlot<int, int>(argSlotMultiply, "argSlotMultiply");

    std::function<void(int, int)> argSlotDivide = [weakSs](int dividend, int divisor) {
        auto shrSs = weakSs.lock();
        if (shrSs != nullptr) {
            if (divisor == 0) {
                throw KARABO_PARAMETER_EXCEPTION("Integer division by 0 is undefined.");
            }
            shrSs->reply(dividend / divisor, dividend % divisor);
        }
    };
    ss->registerSlot<int, int>(argSlotDivide, "argSlotDivide");

    std::function<void(std::string)> argSlotThreeCases = [weakSs](std::string str) {
        auto shrSs = weakSs.lock();
        if (shrSs != nullptr) {
            std::string strUpp = boost::to_upper_copy(str);
            std::string stdLow = boost::to_lower_copy(str);
            shrSs->reply(strUpp, stdLow, str);
        }
    };
    ss->registerSlot<std::string>(argSlotThreeCases, "argSlotThreeCases");

    std::function<void(short)> argSlotZahlen = [weakSs](short digit) {
        auto shrSs = weakSs.lock();
        if (shrSs != nullptr) {
            using VDigit = std::vector<std::string>;
            VDigit ptBR{"Zero", "Um", "Dois", "Três", "Quatro", "Cinco", "Seis", "Sete", "Oito", "Nove"};
            VDigit enUS{"Zero", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine"};
            VDigit deDE{"Null", "Eins", "Zwei", "Drei", "Vier", "Fünf", "Sechs", "Sieben", "Acht", "Neun"};
            VDigit roman{"? (Romans didn't have zero)", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX"};
            int i = digit % 10;
            shrSs->reply(ptBR[i], enUS[i], deDE[i], roman[i]);
        }
    };
    ss->registerSlot<short>(argSlotZahlen, "argSlotZahlen");

    int productCaptMult = 0;
    std::function<void(int, int)> argSlotCaptMultiply = [&productCaptMult](int num1, int num2) {
        productCaptMult = num1 * num2;
    };
    ss->registerSlot<int, int>(argSlotCaptMultiply, "argSlotCaptMultiply");

    ss->start();

    // Calls a slot with two int args and no return value - it modifies a local variable.
    m_deviceClient->execute(deviceId, "argSlotCaptMultiply", KRB_TEST_MAX_TIMEOUT, 12, 4);
    CPPUNIT_ASSERT_EQUAL(48, productCaptMult);

    // Calls a slot with two int args and one int return value.
    int product = m_deviceClient->execute1<int, int, int>(deviceId, "argSlotMultiply", KRB_TEST_MAX_TIMEOUT, 4, 5);
    CPPUNIT_ASSERT_EQUAL(20, product);

    // Calls a slot with two int args and a return value consisting of a tuple of two int values.
    std::tuple<int, int> divRes =
          m_deviceClient->execute2<int, int, int, int>(deviceId, "argSlotDivide", KRB_TEST_MAX_TIMEOUT, 5, 2);
    CPPUNIT_ASSERT_EQUAL(2, std::get<0>(divRes)); // quotient
    CPPUNIT_ASSERT_EQUAL(1, std::get<1>(divRes)); // remainder

    // Calls a slot with a string arg and a return value consisting of a tuple with that string in three
    // variants regarding casing.
    std::tuple<std::string, std::string, std::string> strRes =
          m_deviceClient->execute3<std::string, std::string, std::string, std::string>(
                deviceId, "argSlotThreeCases", KRB_TEST_MAX_TIMEOUT, "a StRING!");
    CPPUNIT_ASSERT_EQUAL(std::string{"A STRING!"}, std::get<0>(strRes));
    CPPUNIT_ASSERT_EQUAL(std::string{"a string!"}, std::get<1>(strRes));
    CPPUNIT_ASSERT_EQUAL(std::string{"a StRING!"}, std::get<2>(strRes));

    // Calls a slot with an int arg and a return value consisting of a tuple with 4 elements.
    std::tuple<std::string, std::string, std::string, std::string> locRes =
          m_deviceClient->execute4<std::string, std::string, std::string, std::string, short>(
                deviceId, "argSlotZahlen", KRB_TEST_MAX_TIMEOUT, short(4));
    CPPUNIT_ASSERT_EQUAL(std::string{"Quatro"}, std::get<0>(locRes));
    CPPUNIT_ASSERT_EQUAL(std::string{"Four"}, std::get<1>(locRes));
    CPPUNIT_ASSERT_EQUAL(std::string{"Vier"}, std::get<2>(locRes));
    CPPUNIT_ASSERT_EQUAL(std::string{"IV"}, std::get<3>(locRes));

    std::clog << " OK" << std::endl;
}


void DeviceClient_Test::testGetSchemaNoWait() {
    std::clog << "testGetSchemaNoWait: " << std::flush;
    // NOTE: Better use new id, see comment in testGetSchema.
    const std::string deviceId("TestedDevice4");
    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          "testServerDeviceClient", "PropertyTest", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Add handler that will be called when schema arrives when triggered by getDeviceSchemaNoWait
    bool schemaReceived = false;
    auto handler = [&schemaReceived, deviceId](const std::string& id, const karabo::data::Schema& schema) {
        if (id == deviceId) schemaReceived = true;
    };
    m_deviceClient->registerSchemaUpdatedMonitor(handler);

    Schema schema(m_deviceClient->getDeviceSchemaNoWait(deviceId));
    // noWait and first request: nothing cached yet, so still empty
    CPPUNIT_ASSERT(schema.empty());

    // Wait a bit until schema arrived and thus handler is called
    unsigned int counter = 0;
    while (counter++ < maxIterWait) {
        if (schemaReceived) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepPerIter));
    }
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for schema update monitor", schemaReceived);
    // Now take from cache
    schema = m_deviceClient->getDeviceSchemaNoWait(deviceId);
    // Check initial maxSize of one exemplary vector
    CPPUNIT_ASSERT(schema.has("vectors.floatProperty"));
    CPPUNIT_ASSERT(schema.hasMaxSize("vectors.floatProperty"));
    CPPUNIT_ASSERT_EQUAL(10u, schema.getMaxSize("vectors.floatProperty"));

    // Now update maxSize - this should trigger the signaling of an updated Schema and the client
    // should be informed since it should be "connected".
    schemaReceived = false;
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(deviceId, "slotUpdateSchema", KRB_TEST_MAX_TIMEOUT));

    // Wait a bit until new schema arrived
    counter = 0;
    while (counter++ < maxIterWait) {
        if (schemaReceived) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepPerIter));
    }
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for schema update monitor", schemaReceived);

    // Now take from cache
    schema = m_deviceClient->getDeviceSchemaNoWait(deviceId);
    CPPUNIT_ASSERT(schema.has("vectors.floatProperty"));
    CPPUNIT_ASSERT(schema.hasMaxSize("vectors.floatProperty"));
    CPPUNIT_ASSERT_EQUAL(20u, schema.getMaxSize("vectors.floatProperty"));

    // Final clean-up
    success = m_deviceClient->killDevice(deviceId, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    std::clog << " OK" << std::endl;
}


void DeviceClient_Test::testConnectionHandling() {
    std::clog << "testConnectionHandling:" << std::flush;
    const std::string serverId("testServerDeviceClient");
    const std::string devId("TestedDevice");
    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(serverId, "PropertyTest", Hash("deviceId", devId), KRB_TEST_MAX_TIMEOUT);
    ////////////////////////////////////////////////////////////////////
    // Test 1)
    // We check that we can get the configuration and a single property
    ////////////////////////////////////////////////////////////////////
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    CPPUNIT_ASSERT_EQUAL(32000000, m_deviceClient->get<int>(devId, "int32Property"));
    // Store all paths to cross check later full configurations:
    std::vector<std::string> paths;
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(devId).getPaths(paths));
    const std::vector<std::string> allPaths(paths);

    ////////////////////////////////////////////////////////////////////
    // Test 2)
    // We kill and restart the device and redo the test to check caching
    // and cache cleaning
    ////////////////////////////////////////////////////////////////////
    success = m_deviceClient->killDevice(devId, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Device not there, so timeout
    CPPUNIT_ASSERT_THROW(m_deviceClient->get(devId), karabo::data::TimeoutException);
    success = m_deviceClient->instantiate(serverId, "PropertyTest", Hash("deviceId", devId, "int32Property", 64000000),
                                          KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    CPPUNIT_ASSERT_EQUAL(64000000, m_deviceClient->get<int>(devId, "int32Property"));

    // Now check that still all paths are there
    paths.clear();
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(devId).getPaths(paths));
    assertIgnoringOrder(allPaths, paths, "killRestart");

    ////////////////////////////////////////////////////////////////////
    // Test 3)
    // We check whether the same as in test 2 works also in case a
    // monitor was registered which makes the device an immortal within
    // the device client and - after the device is shut down, even a
    // "zombie".
    ////////////////////////////////////////////////////////////////////
    bool cfgArrived = false;
    auto deviceMonitor = [&cfgArrived](const std::string&, const karabo::data::Hash&) { cfgArrived = true; };
    m_deviceClient->registerDeviceMonitor(devId, deviceMonitor);
    // TODO: Waiting should not be needed. For the very likely reason I need it here,
    // see DeviceClient::_slotChanged and DeviceClient::killDevice.
    int trials = 100;
    while (--trials >= 0) {
        if (cfgArrived) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    // Kill the device again - within client it shall be a zombie now
    success = m_deviceClient->killDevice(devId, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Device not there again, so timeout
    CPPUNIT_ASSERT_THROW(m_deviceClient->get(devId), karabo::data::TimeoutException);

    // Restart device again with a changed property
    success = m_deviceClient->instantiate(serverId, "PropertyTest", Hash("deviceId", devId, "int32Property", -32000000),
                                          KRB_TEST_MAX_TIMEOUT);
    // Check again all paths and the single property
    Hash config;
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(devId, config));
    paths.clear();
    config.getPaths(paths);
    assertIgnoringOrder(allPaths, paths, "zombie");
    CPPUNIT_ASSERT_EQUAL(-32000000, config.get<int>("int32Property"));

    ////////////////////////////////////////////////////////////////////
    // Test 4)
    // Similar test content - but this time unregister the monitor to
    // get rid of the zombie before re-instantiation and checking again
    ////////////////////////////////////////////////////////////////////
    success = m_deviceClient->killDevice(devId, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    // Now device state in client is "zombie"

    // stop monitoring, i.e. kill zombie
    m_deviceClient->unregisterDeviceMonitor(devId);

    // Restart device again with a changed property
    success = m_deviceClient->instantiate(serverId, "PropertyTest", Hash("deviceId", devId, "int32Property", -64000000),
                                          KRB_TEST_MAX_TIMEOUT);

    // Check once more all paths and the single property
    config.clear();
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(devId, config));
    paths.clear();
    config.getPaths(paths);
    assertIgnoringOrder(allPaths, paths, "killedZombie");
    CPPUNIT_ASSERT_EQUAL(-64000000, config.get<int>("int32Property"));

    std::clog << " OK" << std::endl;
}
