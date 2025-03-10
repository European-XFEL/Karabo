/*
 * File:   Device_Test.cc
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

#include "Device_Test.hh"

#include <chrono>
#include <karabo/core/Device.hh>
#include <karabo/net/EventLoop.hh>
#include <karabo/util/Epochstamp.hh>
#include <karabo/util/Exception.hh>
#include <karabo/util/OverwriteElement.hh>
#include <karabo/util/Schema.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/State.hh>
#include <karabo/util/StringTools.hh>
#include <karabo/util/TableElement.hh>
#include <karabo/util/TimeDuration.hh>
#include <karabo/util/Timestamp.hh>
#include <karabo/util/VectorElement.hh>
#include <karabo/xms/InputChannel.hh>
#include <karabo/xms/OutputChannel.hh>
#include <karabo/xms/SignalSlotable.hh>

#include "CppUnitMacroExtension.hh"


#define KRB_TEST_MAX_TIMEOUT \
    10 // larger than the 6 s input channel reconnect interval, for testOutputRecreatesOnSchemaChange


using karabo::core::DeviceClient;
using karabo::core::DeviceServer;
using karabo::net::EventLoop;
using karabo::util::DOUBLE_ELEMENT;
using karabo::util::Epochstamp;
using karabo::util::Hash;
using karabo::util::INT32_ELEMENT;
using karabo::util::NODE_ELEMENT;
using karabo::util::OVERWRITE_ELEMENT;
using karabo::util::Schema;
using karabo::util::State;
using karabo::util::STRING_ELEMENT;
using karabo::util::TABLE_ELEMENT;
using karabo::util::Timestamp;
using karabo::util::UINT32_ELEMENT;
using karabo::util::VECTOR_FLOAT_ELEMENT;
using karabo::util::VECTOR_STRING_ELEMENT;
using karabo::xms::INPUT_CHANNEL;
using karabo::xms::OUTPUT_CHANNEL;
using karabo::xms::SignalSlotable;
using karabo::xms::SLOT_ELEMENT;

static void assertChildNodesEmpty(const Hash& h);

const std::string fakeClassVersion("FakePackage-1.2.3");
class TestDevice : public karabo::core::Device {
   public:
    KARABO_CLASSINFO(TestDevice, "TestDevice", fakeClassVersion)

    static const int ALARM_HIGH = 1000.0;


    static void expectedParameters(karabo::util::Schema& expected) {
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
              .key("valueWithAlarm")
              .readOnly()
              .alarmHigh(TestDevice::ALARM_HIGH)
              .needsAcknowledging(false)
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
              .setDaqDataType(karabo::util::DaqDataType::TRAIN)
              .commit();

        DOUBLE_ELEMENT(dataSchema).key("data.untagged").alias("UNTAGGED").displayedName("Untagged").readOnly().commit();

        VECTOR_FLOAT_ELEMENT(dataSchema)
              .key("data.intensityTD")
              .tags("doocs,pulseResolved")
              .alias("INTENSITY.TD")
              .displayedName("Intensity TD")
              .readOnly()
              .commit();

        OUTPUT_CHANNEL(expected).key("output").dataSchema(dataSchema).commit();

        // Schema less input channel...
        INPUT_CHANNEL(expected).key("input").commit();

        // Not channel related, but for test that empty node does not get erased
        NODE_ELEMENT(expected).key("emptyNode").commit();
    }


    TestDevice(const karabo::util::Hash& input) : karabo::core::Device(input) {
        // Bind to a slot what now is called from deviceServer:
        KARABO_SLOT(slotTimeTick, unsigned long long /*id*/, unsigned long long /*sec*/, unsigned long long /*frac*/,
                    unsigned long long /*period*/)

        KARABO_SLOT(slotIdOfEpochstamp, unsigned long long /*sec*/, unsigned long long /*frac*/)

        KARABO_SLOT(slotAppendSchema, const karabo::util::Schema);

        KARABO_SLOT(slotUpdateSchema, const karabo::util::Schema);

        KARABO_SLOT(slotSet, Hash);

        KARABO_SLOT(slotToggleState, const Hash);

        KARABO_SLOT(node_slot);

        KARABO_SLOT(slotGetCurrentConfiguration, const std::string /*tags*/);

        KARABO_SLOT(slotUpdateVecString, std::vector<std::string>, int);

        KARABO_SIGNAL("signalA");

        KARABO_SLOT(slotEmitSignalA);

        KARABO_SLOT(slotRegisterOnDataInputEos, const std::string /*inputChannelName*/);

        KARABO_SLOT(slotSendToOutputChannel, const std::string /*channelName*/, int /*intToSend*/);

        KARABO_SLOT(slotSendEos, const std::vector<std::string> /*channelNames*/);
    }


    virtual ~TestDevice() {}


    void slotIdOfEpochstamp(unsigned long long sec, unsigned long long frac) {
        const Timestamp stamp(getTimestamp(Epochstamp(sec, frac)));
        reply(stamp.getTrainId());
    }


    void slotAppendSchema(const Schema sch) {
        appendSchema(sch);
    }


    void slotUpdateSchema(const Schema sch) {
        updateSchema(sch);
    }


    void slotSet(const Hash& h) {
        set(h);
    }


    void slotToggleState(const Hash otherIn) {
        const Epochstamp& stampCountToggles =
              Epochstamp::fromHashAttributes(otherIn.getAttributes("stampCountToggles"));
        const Epochstamp& stampState = Epochstamp::fromHashAttributes(otherIn.getAttributes("stampState"));

        const State& newState = State::fromString(otherIn.get<std::string>("state"));

        Hash otherOut("valueWithAlarm", -1.);
        Hash::Attributes& attrs =
              otherOut.set("countStateToggles", get<unsigned int>("countStateToggles") + 1).getAttributes();
        getTimestamp(stampCountToggles).toHashAttributes(attrs);

        // So "state" and "valueWithAlarm" get timestamp from 'stampState', "countStateToggles" from 'stampCountToggles'
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


    void slotSendToOutputChannel(const std::string& channelName, int intToSend) {
        writeChannel(channelName, Hash("int", intToSend));
    }


    void slotSendEos(const std::vector<std::string>& channelNames) {
        for (const auto& channelName : channelNames) {
            signalEndOfStream(channelName);
        }
    }

    void onData(const karabo::util::Hash& data, const karabo::xms::InputChannel::MetaData& meta) {
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

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device, TestDevice)


// =================================================================================

class TestDeviceBadInit : public karabo::core::Device {
   public:
    KARABO_CLASSINFO(TestDeviceBadInit, "TestDeviceBadInit", "2.9")


    static void expectedParameters(karabo::util::Schema& expected) {
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

        UINT32_ELEMENT(expected).key("delay").assignmentOptional().defaultValue(10u).commit();
    }


    TestDeviceBadInit(const karabo::util::Hash& input) : karabo::core::Device(input) {
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
    }
};

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device, TestDeviceBadInit)

CPPUNIT_TEST_SUITE_REGISTRATION(Device_Test);


Device_Test::Device_Test() {}


Device_Test::~Device_Test() {}


void Device_Test::setUp() {
    // uncomment this if ever testing against a local broker
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);

    // Start central event-loop
    m_eventLoopThread = std::jthread([](std::stop_token stoken) { karabo::net::EventLoop::work(); });
    // Create and start server
    {
        Hash config("serverId", "testServerDevice", "scanPlugins", false, "Logger.priority", "FATAL", "serverFlags",
                    std::vector<std::string>{"Development"});
        m_deviceServer = DeviceServer::create("DeviceServer", config);
        m_deviceServer->finalizeInternalInitialization();
    }

    // Create client
    m_deviceClient = std::make_shared<DeviceClient>(std::string(), false);
    m_deviceClient->initialize();
}


void Device_Test::tearDown() {
    m_deviceServer.reset();
    m_deviceClient.reset();
    EventLoop::stop();
}


void Device_Test::appTestRunner() {
    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          "testServerDevice", "TestDevice", Hash("deviceId", "TestDevice"), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Now all possible individual tests.
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
    m_deviceClient->killDeviceNoWait("TestDevice");
    testBadInit();
}

