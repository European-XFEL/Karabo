/*
 * File:   testDevice.cc
 * Author: gero.flucke@xfel.eu
 *
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
 *
 */

#include <gtest/gtest.h>

#include <chrono>
#include <functional>
#include <karabo/core/Device.hh>
#include <karabo/net/EventLoop.hh>
#include <karabo/xms/InputChannel.hh>
#include <karabo/xms/OutputChannel.hh>
#include <karabo/xms/SignalSlotable.hh>
#include <thread>

#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/data/schema/OverwriteElement.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/TableElement.hh"
#include "karabo/data/schema/VectorElement.hh"
#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/time/TimeDuration.hh"
#include "karabo/data/time/Timestamp.hh"
#include "karabo/data/types/Exception.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/data/types/State.hh"
#include "karabo/data/types/StringTools.hh"

#define KRB_TEST_MAX_TIMEOUT \
    10 // larger than the 6 s input channel reconnect interval, for testOutputRecreatesOnSchemaChange


class TestDeviceTest : public ::testing::Test {
   protected:
    TestDeviceTest();
    ~TestDeviceTest() override;
    void SetUp() override;
    void TearDown() override;

    void testInstanceInfoServer();
    void testGetTimestampSystemInfo();
    void testSchemaInjection();
    void testGetconfigReconfig();
    void testLockClearLock();
    void testUpdateState();
    void testSet();
    void testSetVectorUpdate();
    void testSignal();
    void testBadInit();

    /** Tests that updateSchema resets attributes in the static schema. */
    void testSchemaWithAttrUpdate();

    /** Tests that appendSchema preserves attributes in the static schema. */
    void testSchemaWithAttrAppend();

    /** Tests that updateSchema/appendSchema work well for tags, also inside schema of OutputChannel
     *
     * @param updateSlot which DeviceExampleTest slot to change the schema: "slotUpdateSchema" or "slotAppendSchema"
     */
    void testChangeSchemaOutputChannel(const std::string& updateSlot);

    /** Tests that updateSchema/appendSchema that change schema of output channel will trigger a reconnection
     *
     * @param updateSlot which DeviceExampleTest slot to change the schema: "slotUpdateSchema" or "slotAppendSchema"
     */
    void testOutputRecreatesOnSchemaChange(const std::string& updateSlot);

    /**
     * Test that appendSchemaMaxSize properly creates (and destroys) output channels
     */
    void testOutputRecreatesOnMaxSizeChange();

    /** Test that updateSchema/appendSchema properly creates (and destroys) input/output channels
     *
     * @param updateSlot which DeviceExampleTest slot to change the schema: "slotUpdateSchema" or "slotAppendSchema"
     */
    void testInputOutputChannelInjection(const std::string& updateSlot);

    /**
     * Test calling a slot under a node
     */
    void testNodedSlot();

    bool waitForCondition(std::function<bool()> checker, unsigned int timeoutMs);

    karabo::core::DeviceServer::Pointer m_deviceServer;
    karabo::core::DeviceClient::Pointer m_deviceClient;
    std::jthread m_eventLoopThread;
};


using karabo::core::DeviceClient;
using karabo::core::DeviceServer;
using karabo::data::DOUBLE_ELEMENT;
using karabo::data::Epochstamp;
using karabo::data::Hash;
using karabo::data::INT32_ELEMENT;
using karabo::data::NODE_ELEMENT;
using karabo::data::OVERWRITE_ELEMENT;
using karabo::data::Schema;
using karabo::data::State;
using karabo::data::STRING_ELEMENT;
using karabo::data::TABLE_ELEMENT;
using karabo::data::Timestamp;
using karabo::data::UINT32_ELEMENT;
using karabo::data::VECTOR_FLOAT_ELEMENT;
using karabo::data::VECTOR_INT32_ELEMENT;
using karabo::data::VECTOR_STRING_ELEMENT;
using karabo::net::EventLoop;
using karabo::xms::INPUT_CHANNEL;
using karabo::xms::OUTPUT_CHANNEL;
using karabo::xms::SignalSlotable;
using karabo::xms::SLOT_ELEMENT;


static void assertChildNodesEmpty(const Hash& h);

static const std::string fakeClassVersion{"FakePackage-1.2.3"};


class DeviceExampleTest : public karabo::core::Device {
   public:
    KARABO_CLASSINFO(DeviceExampleTest, "DeviceExampleTest", fakeClassVersion)

    static const int LIMIT_HIGH = 1000.0;

    static void expectedParameters(karabo::data::Schema& expected) {
        OVERWRITE_ELEMENT(expected).key("state").setNewOptions(State::UNKNOWN, State::NORMAL, State::ERROR).commit();

        Schema rowSchema;
        STRING_ELEMENT(rowSchema)
              .key("type")
              .displayedName("Type column")
              .description("Type column")
              .assignmentOptional()
              .defaultValue(std::string())
              .reconfigurable()
              .commit();

        STRING_ELEMENT(rowSchema)
              .key("name")
              .displayedName("Name column")
              .description("Name column")
              .assignmentOptional()
              .defaultValue(std::string())
              .reconfigurable()
              .commit();

        TABLE_ELEMENT(expected)
              .key("table")
              .displayedName("Table property")
              .description("Table with two columns")
              .setColumns(rowSchema)
              .assignmentOptional()
              .defaultValue({Hash("type", "INT", "name", "firstLine"), Hash("type", "BOOL", "name", "secondLine")})
              .reconfigurable()
              .commit();

        DOUBLE_ELEMENT(expected)
              .key("valueWithExc")
              .reconfigurable()
              .assignmentOptional()
              .defaultValue(0.0)
              .maxExc(DeviceExampleTest::LIMIT_HIGH)
              .observerAccess()
              .commit();

        INT32_ELEMENT(expected)
              .key("valueWithLimit")
              .assignmentOptional()
              .defaultValue(0)
              .maxExc(1000)
              .reconfigurable()
              .commit();

        INT32_ELEMENT(expected).key("valueOther").readOnly().initialValue(0).commit();

        UINT32_ELEMENT(expected)
              .key("countStateToggles")
              .description("How often slotToggleState was called")
              .readOnly()
              .initialValue(0u)
              .commit();

        VECTOR_STRING_ELEMENT(expected).key("vecString").readOnly().initialValue({"one", "two", "three"}).commit();

        NODE_ELEMENT(expected).key("node").displayedName("Node").commit();

        SLOT_ELEMENT(expected)
              .key("node.slot")
              .displayedName("Slot")
              .description("Device slot under a node, doing nothing")
              .commit();

        INT32_ELEMENT(expected)
              .key("intInOnData")
              .description("What onData received")
              .readOnly()
              .initialValue(0)
              .commit();

        INT32_ELEMENT(expected)
              .key("numCallsOnInput")
              .description("Count calls to onInput")
              .readOnly()
              .initialValue(0)
              .commit();

        // Schema for output channel
        Schema dataSchema;
        NODE_ELEMENT(dataSchema)
              .key("data")
              .displayedName("Data")
              .setDaqDataType(karabo::data::DaqDataType::TRAIN)
              .commit();

        DOUBLE_ELEMENT(dataSchema).key("data.untagged").alias("UNTAGGED").displayedName("Untagged").readOnly().commit();

        VECTOR_FLOAT_ELEMENT(dataSchema)
              .key("data.intensityTD")
              .tags("doocs,pulseResolved")
              .alias("INTENSITY.TD")
              .displayedName("Intensity TD")
              .readOnly()
              .commit();

        VECTOR_INT32_ELEMENT(dataSchema).key("data.vecInt32").displayedName("Vector Int32").readOnly().commit();

        INT32_ELEMENT(dataSchema).key("int").readOnly().commit();

        OUTPUT_CHANNEL(expected).key("output").dataSchema(dataSchema).commit();

        // Schema less input channel...
        INPUT_CHANNEL(expected).key("input").commit();

        // Not channel related, but for test that empty node does not get erased
        NODE_ELEMENT(expected).key("emptyNode").commit();
    }


    DeviceExampleTest(const karabo::data::Hash& input) : karabo::core::Device(input) {
        // Bind to a slot what now is called from deviceServer:
        KARABO_SLOT(slotTimeTick, unsigned long long /*id*/, unsigned long long /*sec*/, unsigned long long /*frac*/,
                    unsigned long long /*period*/)

        KARABO_SLOT(slotIdOfEpochstamp, unsigned long long /*sec*/, unsigned long long /*frac*/)

        KARABO_SLOT(slotUpdateSchema, const karabo::data::Schema);

        KARABO_SLOT(slotAppendSchema, const karabo::data::Schema);

        KARABO_SLOT(slotAppendSchemaMultiMaxSize, unsigned int);

        KARABO_SLOT(slotSet, Hash);

        KARABO_SLOT(slotToggleState, const Hash);

        KARABO_SLOT(node_slot);

        KARABO_SLOT(slotGetCurrentConfiguration, const std::string /*tags*/);

        KARABO_SLOT(slotUpdateVecString, std::vector<std::string>, int);

        KARABO_SIGNAL("signalA");

        KARABO_SLOT(slotEmitSignalA);

        KARABO_SLOT(slotRegisterOnDataInputEos, const std::string /*inputChannelName*/);

        KARABO_SLOT(slotSendToOutputChannel, const std::string /*channelName*/, Hash /*dataToSend*/);

        KARABO_SLOT(slotSendEos, const std::vector<std::string> /*channelNames*/);
    }


    virtual ~DeviceExampleTest() {}


    void slotIdOfEpochstamp(unsigned long long sec, unsigned long long frac) {
        const Timestamp stamp(getTimestamp(Epochstamp(sec, frac)));
        reply(stamp.getTid());
    }


    void slotUpdateSchema(const Schema sch) {
        updateSchema(sch);
    }

    void slotAppendSchema(const Schema sch) {
        appendSchema(sch);
    }


    void slotAppendSchemaMultiMaxSize(unsigned int maxSize) {
        appendSchemaMultiMaxSize({"output.schema.data.intensityTD", "output.schema.data.vecInt32"}, {maxSize, maxSize});
    }


    void slotSet(const Hash& h) {
        set(h);
    }


    void slotToggleState(const Hash otherIn) {
        const Epochstamp& stampCountToggles =
              Epochstamp::fromHashAttributes(otherIn.getAttributes("stampCountToggles"));
        const Epochstamp& stampState = Epochstamp::fromHashAttributes(otherIn.getAttributes("stampState"));

        const State& newState = State::fromString(otherIn.get<std::string>("state"));

        Hash otherOut("valueWithExc", -1.);
        Hash::Attributes& attrs =
              otherOut.set("countStateToggles", get<unsigned int>("countStateToggles") + 1).getAttributes();
        getTimestamp(stampCountToggles).toHashAttributes(attrs);

        // So "state" and "valueWithExc" get timestamp from 'stampState', "countStateToggles" from 'stampCountToggles'
        updateState(newState, otherOut, getTimestamp(stampState));
    }


    void node_slot() {
        // Nothing to do!
    }


    void slotGetCurrentConfiguration(const std::string& tags) {
        reply(getCurrentConfiguration(tags));
    }


    void slotUpdateVecString(const std::vector<std::string>& updates, int updateType) {
        VectorUpdate type = VectorUpdate::add;
        if (updateType == static_cast<int>(VectorUpdate::addIfNotIn)) {
            type = VectorUpdate::addIfNotIn;
        } else if (updateType == static_cast<int>(VectorUpdate::removeOne)) {
            type = VectorUpdate::removeOne;
        } else if (updateType == static_cast<int>(VectorUpdate::removeAll)) {
            type = VectorUpdate::removeAll;
        } else if (updateType != static_cast<int>(type)) {
            std::ostringstream msg;
            msg << "Invalid updateType: " << updateType
                << ". Expect values equivalent to values of the enum VectorUpdate.";
            throw KARABO_PARAMETER_EXCEPTION(msg.str());
        }

        setVectorUpdate("vecString", updates, type, getActualTimestamp());
    }


    void slotEmitSignalA() {
        emit("signalA");
    }


    void slotRegisterOnDataInputEos(const std::string& inputChannelName) {
        KARABO_ON_DATA(inputChannelName, onData);
        KARABO_ON_INPUT(inputChannelName, onInput);
        KARABO_ON_EOS(inputChannelName, onEos);
    }


    void slotSendToOutputChannel(const std::string& channelName, const Hash& dataToSend) {
        writeChannel(channelName, dataToSend);
    }


    void slotSendEos(const std::vector<std::string>& channelNames) {
        for (const auto& channelName : channelNames) {
            signalEndOfStream(channelName);
        }
    }

    void onData(const karabo::data::Hash& data, const karabo::xms::InputChannel::MetaData& meta) {
        int received = -1;
        if (data.has("int")) {
            received = data.get<int>("int");
        }
        set("intInOnData", received);
    }

    void onInput(const karabo::xms::InputChannel::Pointer& /*input*/) {
        const int soFar = get<int>("numCallsOnInput");
        set("numCallsOnInput", soFar + 1);
    }

    void onEos(const karabo::xms::InputChannel::Pointer& /*input*/) {
        const int oldValue = get<int>("intInOnData");
        set("intInOnData", -oldValue); // just flip sign
    }
};


KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::Device, DeviceExampleTest)


// =================================================================================

class TestDeviceBadInit : public karabo::core::Device {
   public:
    KARABO_CLASSINFO(TestDeviceBadInit, "TestDeviceBadInit", "2.9")


    static void expectedParameters(karabo::data::Schema& expected) {
        OVERWRITE_ELEMENT(expected)
              .key("state")
              .setNewOptions(State::UNKNOWN, State::INIT, State::NORMAL)
              .setNewDefaultValue(State::UNKNOWN)
              .commit();

        STRING_ELEMENT(expected)
              .key("initProblem")
              .assignmentMandatory()
              .options(std::vector<std::string>({"throw", "delay"}))
              .commit();

        STRING_ELEMENT(expected)
              .key("callInPredestruction")
              .description("Instance and slot (dot separated) to be called in preDestruction")
              .assignmentOptional()
              .defaultValue(std::string())
              .commit();

        UINT32_ELEMENT(expected).key("delay").assignmentOptional().defaultValue(10u).commit();
    }


    TestDeviceBadInit(const karabo::data::Hash& input) : karabo::core::Device(input) {
        KARABO_INITIAL_FUNCTION(initialize);
    }


    void initialize() {
        updateState(State::INIT);

        const std::string behaviour(get<std::string>("initProblem"));
        if (behaviour == "throw") {
            // This will be caught by the event loop - if logging is enabled, one can see a printout...
            throw KARABO_SIGNALSLOT_EXCEPTION("Throw during initialization - for test purposes!");
        } else if (behaviour == "delay") {
            std::this_thread::sleep_for(std::chrono::seconds(get<unsigned int>("delay")));
        } // No else - there are not other options!

        updateState(State::NORMAL);
    }


    virtual ~TestDeviceBadInit() {}


    void preDestruction() override {
        set("status", "preDestruction called");

        const std::string toCall(get<std::string>("callInPredestruction"));
        if (toCall.empty()) return;

        const size_t dotPos = toCall.find('.');
        if (dotPos != std::string::npos) {
            const std::string instanceId(toCall.substr(0, dotPos));
            const std::string slot(toCall.substr(dotPos + 1));

            request(instanceId, slot).timeout(5000).receive();
        }
    }
};

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::Device, TestDeviceBadInit)


TestDeviceTest::TestDeviceTest() {}


TestDeviceTest::~TestDeviceTest() {}


void TestDeviceTest::SetUp() {
    // uncomment this if ever testing against a local broker
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);

    // Start central event-loop
    m_eventLoopThread = std::jthread([](std::stop_token stoken) { karabo::net::EventLoop::work(); });
    // Create and start server
    {
        Hash config("serverId", "testServerDevice", "log.level", "FATAL", "serverFlags",
                    std::vector<std::string>{"Development"});
        m_deviceServer = DeviceServer::create("DeviceServer", config);
        m_deviceServer->finalizeInternalInitialization();
    }

    // Create client
    m_deviceClient = std::make_shared<DeviceClient>(std::string(), false);
    m_deviceClient->initialize();
}


void TestDeviceTest::TearDown() {
    m_deviceServer.reset();
    m_deviceClient.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EventLoop::stop();
    if (m_eventLoopThread.joinable()) m_eventLoopThread.join();
}


TEST_F(TestDeviceTest, appTestRunner) {
    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          "testServerDevice", "DeviceExampleTest", Hash("deviceId", "DeviceExampleTest"), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    // Now all possible individual tests.
    testLockClearLock();
    testInstanceInfoServer();
    testGetTimestampSystemInfo();
    testSchemaInjection();
    testSchemaWithAttrUpdate();
    testSchemaWithAttrAppend();
    // Change (i.e. update) schema of existing output channel
    testChangeSchemaOutputChannel("slotUpdateSchema");
    testChangeSchemaOutputChannel("slotAppendSchema");
    // Changing schema of an output channel - it should trigger a reconnection
    testOutputRecreatesOnSchemaChange("slotUpdateSchema");
    testOutputRecreatesOnSchemaChange("slotAppendSchema");
    testOutputRecreatesOnMaxSizeChange();
    // Inject new channels
    testInputOutputChannelInjection("slotUpdateSchema");
    testInputOutputChannelInjection("slotAppendSchema");
    testNodedSlot();
    testGetconfigReconfig();
    testUpdateState();
    testSet();
    testSetVectorUpdate();
    testSignal();

    // testBadInit needs its own device, so clean-up before
    m_deviceClient->killDeviceNoWait("DeviceExampleTest");
    testBadInit();
}


void TestDeviceTest::testLockClearLock() {
    std::clog << "\nTesting lock and slotClearLock of device: " << std::flush;
    const std::string deviceId("DeviceExampleTest");
    const Hash cfg(m_deviceClient->get(deviceId));
    m_deviceClient->set("DeviceExampleTest", "lockedBy", "Anything");
    ASSERT_THROW(m_deviceClient->set("DeviceExampleTest", "lockedBy", "Change"), karabo::data::RemoteException);
    ASSERT_NO_THROW(m_deviceClient->execute(deviceId, "slotClearLock"));
    m_deviceClient->set("DeviceExampleTest", "lockedBy", m_deviceClient->getInstanceId());
    ASSERT_NO_THROW(m_deviceClient->set("DeviceExampleTest", "lockedBy", ""));

    std::clog << "OK." << std::endl;
};


void TestDeviceTest::testInstanceInfoServer() {
    std::clog << "\nTesting instanceInfo and configuration round trip for deviceServer " << std::flush;

    auto sigSlotA = m_deviceServer;
    const int timeOutInMs = 250;
    karabo::data::Hash h;

    ASSERT_NO_THROW(sigSlotA->request("testServerDevice", "slotPing", 1).timeout(timeOutInMs).receive(h));
    ASSERT_EQ(h.get<std::string>("log"), "FATAL");
    ASSERT_TRUE(h.get<int>("serverFlags") == 1ul);

    ASSERT_TRUE(h.get<std::string>("user") != "");

    ASSERT_NO_THROW(sigSlotA->request("testServerDevice", "slotLoggerLevel", "INFO").timeout(timeOutInMs).receive());
    ASSERT_NO_THROW(sigSlotA->request("testServerDevice", "slotPing", 1).timeout(timeOutInMs).receive(h));
    ASSERT_EQ(h.get<std::string>("log"), "INFO");
    ASSERT_NO_THROW(sigSlotA->request("testServerDevice", "slotLoggerLevel", "FATAL").timeout(timeOutInMs).receive());
    ASSERT_NO_THROW(sigSlotA->request("testServerDevice", "slotPing", 1).timeout(timeOutInMs).receive(h));
    ASSERT_EQ(h.get<std::string>("log"), "FATAL");

    std::clog << "OK." << std::endl;
}