void Device_Test::testInstanceInfoServer() {
    std::clog << "\nTesting instanceInfo and configuration round trip for deviceServer " << std::flush;

    auto sigSlotA = m_deviceServer;
    const int timeOutInMs = 250;
    karabo::util::Hash h;

    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("testServerDevice", "slotPing", "testServerDevice", 1, true)
                                  .timeout(timeOutInMs)
                                  .receive(h));
    CPPUNIT_ASSERT_EQUAL(h.get<std::string>("log"), std::string("FATAL"));
    CPPUNIT_ASSERT(h.get<int>("serverFlags") == 1ul);

    CPPUNIT_ASSERT(h.get<std::string>("user") != "");

    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("testServerDevice", "slotLoggerPriority", "INFO").timeout(timeOutInMs).receive());
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("testServerDevice", "slotPing", "testServerDevice", 1, true)
                                  .timeout(timeOutInMs)
                                  .receive(h));
    CPPUNIT_ASSERT_EQUAL(h.get<std::string>("log"), std::string("INFO"));
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("testServerDevice", "slotLoggerPriority", "FATAL").timeout(timeOutInMs).receive());
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("testServerDevice", "slotPing", "testServerDevice", 1, true)
                                  .timeout(timeOutInMs)
                                  .receive(h));
    CPPUNIT_ASSERT_EQUAL(h.get<std::string>("log"), std::string("FATAL"));

    std::clog << "OK." << std::endl;
}


void Device_Test::testGetTimestampSystemInfo() {
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
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA
                ->request("TestDevice", "slotIdOfEpochstamp", 1ull, 2ull) // values here should not matter at all
                .timeout(timeOutInMs)
                .receive(id));
    CPPUNIT_ASSERT_EQUAL(0ull, id);

    // Also slotGetTime has zero train id
    Epochstamp now;
    Hash timeHash;
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotGetTime", Hash()).timeout(timeOutInMs).receive(timeHash));
    CPPUNIT_ASSERT(timeHash.has("time"));
    CPPUNIT_ASSERT(timeHash.get<bool>("time"));
    const Timestamp stamp(Timestamp::fromHashAttributes(timeHash.getAttributes("time")));
    CPPUNIT_ASSERT_EQUAL(0ull, stamp.getTrainId());
    CPPUNIT_ASSERT(stamp.getEpochstamp() > now);
    CPPUNIT_ASSERT(timeHash.has("reference"));
    CPPUNIT_ASSERT(timeHash.get<bool>("reference"));
    CPPUNIT_ASSERT(timeHash.has("timeServerId"));
    CPPUNIT_ASSERT_EQUAL(std::string("None"), timeHash.get<std::string>("timeServerId"));

    // Now send a time tick...
    const unsigned long long seconds = 1559600000ull; // About June 3rd, 2019, 10 pm GMT
    const unsigned long long startId = 100ull;
    const unsigned long long fracAttoSecs = 2ull * periodInAttoSec + 1100ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA
                                  ->request("TestDevice", "slotTimeTick",
                                            // id,     sec,   frac(attosec), period(microsec)
                                            startId, seconds, fracAttoSecs, periodInMicroSec)
                                  .timeout(timeOutInMs)
                                  .receive());

    timeHash.clear();
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotGetTime", Hash()).timeout(timeOutInMs).receive(timeHash));
    const Timestamp stamp2(Timestamp::fromHashAttributes(timeHash.getAttributes("time")));
    const Timestamp refStamp(Timestamp::fromHashAttributes(timeHash.getAttributes("reference")));
    CPPUNIT_ASSERT_GREATEREQUAL(startId, stamp2.getTrainId());
    CPPUNIT_ASSERT_EQUAL(startId, refStamp.getTrainId());
    CPPUNIT_ASSERT_EQUAL(seconds, refStamp.getSeconds());
    CPPUNIT_ASSERT_EQUAL(fracAttoSecs, refStamp.getFractionalSeconds());

    Hash systemHash;
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotGetSystemInfo", Hash()).timeout(timeOutInMs).receive(systemHash));
    CPPUNIT_ASSERT_EQUAL(systemHash.has("timeInfo"), true);
    timeHash = systemHash.get<Hash>("timeInfo");
    CPPUNIT_ASSERT_EQUAL(systemHash.has("user"), true);
    CPPUNIT_ASSERT_EQUAL(systemHash.has("broker"), true);
    CPPUNIT_ASSERT_EQUAL(timeHash.has("reference"), true);
    CPPUNIT_ASSERT_EQUAL(timeHash.has("time"), true);
    CPPUNIT_ASSERT_EQUAL(timeHash.has("timeServerId"), true);

    // ...and test real calculations of id
    // 1) exact match
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotIdOfEpochstamp", seconds, 2ull * periodInAttoSec + 1100ull)
                .timeout(timeOutInMs)
                .receive(id));
    CPPUNIT_ASSERT_EQUAL(startId, id);

    // 2) end of id
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotIdOfEpochstamp", seconds, 3ull * periodInAttoSec + 1099ull)
                .timeout(timeOutInMs)
                .receive(id));
    CPPUNIT_ASSERT_EQUAL(startId, id);

    // 3) multiple of period above - but same second
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotIdOfEpochstamp", seconds, 5ull * periodInAttoSec + 1100ull)
                .timeout(timeOutInMs)
                .receive(id));
    CPPUNIT_ASSERT_EQUAL(startId + 3ull, id);

    // 4) multiple of period plus a bit above - next second
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotIdOfEpochstamp", seconds + 1ull, 5ull * periodInAttoSec + 1105ull)
                .timeout(timeOutInMs)
                .receive(id));
    CPPUNIT_ASSERT_EQUAL(startId + 13ull, id);

    // 5) just before
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotIdOfEpochstamp", seconds, 2ull * periodInAttoSec + 1090ull)
                .timeout(timeOutInMs)
                .receive(id));
    CPPUNIT_ASSERT_EQUAL(startId - 1ull, id);

    // 6) several before - but same second
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotIdOfEpochstamp", seconds, 1ull).timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId - 3ull, id);

    // 7) several before - previous second
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotIdOfEpochstamp", seconds - 1ull, 5ull * periodInAttoSec + 1110ull)
                .timeout(timeOutInMs)
                .receive(id));
    CPPUNIT_ASSERT_EQUAL(startId - 7ull, id);

    // 8) so much in the past that a negative id would be calculated which leads to zero
    id = 1111ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp", seconds - 100ull, 1110ull)
                                  .timeout(timeOutInMs)
                                  .receive(id));
    CPPUNIT_ASSERT_EQUAL(0ull, id);

    std::clog << "OK." << std::endl;
}