void TestDeviceTest::testGetTimestampSystemInfo() {
    std::clog << "Testing timeInfo and systemInfo for device: " << std::flush;

    // This tests the extrapolations done in Device::getTimestamp(Epochstamp& epoch)
    // and Device::slotGetTime().

    // Setup a communication helper
    auto sigSlotA = m_deviceServer;

    const int timeOutInMs = 250;
    const unsigned long long periodInMicroSec = 100000ull; // some tests below assume this to be 0.1 s
    const unsigned long long periodInAttoSec = periodInMicroSec * 1000000000000ull;
    // Before first received time tick, always return train id 0
    unsigned long long id = 1111ull;
    ASSERT_NO_THROW(
          sigSlotA
                ->request("DeviceExampleTest", "slotIdOfEpochstamp", 1ull, 2ull) // values here should not matter at all
                .timeout(timeOutInMs)
                .receive(id));
    ASSERT_EQ(0ull, id);

    // Also slotGetTime has zero train id
    Epochstamp now;
    Hash timeHash;
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotGetTime", Hash()).timeout(timeOutInMs).receive(timeHash));
    ASSERT_TRUE(timeHash.has("time"));
    ASSERT_TRUE(timeHash.get<bool>("time"));
    const Timestamp stamp(Timestamp::fromHashAttributes(timeHash.getAttributes("time")));
    ASSERT_EQ(0ull, stamp.getTid());
    ASSERT_TRUE(stamp.getEpochstamp() > now);
    ASSERT_TRUE(timeHash.has("reference"));
    ASSERT_TRUE(timeHash.get<bool>("reference"));
    ASSERT_TRUE(timeHash.has("timeServerId"));
    ASSERT_EQ("None", timeHash.get<std::string>("timeServerId"));

    // Now send a time tick...
    const unsigned long long seconds = 1559600000ull; // About June 3rd, 2019, 10 pm GMT
    const unsigned long long startId = 100ull;
    const unsigned long long fracAttoSecs = 2ull * periodInAttoSec + 1100ull;
    ASSERT_NO_THROW(sigSlotA
                          ->request("DeviceExampleTest", "slotTimeTick",
                                    // id,     sec,   frac(attosec), period(microsec)
                                    startId, seconds, fracAttoSecs, periodInMicroSec)
                          .timeout(timeOutInMs)
                          .receive());

    timeHash.clear();
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotGetTime", Hash()).timeout(timeOutInMs).receive(timeHash));
    const Timestamp stamp2(Timestamp::fromHashAttributes(timeHash.getAttributes("time")));
    const Timestamp refStamp(Timestamp::fromHashAttributes(timeHash.getAttributes("reference")));
    ASSERT_LE(startId, stamp2.getTid());
    ASSERT_EQ(startId, refStamp.getTid());
    ASSERT_EQ(seconds, refStamp.getSeconds());
    ASSERT_EQ(fracAttoSecs, refStamp.getFractionalSeconds());

    Hash systemHash;
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotGetSystemInfo", Hash()).timeout(timeOutInMs).receive(systemHash));
    ASSERT_EQ(systemHash.has("timeInfo"), true);
    timeHash = systemHash.get<Hash>("timeInfo");
    ASSERT_EQ(systemHash.has("user"), true);
    ASSERT_EQ(systemHash.has("broker"), true);
    ASSERT_EQ(timeHash.has("reference"), true);
    ASSERT_EQ(timeHash.has("time"), true);
    ASSERT_EQ(timeHash.has("timeServerId"), true);

    // ...and test real calculations of id
    // 1) exact match
    id = 0ull;
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotIdOfEpochstamp", seconds, 2ull * periodInAttoSec + 1100ull)
                .timeout(timeOutInMs)
                .receive(id));
    ASSERT_EQ(startId, id);

    // 2) end of id
    id = 0ull;
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotIdOfEpochstamp", seconds, 3ull * periodInAttoSec + 1099ull)
                .timeout(timeOutInMs)
                .receive(id));
    ASSERT_EQ(startId, id);

    // 3) multiple of period above - but same second
    id = 0ull;
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotIdOfEpochstamp", seconds, 5ull * periodInAttoSec + 1100ull)
                .timeout(timeOutInMs)
                .receive(id));
    ASSERT_EQ(startId + 3ull, id);

    // 4) multiple of period plus a bit above - next second
    id = 0ull;
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotIdOfEpochstamp", seconds + 1ull, 5ull * periodInAttoSec + 1105ull)
                .timeout(timeOutInMs)
                .receive(id));
    ASSERT_EQ(startId + 13ull, id);

    // 5) just before
    id = 0ull;
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotIdOfEpochstamp", seconds, 2ull * periodInAttoSec + 1090ull)
                .timeout(timeOutInMs)
                .receive(id));
    ASSERT_EQ(startId - 1ull, id);

    // 6) several before - but same second
    id = 0ull;
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotIdOfEpochstamp", seconds, 1ull).timeout(timeOutInMs).receive(id));
    ASSERT_EQ(startId - 3ull, id);

    // 7) several before - previous second
    id = 0ull;
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotIdOfEpochstamp", seconds - 1ull, 5ull * periodInAttoSec + 1110ull)
                .timeout(timeOutInMs)
                .receive(id));
    ASSERT_EQ(startId - 7ull, id);

    // 8) so much in the past that a negative id would be calculated which leads to zero
    id = 1111ull;
    ASSERT_NO_THROW(sigSlotA->request("DeviceExampleTest", "slotIdOfEpochstamp", seconds - 100ull, 1110ull)
                          .timeout(timeOutInMs)
                          .receive(id));
    ASSERT_EQ(0ull, id);

    std::clog << "OK." << std::endl;
}


void TestDeviceTest::testSchemaInjection() {
    // Setup a communication helper
    auto sigSlotA = m_deviceServer;

    // Timeout, in milliseconds, for a request for one of the test device slots.
    const int requestTimeoutMs = 2000;
    // Time, in milliseconds, to wait for DeviceClient to update its internal cache after a schema change.
    const int cacheUpdateWaitMs = 1000;

    // Checks that appendSchema really appends.
    // ----------
    Schema schema;
    INT32_ELEMENT(schema).key("injectedInt32").assignmentOptional().defaultValue(1).reconfigurable().commit();

    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotAppendSchema", schema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient.
    ASSERT_TRUE(
          waitForCondition([this] { return m_deviceClient->getActiveSchema("DeviceExampleTest").has("injectedInt32"); },
                           cacheUpdateWaitMs));

    int injectedInt32;
    m_deviceClient->get("DeviceExampleTest", "injectedInt32", injectedInt32);
    ASSERT_TRUE(injectedInt32 == 1);
    m_deviceClient->set("DeviceExampleTest", "injectedInt32", 5);
    m_deviceClient->get("DeviceExampleTest", "injectedInt32", injectedInt32);
    ASSERT_TRUE(injectedInt32 == 5);

    // Checks that injecting a new attribute keeps the previously set value.
    // ----------
    INT32_ELEMENT(schema).key("injectedInt32").assignmentOptional().defaultValue(2).reconfigurable().minInc(1).commit();

    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotAppendSchema", schema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient
    ASSERT_TRUE(waitForCondition(
          [this] {
              return m_deviceClient->getActiveSchema("DeviceExampleTest").getDefaultValue<int>("injectedInt32") == 2;
          },
          cacheUpdateWaitMs));

    m_deviceClient->get("DeviceExampleTest", "injectedInt32", injectedInt32);
    ASSERT_TRUE(injectedInt32 == 5);
    Schema devFullSchema;
    devFullSchema = m_deviceClient->getDeviceSchema("DeviceExampleTest");
    ASSERT_TRUE(devFullSchema.getMinInc<int>("injectedInt32") == 1);
    ASSERT_TRUE(devFullSchema.getDisplayType("lockedBy") == "lockedBy");

    // Checks that doing updateSchema keeps previously set value and preserves the property
    // of type TABLE_ELEMENT in the device's static schema.
    // ----------
    INT32_ELEMENT(schema)
          .key("injectedInt32")
          .assignmentOptional()
          .defaultValue(3)
          .reconfigurable()
          .minInc(2)
          .maxInc(10)
          .commit();

    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotUpdateSchema", schema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient
    ASSERT_TRUE(waitForCondition(
          [this]() {
              return m_deviceClient->getActiveSchema("DeviceExampleTest").getDefaultValue<int>("injectedInt32") == 3;
          },
          cacheUpdateWaitMs));

    m_deviceClient->get("DeviceExampleTest", "injectedInt32", injectedInt32);
    ASSERT_TRUE(injectedInt32 == 5);
    devFullSchema = m_deviceClient->getDeviceSchema("DeviceExampleTest");
    ASSERT_TRUE(devFullSchema.getMinInc<int>("injectedInt32") == 2);
    ASSERT_TRUE(devFullSchema.getMaxInc<int>("injectedInt32") == 10);

    ASSERT_TRUE(m_deviceClient->getActiveSchema("DeviceExampleTest").has("table"));
    const std::vector<Hash>& tableAfterUpdate = m_deviceClient->get<std::vector<Hash>>("DeviceExampleTest", "table");
    ASSERT_TRUE(tableAfterUpdate.size() == 2);
    const Hash& firstRowAfterUpdate = tableAfterUpdate[0];
    ASSERT_TRUE(firstRowAfterUpdate.get<std::string>("name") == "firstLine");

    // Checks that doing updateSchema with something else loses injectedInt32.
    // ----------
    Schema sndSchema;
    INT32_ELEMENT(sndSchema).key("somethingElse").assignmentOptional().defaultValue(4).reconfigurable().commit();

    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotUpdateSchema", sndSchema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient
    ASSERT_TRUE(waitForCondition(
          [this]() { return m_deviceClient->getActiveSchema("DeviceExampleTest").has("somethingElse"); },
          cacheUpdateWaitMs));

    std::vector<std::string> propertiesPaths = m_deviceClient->getProperties("DeviceExampleTest");
    int freq = std::count(propertiesPaths.begin(), propertiesPaths.end(), "injectedInt32");
    ASSERT_TRUE(freq == 0);
    freq = std::count(propertiesPaths.begin(), propertiesPaths.end(), "somethingElse");
    ASSERT_TRUE(freq == 1);

    // Checks that updateSchema for a parameter three times keeps the original value.
    // This verifies that the schema parsing check is correct.
    // ----------
    m_deviceClient->set<int>("DeviceExampleTest", "somethingElse", 42);
    Schema trdSchema;
    INT32_ELEMENT(trdSchema).key("somethingElse").assignmentOptional().defaultValue(5).reconfigurable().commit();

    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotUpdateSchema", trdSchema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient
    ASSERT_TRUE(waitForCondition(
          [this] {
              return m_deviceClient->getActiveSchema("DeviceExampleTest").getDefaultValue<int>("somethingElse") == 5;
          },
          cacheUpdateWaitMs));

    Schema forthSchema;
    INT32_ELEMENT(forthSchema).key("somethingElse").assignmentOptional().defaultValue(6).reconfigurable().commit();

    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotUpdateSchema", forthSchema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient
    ASSERT_TRUE(waitForCondition(
          [this] {
              return m_deviceClient->getActiveSchema("DeviceExampleTest").getDefaultValue<int>("somethingElse") == 6;
          },
          cacheUpdateWaitMs));

    Schema fifthSchema;

    INT32_ELEMENT(fifthSchema)
          .key("somethingElse")
          .assignmentOptional()
          .defaultValue(7)
          .minInc(3)
          .reconfigurable()
          .commit();

    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotUpdateSchema", fifthSchema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient
    ASSERT_TRUE(waitForCondition(
          [this] {
              return m_deviceClient->getActiveSchema("DeviceExampleTest").getDefaultValue<int>("somethingElse") == 7;
          },
          cacheUpdateWaitMs));

    ASSERT_TRUE(m_deviceClient->get<int>("DeviceExampleTest", "somethingElse") == 42);

    // Checks that doing updateSchema with an empty schema resets the device to its
    // base schema.
    // ----------
    Schema emptySchema;

    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotUpdateSchema", emptySchema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient.
    ASSERT_TRUE(waitForCondition(
          [this]() { return !m_deviceClient->getActiveSchema("DeviceExampleTest").has("somethingElse"); },
          cacheUpdateWaitMs));

    propertiesPaths = m_deviceClient->getProperties("DeviceExampleTest");
    freq = std::count(propertiesPaths.begin(), propertiesPaths.end(), "somethingElse");
    ASSERT_TRUE(freq == 0);
    devFullSchema = m_deviceClient->getDeviceSchema("DeviceExampleTest");
    Schema devStaticSchema = m_deviceClient->getClassSchema("testServerDevice", "DeviceExampleTest");
    ASSERT_TRUE(karabo::data::similar(devFullSchema, devStaticSchema));

    // Checks that appending several times in a row, quickly, sets all values.
    // ----------
    const std::string propertyStr("property");
    for (int i = 0; i < 10; i++) {
        Schema schemaIdx;

        INT32_ELEMENT(schemaIdx)
              .key(propertyStr + std::to_string(i))
              .assignmentOptional()
              .defaultValue(i)
              .reconfigurable()
              .commit();

        ASSERT_NO_THROW(sigSlotA->request("DeviceExampleTest", "slotAppendSchema", schemaIdx)
                              .timeout(requestTimeoutMs)
                              .receive());
    }

    // Waits for the updated schema to be available from the DeviceClient.
    ASSERT_TRUE(waitForCondition(
          [this, &propertyStr]() {
              return m_deviceClient->getDeviceSchema("DeviceExampleTest").has(propertyStr + "9");
          },
          cacheUpdateWaitMs));

    propertiesPaths = m_deviceClient->getProperties("DeviceExampleTest");
    devFullSchema = m_deviceClient->getDeviceSchema("DeviceExampleTest");
    for (int i = 0; i < 10; i++) {
        std::string keyStr = propertyStr + std::to_string(i);
        freq = std::count(propertiesPaths.begin(), propertiesPaths.end(), keyStr);
        ASSERT_TRUE(freq == 1);
        ASSERT_TRUE(devFullSchema.has(keyStr));
        ASSERT_TRUE(m_deviceClient->get<int>("DeviceExampleTest", keyStr) == i);
    }

    // Asserts that all the appendSchema calls from the latest changes preserved the
    // TABLE_ELEMENT in the device's static schema.
    ASSERT_TRUE(m_deviceClient->getDeviceSchema("DeviceExampleTest").has("table"));
    const std::vector<Hash>& tableAfterInsert = m_deviceClient->get<std::vector<Hash>>("DeviceExampleTest", "table");
    ASSERT_TRUE(tableAfterInsert.size() == 2);
    const Hash& firstRowAfterInsert = tableAfterInsert[0];
    ASSERT_TRUE(firstRowAfterInsert.get<std::string>("name") == "firstLine");

    // Reset to static Schema for next test
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotUpdateSchema", Schema()).timeout(requestTimeoutMs).receive());
}


void TestDeviceTest::testSchemaWithAttrUpdate() {
    // Setup a communication helper
    auto sigSlotA = m_deviceServer;

    // Timeout, in milliseconds, for a request for one of the test device slots.
    const int requestTimeoutMs = 2000;
    // Time, in milliseconds, to wait for DeviceClient to update its internal cache after a schema change.
    const int cacheUpdateWaitMs = 1000;

    // Updates 'maxExc'
    Schema schema;
    double maxHighValue = 2.0 * DeviceExampleTest::LIMIT_HIGH;
    DOUBLE_ELEMENT(schema)
          .key("valueWithExc")
          .assignmentOptional()
          .defaultValue(0.0) //
          .reconfigurable()
          .maxExc(maxHighValue)
          .commit();
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotUpdateSchema", schema).timeout(requestTimeoutMs).receive());

    // Checks that the updated attribute will be available within an interval.
    ASSERT_TRUE(waitForCondition(
          [this, maxHighValue] {
              return m_deviceClient->getDeviceSchema("DeviceExampleTest").getMaxExc<double>("valueWithExc") ==
                     maxHighValue;
          },
          cacheUpdateWaitMs));

    // Tests that doing updateSchema with something new resets the maxExc.
    Schema someNewSchema;
    INT32_ELEMENT(someNewSchema).key("somethingNew").assignmentOptional().defaultValue(4).reconfigurable().commit();
    ASSERT_NO_THROW(sigSlotA->request("DeviceExampleTest", "slotUpdateSchema", someNewSchema)
                          .timeout(requestTimeoutMs)
                          .receive());
    // Checks that the reset attribute will be available within an interval.
    ASSERT_TRUE(waitForCondition(
          [this] {
              return m_deviceClient->getDeviceSchema("DeviceExampleTest").getMaxExc<double>("valueWithExc") ==
                     DeviceExampleTest::LIMIT_HIGH;
          },
          cacheUpdateWaitMs));

    // Reset to static Schema for next test
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotUpdateSchema", Schema()).timeout(requestTimeoutMs).receive());
}


void TestDeviceTest::testSchemaWithAttrAppend() {
    // Setup a communication helper
    auto sigSlotA = m_deviceServer;

    // Timeout, in milliseconds, for a request for one of the test device slots.
    const int requestTimeoutMs = 2000;
    // Time, in milliseconds, to wait for DeviceClient to update its internal cache after a schema change.
    const int cacheUpdateWaitMs = 1000;

    // Updates 'maxExc'
    Schema schema;
    double maxHighValue = 2.0 * DeviceExampleTest::LIMIT_HIGH;
    DOUBLE_ELEMENT(schema)
          .key("valueWithExc") //
          .assignmentOptional()
          .defaultValue(0.0)
          .reconfigurable()
          .maxExc(maxHighValue)
          .commit();
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotUpdateSchema", schema).timeout(requestTimeoutMs).receive());

    // Checks that the updated attribute will be available within an interval.
    ASSERT_TRUE(waitForCondition(
          [this, maxHighValue] {
              return m_deviceClient->getDeviceSchema("DeviceExampleTest").getMaxExc<double>("valueWithExc") ==
                     maxHighValue;
          },
          cacheUpdateWaitMs));

    // Tests that doing appendSchema with something new keeps the maxExc.
    Schema someNewSchema;
    INT32_ELEMENT(someNewSchema).key("somethingNew").assignmentOptional().defaultValue(4).reconfigurable().commit();
    ASSERT_NO_THROW(sigSlotA->request("DeviceExampleTest", "slotAppendSchema", someNewSchema)
                          .timeout(requestTimeoutMs)
                          .receive());
    // Checks that the reset attribute will be available within an interval.
    ASSERT_TRUE(waitForCondition(
          [this, maxHighValue] {
              return m_deviceClient->getDeviceSchema("DeviceExampleTest").getMaxExc<double>("valueWithExc") ==
                     maxHighValue;
          },
          cacheUpdateWaitMs));

    // Reset to static Schema for next test
    ASSERT_NO_THROW(
          sigSlotA->request("DeviceExampleTest", "slotUpdateSchema", Schema()).timeout(requestTimeoutMs).receive());
}


void TestDeviceTest::testChangeSchemaOutputChannel(const std::string& updateSlot) {
    std::clog << "Start testChangeSchemaOutputChannel for " << updateSlot << ": " << std::flush;
    // Timeout, in milliseconds, for a request for one of the test device slots.
    const int requestTimeoutMs = 2000;


    // Not using m_deviceClient->getDeviceSchema("DeviceExampleTest") since its cache might not be up-to-date yet
    // from schema "erasure" at the end of the previous run of this method with another 'updateSlot' value.
    // Our order guarantee does not apply since the m_deviceServer requested the update and not the m_device client...
    Schema deviceSchema;
    ASSERT_NO_THROW(m_deviceServer->request("DeviceExampleTest", "slotGetSchema", false)
                          .timeout(requestTimeoutMs)
                          .receive(deviceSchema));
    ASSERT_EQ("INTENSITY.TD", deviceSchema.getAliasFromKey<std::string>("output.schema.data.intensityTD"))
          << karabo::data::toString(deviceSchema);
    ASSERT_EQ("output.schema.data.intensityTD", deviceSchema.getKeyFromAlias<std::string>("INTENSITY.TD"))
          << karabo::data::toString(deviceSchema);
    ASSERT_EQ("UNTAGGED", deviceSchema.getAliasFromKey<std::string>("output.schema.data.untagged"))
          << karabo::data::toString(deviceSchema);
    ASSERT_EQ("output.schema.data.untagged", deviceSchema.getKeyFromAlias<std::string>("UNTAGGED"))
          << karabo::data::toString(deviceSchema);

    // Now inject more things with tags:
    // * as normal property (taggedProperty)
    // * as output channel data (data.intensityTD2)
    // Also deal with aliases:
    // * inject property with alias (data.intensityTD2)
    // * change alias of an existing property (untagged)
    Schema schema;
    {
        DOUBLE_ELEMENT(schema)
              .key("taggedProperty")
              .tags("doocs,pulseResolved")
              .displayedName("Tagged property")
              .readOnly()
              .commit();

        DOUBLE_ELEMENT(schema)
              .key("differentlyTaggedProperty")
              .tags("nodoocs")
              .displayedName("Diff. tagged property")
              .readOnly()
              .commit();

        Schema dataSchema;
        NODE_ELEMENT(dataSchema)
              .key("data")
              .displayedName("Data")
              .setDaqDataType(karabo::data::DaqDataType::TRAIN)
              .commit();

        VECTOR_FLOAT_ELEMENT(dataSchema)
              .key("data.intensityTD2")
              .tags("doocs,pulseResolved")
              .alias("INTENSITY.TD2")
              .displayedName("Intensity TD 2")
              .readOnly()
              .commit();

        // Here overwrite only alias
        DOUBLE_ELEMENT(dataSchema)
              .key("data.untagged")
              .alias("UNTAGGED.CHANGED")
              .displayedName("Untagged")
              .readOnly()
              .commit();

        DOUBLE_ELEMENT(dataSchema).key("data.untagged2").displayedName("Untagged2").readOnly().commit();

        OUTPUT_CHANNEL(schema).key("output").dataSchema(dataSchema).commit();
    }

    ASSERT_NO_THROW(
          m_deviceServer->request("DeviceExampleTest", updateSlot, schema).timeout(requestTimeoutMs).receive());

    // Check aliases
    ASSERT_NO_THROW(m_deviceServer->request("DeviceExampleTest", "slotGetSchema", false)
                          .timeout(requestTimeoutMs)
                          .receive(deviceSchema));
    ASSERT_EQ("UNTAGGED.CHANGED", deviceSchema.getAliasFromKey<std::string>("output.schema.data.untagged"))
          << karabo::data::toString(deviceSchema);
    ASSERT_EQ("output.schema.data.untagged", deviceSchema.getKeyFromAlias<std::string>("UNTAGGED.CHANGED"))
          << karabo::data::toString(deviceSchema);
    ASSERT_EQ("INTENSITY.TD2", deviceSchema.getAliasFromKey<std::string>("output.schema.data.intensityTD2"))
          << karabo::data::toString(deviceSchema);
    ASSERT_EQ("output.schema.data.intensityTD2", deviceSchema.getKeyFromAlias<std::string>("INTENSITY.TD2"))
          << karabo::data::toString(deviceSchema);

    // Reset to static Schema for next test
    ASSERT_NO_THROW(m_deviceServer->request("DeviceExampleTest", "slotUpdateSchema", Schema())
                          .timeout(requestTimeoutMs)
                          .receive());
    std::clog << "OK." << std::endl;
}