void Device_Test::testSchemaInjection() {
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

    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotAppendSchema", schema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient.
    CPPUNIT_ASSERT(waitForCondition(
          [this] { return m_deviceClient->getActiveSchema("TestDevice").has("injectedInt32"); }, cacheUpdateWaitMs));

    int injectedInt32;
    m_deviceClient->get("TestDevice", "injectedInt32", injectedInt32);
    CPPUNIT_ASSERT(injectedInt32 == 1);
    m_deviceClient->set("TestDevice", "injectedInt32", 5);
    m_deviceClient->get("TestDevice", "injectedInt32", injectedInt32);
    CPPUNIT_ASSERT(injectedInt32 == 5);

    // Checks that injecting a new attribute keeps the previously set value.
    // ----------
    INT32_ELEMENT(schema).key("injectedInt32").assignmentOptional().defaultValue(2).reconfigurable().minInc(1).commit();

    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotAppendSchema", schema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient
    CPPUNIT_ASSERT(waitForCondition(
          [this] { return m_deviceClient->getActiveSchema("TestDevice").getDefaultValue<int>("injectedInt32") == 2; },
          cacheUpdateWaitMs));

    m_deviceClient->get("TestDevice", "injectedInt32", injectedInt32);
    CPPUNIT_ASSERT(injectedInt32 == 5);
    Schema devFullSchema;
    devFullSchema = m_deviceClient->getDeviceSchema("TestDevice");
    CPPUNIT_ASSERT(devFullSchema.getMinInc<int>("injectedInt32") == 1);
    CPPUNIT_ASSERT(devFullSchema.getDisplayType("lockedBy") == "lockedBy");

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

    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotUpdateSchema", schema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient
    CPPUNIT_ASSERT(waitForCondition(
          [this]() { return m_deviceClient->getActiveSchema("TestDevice").getDefaultValue<int>("injectedInt32") == 3; },
          cacheUpdateWaitMs));

    m_deviceClient->get("TestDevice", "injectedInt32", injectedInt32);
    CPPUNIT_ASSERT(injectedInt32 == 5);
    devFullSchema = m_deviceClient->getDeviceSchema("TestDevice");
    CPPUNIT_ASSERT(devFullSchema.getMinInc<int>("injectedInt32") == 2);
    CPPUNIT_ASSERT(devFullSchema.getMaxInc<int>("injectedInt32") == 10);

    CPPUNIT_ASSERT(m_deviceClient->getActiveSchema("TestDevice").has("table"));
    const std::vector<Hash>& tableAfterUpdate = m_deviceClient->get<std::vector<Hash>>("TestDevice", "table");
    CPPUNIT_ASSERT(tableAfterUpdate.size() == 2);
    const Hash& firstRowAfterUpdate = tableAfterUpdate[0];
    CPPUNIT_ASSERT(firstRowAfterUpdate.get<std::string>("name") == "firstLine");

    // Checks that doing updateSchema with something else loses injectedInt32.
    // ----------
    Schema sndSchema;
    INT32_ELEMENT(sndSchema).key("somethingElse").assignmentOptional().defaultValue(4).reconfigurable().commit();

    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotUpdateSchema", sndSchema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient
    CPPUNIT_ASSERT(waitForCondition(
          [this]() { return m_deviceClient->getActiveSchema("TestDevice").has("somethingElse"); }, cacheUpdateWaitMs));

    std::vector<std::string> propertiesPaths = m_deviceClient->getProperties("TestDevice");
    int freq = std::count(propertiesPaths.begin(), propertiesPaths.end(), "injectedInt32");
    CPPUNIT_ASSERT(freq == 0);
    freq = std::count(propertiesPaths.begin(), propertiesPaths.end(), "somethingElse");
    CPPUNIT_ASSERT(freq == 1);

    // Checks that updateSchema for a parameter three times keeps the original value.
    // This verifies that the schema parsing check is correct.
    // ----------
    m_deviceClient->set<int>("TestDevice", "somethingElse", 42);
    Schema trdSchema;
    INT32_ELEMENT(trdSchema).key("somethingElse").assignmentOptional().defaultValue(5).reconfigurable().commit();

    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotUpdateSchema", trdSchema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient
    CPPUNIT_ASSERT(waitForCondition(
          [this] { return m_deviceClient->getActiveSchema("TestDevice").getDefaultValue<int>("somethingElse") == 5; },
          cacheUpdateWaitMs));

    Schema forthSchema;
    INT32_ELEMENT(forthSchema).key("somethingElse").assignmentOptional().defaultValue(6).reconfigurable().commit();

    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotUpdateSchema", forthSchema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient
    CPPUNIT_ASSERT(waitForCondition(
          [this] { return m_deviceClient->getActiveSchema("TestDevice").getDefaultValue<int>("somethingElse") == 6; },
          cacheUpdateWaitMs));

    Schema fifthSchema;

    INT32_ELEMENT(fifthSchema)
          .key("somethingElse")
          .assignmentOptional()
          .defaultValue(7)
          .minInc(3)
          .reconfigurable()
          .commit();

    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotUpdateSchema", fifthSchema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient
    CPPUNIT_ASSERT(waitForCondition(
          [this] { return m_deviceClient->getActiveSchema("TestDevice").getDefaultValue<int>("somethingElse") == 7; },
          cacheUpdateWaitMs));

    CPPUNIT_ASSERT(m_deviceClient->get<int>("TestDevice", "somethingElse") == 42);

    // Checks that doing updateSchema with an empty schema resets the device to its
    // base schema.
    // ----------
    Schema emptySchema;

    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotUpdateSchema", emptySchema).timeout(requestTimeoutMs).receive());

    // Waits for the updated schema to be available from the DeviceClient.
    CPPUNIT_ASSERT(waitForCondition(
          [this]() { return !m_deviceClient->getActiveSchema("TestDevice").has("somethingElse"); }, cacheUpdateWaitMs));

    propertiesPaths = m_deviceClient->getProperties("TestDevice");
    freq = std::count(propertiesPaths.begin(), propertiesPaths.end(), "somethingElse");
    CPPUNIT_ASSERT(freq == 0);
    devFullSchema = m_deviceClient->getDeviceSchema("TestDevice");
    Schema devStaticSchema = m_deviceClient->getClassSchema("testServerDevice", "TestDevice");
    CPPUNIT_ASSERT(karabo::util::similar(devFullSchema, devStaticSchema));

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

        CPPUNIT_ASSERT_NO_THROW(
              sigSlotA->request("TestDevice", "slotAppendSchema", schemaIdx).timeout(requestTimeoutMs).receive());
    }

    // Waits for the updated schema to be available from the DeviceClient.
    CPPUNIT_ASSERT(waitForCondition(
          [this, &propertyStr]() { return m_deviceClient->getDeviceSchema("TestDevice").has(propertyStr + "9"); },
          cacheUpdateWaitMs));

    propertiesPaths = m_deviceClient->getProperties("TestDevice");
    devFullSchema = m_deviceClient->getDeviceSchema("TestDevice");
    for (int i = 0; i < 10; i++) {
        std::string keyStr = propertyStr + std::to_string(i);
        freq = std::count(propertiesPaths.begin(), propertiesPaths.end(), keyStr);
        CPPUNIT_ASSERT(freq == 1);
        CPPUNIT_ASSERT(devFullSchema.has(keyStr));
        CPPUNIT_ASSERT(m_deviceClient->get<int>("TestDevice", keyStr) == i);
    }

    // Asserts that all the appendSchema calls from the latest changes preserved the
    // TABLE_ELEMENT in the device's static schema.
    CPPUNIT_ASSERT(m_deviceClient->getDeviceSchema("TestDevice").has("table"));
    const std::vector<Hash>& tableAfterInsert = m_deviceClient->get<std::vector<Hash>>("TestDevice", "table");
    CPPUNIT_ASSERT(tableAfterInsert.size() == 2);
    const Hash& firstRowAfterInsert = tableAfterInsert[0];
    CPPUNIT_ASSERT(firstRowAfterInsert.get<std::string>("name") == "firstLine");

    // Reset to static Schema for next test
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotUpdateSchema", Schema()).timeout(requestTimeoutMs).receive());
}


void Device_Test::testSchemaWithAttrUpdate() {
    // Setup a communication helper
    auto sigSlotA = m_deviceServer;

    // Timeout, in milliseconds, for a request for one of the test device slots.
    const int requestTimeoutMs = 2000;
    // Time, in milliseconds, to wait for DeviceClient to update its internal cache after a schema change.
    const int cacheUpdateWaitMs = 1000;

    // Updates 'alarmHigh'
    Schema schema;
    double alarmHighValue = 2.0 * TestDevice::ALARM_HIGH;
    DOUBLE_ELEMENT(schema)
          .key("valueWithAlarm")
          .readOnly()
          .alarmHigh(alarmHighValue)
          .needsAcknowledging(false)
          .commit();
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotUpdateSchema", schema).timeout(requestTimeoutMs).receive());

    // Checks that the updated attribute will be available within an interval.
    CPPUNIT_ASSERT(waitForCondition(
          [this, alarmHighValue] {
              return m_deviceClient->getDeviceSchema("TestDevice").getAlarmHigh<double>("valueWithAlarm") ==
                     alarmHighValue;
          },
          cacheUpdateWaitMs));

    // Tests that doing updateSchema with something new resets the AlarmHigh.
    Schema someNewSchema;
    INT32_ELEMENT(someNewSchema).key("somethingNew").assignmentOptional().defaultValue(4).reconfigurable().commit();
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotUpdateSchema", someNewSchema).timeout(requestTimeoutMs).receive());
    // Checks that the reset attribute will be available within an interval.
    CPPUNIT_ASSERT(waitForCondition(
          [this] {
              return m_deviceClient->getDeviceSchema("TestDevice").getAlarmHigh<double>("valueWithAlarm") ==
                     TestDevice::ALARM_HIGH;
          },
          cacheUpdateWaitMs));

    // Updates 'alarmHigh' by using 'slotUpdateSchemaAttributes' - this
    // is what the GUI Server would do when instantiating a device.
    alarmHighValue *= 2.0; // 4 * TestDevice::ALARM_HIGH
    std::vector<Hash> newAttrs{Hash("path", "valueWithAlarm", "attribute", "alarmHigh", "value", alarmHighValue)};
    Hash dummy;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchemaAttributes", newAttrs)
                                  .timeout(requestTimeoutMs)
                                  .receive(dummy));
    // Checks that the new attribute value will be available within an interval.
    CPPUNIT_ASSERT(waitForCondition(
          [this, alarmHighValue] {
              return m_deviceClient->getDeviceSchema("TestDevice").getAlarmHigh<double>("valueWithAlarm") ==
                     alarmHighValue;
          },
          cacheUpdateWaitMs));

    // Tests that doing updateSchema with something new resets the AlarmHigh.
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotUpdateSchema", someNewSchema).timeout(requestTimeoutMs).receive());
    // Checks that the reset attribute will be available within an interval.
    CPPUNIT_ASSERT(waitForCondition(
          [this] {
              return m_deviceClient->getDeviceSchema("TestDevice").getAlarmHigh<double>("valueWithAlarm") ==
                     TestDevice::ALARM_HIGH;
          },
          cacheUpdateWaitMs));


    // Reset to static Schema for next test
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotUpdateSchema", Schema()).timeout(requestTimeoutMs).receive());
}