void TestDeviceTest::testOutputRecreatesOnSchemaChange(const std::string& updateSlot) {
    std::clog << "Start testOutputRecreatesOnSchemaChange for " << updateSlot << ": " << std::flush;

    const std::string& senderId("DeviceExampleTest");
    const std::string& receiverId("receiver");

    // Setup receiver device that should connect.
    std::pair<bool, std::string> success =
          m_deviceClient->instantiate("testServerDevice", "DeviceExampleTest",
                                      Hash("deviceId", receiverId, "input.connectedOutputChannels",
                                           std::vector<std::string>({senderId + ":output"})),
                                      KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;
    // Test connection is setup
    ASSERT_TRUE(waitForCondition(
          [this, receiverId]() {
              return m_deviceClient->get<std::vector<std::string>>(receiverId, "input.missingConnections").empty();
          },
          KRB_TEST_MAX_TIMEOUT * 1000))
          << karabo::data::toString(m_deviceClient->get(receiverId));

    // Tell server (as helper) to listen for updates of "input.missingConnections"
    // Note: Since we cannot remove the slot from the server again, we choose a test run dependent slot name
    //       and disconnect at the end. So the slot lambda (that takes variables that are local to the test by
    //       reference) cannot be called later - it would likely crash.
    std::mutex connectionChangesMutex;
    std::vector<std::vector<std::string>> connectionChanges;
    auto changedHandler = [&connectionChanges, &connectionChangesMutex, receiverId](const karabo::data::Hash& h,
                                                                                    const std::string& id) {
        if (id == receiverId && h.has("input.missingConnections")) {
            std::lock_guard<std::mutex> lock(connectionChangesMutex);
            connectionChanges.push_back(h.get<std::vector<std::string>>("input.missingConnections"));
        }
    };
    const std::string slotConnectionChanged("slotConnectionChanged_" + updateSlot);
    m_deviceServer->registerSlot<karabo::data::Hash, std::string>(changedHandler, slotConnectionChanged);
    const bool connected = m_deviceServer->connect(receiverId, "signalChanged", slotConnectionChanged);
    ASSERT_TRUE(connected);

    // Create several schema injections that should trigger output channel reconnection (or not).
    // The Boolean tells whether "output" channel is recreated (and thus reconnection happens)
    // when injected and when injection is removed by updating with an empty Schema.
    std::vector<std::tuple<Schema, bool>> schemasToInject;
    // Schema where OUTPUT_CHANNEL is explicitly changed
    Schema schema1;
    Schema dataSchema;
    INT32_ELEMENT(dataSchema).key("injectedInt32").readOnly().commit();
    OUTPUT_CHANNEL(schema1).key("output").dataSchema(dataSchema).commit();
    schemasToInject.push_back(std::make_tuple(std::move(schema1), true));
    // Schema where output schema is changed silently, i.e. w/o mentioning OUTPUT_CHANNEL
    Schema schema2;
    NODE_ELEMENT(schema2).key("output").commit();
    NODE_ELEMENT(schema2).key("output.schema").commit();
    INT32_ELEMENT(schema2).key("output.schema.injectedInt32").readOnly().commit();
    schemasToInject.push_back(std::make_tuple(std::move(schema2), true));
    // Schema where something else changed - channel is untouched
    Schema schema3;
    INT32_ELEMENT(schema3).key("injectedUnrelated").assignmentOptional().defaultValue(1).reconfigurable().commit();
    schemasToInject.push_back(std::make_tuple(std::move(schema3), false));

    for (const std::tuple<Schema, bool>& schemaAndDisconnect : schemasToInject) {
        const Schema& schemaToInject = std::get<0>(schemaAndDisconnect);
        const bool triggerReconnect = std::get<1>(schemaAndDisconnect);
        ASSERT_NO_THROW(m_deviceServer->request(senderId, updateSlot, schemaToInject)
                              .timeout(KRB_TEST_MAX_TIMEOUT * 1000)
                              .receive());

        // If output channel schema changed, we expect that the channel was recreated and thus the
        // InputChannel of the receiver was disconnected and reconnected. Both should trigger a change of the
        // input channel's missingConnections property which should trigger a call to our "injected" slot
        // that is connected to 'signalChanged'.
        // If triggerReconnect is false, nothing such happens and we run into the timeout :-(.
        const bool changed = waitForCondition(
              [this, &connectionChanges, &connectionChangesMutex]() {
                  std::lock_guard<std::mutex> lock(connectionChangesMutex);
                  return connectionChanges.size() >= 2ul;
              },
              KRB_TEST_MAX_TIMEOUT * 1000);
        {
            std::lock_guard<std::mutex> lock(connectionChangesMutex);
            ASSERT_EQ(triggerReconnect, changed) << karabo::data::toString(connectionChanges);
            if (triggerReconnect) {
                ASSERT_EQ(2ul, connectionChanges.size()) << karabo::data::toString(connectionChanges);
                ASSERT_EQ(std::vector<std::string>({senderId + ":output"}), connectionChanges[0]);
                ASSERT_EQ(std::vector<std::string>(), connectionChanges[1]);
            }
        }
        // Remove schema changes again:
        ASSERT_NO_THROW(m_deviceServer->request(senderId, "slotUpdateSchema", Schema())
                              .timeout(KRB_TEST_MAX_TIMEOUT * 1000)
                              .receive());
        if (triggerReconnect) {
            // If schema changed in the first place, it changes back now and thus has to reconnect
            const bool changed = waitForCondition(
                  [this, &connectionChanges, &connectionChangesMutex]() {
                      std::lock_guard<std::mutex> lock(connectionChangesMutex);
                      return connectionChanges.size() >= 4ul; // two more than before
                  },
                  KRB_TEST_MAX_TIMEOUT * 2000); // Factor two: reconnection cycle is included!
            std::lock_guard<std::mutex> lock(connectionChangesMutex);
            ASSERT_TRUE(changed) << karabo::data::toString(connectionChanges);
            ASSERT_EQ(4ul, connectionChanges.size()) << karabo::data::toString(connectionChanges);
            ASSERT_EQ(std::vector<std::string>({senderId + ":output"}), connectionChanges[2]);
            ASSERT_EQ(std::vector<std::string>(), connectionChanges[3]);
        }
        // Clean-up for next round
        std::lock_guard<std::mutex> lock(connectionChangesMutex);
        connectionChanges.clear();
    }

    // Clean up
    m_deviceServer->disconnect(receiverId, "signalChanged", slotConnectionChanged);
    // Cannot remove slotConnectionChanged...
    success = m_deviceClient->killDevice(receiverId, KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;
    std::clog << "OK." << std::endl;
}


void TestDeviceTest::testOutputRecreatesOnMaxSizeChange() {
    std::clog << "Start testOutputRecreatesOnMaxSizeChange:" << std::flush;

    // This tests that Device::appendSchemaMaxSize recreates output channels with the proper schema for validation
    // and that sending of data that does not comply with schema fails

    const std::string& senderId("DeviceExampleTest");
    const std::string& receiverId("receiver");
    constexpr int timeoutMs = KRB_TEST_MAX_TIMEOUT * 1000;

    // Setup receiver device that should connect.
    std::pair<bool, std::string> success =
          m_deviceClient->instantiate("testServerDevice", "DeviceExampleTest",
                                      Hash("deviceId", receiverId, "input.connectedOutputChannels",
                                           std::vector<std::string>({senderId + ":output"})),
                                      KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    // This registers handlers for "input":
    ASSERT_NO_THROW(
          m_deviceServer->request(receiverId, "slotRegisterOnDataInputEos", "input").timeout(timeoutMs).receive());

    // Test that connection is setup
    ASSERT_TRUE(waitForCondition(
          [this, receiverId]() {
              return m_deviceClient->get<std::vector<std::string>>(receiverId, "input.missingConnections").empty();
          },
          timeoutMs))
          << karabo::data::toString(m_deviceClient->get(receiverId));


    // Tell server (as helper) to listen for updates of "input.missingConnections"
    std::mutex connectionChangesMutex;
    std::vector<std::vector<std::string>> connectionChanges;
    auto changedHandler = [&connectionChanges, &connectionChangesMutex, receiverId](const karabo::data::Hash& h,
                                                                                    const std::string& id) {
        if (id == receiverId && h.has("input.missingConnections")) {
            std::lock_guard<std::mutex> lock(connectionChangesMutex);
            connectionChanges.push_back(h.get<std::vector<std::string>>("input.missingConnections"));
        }
    };
    // See comments in testOutputRecreatesOnSchemaChange about uniqueness of name of slot added to server
    const std::string slotConnectionChanged("slotConnectionChanged_slotAppendSchemaMultiMaxSize");
    m_deviceServer->registerSlot<karabo::data::Hash, std::string>(changedHandler, slotConnectionChanged);
    const bool connected = m_deviceServer->connect(receiverId, "signalChanged", slotConnectionChanged);
    ASSERT_TRUE(connected);

    const unsigned int maxSize = 10;
    ASSERT_NO_THROW(
          m_deviceServer->request(senderId, "slotAppendSchemaMultiMaxSize", maxSize).timeout(timeoutMs).receive());

    // The output channel schema changed, so we expect that the channel was recreated and thus the
    // InputChannel of the receiver was disconnected and reconnected. Both should trigger a change of the
    // input channel's missingConnections property which should trigger a call to our "injected" slot
    // that is connected to 'signalChanged'.
    const bool changed = waitForCondition(
          [this, &connectionChanges, &connectionChangesMutex]() {
              std::lock_guard<std::mutex> lock(connectionChangesMutex);
              return connectionChanges.size() >= 2ul;
          },
          timeoutMs);
    {
        std::lock_guard<std::mutex> lock(connectionChangesMutex);
        ASSERT_TRUE(changed) << karabo::data::toString(connectionChanges);
        ASSERT_EQ(2ul, connectionChanges.size()) << karabo::data::toString(connectionChanges);
        ASSERT_EQ(std::vector<std::string>({senderId + ":output"}), connectionChanges[0]);
        ASSERT_EQ(std::vector<std::string>(), connectionChanges[1]);
        connectionChanges.clear(); // for next usage
    }
    // Make sure that "intInOnData" is not what it shall be later, after sending data
    ASSERT_TRUE(42 != m_deviceClient->get<int>(receiverId, "intInOnData"));

    // Sending data succeeds since vectors fit into maxSize
    Hash dataToSend("int", 42, "data",
                    Hash("untagged", 4.2, "intensityTD", std::vector<float>(maxSize, 3.7f), "vecInt32",
                         std::vector<int>(maxSize, 1)));
    ASSERT_NO_THROW(m_deviceServer->request(senderId, "slotSendToOutputChannel", "output", dataToSend)
                          .timeout(timeoutMs)
                          .receive());
    // Check that data arrived and onData handler is called
    waitForCondition([this, receiverId]() { return (42 == m_deviceClient->get<int>(receiverId, "intInOnData")); },
                     timeoutMs);
    ASSERT_EQ(42, m_deviceClient->get<int>(receiverId, "intInOnData"))
          << karabo::data::toString(m_deviceClient->get(receiverId));

    // Make vecInt32 longer than limit - writing to output channel will fail
    dataToSend.get<std::vector<int>>("data.vecInt32").push_back(22);
    try {
        m_deviceServer->request(senderId, "slotSendToOutputChannel", "output", dataToSend).timeout(timeoutMs).receive();
        FAIL() << "No exception";
    } catch (karabo::data::RemoteException& e) {
        const std::string& msg = e.userFriendlyMsg();
        ASSERT_TRUE(msg.find("schema mismatch") != std::string::npos) << msg;
        ASSERT_TRUE(msg.find("Number of elements (11)") != std::string::npos) << msg;
        ASSERT_TRUE(msg.find("greater than upper bound (10)") != std::string::npos) << msg;
        ASSERT_TRUE(msg.find("\"data.vecInt32\"") != std::string::npos) << msg;
    }
    // Enlarge the allowed vector size
    ASSERT_NO_THROW(
          m_deviceServer->request(senderId, "slotAppendSchemaMultiMaxSize", maxSize + 1).timeout(timeoutMs).receive());
    // Wait until connected again
    waitForCondition(
          [this, &connectionChanges, &connectionChangesMutex]() {
              std::lock_guard<std::mutex> lock(connectionChangesMutex);
              return connectionChanges.size() >= 2ul;
          },
          timeoutMs);
    // Now the data to send complies with the schema and can be sent
    dataToSend.set("int", 77);
    ASSERT_NO_THROW(m_deviceServer->request(senderId, "slotSendToOutputChannel", "output", dataToSend)
                          .timeout(timeoutMs)
                          .receive());
    // Check that data arrived and onData handler is called
    waitForCondition([this, receiverId]() { return (77 == m_deviceClient->get<int>(receiverId, "intInOnData")); },
                     timeoutMs);
    ASSERT_EQ(77, m_deviceClient->get<int>(receiverId, "intInOnData"))
          << karabo::data::toString(m_deviceClient->get(receiverId));


    // Clean up
    m_deviceServer->disconnect(receiverId, "signalChanged", slotConnectionChanged);
    // Cannot remove slotConnectionChanged...
    // Remove schema changes again:
    ASSERT_NO_THROW(m_deviceServer->request(senderId, "slotUpdateSchema", Schema()).timeout(timeoutMs).receive());
    success = m_deviceClient->killDevice(receiverId, KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    std::clog << "OK." << std::endl;
}

void TestDeviceTest::testInputOutputChannelInjection(const std::string& updateSlot) {
    std::clog << "Start testInputOutputChannelInjection for " << updateSlot << ": " << std::flush;

    // Setup a communication helper
    auto sigSlot = m_deviceServer;

    // Timeout, in milliseconds, for a request for one of the test device slots.
    const int requestTimeoutMs = 2000;
    // Time, in milliseconds, to wait for DeviceClient to update its internal cache after a schema change.
    const int cacheUpdateWaitMs = 1000;

    // At the beginning, only the static channel is there:
    std::vector<std::string> outputChannels;
    ASSERT_NO_THROW(sigSlot->request("DeviceExampleTest", "slotGetOutputChannelNames")
                          .timeout(requestTimeoutMs)
                          .receive(outputChannels));
    ASSERT_EQ(1ul, outputChannels.size());
    ASSERT_EQ(std::string("output"), outputChannels[0]);

    // Checks that updateSlot creates injected input and output channels
    // ----------
    Schema dataSchema;
    INT32_ELEMENT(dataSchema).key("int32").readOnly().commit();
    Schema schema;
    OUTPUT_CHANNEL(schema).key("injectedOutput").dataSchema(dataSchema).commit();
    INPUT_CHANNEL(schema).key("injectedInput").commit();
    OVERWRITE_ELEMENT(schema)
          .key("injectedInput.connectedOutputChannels")
          .setNewDefaultValue<std::vector<std::string>>(
                {"DeviceExampleTest:injectedOutput", "DeviceExampleTest:output"})
          .commit();
    NODE_ELEMENT(schema)
          .key("emptyNode") // Already in static schema - but without leaves
          .commit();
    INT32_ELEMENT(schema).key("emptyNode.anInt32").readOnly().initialValue(42).commit();

    ASSERT_NO_THROW(sigSlot->request("DeviceExampleTest", updateSlot, schema).timeout(requestTimeoutMs).receive());

    // Now, also the injectedOutput is there:
    outputChannels.clear();
    ASSERT_NO_THROW(sigSlot->request("DeviceExampleTest", "slotGetOutputChannelNames")
                          .timeout(requestTimeoutMs)
                          .receive(outputChannels));
    ASSERT_EQ(2ul, outputChannels.size());
    ASSERT_TRUE(std::find(outputChannels.begin(), outputChannels.end(), "output") != outputChannels.end());
    ASSERT_TRUE(std::find(outputChannels.begin(), outputChannels.end(), "injectedOutput") != outputChannels.end());

    // Check that, after some time, the injected input is connected to both, the injected and the static output
    auto inputsConnected = [this]() {
        const Hash cfg(m_deviceClient->get("DeviceExampleTest"));
        if (cfg.has("output.connections") && cfg.has("injectedOutput.connections")) {
            std::vector<Hash> tableStatic, tableInjected;
            cfg.get("output.connections", tableStatic);
            cfg.get("injectedOutput.connections", tableInjected);
            if (tableStatic.size() == 1ul && tableInjected.size() == 1ul &&
                tableStatic[0].get<std::string>("remoteId") == "DeviceExampleTest:injectedInput" &&
                tableInjected[0].get<std::string>("remoteId") == "DeviceExampleTest:injectedInput"
                // Also ensure the injected property is there
                && cfg.has("emptyNode.anInt32")) {
                return true;
            }
        }
        return false;
    };
    bool ok = waitForCondition(
          inputsConnected,
          cacheUpdateWaitMs * 20); // longer timeout: automatic connection tries happen only every 5 seconds

    ASSERT_TRUE(ok) << karabo::data::toString(m_deviceClient->get("DeviceExampleTest"));

    // Now START test that re-injecting an input channel keeps handlers registered with KARABO_ON_DATA.
    // Register data handler for "injectedInput" channel
    ASSERT_NO_THROW(sigSlot->request("DeviceExampleTest", "slotRegisterOnDataInputEos", "injectedInput")
                          .timeout(requestTimeoutMs)
                          .receive());
    // Check that initially "intInOnData" is not one, i.e. ensure that following actions will make it one.
    // (It is either zero [initial value] or -2 [from previous run of this test with other updateSlot].)
    ASSERT_TRUE(1 != m_deviceClient->get<int>("DeviceExampleTest", "intInOnData"));
    const int countEosCalls = m_deviceClient->get<int>("DeviceExampleTest", "numCallsOnInput");

    // Request data to be sent from "output" to "injectedInput" channel
    Hash dataToSend(
          "int", 1, "data",
          Hash("untagged", 4.2, "intensityTD", std::vector<float>(10, 3.7f), "vecInt32", std::vector<int>(5, 1)));
    ASSERT_NO_THROW(sigSlot->request("DeviceExampleTest", "slotSendToOutputChannel", "output", dataToSend)
                          .timeout(requestTimeoutMs)
                          .receive());
    // Check that data arrived and onData/onInput handlers called
    waitForCondition(
          [this, countEosCalls]() {
              return (1 == m_deviceClient->get<int>("DeviceExampleTest", "intInOnData") &&
                      countEosCalls + 1 == m_deviceClient->get<int>("DeviceExampleTest", "numCallsOnInput"));
          },
          cacheUpdateWaitMs);
    ASSERT_EQ(1, m_deviceClient->get<int>("DeviceExampleTest", "intInOnData"));
    ASSERT_EQ(countEosCalls + 1, m_deviceClient->get<int>("DeviceExampleTest", "numCallsOnInput"));

    // Request EOS to be sent to "injectedInput" channel.
    // All outputs an input is connected to have to send EOS to get the eos handler called...
    ASSERT_NO_THROW(
          sigSlot->request("DeviceExampleTest", "slotSendEos", std::vector<std::string>({"output", "injectedOutput"}))
                .timeout(requestTimeoutMs)
                .receive());
    // Check that EOS arrived and flipped sign
    waitForCondition([this]() { return (-1 == m_deviceClient->get<int>("DeviceExampleTest", "intInOnData")); },
                     cacheUpdateWaitMs);
    ASSERT_EQ(-1, m_deviceClient->get<int>("DeviceExampleTest", "intInOnData"));

    // Re-inject input - channel will be recreated and onData handler should be passed to new incarnation
    Schema inputOnlySchema;
    INPUT_CHANNEL(inputOnlySchema).key("injectedInput").commit();
    // Note that here we need to use "slotAppendSchema" and not updateSlot since "slotUpdateSchema" would erase
    // "injectedInput".
    ASSERT_NO_THROW(sigSlot->request("DeviceExampleTest", "slotAppendSchema", inputOnlySchema)
                          .timeout(requestTimeoutMs)
                          .receive());
    // Wait for connection being re-established
    // HACK: Without sleep might be fooled, i.e. traces of connection of previous input channel not yet erased...
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ok = waitForCondition(inputsConnected, cacheUpdateWaitMs * 20); // longer timeout again, see above
    ASSERT_TRUE(ok) << karabo::data::toString(m_deviceClient->get("DeviceExampleTest"));
    // Request again data to be sent from "output" to "injectedInput" channel
    dataToSend.set("int", 2);
    ASSERT_NO_THROW(sigSlot->request("DeviceExampleTest", "slotSendToOutputChannel", "output", dataToSend)
                          .timeout(requestTimeoutMs)
                          .receive());
    // Check that new data arrived
    waitForCondition(
          [this, countEosCalls]() {
              return (2 == m_deviceClient->get<int>("DeviceExampleTest", "intInOnData") &&
                      countEosCalls + 2 == m_deviceClient->get<int>("DeviceExampleTest", "numCallsOnInput"));
          },
          cacheUpdateWaitMs);
    ASSERT_EQ(2, m_deviceClient->get<int>("DeviceExampleTest", "intInOnData"));
    ASSERT_EQ(countEosCalls + 2, m_deviceClient->get<int>("DeviceExampleTest", "numCallsOnInput"));
    // Request EOS to be sent again
    ASSERT_NO_THROW(
          sigSlot->request("DeviceExampleTest", "slotSendEos", std::vector<std::string>({"output", "injectedOutput"}))
                .timeout(requestTimeoutMs)
                .receive());
    // Check that EOS arrived and flipped sign again
    waitForCondition([this]() { return (-2 == m_deviceClient->get<int>("DeviceExampleTest", "intInOnData")); },
                     cacheUpdateWaitMs);
    ASSERT_EQ(-2, m_deviceClient->get<int>("DeviceExampleTest", "intInOnData"));
    //
    // END test that re-injecting input channels keeps handlers registered with KARABO_ON_DATA/KARABO_ON_EOS!

    // Remove the channels again:
    ASSERT_NO_THROW(
          sigSlot->request("DeviceExampleTest", "slotUpdateSchema", Schema()).timeout(requestTimeoutMs).receive());
    // Now only the static OutputChannel is kept
    outputChannels.clear();
    ASSERT_NO_THROW(sigSlot->request("DeviceExampleTest", "slotGetOutputChannelNames")
                          .timeout(requestTimeoutMs)
                          .receive(outputChannels));
    ASSERT_EQ(1ul, outputChannels.size());
    ASSERT_EQ("output", outputChannels[0]);

    // TODO: We directly call slotGetConfiguration instead of using m_deviceClient->get("DeviceExampleTest"):
    //       Looks like the client cache will not erase removed properties.
    Hash cfg;
    std::string dummy;
    sigSlot->request("DeviceExampleTest", "slotGetConfiguration").timeout(requestTimeoutMs).receive(cfg, dummy);
    ASSERT_TRUE(!cfg.has("injectedOutput")) << karabo::data::toString(cfg);
    ASSERT_TRUE(!cfg.has("injectedInput")) << karabo::data::toString(cfg);
    // Not channel related - 'emptyNode' kept, but injected anInt32 not:
    ASSERT_TRUE(!cfg.has("emptyNode.anInt32")) << karabo::data::toString(cfg);
    ASSERT_TRUE(cfg.has("emptyNode")) << karabo::data::toString(cfg);

    std::clog << "OK." << std::endl;
}


void TestDeviceTest::testNodedSlot() {
    std::clog << "Start testNodedSlot: " << std::flush;
    // Note that calling "node_slot" would work as well... :-|
    ASSERT_NO_THROW(m_deviceClient->execute("DeviceExampleTest", "node.slot", KRB_TEST_MAX_TIMEOUT));

    // Check also that slot is properly recorded as lastCommand
    ASSERT_EQ(std::string("node.slot <- ") += m_deviceClient->getInstanceId(),
              m_deviceClient->get<std::string>("DeviceExampleTest", "lastCommand"));

    std::clog << "OK." << std::endl;
}


void TestDeviceTest::testGetconfigReconfig() {
    std::clog << "Start testGetconfigReconfig: " << std::flush;

    const int timeoutInMs = 10000;
    const std::string deviceId("DeviceExampleTest");

    // Check device properties
    Hash cfgHash;
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(cfgHash));
    ASSERT_EQ(deviceId, cfgHash.get<std::string>("deviceId"));
    ASSERT_EQ("DeviceExampleTest", cfgHash.get<std::string>("classId"));
    ASSERT_EQ(fakeClassVersion, cfgHash.get<std::string>("classVersion"));
    ASSERT_EQ(karabo::util::Version::getVersion(), cfgHash.get<std::string>("karaboVersion"));
    ASSERT_EQ("testServerDevice", cfgHash.get<std::string>("serverId"));
    ASSERT_EQ(::getpid(), cfgHash.get<int>("pid"));

    // test pipeline channel schema is an empty node or has empty nodes under it.
    assertChildNodesEmpty(cfgHash.get<Hash>("output.schema"));

    // But we can set the performance statistics
    cfgHash.clear();
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotReconfigure", Hash("performanceStatistics.enable", true))
                          .timeout(timeoutInMs)
                          .receive());
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(cfgHash));
    ASSERT_EQ(true, cfgHash.get<bool>("performanceStatistics.enable"));

    // Test the lastCommand for slotReconfigure
    ASSERT_EQ(std::string("slotReconfigure <- ") += m_deviceServer->getInstanceId(),
              cfgHash.get<std::string>("lastCommand"));

    // Now try to set performanceStatistics again, but with an old timestamp - that should not be taken!
    const Timestamp enableTimestamp(
          Timestamp::fromHashAttributes(cfgHash.getAttributes("performanceStatistics.enable")));
    const Epochstamp pastEpochstamp(
          enableTimestamp.getSeconds() - 3ull * 3600ull, // 3 hours back: no CET/CEST vs UTC confusion
          enableTimestamp.getFractionalSeconds());
    const Timestamp pastTimestamp(pastEpochstamp, enableTimestamp.getTimeId());
    Hash hToSet;
    Hash::Attributes& attrs = hToSet.set("performanceStatistics.enable", false).getAttributes();
    pastTimestamp.toHashAttributes(attrs);
    cfgHash.clear();
    const Timestamp beforeSetStamp;
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotReconfigure", hToSet).timeout(timeoutInMs).receive());
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(cfgHash));

    const Timestamp receivedStamp(Timestamp::fromHashAttributes(cfgHash.getAttributes("performanceStatistics.enable")));
    ASSERT_TRUE(receivedStamp != pastTimestamp) << receivedStamp.toIso8601Ext() << " " << pastTimestamp.toIso8601Ext();
    ASSERT_TRUE(receivedStamp.getEpochstamp() > beforeSetStamp.getEpochstamp())
          << receivedStamp.toIso8601Ext() << " " << beforeSetStamp.toIso8601Ext(); // cannot compare Timestamps

    // Now test slotGetConfigurationSlice
    const std::vector<std::string> selectedPaths({"performanceStatistics.enable", "vecString", "table"});
    Hash arg("paths", selectedPaths);
    const Hash slice =
          m_deviceClient->execute1<Hash, Hash>(deviceId, "slotGetConfigurationSlice", timeoutInMs / 1000, arg);
    ASSERT_EQ(static_cast<size_t>(3u), slice.size());

    // Remove all non-selected paths from full config ('cfgHash') and then check full equality, i.e. values and
    // attributes (e.g. timestamp).
    std::vector<std::string> allPaths;
    cfgHash.getPaths(allPaths);
    for (const std::string& path : allPaths) {
        if (std::find(selectedPaths.begin(), selectedPaths.end(), path) != selectedPaths.end() ||
            boost::algorithm::starts_with(path, "table")) { // paths contains "table[0].type" etc.
            continue;
        }
        cfgHash.erasePath(path);
    }
    ASSERT_TRUE(slice.fullyEquals(cfgHash, false)) << karabo::data::toString(cfgHash) << " vs\n"
                                                   << karabo::data::toString(slice); // false: order does not matter

    // Request for a non-existing path fails with remote exception that originates from a ParameterException
    bool exceptionCaught = false;
    arg.set("paths", std::vector<std::string>(1, "not_a_property"));
    try {
        m_deviceClient->execute1<Hash, Hash>(deviceId, "slotGetConfigurationSlice", timeoutInMs / 1000, arg);
    } catch (const karabo::data::RemoteException& e) {
        exceptionCaught = true;
        const std::string& det = e.details();
        ASSERT_TRUE(det.find("Exception Type....:  Parameter Exception") != std::string::npos) << det;
        ASSERT_TRUE(det.find("Key 'not_a_property' does not exist") != std::string::npos) << det;
    } catch (...) {
    };
    ASSERT_TRUE(exceptionCaught);

    std::clog << "OK." << std::endl;
}