void Device_Test::testSchemaWithAttrAppend() {
    // Setup a communication helper
    auto sigSlotA = m_deviceServer;

    // Timeout, in milliseconds, for a request for one of the test device slots.
    const int requestTimeoutMs = 2000;
    // Time, in milliseconds, to wait for DeviceClient to update its internal cache after a schema change.
    const int cacheUpdateWaitMs = 1000;

    // Updates 'alarmHigh'
    Schema schema;
    double alarmHighValue = 2.0 * TestDevice::ALARM_HIGH;
    DOUBLE_ELEMENT(schema)
          .key("valueWithAlarm")
          .readOnly()
          .alarmHigh(alarmHighValue)
          .needsAcknowledging(false)
          .commit();
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotUpdateSchema", schema).timeout(requestTimeoutMs).receive());

    // Checks that the updated attribute will be available within an interval.
    CPPUNIT_ASSERT(waitForCondition(
          [this, alarmHighValue] {
              return m_deviceClient->getDeviceSchema("TestDevice").getAlarmHigh<double>("valueWithAlarm") ==
                     alarmHighValue;
          },
          cacheUpdateWaitMs));

    // Tests that doing appendSchema with something new keeps the AlarmHigh.
    Schema someNewSchema;
    INT32_ELEMENT(someNewSchema).key("somethingNew").assignmentOptional().defaultValue(4).reconfigurable().commit();
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotAppendSchema", someNewSchema).timeout(requestTimeoutMs).receive());
    // Checks that the reset attribute will be available within an interval.
    CPPUNIT_ASSERT(waitForCondition(
          [this, alarmHighValue] {
              return m_deviceClient->getDeviceSchema("TestDevice").getAlarmHigh<double>("valueWithAlarm") ==
                     alarmHighValue;
          },
          cacheUpdateWaitMs));

    // Updates 'alarmHigh' by using 'slotUpdateSchemaAttributes' - this
    // is what the GUI Server would do when instantiating a device.
    alarmHighValue *= 2.0; // 4 * TestDevice::ALARM_HIGH
    std::vector<Hash> newAttrs{Hash("path", "valueWithAlarm", "attribute", "alarmHigh", "value", alarmHighValue)};
    Hash dummy;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchemaAttributes", newAttrs)
                                  .timeout(requestTimeoutMs)
                                  .receive(dummy));
    // Checks that the new attribute value will be available within an interval.
    CPPUNIT_ASSERT(waitForCondition(
          [this, alarmHighValue] {
              return m_deviceClient->getDeviceSchema("TestDevice").getAlarmHigh<double>("valueWithAlarm") ==
                     alarmHighValue;
          },
          cacheUpdateWaitMs));

    // Reset to static Schema for next test
    CPPUNIT_ASSERT_NO_THROW(
          sigSlotA->request("TestDevice", "slotUpdateSchema", Schema()).timeout(requestTimeoutMs).receive());
}


void Device_Test::testChangeSchemaOutputChannel(const std::string& updateSlot) {
    std::clog << "Start testChangeSchemaOutputChannel for " << updateSlot << ": " << std::flush;
    // Timeout, in milliseconds, for a request for one of the test device slots.
    const int requestTimeoutMs = 2000;


    // Not using m_deviceClient->getDeviceSchema("TestDevice") since its cache might not be up-to-date yet
    // from schema "erasure" at the end of the previous run of this method with another 'updateSlot' value.
    // Our order guarantee does not apply since the m_deviceServer requested the update and not the m_device client...
    Schema deviceSchema;
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request("TestDevice", "slotGetSchema", false)
                                  .timeout(requestTimeoutMs)
                                  .receive(deviceSchema));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(deviceSchema), std::string("INTENSITY.TD"),
                                 deviceSchema.getAliasFromKey<std::string>("output.schema.data.intensityTD"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(deviceSchema), std::string("output.schema.data.intensityTD"),
                                 deviceSchema.getKeyFromAlias<std::string>("INTENSITY.TD"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(deviceSchema), std::string("UNTAGGED"),
                                 deviceSchema.getAliasFromKey<std::string>("output.schema.data.untagged"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(deviceSchema), std::string("output.schema.data.untagged"),
                                 deviceSchema.getKeyFromAlias<std::string>("UNTAGGED"));

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
              .setDaqDataType(karabo::util::DaqDataType::TRAIN)
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

    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request("TestDevice", updateSlot, schema).timeout(requestTimeoutMs).receive());

    // Check aliases
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request("TestDevice", "slotGetSchema", false)
                                  .timeout(requestTimeoutMs)
                                  .receive(deviceSchema));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(deviceSchema), std::string("UNTAGGED.CHANGED"),
                                 deviceSchema.getAliasFromKey<std::string>("output.schema.data.untagged"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(deviceSchema), std::string("output.schema.data.untagged"),
                                 deviceSchema.getKeyFromAlias<std::string>("UNTAGGED.CHANGED"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(deviceSchema), std::string("INTENSITY.TD2"),
                                 deviceSchema.getAliasFromKey<std::string>("output.schema.data.intensityTD2"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(deviceSchema), std::string("output.schema.data.intensityTD2"),
                                 deviceSchema.getKeyFromAlias<std::string>("INTENSITY.TD2"));

    // Reset to static Schema for next test
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request("TestDevice", "slotUpdateSchema", Schema()).timeout(requestTimeoutMs).receive());
    std::clog << "OK." << std::endl;
}