void TestDeviceTest::testUpdateState() {
    const std::string deviceId("DeviceExampleTest");

    // Check initial state of test device
    const State state(m_deviceClient->get<State>(deviceId, "state"));
    ASSERT_TRUE(state == State::UNKNOWN) << "State is " << state.name();
    ASSERT_EQ(0u, m_deviceClient->get<unsigned int>(deviceId, "countStateToggles"));
    ASSERT_TRUE(std::abs(-1. - m_deviceClient->get<double>(deviceId, "valueWithExc")) > 1.e-7);

    const int timeOutInMs = 1000 * KRB_TEST_MAX_TIMEOUT;
    Hash hash;
    m_deviceServer->request(deviceId, "slotPing", 1).timeout(timeOutInMs).receive(hash);
    ASSERT_EQ("unknown", hash.get<std::string>("status"));

    // Prepare Hash argument to slotToggleState with two different time stamps
    const Epochstamp stampToggle(1575296000ull, 1111ull);
    const Epochstamp stampState(1575297000ull, 2222ull);
    Hash msg;
    stampToggle.toHashAttributes(msg.set("stampCountToggles", 0).getAttributes());
    stampState.toHashAttributes(msg.set("stampState", 0).getAttributes());
    msg.set("state", "NORMAL");

    // Send state update request and...
    // ... test its (implicit) reply value,
    std::string reply;
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotToggleState", msg).timeout(timeOutInMs).receive(reply));
    ASSERT_EQ("NORMAL", reply);
    m_deviceServer->request(deviceId, "slotPing", 1).timeout(timeOutInMs).receive(hash);
    ASSERT_EQ("ok", hash.get<std::string>("status"));

    // ... test that the state was switched,
    const State state1(m_deviceClient->get<State>(deviceId, "state"));
    ASSERT_TRUE(state1 == State::NORMAL) << "State is " << state1.name();

    // ... test that other values updated as well,
    ASSERT_EQ(1u, m_deviceClient->get<unsigned int>(deviceId, "countStateToggles"));
    ASSERT_NEAR(-1., m_deviceClient->get<double>(deviceId, "valueWithExc"), 1.e-7);

    reply = "";
    msg.set("state", "ERROR");
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotToggleState", msg).timeout(timeOutInMs).receive(reply));
    ASSERT_EQ("ERROR", reply);
    m_deviceServer->request(deviceId, "slotPing", 1).timeout(timeOutInMs).receive(hash);
    ASSERT_EQ("error", hash.get<std::string>("status"));

    // ... test that the state was switched,
    const State state2 = m_deviceClient->get<State>(deviceId, "state");
    ASSERT_TRUE(state2 == State::ERROR) << "State is " << state2.name();

    reply = "";
    msg.set("state", "NORMAL");
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotToggleState", msg).timeout(timeOutInMs).receive(reply));
    ASSERT_EQ("NORMAL", reply);
    m_deviceServer->request(deviceId, "slotPing", 1).timeout(timeOutInMs).receive(hash);
    ASSERT_EQ("ok", hash.get<std::string>("status"));

    // ... test that the state was switched,
    const State state3 = m_deviceClient->get<State>(deviceId, "state");
    ASSERT_TRUE(state3 == State::NORMAL) << "State is " << state3.name();

    // ... and finally test the desired timestamps:
    //     * state and valueWithExc get the same as given explicitly to updateState
    //     * countStateToggles gets the one mingled into the 'other' Hash
    const auto atto = karabo::data::TIME_UNITS::ATTOSEC;
    const Hash cfg(m_deviceClient->get(deviceId));
    const Epochstamp stampStateNew(Epochstamp::fromHashAttributes(cfg.getAttributes("state")));
    ASSERT_TRUE(stampStateNew == stampState) << stampStateNew.toIso8601(atto) << " != " << stampState.toIso8601(atto);

    const Epochstamp stampValue(Epochstamp::fromHashAttributes(cfg.getAttributes("valueWithExc")));
    ASSERT_TRUE(stampValue == stampState) << stampValue.toIso8601(atto) << " != " << stampState.toIso8601(atto);

    const Epochstamp stampToggleNew(Epochstamp::fromHashAttributes(cfg.getAttributes("countStateToggles")));
    ASSERT_TRUE(stampToggleNew == stampToggle)
          << stampToggleNew.toIso8601(atto) << " != " << stampToggle.toIso8601(atto);
}