void Device_Test::testOutputRecreatesOnSchemaChange(const std::string& updateSlot) {
    std::clog << "Start testOutputRecreatesOnSchemaChange for " << updateSlot << ": " << std::flush;

    const std::string& senderId("TestDevice");
    const std::string& receiverId("receiver");

    // Setup receiver device that should connect.
    std::pair<bool, std::string> success =
          m_deviceClient->instantiate("testServerDevice", "TestDevice",
                                      Hash("deviceId", receiverId, "input.connectedOutputChannels",
                                           std::vector<std::string>({senderId + ":output"})),
                                      KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    // Test connection is setup
    CPPUNIT_ASSERT_MESSAGE(toString(m_deviceClient->get(receiverId)),
                           waitForCondition(
                                 [this, receiverId]() {
                                     return m_deviceClient
                                           ->get<std::vector<std::string>>(receiverId, "input.missingConnections")
                                           .empty();
                                 },
                                 KRB_TEST_MAX_TIMEOUT * 1000));

    // Tell server (as helper) to listen for updates of "input.missingConnections"
    // Note: Since we cannot remove the slot from the server again, we choose a test run dependent slot name
    //       and disconnect at the end. So the slot lambda (that takes variables that are local to the test by
    //       reference) cannot be called later - it would likely crash.
    std::mutex connectionChangesMutex;
    std::vector<std::vector<std::string>> connectionChanges;
    auto changedHandler = [&connectionChanges, &connectionChangesMutex, receiverId](const karabo::util::Hash& h,
                                                                                    const std::string& id) {
        if (id == receiverId && h.has("input.missingConnections")) {
            std::lock_guard<std::mutex> lock(connectionChangesMutex);
            connectionChanges.push_back(h.get<std::vector<std::string>>("input.missingConnections"));
        }
    };
    const std::string slotConnectionChanged("slotConnectionChanged_" + updateSlot);
    m_deviceServer->registerSlot<karabo::util::Hash, std::string>(changedHandler, slotConnectionChanged);
    const bool connected = m_deviceServer->connect(receiverId, "signalChanged", "", slotConnectionChanged);
    CPPUNIT_ASSERT(connected);

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
        CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request(senderId, updateSlot, schemaToInject)
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
            CPPUNIT_ASSERT_EQUAL_MESSAGE(karabo::util::toString(connectionChanges), triggerReconnect, changed);
            if (triggerReconnect) {
                CPPUNIT_ASSERT_EQUAL_MESSAGE(karabo::util::toString(connectionChanges), 2ul, connectionChanges.size());
                CPPUNIT_ASSERT_EQUAL(std::vector<std::string>({senderId + ":output"}), connectionChanges[0]);
                CPPUNIT_ASSERT_EQUAL(std::vector<std::string>(), connectionChanges[1]);
            }
        }
        // Remove schema changes again:
        CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request(senderId, "slotUpdateSchema", Schema())
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
            CPPUNIT_ASSERT_MESSAGE(karabo::util::toString(connectionChanges), changed);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(karabo::util::toString(connectionChanges), 4ul, connectionChanges.size());
            CPPUNIT_ASSERT_EQUAL(std::vector<std::string>({senderId + ":output"}), connectionChanges[2]);
            CPPUNIT_ASSERT_EQUAL(std::vector<std::string>(), connectionChanges[3]);
        }
        // Clean-up for next round
        std::lock_guard<std::mutex> lock(connectionChangesMutex);
        connectionChanges.clear();
    }

    // Clean up
    m_deviceServer->disconnect(receiverId, "signalChanged", "", slotConnectionChanged);
    // Cannot remove slotConnectionChanged...
    success = m_deviceClient->killDevice(receiverId, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    std::clog << "OK." << std::endl;
}

void Device_Test::testInputOutputChannelInjection(const std::string& updateSlot) {
    std::clog << "Start testInputOutputChannelInjection for " << updateSlot << ": " << std::flush;

    // Setup a communication helper
    auto sigSlot = m_deviceServer;

    // Timeout, in milliseconds, for a request for one of the test device slots.
    const int requestTimeoutMs = 2000;
    // Time, in milliseconds, to wait for DeviceClient to update its internal cache after a schema change.
    const int cacheUpdateWaitMs = 1000;

    // At the beginning, only the static channel is there:
    std::vector<std::string> outputChannels;
    CPPUNIT_ASSERT_NO_THROW(sigSlot->request("TestDevice", "slotGetOutputChannelNames")
                                  .timeout(requestTimeoutMs)
                                  .receive(outputChannels));
    CPPUNIT_ASSERT_EQUAL(1ul, outputChannels.size());
    CPPUNIT_ASSERT_EQUAL(std::string("output"), outputChannels[0]);

    // Checks that appendSchema creates injected input and output channels
    // ----------
    Schema dataSchema;
    INT32_ELEMENT(dataSchema).key("int32").readOnly().commit();
    Schema schema;
    OUTPUT_CHANNEL(schema).key("injectedOutput").dataSchema(dataSchema).commit();
    INPUT_CHANNEL(schema).key("injectedInput").dataSchema(dataSchema).commit();
    OVERWRITE_ELEMENT(schema)
          .key("injectedInput.connectedOutputChannels")
          .setNewDefaultValue<std::vector<std::string>>({"TestDevice:injectedOutput", "TestDevice:output"})
          .commit();
    NODE_ELEMENT(schema)
          .key("emptyNode") // Already in static schema - but without leaves
          .commit();
    INT32_ELEMENT(schema).key("emptyNode.anInt32").readOnly().initialValue(42).commit();

    CPPUNIT_ASSERT_NO_THROW(sigSlot->request("TestDevice", updateSlot, schema).timeout(requestTimeoutMs).receive());

    // Now, also the injectedOutput is there:
    outputChannels.clear();
    CPPUNIT_ASSERT_NO_THROW(sigSlot->request("TestDevice", "slotGetOutputChannelNames")
                                  .timeout(requestTimeoutMs)
                                  .receive(outputChannels));
    CPPUNIT_ASSERT_EQUAL(2ul, outputChannels.size());
    CPPUNIT_ASSERT(std::find(outputChannels.begin(), outputChannels.end(), "output") != outputChannels.end());
    CPPUNIT_ASSERT(std::find(outputChannels.begin(), outputChannels.end(), "injectedOutput") != outputChannels.end());

    // Check that, after some time, the injected input is connected to both, the injected and the static output
    auto inputsConnected = [this]() {
        const Hash cfg(m_deviceClient->get("TestDevice"));
        if (cfg.has("output.connections") && cfg.has("injectedOutput.connections")) {
            std::vector<Hash> tableStatic, tableInjected;
            cfg.get("output.connections", tableStatic);
            cfg.get("injectedOutput.connections", tableInjected);
            if (tableStatic.size() == 1ul && tableInjected.size() == 1ul &&
                tableStatic[0].get<std::string>("remoteId") == "TestDevice:injectedInput" &&
                tableInjected[0].get<std::string>("remoteId") == "TestDevice:injectedInput"
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

    CPPUNIT_ASSERT_MESSAGE(toString(m_deviceClient->get("TestDevice")), ok);

    // Now START test that re-injecting an input channel keeps handlers registered with KARABO_ON_DATA.
    // Register data handler for "injectedInput" channel
    CPPUNIT_ASSERT_NO_THROW(sigSlot->request("TestDevice", "slotRegisterOnDataInputEos", "injectedInput")
                                  .timeout(requestTimeoutMs)
                                  .receive());
    // Check that initially "intInOnData" is not one, i.e. ensure that following actions will make it one.
    // (It is either zero [initial value] or -2 [from previous run of this test with other updateSlot].)
    CPPUNIT_ASSERT(1 != m_deviceClient->get<int>("TestDevice", "intInOnData"));
    const int countEosCalls = m_deviceClient->get<int>("TestDevice", "numCallsOnInput");

    // Request data to be sent from "output" to "injectedInput" channel
    CPPUNIT_ASSERT_NO_THROW(
          sigSlot->request("TestDevice", "slotSendToOutputChannel", "output", 1).timeout(requestTimeoutMs).receive());
    // Check that data arrived and onData/onInput handlers called
    waitForCondition(
          [this, countEosCalls]() {
              return (1 == m_deviceClient->get<int>("TestDevice", "intInOnData") &&
                      countEosCalls + 1 == m_deviceClient->get<int>("TestDevice", "numCallsOnInput"));
          },
          cacheUpdateWaitMs);
    CPPUNIT_ASSERT_EQUAL(1, m_deviceClient->get<int>("TestDevice", "intInOnData"));
    CPPUNIT_ASSERT_EQUAL(countEosCalls + 1, m_deviceClient->get<int>("TestDevice", "numCallsOnInput"));

    // Request EOS to be sent to "injectedInput" channel.
    // All outputs an input is connected to have to send EOS to get the eos handler called...
    CPPUNIT_ASSERT_NO_THROW(
          sigSlot->request("TestDevice", "slotSendEos", std::vector<std::string>({"output", "injectedOutput"}))
                .timeout(requestTimeoutMs)
                .receive());
    // Check that EOS arrived and flipped sign
    waitForCondition([this]() { return (-1 == m_deviceClient->get<int>("TestDevice", "intInOnData")); },
                     cacheUpdateWaitMs);
    CPPUNIT_ASSERT_EQUAL(-1, m_deviceClient->get<int>("TestDevice", "intInOnData"));

    // Re-inject input - channel will be recreated and onData handler should be passed to new incarnation
    Schema inputOnlySchema;
    INPUT_CHANNEL(inputOnlySchema).key("injectedInput").dataSchema(dataSchema).commit();
    // Note that here we need to use "slotAppendSchema" and not updateSlot since "slotUpdateSchema" would erase
    // "injectedInput".
    CPPUNIT_ASSERT_NO_THROW(
          sigSlot->request("TestDevice", "slotAppendSchema", inputOnlySchema).timeout(requestTimeoutMs).receive());
    // Wait for connection being re-established
    // HACK: Without sleep might be fooled, i.e. traces of connection of previous input channel not yet erased...
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    ok = waitForCondition(inputsConnected, cacheUpdateWaitMs * 20); // longer timeout again, see above
    CPPUNIT_ASSERT_MESSAGE(toString(m_deviceClient->get("TestDevice")), ok);
    // Request again data to be sent from "output" to "injectedInput" channel
    CPPUNIT_ASSERT_NO_THROW(
          sigSlot->request("TestDevice", "slotSendToOutputChannel", "output", 2).timeout(requestTimeoutMs).receive());
    // Check that new data arrived
    waitForCondition(
          [this, countEosCalls]() {
              return (2 == m_deviceClient->get<int>("TestDevice", "intInOnData") &&
                      countEosCalls + 2 == m_deviceClient->get<int>("TestDevice", "numCallsOnInput"));
          },
          cacheUpdateWaitMs);
    CPPUNIT_ASSERT_EQUAL(2, m_deviceClient->get<int>("TestDevice", "intInOnData"));
    CPPUNIT_ASSERT_EQUAL(countEosCalls + 2, m_deviceClient->get<int>("TestDevice", "numCallsOnInput"));
    // Request EOS to be sent again
    CPPUNIT_ASSERT_NO_THROW(
          sigSlot->request("TestDevice", "slotSendEos", std::vector<std::string>({"output", "injectedOutput"}))
                .timeout(requestTimeoutMs)
                .receive());
    // Check that EOS arrived and flipped sign again
    waitForCondition([this]() { return (-2 == m_deviceClient->get<int>("TestDevice", "intInOnData")); },
                     cacheUpdateWaitMs);
    CPPUNIT_ASSERT_EQUAL(-2, m_deviceClient->get<int>("TestDevice", "intInOnData"));
    //
    // END test that re-injecting input channels keeps handlers registered with KARABO_ON_DATA/KARABO_ON_EOS!

    // Remove the channels again:
    CPPUNIT_ASSERT_NO_THROW(
          sigSlot->request("TestDevice", "slotUpdateSchema", Schema()).timeout(requestTimeoutMs).receive());
    // Now only the static OutputChannel is kept
    outputChannels.clear();
    CPPUNIT_ASSERT_NO_THROW(sigSlot->request("TestDevice", "slotGetOutputChannelNames")
                                  .timeout(requestTimeoutMs)
                                  .receive(outputChannels));
    CPPUNIT_ASSERT_EQUAL(1ul, outputChannels.size());
    CPPUNIT_ASSERT_EQUAL(std::string("output"), outputChannels[0]);

    // TODO: We directly call slotGetConfiguration instead of using m_deviceClient->get("TestDevice"):
    //       Looks like the client cache will not erase removed properties.
    Hash cfg;
    std::string dummy;
    sigSlot->request("TestDevice", "slotGetConfiguration").timeout(requestTimeoutMs).receive(cfg, dummy);
    CPPUNIT_ASSERT_MESSAGE(toString(cfg), !cfg.has("injectedOutput"));
    CPPUNIT_ASSERT_MESSAGE(toString(cfg), !cfg.has("injectedInput"));
    // Not channel related - 'emptyNode' kept, but injected anInt32 not:
    CPPUNIT_ASSERT_MESSAGE(toString(cfg), !cfg.has("emptyNode.anInt32"));
    CPPUNIT_ASSERT_MESSAGE(toString(cfg), cfg.has("emptyNode"));

    std::clog << "OK." << std::endl;
}


void Device_Test::testNodedSlot() {
    std::clog << "Start testNodedSlot: " << std::flush;
    // Note that calling "node_slot" would work as well... :-|
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestDevice", "node.slot", KRB_TEST_MAX_TIMEOUT));

    // Check also that slot is properly recorded as lastCommand
    CPPUNIT_ASSERT_EQUAL(std::string("node.slot <- ") += m_deviceClient->getInstanceId(),
                         m_deviceClient->get<std::string>("TestDevice", "lastCommand"));

    std::clog << "OK." << std::endl;
}


void Device_Test::testGetconfigReconfig() {
    std::clog << "Start testGetconfigReconfig: " << std::flush;

    const int timeoutInMs = 10000;
    const std::string deviceId("TestDevice");

    // Check device properties
    Hash cfgHash;
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(cfgHash));
    CPPUNIT_ASSERT_EQUAL(deviceId, cfgHash.get<std::string>("deviceId"));
    CPPUNIT_ASSERT_EQUAL(std::string("TestDevice"), cfgHash.get<std::string>("classId"));
    CPPUNIT_ASSERT_EQUAL(fakeClassVersion, cfgHash.get<std::string>("classVersion"));
    CPPUNIT_ASSERT_EQUAL(karabo::util::Version::getVersion(), cfgHash.get<std::string>("karaboVersion"));
    CPPUNIT_ASSERT_EQUAL(std::string("testServerDevice"), cfgHash.get<std::string>("serverId"));
    CPPUNIT_ASSERT_EQUAL(::getpid(), cfgHash.get<int>("pid"));

    // test pipeline channel schema is an empty node or has empty nodes under it.
    assertChildNodesEmpty(cfgHash.get<Hash>("output.schema"));

    // But we can set the performance statistics
    cfgHash.clear();
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotReconfigure", Hash("performanceStatistics.enable", true))
                .timeout(timeoutInMs)
                .receive());
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(cfgHash));
    CPPUNIT_ASSERT_EQUAL(true, cfgHash.get<bool>("performanceStatistics.enable"));

    // Test the lastCommand for slotReconfigure
    CPPUNIT_ASSERT_EQUAL(std::string("slotReconfigure <- ") += m_deviceServer->getInstanceId(),
                         cfgHash.get<std::string>("lastCommand"));

    // Now try to set performanceStatistics again, but with an old timestamp - that should not be taken!
    const Timestamp enableTimestamp(
          Timestamp::fromHashAttributes(cfgHash.getAttributes("performanceStatistics.enable")));
    const Epochstamp pastEpochstamp(
          enableTimestamp.getSeconds() - 3ull * 3600ull, // 3 hours back: no CET/CEST vs UTC confusion
          enableTimestamp.getFractionalSeconds());
    const Timestamp pastTimestamp(pastEpochstamp, enableTimestamp.getTrainstamp());
    Hash hToSet;
    Hash::Attributes& attrs = hToSet.set("performanceStatistics.enable", false).getAttributes();
    pastTimestamp.toHashAttributes(attrs);
    cfgHash.clear();
    const Timestamp beforeSetStamp;
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotReconfigure", hToSet).timeout(timeoutInMs).receive());
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(cfgHash));

    const Timestamp receivedStamp(Timestamp::fromHashAttributes(cfgHash.getAttributes("performanceStatistics.enable")));
    CPPUNIT_ASSERT_MESSAGE(receivedStamp.toIso8601Ext() += " " + pastTimestamp.toIso8601Ext(),
                           receivedStamp != pastTimestamp);
    CPPUNIT_ASSERT_MESSAGE(receivedStamp.toIso8601Ext() += " " + beforeSetStamp.toIso8601Ext(),
                           receivedStamp.getEpochstamp() > beforeSetStamp.getEpochstamp()); // cannot compare Timestamps

    // Now test slotGetConfigurationSlice
    const std::vector<std::string> selectedPaths({"performanceStatistics.enable", "vecString", "table"});
    Hash arg("paths", selectedPaths);
    const Hash slice =
          m_deviceClient->execute1<Hash, Hash>(deviceId, "slotGetConfigurationSlice", timeoutInMs / 1000, arg);
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3u), slice.size());

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
    CPPUNIT_ASSERT_MESSAGE(toString(cfgHash) + " vs\n" + toString(slice),
                           slice.fullyEquals(cfgHash, false)); // false: order does not matter

    // Request for a non-existing path fails with remote exception that originates from a ParameterException
    bool exceptionCaught = false;
    arg.set("paths", std::vector<std::string>(1, "not_a_property"));
    try {
        m_deviceClient->execute1<Hash, Hash>(deviceId, "slotGetConfigurationSlice", timeoutInMs / 1000, arg);
    } catch (const karabo::util::RemoteException& e) {
        exceptionCaught = true;
        const std::string& det = e.details();
        CPPUNIT_ASSERT_MESSAGE(det, det.find("Exception Type....:  Parameter Exception") != std::string::npos);
        CPPUNIT_ASSERT_MESSAGE(det, det.find("Key 'not_a_property' does not exist") != std::string::npos);
    } catch (...) {
    };
    CPPUNIT_ASSERT(exceptionCaught);

    std::clog << "OK." << std::endl;
}