void TestDeviceTest::testSet() {
    std::clog << "Start testSet: " << std::flush;
    const int timeoutInMs = KRB_TEST_MAX_TIMEOUT * 1000;
    const std::string deviceId("DeviceExampleTest");

    // Setting a non-existing value throws
    ASSERT_THROW(m_deviceServer->request(deviceId, "slotSet", Hash("nonExistParam", 0)).timeout(timeoutInMs).receive(),
                 karabo::data::RemoteException);

    // Setting a reconfigurable property outside its validation limits throws
    // (and even other valid changes in the same set(..) are ignored).
    Hash hash;
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash));
    ASSERT_EQ(0, hash.get<int>("valueWithLimit"));
    ASSERT_EQ(0, hash.get<int>("valueOther"));
    ASSERT_THROW(m_deviceServer
                       ->request(deviceId, "slotSet",
                                 Hash("valueWithLimit", 1000, // hit slimit
                                      "valueOther", 2000))    // would be OK
                       .timeout(timeoutInMs)
                       .receive(),
                 karabo::data::RemoteException);
    Hash hash2;
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash2));
    ASSERT_TRUE(hash2.fullyEquals(hash)); // Also valueOther did not change


    // Other settings work
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotSet", Hash("valueWithLimit", 999, "valueOther", 2000))
                          .timeout(timeoutInMs)
                          .receive());
    hash2.clear();
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash2));
    ASSERT_EQ(999, hash2.get<int>("valueWithLimit"));
    ASSERT_EQ(2000, hash2.get<int>("valueOther"));

    std::clog << "OK." << std::endl;
}