void Device_Test::testUpdateState() {
    const std::string deviceId("TestDevice");

    // Check initial state of test device
    const State state(m_deviceClient->get<State>(deviceId, "state"));
    CPPUNIT_ASSERT_MESSAGE("State is " + state.name(), state == State::UNKNOWN);
    CPPUNIT_ASSERT_EQUAL(0u, m_deviceClient->get<unsigned int>(deviceId, "countStateToggles"));
    CPPUNIT_ASSERT(std::abs(-1. - m_deviceClient->get<double>(deviceId, "valueWithAlarm")) > 1.e-7);

    const int timeOutInMs = 1000 * KRB_TEST_MAX_TIMEOUT;
    Hash hash;
    m_deviceServer->request(deviceId, "slotPing", deviceId, 1, false).timeout(timeOutInMs).receive(hash);
    CPPUNIT_ASSERT_EQUAL(std::string("unknown"), hash.get<std::string>("status"));

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
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotToggleState", msg).timeout(timeOutInMs).receive(reply));
    CPPUNIT_ASSERT_EQUAL(std::string("NORMAL"), reply);
    m_deviceServer->request(deviceId, "slotPing", deviceId, 1, false).timeout(timeOutInMs).receive(hash);
    CPPUNIT_ASSERT_EQUAL(std::string("ok"), hash.get<std::string>("status"));

    // ... test that the state was switched,
    const State state1(m_deviceClient->get<State>(deviceId, "state"));
    CPPUNIT_ASSERT_MESSAGE("State is " + state1.name(), state1 == State::NORMAL);

    // ... test that other values updated as well,
    CPPUNIT_ASSERT_EQUAL(1u, m_deviceClient->get<unsigned int>(deviceId, "countStateToggles"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1., m_deviceClient->get<double>(deviceId, "valueWithAlarm"), 1.e-7);

    reply = "";
    msg.set("state", "ERROR");
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotToggleState", msg).timeout(timeOutInMs).receive(reply));
    CPPUNIT_ASSERT_EQUAL(std::string("ERROR"), reply);
    m_deviceServer->request(deviceId, "slotPing", deviceId, 1, false).timeout(timeOutInMs).receive(hash);
    CPPUNIT_ASSERT_EQUAL(std::string("error"), hash.get<std::string>("status"));

    // ... test that the state was switched,
    const State state2 = m_deviceClient->get<State>(deviceId, "state");
    CPPUNIT_ASSERT_MESSAGE("State is " + state2.name(), state2 == State::ERROR);

    reply = "";
    msg.set("state", "NORMAL");
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotToggleState", msg).timeout(timeOutInMs).receive(reply));
    CPPUNIT_ASSERT_EQUAL(std::string("NORMAL"), reply);
    m_deviceServer->request(deviceId, "slotPing", deviceId, 1, false).timeout(timeOutInMs).receive(hash);
    CPPUNIT_ASSERT_EQUAL(std::string("ok"), hash.get<std::string>("status"));

    // ... test that the state was switched,
    const State state3 = m_deviceClient->get<State>(deviceId, "state");
    CPPUNIT_ASSERT_MESSAGE("State is " + state3.name(), state3 == State::NORMAL);

    // ... and finally test the desired timestamps:
    //     * state and valueWithAlarm get the same as given explicitly to updateState
    //     * countStateToggles gets the one mingled into the 'other' Hash
    const auto atto = karabo::util::ATTOSEC;
    const Hash cfg(m_deviceClient->get(deviceId));
    const Epochstamp stampStateNew(Epochstamp::fromHashAttributes(cfg.getAttributes("state")));
    CPPUNIT_ASSERT_MESSAGE(stampStateNew.toIso8601(atto) += " != " + stampState.toIso8601(atto),
                           stampStateNew == stampState);

    const Epochstamp stampValue(Epochstamp::fromHashAttributes(cfg.getAttributes("valueWithAlarm")));
    CPPUNIT_ASSERT_MESSAGE(stampValue.toIso8601(atto) += " != " + stampState.toIso8601(atto), stampValue == stampState);

    const Epochstamp stampToggleNew(Epochstamp::fromHashAttributes(cfg.getAttributes("countStateToggles")));
    CPPUNIT_ASSERT_MESSAGE(stampToggleNew.toIso8601(atto) += " != " + stampToggle.toIso8601(atto),
                           stampToggleNew == stampToggle);
}


void Device_Test::testSet() {
    std::clog << "Start testSet: " << std::flush;
    const int timeoutInMs = KRB_TEST_MAX_TIMEOUT * 1000;
    const std::string deviceId("TestDevice");

    // Setting a non-existing value throws
    CPPUNIT_ASSERT_THROW(
          m_deviceServer->request(deviceId, "slotSet", Hash("nonExistParam", 0)).timeout(timeoutInMs).receive(),
          karabo::util::RemoteException);

    // Setting a reconfigurable property outside its validation limits throws
    // (and even other valid changes in the same set(..) are ignored).
    Hash hash;
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash));
    CPPUNIT_ASSERT_EQUAL(0, hash.get<int>("valueWithLimit"));
    CPPUNIT_ASSERT_EQUAL(0, hash.get<int>("valueOther"));
    CPPUNIT_ASSERT_THROW(m_deviceServer
                               ->request(deviceId, "slotSet",
                                         Hash("valueWithLimit", 1000, // hit slimit
                                              "valueOther", 2000))    // would be OK
                               .timeout(timeoutInMs)
                               .receive(),
                         karabo::util::RemoteException);
    Hash hash2;
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash2));
    CPPUNIT_ASSERT(hash2.fullyEquals(hash)); // Also valueOther did not change


    // Other settings work
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotSet", Hash("valueWithLimit", 999, "valueOther", 2000))
                .timeout(timeoutInMs)
                .receive());
    hash2.clear();
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash2));
    CPPUNIT_ASSERT_EQUAL(999, hash2.get<int>("valueWithLimit"));
    CPPUNIT_ASSERT_EQUAL(2000, hash2.get<int>("valueOther"));

    std::clog << "OK." << std::endl;
}

void Device_Test::testSetVectorUpdate() {
    std::clog << "Start testSetVectorUpdate: " << std::flush;
    const int timeoutInMs = 10000;
    const std::string deviceId("TestDevice");

    using VectorUpdate = karabo::core::Device::VectorUpdate;

    Hash hash;
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash));
    CPPUNIT_ASSERT_EQUAL(std::vector<std::string>({"one", "two", "three"}),
                         hash.get<std::vector<std::string>>("vecString"));

    // Test adding
    // The "three"s will all added times (although it is already in)!
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer
                                  ->request(deviceId, "slotUpdateVecString",
                                            std::vector<std::string>({"three", "three", "one"}),
                                            static_cast<int>(VectorUpdate::add))
                                  .timeout(timeoutInMs)
                                  .receive());
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash));
    CPPUNIT_ASSERT_EQUAL(std::vector<std::string>({"one", "two", "three", "three", "three", "one"}),
                         hash.get<std::vector<std::string>>("vecString"));

    // Test addIfNotIn
    // Since "one" is already in, it will not be added again
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer
                                  ->request(deviceId, "slotUpdateVecString", std::vector<std::string>({"one", "seven"}),
                                            static_cast<int>(VectorUpdate::addIfNotIn))
                                  .timeout(timeoutInMs)
                                  .receive());
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash));
    CPPUNIT_ASSERT_EQUAL(std::vector<std::string>({"one", "two", "three", "three", "three", "one", "seven"}),
                         hash.get<std::vector<std::string>>("vecString"));

    // Test removeOne
    // Only first "one" and first "three" will be removed, "notIn" is ignored
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer
                                  ->request(deviceId, "slotUpdateVecString",
                                            std::vector<std::string>({"three", "one", "notIn"}),
                                            static_cast<int>(VectorUpdate::removeOne))
                                  .timeout(timeoutInMs)
                                  .receive());
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash));
    CPPUNIT_ASSERT_EQUAL(std::vector<std::string>({"two", "three", "three", "one", "seven"}),
                         hash.get<std::vector<std::string>>("vecString"));

    // Test removeAll
    // all "three"s and the "two" will be removed, "notIn" is ignored
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer
                                  ->request(deviceId, "slotUpdateVecString",
                                            std::vector<std::string>({"two", "notIn", "three"}),
                                            static_cast<int>(VectorUpdate::removeAll))
                                  .timeout(timeoutInMs)
                                  .receive());
    CPPUNIT_ASSERT_NO_THROW(
          m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash));
    CPPUNIT_ASSERT_EQUAL(std::vector<std::string>({"one", "seven"}), hash.get<std::vector<std::string>>("vecString"));

    // Finally test invalid updateType
    CPPUNIT_ASSERT_THROW(m_deviceServer->request(deviceId, "slotUpdateVecString", std::vector<std::string>(), 0)
                               .timeout(timeoutInMs)
                               .receive(),
                         karabo::util::RemoteException);

    std::clog << "OK." << std::endl;
}


void Device_Test::testSignal() {
    // Test that signals registered in constructor of devices inheriting from Device carry the signalInstanceId in
    // header (in 2.10.0 the SignalSlotable::init method is called after the constructor, so no id yet when
    // registering).

    std::string signalInstanceId;
    DeviceServer::WeakPointer weakServer(m_deviceServer);
    std::function<void()> slot = [&signalInstanceId, weakServer]() {
        DeviceServer::Pointer ptr(weakServer.lock());
        if (ptr) {
            const karabo::util::Hash::Pointer header(ptr->getSenderInfo("slotForSignalA")->getHeaderOfSender());
            signalInstanceId = header->get<std::string>("signalInstanceId");
        } else {
            std::clog << "DeviceServer pointer invalid!" << std::endl; // Should be impossible
        }
    };
    m_deviceServer->registerSlot(slot, "slotForSignalA");
    CPPUNIT_ASSERT(m_deviceServer->connect("TestDevice", "signalA", "", "slotForSignalA"));
    // If request returns, we can be sure that the signal has been received.
    // That order would be undefined if instead of 'm_deviceServer->request' we would use
    // 'm_deviceClient->execute' since signal is emitted to m_deviceServer.
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request("TestDevice", "slotEmitSignalA").timeout(5000).receive());

    CPPUNIT_ASSERT_EQUAL(std::string("TestDevice"), signalInstanceId);

    // Clean up
    m_deviceServer->disconnect("TestDevice", "signalA", "", "slotForSignalA");
    // m_deviceServer->removeSlot("slotForSignalA"); private, but who cares here...
}

void Device_Test::testBadInit() {
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
    CPPUNIT_ASSERT_NO_THROW(requestor.receive(ok, dummy));
    CPPUNIT_ASSERT(ok);

    // After instantiation, state switches to INIT, as soon as initialisation method runs.
    State devState(State::UNKNOWN);
    bool waitOk = waitForCondition(
          [this, devId, &devState]() {
              devState = m_deviceClient->get<State>(devId, "state");
              return (devState == State::INIT);
          },
          2000);
    CPPUNIT_ASSERT_MESSAGE(devState.name(), waitOk);

    // At end of initialization, state changes to NORMAL - wait for it...
    waitOk = waitForCondition(
          [this, devId, &devState]() {
              devState = m_deviceClient->get<State>(devId, "state");
              return (devState == State::NORMAL);
          },
          (delayInSec + 2) * 1000); // wait longer than delaying sleep
    CPPUNIT_ASSERT_MESSAGE(devState.name(), waitOk);

    m_deviceClient->killDeviceNoWait(devId);

    //
    // Case 2: The initialization method fails with an exception:
    //
    devId.back() = '2'; // let's take a new id to avoid delays until the previous device is down
    std::atomic<bool> instanceNewCalled(false);
    m_deviceClient->registerInstanceNewMonitor([&instanceNewCalled, devId](const karabo::util::Hash& topologyEntry) {
        if (topologyEntry.has("device." + devId)) instanceNewCalled = true;
    });
    std::atomic<bool> instanceGoneCalled(false);
    m_deviceClient->registerInstanceGoneMonitor(
          [&instanceGoneCalled, devId](const std::string& instanceId, const karabo::util::Hash& info) {
              if (devId == instanceId) instanceGoneCalled = true;
          });
    requestor = m_deviceServer
                      ->request("", "slotStartDevice",
                                Hash("classId", "TestDeviceBadInit", "deviceId", devId, "configuration",
                                     Hash("initProblem", "throw")))
                      .timeout(2000); // starting a device takes at least one second...
    // Despite the failing initialization, the device replies successfully, no timeout:
    ok = false;
    CPPUNIT_ASSERT_NO_THROW(requestor.receive(ok, dummy));
    CPPUNIT_ASSERT(ok);

    const bool newAndGone = waitForCondition(
          [&instanceNewCalled, &instanceGoneCalled]() { return (instanceNewCalled && instanceGoneCalled); }, 5000);
    CPPUNIT_ASSERT(newAndGone);
    // Reset handlers that use references to local variables
    m_deviceClient->registerInstanceNewMonitor([](const karabo::util::Hash&) {});
    m_deviceClient->registerInstanceGoneMonitor([](const std::string&, const karabo::util::Hash&) {});

    //
    // Case 3: A very long lasting initialization method (as case 1), with a try to shutdown while initialization:
    //

    devId.back() += 1; // another id again, see above ('+= 1' for Alessandro... )
    requestor = m_deviceServer
                      ->request("", "slotStartDevice",
                                Hash("classId", "TestDeviceBadInit", "deviceId", devId, "configuration",
                                     Hash("initProblem", "delay", "delay", delayInSec)))
                      .timeout(2000); // starting a device takes at least one second...
    // Although initialization sleeps 'delayInSec', no timeout within the 2 seconds we allow for that
    ok = false;
    CPPUNIT_ASSERT_NO_THROW(requestor.receive(ok, dummy));
    CPPUNIT_ASSERT(ok);

    // After instantiation, state switches to INIT, as soon as initialize method runs
    waitOk = waitForCondition(
          [this, devId, &devState]() {
              devState = m_deviceClient->get<State>(devId, "state");
              return (devState == State::INIT);
          },
          2000);
    const Epochstamp initStartedTime;
    CPPUNIT_ASSERT_MESSAGE(devState.name(), waitOk);

    // We kill the device that is still initializing: It will not die immediately (only once initialization is done),
    // but preDestruction is called.
    // Do not use client->killDevice(devId): that waits until device is really gone (not only that slotKillDevice is
    // finished). Neither use m_deviceServer to request slotKillDevice - see Device::slotKillDevice
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(devId, "slotKillDevice"));
    CPPUNIT_ASSERT_EQUAL(std::string("preDestruction called"), m_deviceClient->get<std::string>(devId, "status"));

    // Now wait until device is gone - will take until initialize method has finished!
    std::vector<std::string> devs;
    waitOk = waitForCondition(
          [this, devId, &devs]() {
              devs = m_deviceClient->getDevices(m_deviceServer->getInstanceId());
              return (std::find(devs.begin(), devs.end(), devId) == devs.end());
          },
          (delayInSec + 2) * 1000); // Longer than the delay in initialize()
    const karabo::util::TimeDuration duration(initStartedTime.elapsed());
    // Verify that device gone
    using karabo::util::toString;
    CPPUNIT_ASSERT_MESSAGE(toString(devs), waitOk);

    // The initialization (that blocked device going down) should have lasted about delayInSec seconds.
    // We allow for some contingency:
    const karabo::util::TimeDuration testDuration(delayInSec * 3. / 4., 0ull); // implicit conversions happening....
    std::stringstream sstr;
    sstr << duration << " " << testDuration;
    CPPUNIT_ASSERT_MESSAGE(sstr.str(), duration > testDuration);
}

bool Device_Test::waitForCondition(std::function<bool()> checker, unsigned int timeoutMillis) {
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
    CPPUNIT_ASSERT(keys.size() == 1);
    CPPUNIT_ASSERT_NO_THROW(child = h.get<Hash>(keys[0]));

    // process child node all the way till we hit an empty child.
    assertChildNodesEmpty(child);
}