void TestDeviceTest::testSetVectorUpdate() {
    std::clog << "Start testSetVectorUpdate: " << std::flush;
    const int timeoutInMs = 10000;
    const std::string deviceId("DeviceExampleTest");

    using VectorUpdate = karabo::core::Device::VectorUpdate;

    Hash hash;
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash));
    ASSERT_EQ(std::vector<std::string>({"one", "two", "three"}), hash.get<std::vector<std::string>>("vecString"));

    // Test adding
    // The "three"s will all added times (although it is already in)!
    ASSERT_NO_THROW(m_deviceServer
                          ->request(deviceId, "slotUpdateVecString",
                                    std::vector<std::string>({"three", "three", "one"}),
                                    static_cast<int>(VectorUpdate::add))
                          .timeout(timeoutInMs)
                          .receive());
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash));
    ASSERT_EQ(std::vector<std::string>({"one", "two", "three", "three", "three", "one"}),
              hash.get<std::vector<std::string>>("vecString"));

    // Test addIfNotIn
    // Since "one" is already in, it will not be added again
    ASSERT_NO_THROW(m_deviceServer
                          ->request(deviceId, "slotUpdateVecString", std::vector<std::string>({"one", "seven"}),
                                    static_cast<int>(VectorUpdate::addIfNotIn))
                          .timeout(timeoutInMs)
                          .receive());
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash));
    ASSERT_EQ(std::vector<std::string>({"one", "two", "three", "three", "three", "one", "seven"}),
              hash.get<std::vector<std::string>>("vecString"));

    // Test removeOne
    // Only first "one" and first "three" will be removed, "notIn" is ignored
    ASSERT_NO_THROW(m_deviceServer
                          ->request(deviceId, "slotUpdateVecString",
                                    std::vector<std::string>({"three", "one", "notIn"}),
                                    static_cast<int>(VectorUpdate::removeOne))
                          .timeout(timeoutInMs)
                          .receive());
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash));
    ASSERT_EQ(std::vector<std::string>({"two", "three", "three", "one", "seven"}),
              hash.get<std::vector<std::string>>("vecString"));

    // Test removeAll
    // all "three"s and the "two" will be removed, "notIn" is ignored
    ASSERT_NO_THROW(m_deviceServer
                          ->request(deviceId, "slotUpdateVecString",
                                    std::vector<std::string>({"two", "notIn", "three"}),
                                    static_cast<int>(VectorUpdate::removeAll))
                          .timeout(timeoutInMs)
                          .receive());
    ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash));
    ASSERT_EQ(std::vector<std::string>({"one", "seven"}), hash.get<std::vector<std::string>>("vecString"));

    // Finally test invalid updateType
    ASSERT_THROW(m_deviceServer->request(deviceId, "slotUpdateVecString", std::vector<std::string>(), 0)
                       .timeout(timeoutInMs)
                       .receive(),
                 karabo::data::RemoteException);

    std::clog << "OK." << std::endl;
}


void TestDeviceTest::testSignal() {
    // Test that signals registered in constructor of devices inheriting from Device carry the signalInstanceId in
    // header (in 2.10.0 the SignalSlotable::init method is called after the constructor, so no id yet when
    // registering).

    std::string signalInstanceId;
    DeviceServer::WeakPointer weakServer(m_deviceServer);
    std::function<void()> slot = [&signalInstanceId, weakServer]() {
        DeviceServer::Pointer ptr(weakServer.lock());
        if (ptr) {
            const karabo::data::Hash::ConstPointer header(ptr->getSenderInfo("slotForSignalA")->getHeaderOfSender());
            signalInstanceId = header->get<std::string>("signalInstanceId");
        } else {
            std::clog << "DeviceServer pointer invalid!" << std::endl; // Should be impossible
        }
    };
    m_deviceServer->registerSlot(slot, "slotForSignalA");
    ASSERT_TRUE(m_deviceServer->connect("DeviceExampleTest", "signalA", "slotForSignalA"));
    // If request returns, we can be sure that the signal has been received.
    // That order would be undefined if instead of 'm_deviceServer->request' we would use
    // 'm_deviceClient->execute' since signal is emitted to m_deviceServer.
    ASSERT_NO_THROW(m_deviceServer->request("DeviceExampleTest", "slotEmitSignalA").timeout(5000).receive());

    ASSERT_EQ("DeviceExampleTest", signalInstanceId);

    // Clean up
    m_deviceServer->disconnect("DeviceExampleTest", "signalA", "slotForSignalA");
    // m_deviceServer->removeSlot("slotForSignalA"); private, but who cares here...
}

void TestDeviceTest::testBadInit() {
    // HACK against topology caching in DeviceClient:
    // If we do not call getDevices() here, but run this as the last test within appTestRunner() (if it is the first,
    // it's fine!!!), the getDevices("<serverId>") below in the test case 3 waiting condition is fooled and returns an
    // empty list when called the first time. The log tells us
    //    DEBUG  karabo.core.DeviceClient  : testServerDevice still in runtime description - call _slotInstanceGone
    // and we see that the getDevices(..) call triggered a topology gathering.
    // So there is bug in the topology caching mechanism...
    m_deviceClient->getDevices();
    // HACK end

    const unsigned int delayInSec = 5;

    //
    // Case 1: A very long lasting initialization method:
    //
    std::string devId("BadInitDevice/1");
    auto requestor = m_deviceServer
                           ->request("", "slotStartDevice",
                                     Hash("classId", "TestDeviceBadInit", "deviceId", devId, "configuration",
                                          Hash("initProblem", "delay", "delay", delayInSec)))
                           .timeout(2000); // starting a device takes at least one second...
    // Although initialization sleeps delayInSec, no timeout within the 2 seconds we allow for that
    bool ok = false;
    std::string dummy;
    ASSERT_NO_THROW(requestor.receive(ok, dummy));
    ASSERT_TRUE(ok);

    // After instantiation, state switches to INIT, as soon as initialisation method runs.
    State devState(State::UNKNOWN);
    bool waitOk = waitForCondition(
          [this, devId, &devState]() {
              devState = m_deviceClient->get<State>(devId, "state");
              return (devState == State::INIT);
          },
          2000);
    ASSERT_TRUE(waitOk) << devState.name();

    // At end of initialization, state changes to NORMAL - wait for it...
    waitOk = waitForCondition(
          [this, devId, &devState]() {
              devState = m_deviceClient->get<State>(devId, "state");
              return (devState == State::NORMAL);
          },
          (delayInSec + 2) * 1000); // wait longer than delaying sleep
    ASSERT_TRUE(waitOk) << devState.name();

    m_deviceClient->killDeviceNoWait(devId);

    //
    // Case 2: The initialization method fails with an exception:
    //
    devId.back() = '2'; // let's take a new id to avoid delays until the previous device is down
    std::atomic<bool> instanceNewCalled(false);
    m_deviceClient->registerInstanceNewMonitor([&instanceNewCalled, devId](const karabo::data::Hash& topologyEntry) {
        if (topologyEntry.has("device." + devId)) instanceNewCalled = true;
    });
    std::atomic<bool> instanceGoneCalled(false);
    m_deviceClient->registerInstanceGoneMonitor(
          [&instanceGoneCalled, devId](const std::string& instanceId, const karabo::data::Hash& info) {
              if (devId == instanceId) instanceGoneCalled = true;
          });
    requestor = m_deviceServer
                      ->request("", "slotStartDevice",
                                Hash("classId", "TestDeviceBadInit", "deviceId", devId, "configuration",
                                     Hash("initProblem", "throw")))
                      .timeout(2000); // starting a device takes at least one second...
    // Despite the failing initialization, the device replies successfully, no timeout:
    ok = false;
    ASSERT_NO_THROW(requestor.receive(ok, dummy));
    ASSERT_TRUE(ok);

    const bool newAndGone = waitForCondition(
          [&instanceNewCalled, &instanceGoneCalled]() { return (instanceNewCalled && instanceGoneCalled); }, 5000);
    ASSERT_TRUE(newAndGone);
    // Reset handlers that use references to local variables
    m_deviceClient->registerInstanceNewMonitor([](const karabo::data::Hash&) {});
    m_deviceClient->registerInstanceGoneMonitor([](const std::string&, const karabo::data::Hash&) {});

    //
    // Case 3: A very long lasting initialization method (as case 1), with a try to shutdown while initialization:
    //

    devId.back() += 1; // another id again, see above ('+= 1' for Alessandro... )
    requestor = m_deviceServer
                      ->request("", "slotStartDevice",
                                Hash("classId", "TestDeviceBadInit", "deviceId", devId, "configuration",
                                     Hash("initProblem", "delay", "delay", delayInSec, "callInPredestruction",
                                          m_deviceServer->getInstanceId() + ".onPredestruction")))
                      .timeout(2000); // starting a device takes at least one second...
    // Although initialization sleeps 'delayInSec', no timeout within the 2 seconds we allow for that
    ok = false;
    ASSERT_NO_THROW(requestor.receive(ok, dummy));
    ASSERT_TRUE(ok);

    auto onPreDestructionCalled = std::make_shared<bool>(false); // shared_ptr, avoids lifetime issues after test
    m_deviceServer->registerSlot([onPreDestructionCalled]() { *onPreDestructionCalled = true; }, "onPredestruction");

    // After instantiation, state switches to INIT, as soon as initialize method runs
    waitOk = waitForCondition(
          [this, devId, &devState]() {
              devState = m_deviceClient->get<State>(devId, "state");
              return (devState == State::INIT);
          },
          2000);
    const Epochstamp initStartedTime;
    ASSERT_TRUE(waitOk) << devState.name();

    // We kill the device that is still initializing: It will not die immediately (only once initialization is done),
    // but preDestruction is called.
    ASSERT_NO_THROW(m_deviceClient->execute(devId, "slotKillDevice"));

    // Now we see from our handler that onPreDestruction was called
    waitOk = waitForCondition([onPreDestructionCalled]() { return (*onPreDestructionCalled); },
                              // wait long enough that initialization is done (though should come earlier)
                              (delayInSec + 2) * 1000);
    ASSERT_TRUE(*onPreDestructionCalled);

    // Now wait until device is gone - will take until initialize method has finished!
    std::vector<std::string> devs;
    waitOk = waitForCondition(
          [this, devId, &devs]() {
              devs = m_deviceClient->getDevices(m_deviceServer->getInstanceId());
              return (std::find(devs.begin(), devs.end(), devId) == devs.end());
          },
          (delayInSec + 2) * 1000); // Longer than the delay in initialize()
    const karabo::data::TimeDuration duration(initStartedTime.elapsed());
    // Verify that device gone
    using karabo::data::toString;
    ASSERT_TRUE(waitOk) << karabo::data::toString(devs);

    // The initialization (that blocked device going down) should have lasted about delayInSec seconds.
    // We allow for some contingency:
    const karabo::data::TimeDuration testDuration(delayInSec * 3. / 4., 0ull); // implicit conversions happening....
    std::stringstream sstr;
    sstr << duration << " " << testDuration;
    ASSERT_TRUE(duration > testDuration) << sstr.str();
}


bool TestDeviceTest::waitForCondition(std::function<bool()> checker, unsigned int timeoutMillis) {
    constexpr unsigned int sleepIntervalMillis = 5;
    unsigned int numOfWaits = 0;
    const unsigned int maxNumOfWaits = static_cast<unsigned int>(std::ceil(timeoutMillis / sleepIntervalMillis));
    while (numOfWaits < maxNumOfWaits && !checker()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepIntervalMillis));
        numOfWaits++;
    }
    return (numOfWaits < maxNumOfWaits);
}


void assertChildNodesEmpty(const Hash& h) {
    if (h.empty()) {
        return;
    }
    std::vector<std::string> keys;
    Hash child;

    h.getKeys(keys);

    // We expect only one element (and this has to be a hash)
    ASSERT_TRUE(keys.size() == 1);
    ASSERT_NO_THROW(child = h.get<Hash>(keys[0]));

    // process child node all the way till we hit an empty child.
    assertChildNodesEmpty(child);
}
