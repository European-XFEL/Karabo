/*
 * File:   Device_Test.cc
 * Author: gero.flucke@xfel.eu
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 *
 */

#include "Device_Test.hh"

#include <karabo/core/Device.hh>
#include <karabo/net/EventLoop.hh>
#include <karabo/xms/SignalSlotable.hh>
#include <karabo/xms/OutputChannel.hh>
#include <karabo/util/Epochstamp.hh>
#include <karabo/util/OverwriteElement.hh>
#include <karabo/util/Schema.hh>
#include <karabo/util/State.hh>
#include <karabo/util/StringTools.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/VectorElement.hh>
#include <karabo/util/Timestamp.hh>
#include <karabo/util/TimeDuration.hh>

#define KRB_TEST_MAX_TIMEOUT 5

using karabo::net::EventLoop;
using karabo::util::Hash;
using karabo::util::Epochstamp;
using karabo::util::Schema;
using karabo::util::State;
using karabo::util::Timestamp;
using karabo::util::DOUBLE_ELEMENT;
using karabo::util::INT32_ELEMENT;
using karabo::util::NODE_ELEMENT;
using karabo::util::OVERWRITE_ELEMENT;
using karabo::util::STRING_ELEMENT;
using karabo::util::TABLE_ELEMENT;
using karabo::util::UINT32_ELEMENT;
using karabo::util::VECTOR_FLOAT_ELEMENT;
using karabo::core::DeviceServer;
using karabo::core::DeviceClient;
using karabo::xms::SignalSlotable;
using karabo::xms::OUTPUT_CHANNEL;
using karabo::xms::SLOT_ELEMENT;

class TestDevice : public karabo::core::Device<> {

public:

    KARABO_CLASSINFO(TestDevice, "TestDevice", "2.1")

    static const int ALARM_HIGH = 1000.0;


    static void expectedParameters(karabo::util::Schema& expected) {

        OVERWRITE_ELEMENT(expected).key("state")
                .setNewOptions(State::UNKNOWN, State::NORMAL)
                .commit();

        Schema rowSchema;
        STRING_ELEMENT(rowSchema).key("type")
                .displayedName("Type column")
                .description("Type column")
                .assignmentOptional().noDefaultValue()
                .reconfigurable()
                .commit();

        STRING_ELEMENT(rowSchema).key("name")
                .displayedName("Name column")
                .description("Name column")
                .assignmentOptional().noDefaultValue()
                .reconfigurable()
                .commit();

        TABLE_ELEMENT(expected).key("table")
                .displayedName("Table property")
                .description("Table with two columns")
                .setColumns(rowSchema)
                .assignmentOptional().defaultValue({Hash("type", "INT", "name", "firstLine"),
                                                   Hash("type", "BOOL", "name", "secondLine")})
                .reconfigurable()
                .commit();

        DOUBLE_ELEMENT(expected).key("valueWithAlarm")
                .readOnly()
                .alarmHigh(TestDevice::ALARM_HIGH).needsAcknowledging(false)
                .observerAccess()
                .commit();

        UINT32_ELEMENT(expected).key("countStateToggles")
                .description("How often slotToggleState was called")
                .readOnly().initialValue(0u)
                .commit();

        NODE_ELEMENT(expected).key("node")
                .displayedName("Node")
                .commit();

        SLOT_ELEMENT(expected).key("node.slot")
                .displayedName("Slot")
                .description("Device slot under a node, doing nothing")
                .commit();

        // Schema for output channel
        Schema dataSchema;
        NODE_ELEMENT(dataSchema).key("data")
                .displayedName("Data")
                .setDaqDataType(karabo::util::DaqDataType::TRAIN)
                .commit();

        DOUBLE_ELEMENT(dataSchema).key("data.untagged")
                .alias("UNTAGGED")
                .displayedName("Untagged")
                .readOnly()
                .commit();

        VECTOR_FLOAT_ELEMENT(dataSchema).key("data.intensityTD")
                .tags("doocs,pulseResolved")
                .alias("INTENSITY.TD")
                .displayedName("Intensity TD")
                .readOnly()
                .commit();

        OUTPUT_CHANNEL(expected).key("output")
                .dataSchema(dataSchema)
                .commit();

    }


    TestDevice(const karabo::util::Hash& input) : karabo::core::Device<>(input) {
        // Bind to a slot what now is called from deviceServer:
        KARABO_SLOT(slotTimeTick, unsigned long long /*id*/, unsigned long long /*sec*/, unsigned long long /*frac*/, unsigned long long /*period*/)

        KARABO_SLOT(slotIdOfEpochstamp, unsigned long long /*sec*/, unsigned long long /*frac*/)

        KARABO_SLOT(slotAppendSchema, const karabo::util::Schema);

        KARABO_SLOT(slotUpdateSchema, const karabo::util::Schema);

        KARABO_SLOT(slotToggleState, const Hash);

        KARABO_SLOT(node_slot);

        KARABO_SLOT(slotGetCurrentConfiguration, const std::string /*tags*/);
    }


    virtual ~TestDevice() {
    }


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


    void slotToggleState(const Hash otherIn) {

        const Epochstamp& stampCountToggles = Epochstamp::fromHashAttributes(otherIn.getAttributes("stampCountToggles"));
        const Epochstamp& stampState = Epochstamp::fromHashAttributes(otherIn.getAttributes("stampState"));

        const State& newState = (getState() == State::UNKNOWN ? State::NORMAL : State::UNKNOWN);

        Hash otherOut("valueWithAlarm", -1.);
        Hash::Attributes& attrs = otherOut.set("countStateToggles", get<unsigned int>("countStateToggles") + 1).getAttributes();
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
};

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, TestDevice)


// =================================================================================

class TestDeviceBadInit : public karabo::core::Device<> {

public:


    KARABO_CLASSINFO(TestDeviceBadInit, "TestDeviceBadInit", "2.9")


    static void expectedParameters(karabo::util::Schema& expected) {

        OVERWRITE_ELEMENT(expected).key("state")
                .setNewOptions(State::UNKNOWN, State::INIT, State::NORMAL)
                .setNewDefaultValue(State::UNKNOWN)
                .commit();

        STRING_ELEMENT(expected).key("initProblem")
                .assignmentMandatory()
                .options(std::vector<std::string>({"throw", "delay"}))
        .commit();

        UINT32_ELEMENT(expected).key("delay")
                .assignmentOptional()
                .defaultValue(10u)
                .commit();
    }


    TestDeviceBadInit(const karabo::util::Hash& input) : karabo::core::Device<>(input) {

        KARABO_INITIAL_FUNCTION(initialize);
    }


    void initialize() {

        updateState(State::INIT);

        const std::string behaviour(get<std::string>("initProblem"));
        if (behaviour == "throw") {
            // This will be caught by the event loop - if logging is enabled, one can see a printout...
            throw KARABO_SIGNALSLOT_EXCEPTION("Throw during initialization - for test purposes!");
        } else if (behaviour == "delay") {
            boost::this_thread::sleep(boost::posix_time::seconds(get<unsigned int>("delay")));
        } // No else - there are not other options!

        updateState(State::NORMAL);
    }


    virtual ~TestDeviceBadInit() {
    }


    void preDestruction() override {
        set("status", "preDestruction called");
    }

};

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, TestDeviceBadInit)


CPPUNIT_TEST_SUITE_REGISTRATION(Device_Test);


Device_Test::Device_Test() {
}


Device_Test::~Device_Test() {
}


void Device_Test::setUp() {
    // uncomment this if ever testing against a local broker
    //setenv("KARABO_BROKER", "tcp://localhost:7777", true);

    // Start central event-loop
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));
    // Create and start server
    {
        Hash config("serverId", "testServerDevice", "scanPlugins", false, "Logger.priority", "FATAL");
        m_deviceServer = DeviceServer::create("DeviceServer", config);
        m_deviceServer->finalizeInternalInitialization();
    }

    // Create client
    m_deviceClient = boost::make_shared<DeviceClient>();
}


void Device_Test::tearDown() {
    m_deviceServer.reset();
    m_deviceClient.reset();

    EventLoop::stop();
    m_eventLoopThread.join();
}


void Device_Test::appTestRunner() {

    std::pair<bool, std::string> success = m_deviceClient->instantiate("testServerDevice", "TestDevice",
                                                                       Hash("deviceId", "TestDevice"),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Now all possible individual tests.
    testGetTimestamp();
    testSchemaInjection();
    testSchemaWithAttrUpdate();
    testSchemaWithAttrAppend();
    testChangeSchemaOutputChannel("slotUpdateSchema");
    testChangeSchemaOutputChannel("slotAppendSchema");
    testNodedSlot();
    testGetSet();
    testUpdateState();
    // Last tests needs its own device, so clean-up before
    m_deviceClient->killDeviceNoWait("TestDevice");
    testBadInit();
}


void Device_Test::testGetTimestamp() {
    // This tests the extrapolations done in Device::getTimestamp(Epochstamp& epoch)
    // and Device::slotGetTime().

    // Setup a communication helper
    auto sigSlotA = m_deviceServer;

    const int timeOutInMs = 250;
    const unsigned long long periodInMicroSec = 100000ull; // some tests below assume this to be 0.1 s
    const unsigned long long periodInAttoSec = periodInMicroSec * 1000000000000ull;
    // Before first received time tick, always return train id 0
    unsigned long long id = 1111ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              1ull, 2ull) // values here should not matter at all
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(0ull, id);

    // Also slotGetTime has zero train id
    Epochstamp now;
    Hash timeHash;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotGetTime").timeout(timeOutInMs).receive(timeHash));
    CPPUNIT_ASSERT(timeHash.has("time"));
    CPPUNIT_ASSERT(timeHash.get<bool>("time"));
    const Timestamp stamp(Timestamp::fromHashAttributes(timeHash.getAttributes("time")));
    CPPUNIT_ASSERT_EQUAL(0ull, stamp.getTrainId());
    CPPUNIT_ASSERT(stamp.getEpochstamp() > now);

    // Now send a time tick...
    const unsigned long long seconds = 1559600000ull; // About June 3rd, 2019, 10 pm GMT
    const unsigned long long startId = 100ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotTimeTick",
                                              // id,     sec,    frac (attosec), period (microsec)
                                              startId, seconds, 2ull * periodInAttoSec + 1100ull, periodInMicroSec)
                            .timeout(timeOutInMs).receive());

    timeHash.clear();
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotGetTime").timeout(timeOutInMs).receive(timeHash));
    const Timestamp stamp2(Timestamp::fromHashAttributes(timeHash.getAttributes("time")));
    CPPUNIT_ASSERT_GREATEREQUAL(startId, stamp2.getTrainId());

    // ...and test real calculations of id
    // 1) exact match
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds, 2ull * periodInAttoSec + 1100ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId, id);

    // 2) end of id
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds, 3ull * periodInAttoSec + 1099ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId, id);

    // 3) multiple of period above - but same second
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds, 5ull * periodInAttoSec + 1100ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId + 3ull, id);

    // 4) multiple of period plus a bit above - next second
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds + 1ull, 5ull * periodInAttoSec + 1105ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId + 13ull, id);

    // 5) just before
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds, 2ull * periodInAttoSec + 1090ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId - 1ull, id);

    // 6) several before - but same second
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds, 1ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId - 3ull, id);

    // 7) several before - previous second
    id = 0ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds - 1ull, 5ull * periodInAttoSec + 1110ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(startId - 7ull, id);

    // 8) so much in the past that a negative id would be calculated which leads to zero
    id = 1111ull;
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotIdOfEpochstamp",
                                              seconds - 100ull, 1110ull)
                            .timeout(timeOutInMs).receive(id));
    CPPUNIT_ASSERT_EQUAL(0ull, id);
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
    INT32_ELEMENT(schema).key("injectedInt32")
            .assignmentOptional().defaultValue(1)
            .reconfigurable()
            .commit();

    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotAppendSchema", schema)
                           .timeout(requestTimeoutMs)
                           .receive());

    // Waits for the updated schema to be available from the DeviceClient.
    CPPUNIT_ASSERT(waitForCondition([this]{
        return m_deviceClient->getActiveSchema("TestDevice").has("injectedInt32");
    }, cacheUpdateWaitMs));

    int injectedInt32;
    m_deviceClient->get("TestDevice", "injectedInt32", injectedInt32);
    CPPUNIT_ASSERT(injectedInt32 == 1);
    m_deviceClient->set("TestDevice", "injectedInt32", 5);
    m_deviceClient->get("TestDevice", "injectedInt32", injectedInt32);
    CPPUNIT_ASSERT(injectedInt32 == 5);

    // Checks that injecting a new attribute keeps the previously set value.
    // ----------
    INT32_ELEMENT(schema).key("injectedInt32")
            .assignmentOptional().defaultValue(2)
            .reconfigurable().minInc(1)
            .commit();

    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotAppendSchema", schema)
                            .timeout(requestTimeoutMs)
                            .receive());

    // Waits for the updated schema to be available from the DeviceClient
    CPPUNIT_ASSERT(waitForCondition([this] {
        return m_deviceClient->getActiveSchema("TestDevice").getDefaultValue<int>("injectedInt32") == 2;
    }, cacheUpdateWaitMs));

    m_deviceClient->get("TestDevice", "injectedInt32", injectedInt32);
    CPPUNIT_ASSERT(injectedInt32 == 5);
    Schema devFullSchema;
    devFullSchema = m_deviceClient->getDeviceSchema("TestDevice");
    CPPUNIT_ASSERT(devFullSchema.getMinInc<int>("injectedInt32") == 1);

    // Checks that doing updateSchema keeps previously set value and preserves the property
    // of type TABLE_ELEMENT in the device's static schema.
    // ----------
    INT32_ELEMENT(schema).key("injectedInt32")
            .assignmentOptional().defaultValue(3)
            .reconfigurable().minInc(2).maxInc(10)
            .commit();

    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchema", schema)
                            .timeout(requestTimeoutMs)
                            .receive());

    // Waits for the updated schema to be available from the DeviceClient
    CPPUNIT_ASSERT(waitForCondition([this]() {
        return m_deviceClient->getActiveSchema("TestDevice").getDefaultValue<int>("injectedInt32") == 3;
    }, cacheUpdateWaitMs));

    m_deviceClient->get("TestDevice", "injectedInt32", injectedInt32);
    CPPUNIT_ASSERT(injectedInt32 == 5);
    devFullSchema = m_deviceClient->getDeviceSchema("TestDevice");
    CPPUNIT_ASSERT(devFullSchema.getMinInc<int>("injectedInt32") == 2);
    CPPUNIT_ASSERT(devFullSchema.getMaxInc<int>("injectedInt32") == 10);

    CPPUNIT_ASSERT(m_deviceClient->getActiveSchema("TestDevice").has("table"));
    const std::vector<Hash> &tableAfterUpdate = m_deviceClient->get<std::vector < Hash >> ("TestDevice", "table");
    CPPUNIT_ASSERT(tableAfterUpdate.size() == 2);
    const Hash &firstRowAfterUpdate = tableAfterUpdate[0];
    CPPUNIT_ASSERT(firstRowAfterUpdate.get<std::string>("name") == "firstLine");

    // Checks that doing updateSchema with something else loses injectedInt32.
    // ----------
    Schema sndSchema;
    INT32_ELEMENT(sndSchema).key("somethingElse")
            .assignmentOptional().defaultValue(4)
            .reconfigurable()
            .commit();

    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchema", sndSchema)
                            .timeout(requestTimeoutMs)
                            .receive());

    // Waits for the updated schema to be available from the DeviceClient
    CPPUNIT_ASSERT(waitForCondition([this]() {
        return m_deviceClient->getActiveSchema("TestDevice").has("somethingElse");
    }, cacheUpdateWaitMs));

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
    INT32_ELEMENT(trdSchema).key("somethingElse")
            .assignmentOptional().defaultValue(5)
            .reconfigurable()
            .commit();

    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchema", trdSchema)
                            .timeout(requestTimeoutMs)
                            .receive());

    // Waits for the updated schema to be available from the DeviceClient
    CPPUNIT_ASSERT(waitForCondition([this] {
        return m_deviceClient->getActiveSchema("TestDevice").getDefaultValue<int>("somethingElse") == 5;
    }, cacheUpdateWaitMs));

    Schema forthSchema;
    INT32_ELEMENT(forthSchema).key("somethingElse")
            .assignmentOptional().defaultValue(6)
            .reconfigurable()
            .commit();

    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchema", forthSchema)
                            .timeout(requestTimeoutMs)
                            .receive());

    // Waits for the updated schema to be available from the DeviceClient
    CPPUNIT_ASSERT(waitForCondition([this] {
        return m_deviceClient->getActiveSchema("TestDevice").getDefaultValue<int>("somethingElse") == 6;
    }, cacheUpdateWaitMs));

    Schema fifthSchema;

    INT32_ELEMENT(fifthSchema).key("somethingElse")
            .assignmentOptional().defaultValue(7)
            .minInc(3)
            .reconfigurable()
            .commit();

    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchema", fifthSchema)
                            .timeout(requestTimeoutMs)
                            .receive());

    // Waits for the updated schema to be available from the DeviceClient
    CPPUNIT_ASSERT(waitForCondition([this] {
        return m_deviceClient->getActiveSchema("TestDevice").getDefaultValue<int>("somethingElse") == 7;
    }, cacheUpdateWaitMs));

    CPPUNIT_ASSERT(m_deviceClient->get<int>("TestDevice", "somethingElse") == 42);

    // Checks that doing updateSchema with an empty schema resets the device to its
    // base schema.
    // ----------
    Schema emptySchema;

    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchema", emptySchema)
                            .timeout(requestTimeoutMs)
                            .receive());

    // Waits for the updated schema to be available from the DeviceClient.
    CPPUNIT_ASSERT(waitForCondition([this]() {
        return !m_deviceClient->getActiveSchema("TestDevice").has("somethingElse");
    }, cacheUpdateWaitMs));

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

        INT32_ELEMENT(schemaIdx).key(propertyStr + std::to_string(i))
                .assignmentOptional().defaultValue(i)
                .reconfigurable()
                .commit();

        CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotAppendSchema", schemaIdx)
                                .timeout(requestTimeoutMs)
                                .receive());
    }

    // Waits for the updated schema to be available from the DeviceClient.
    CPPUNIT_ASSERT(waitForCondition([this, &propertyStr]() {
        return m_deviceClient->getDeviceSchema("TestDevice").has(propertyStr + "9");
    }, cacheUpdateWaitMs));

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
    const std::vector<Hash> &tableAfterInsert = m_deviceClient->get<std::vector < Hash >> ("TestDevice", "table");
    CPPUNIT_ASSERT(tableAfterInsert.size() == 2);
    const Hash &firstRowAfterInsert = tableAfterInsert[0];
    CPPUNIT_ASSERT(firstRowAfterInsert.get<std::string>("name") == "firstLine");

    // Reset to static Schema for next test
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchema", Schema())
                            .timeout(requestTimeoutMs)
                            .receive());
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
    DOUBLE_ELEMENT(schema).key("valueWithAlarm")
            .readOnly()
            .alarmHigh(alarmHighValue).needsAcknowledging(false)
            .commit();
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchema", schema)
                           .timeout(requestTimeoutMs)
                            .receive());

    // Checks that the updated attribute will be available within an interval.
    CPPUNIT_ASSERT(waitForCondition([this, alarmHighValue] {
        return m_deviceClient->getDeviceSchema("TestDevice").getAlarmHigh<double>("valueWithAlarm") == alarmHighValue;
    }, cacheUpdateWaitMs));

    // Tests that doing updateSchema with something new resets the AlarmHigh.
    Schema someNewSchema;
    INT32_ELEMENT(someNewSchema).key("somethingNew")
            .assignmentOptional().defaultValue(4)
            .reconfigurable()
            .commit();
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchema", someNewSchema)
                           .timeout(requestTimeoutMs)
                           .receive());
    // Checks that the reset attribute will be available within an interval.
    CPPUNIT_ASSERT(waitForCondition([this] {
        return m_deviceClient->getDeviceSchema("TestDevice").getAlarmHigh<double>("valueWithAlarm") == TestDevice::ALARM_HIGH;
    }, cacheUpdateWaitMs));

    // Updates 'alarmHigh' by using 'slotUpdateSchemaAttributes' - this
    // is what the GUI Server would do when instantiating a device.
    alarmHighValue *= 2.0; // 4 * TestDevice::ALARM_HIGH
    std::vector<Hash> newAttrs { Hash("path", "valueWithAlarm",
                                      "attribute", "alarmHigh",
                                      "value", alarmHighValue)};
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchemaAttributes", newAttrs)
                           .timeout(requestTimeoutMs)
                           .receive());
    // Checks that the new attribute value will be available within an interval.
    CPPUNIT_ASSERT(waitForCondition([this, alarmHighValue] {
        return m_deviceClient->getDeviceSchema("TestDevice").getAlarmHigh<double>("valueWithAlarm") == alarmHighValue;
    }, cacheUpdateWaitMs));

    // Tests that doing updateSchema with something new resets the AlarmHigh.
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchema", someNewSchema)
                           .timeout(requestTimeoutMs)
                           .receive());
    // Checks that the reset attribute will be available within an interval.
    CPPUNIT_ASSERT(waitForCondition([this] {
        return m_deviceClient->getDeviceSchema("TestDevice").getAlarmHigh<double>("valueWithAlarm") == TestDevice::ALARM_HIGH;
    }, cacheUpdateWaitMs));


    // Reset to static Schema for next test
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchema", Schema())
                            .timeout(requestTimeoutMs)
                            .receive());
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
    DOUBLE_ELEMENT(schema).key("valueWithAlarm")
            .readOnly()
            .alarmHigh(alarmHighValue).needsAcknowledging(false)
            .commit();
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchema", schema)
                            .timeout(requestTimeoutMs)
                            .receive());

    // Checks that the updated attribute will be available within an interval.
    CPPUNIT_ASSERT(waitForCondition([this, alarmHighValue] {
        return m_deviceClient->getDeviceSchema("TestDevice").getAlarmHigh<double>("valueWithAlarm") == alarmHighValue;
    }, cacheUpdateWaitMs));

    // Tests that doing appendSchema with something new keeps the AlarmHigh.
    Schema someNewSchema;
    INT32_ELEMENT(someNewSchema).key("somethingNew")
            .assignmentOptional().defaultValue(4)
            .reconfigurable()
            .commit();
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotAppendSchema", someNewSchema)
                            .timeout(requestTimeoutMs)
                            .receive());
    // Checks that the reset attribute will be available within an interval.
    CPPUNIT_ASSERT(waitForCondition([this, alarmHighValue] {
        return m_deviceClient->getDeviceSchema("TestDevice").getAlarmHigh<double>("valueWithAlarm") == alarmHighValue;
    }, cacheUpdateWaitMs));

    // Updates 'alarmHigh' by using 'slotUpdateSchemaAttributes' - this
    // is what the GUI Server would do when instantiating a device.
    alarmHighValue *= 2.0; // 4 * TestDevice::ALARM_HIGH
    std::vector<Hash> newAttrs{Hash("path", "valueWithAlarm",
                                    "attribute", "alarmHigh",
                                    "value", alarmHighValue)};
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchemaAttributes", newAttrs)
                            .timeout(requestTimeoutMs)
                            .receive());
    // Checks that the new attribute value will be available within an interval.
    CPPUNIT_ASSERT(waitForCondition([this, alarmHighValue] {
        return m_deviceClient->getDeviceSchema("TestDevice").getAlarmHigh<double>("valueWithAlarm") == alarmHighValue;
    }, cacheUpdateWaitMs));

    // Reset to static Schema for next test
    CPPUNIT_ASSERT_NO_THROW(sigSlotA->request("TestDevice", "slotUpdateSchema", Schema())
                            .timeout(requestTimeoutMs)
                            .receive());
}


void Device_Test::testChangeSchemaOutputChannel(const std::string& updateSlot) {
    std::clog << "Start testChangeSchemaOutputChannel for " << updateSlot << ": " << std::flush;
    // Timeout, in milliseconds, for a request for one of the test device slots.
    const int requestTimeoutMs = 2000;

    Hash filteredCfg;
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request("TestDevice", "slotGetCurrentConfiguration", "doocs")
                            .timeout(requestTimeoutMs)
                            .receive(filteredCfg));
    // Check that contains the "doocs" tagged property from output channel schema, but nothing else
    std::set<std::string> filteredPaths;
    filteredCfg.getPaths(filteredPaths);
    CPPUNIT_ASSERT_EQUAL(1ul, filteredPaths.size());
    CPPUNIT_ASSERT_EQUAL(std::string("output.schema.data.intensityTD"), *(filteredPaths.begin()));

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
        DOUBLE_ELEMENT(schema).key("taggedProperty")
                .tags("doocs,pulseResolved")
                .displayedName("Tagged property")
                .readOnly()
                .commit();

        DOUBLE_ELEMENT(schema).key("differentlyTaggedProperty")
                .tags("nodoocs")
                .displayedName("Diff. tagged property")
                .readOnly()
                .commit();

        Schema dataSchema;
        NODE_ELEMENT(dataSchema).key("data")
                .displayedName("Data")
                .setDaqDataType(karabo::util::DaqDataType::TRAIN)
                .commit();

        VECTOR_FLOAT_ELEMENT(dataSchema).key("data.intensityTD2")
                .tags("doocs,pulseResolved")
                .alias("INTENSITY.TD2")
                .displayedName("Intensity TD 2")
                .readOnly()
                .commit();

        // Here overwrite only alias
        DOUBLE_ELEMENT(dataSchema).key("data.untagged")
                .alias("UNTAGGED.CHANGED")
                .displayedName("Untagged")
                .readOnly()
                .commit();

        DOUBLE_ELEMENT(dataSchema).key("data.untagged2")
                .displayedName("Untagged2")
                .readOnly()
                .commit();

        OUTPUT_CHANNEL(schema).key("output")
                .dataSchema(dataSchema)
                .commit();
    }

    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request("TestDevice", updateSlot, schema)
                            .timeout(requestTimeoutMs)
                            .receive());

    filteredCfg.clear();
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request("TestDevice", "slotGetCurrentConfiguration", "doocs")
                            .timeout(requestTimeoutMs)
                            .receive(filteredCfg));
    // Check that contains also the newly introduced "doocs" tagged properties, but nothing else
    filteredPaths.clear();
    filteredCfg.getPaths(filteredPaths);
    CPPUNIT_ASSERT_EQUAL(3ul, filteredPaths.size());
    CPPUNIT_ASSERT_MESSAGE(toString(filteredCfg),
                           std::find(filteredPaths.begin(), filteredPaths.end(), "taggedProperty") != filteredPaths.end());
    CPPUNIT_ASSERT_MESSAGE(toString(filteredCfg),
                           std::find(filteredPaths.begin(), filteredPaths.end(), "output.schema.data.intensityTD")
                           != filteredPaths.end());
    CPPUNIT_ASSERT_MESSAGE(toString(filteredCfg),
                           std::find(filteredPaths.begin(), filteredPaths.end(), "output.schema.data.intensityTD2")
                           != filteredPaths.end());
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
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request("TestDevice", "slotUpdateSchema", Schema())
                            .timeout(requestTimeoutMs)
                            .receive());
    std::clog << "OK." << std::endl;
}


void Device_Test::testNodedSlot() {

    // Note that calling "node_slot" would work as well... :-|
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute("TestDevice", "node.slot", KRB_TEST_MAX_TIMEOUT));
}

void Device_Test::testGetSet() {
    const int timeoutInMs = 10000;
    const std::string deviceId("TestDevice");

    // Check default visibility value
    Hash hash;
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash));
    CPPUNIT_ASSERT_EQUAL(deviceId, hash.get<std::string>("deviceId"));
    CPPUNIT_ASSERT_EQUAL(static_cast<int> (karabo::util::Schema::OBSERVER), hash.get<int>("visibility"));

    // We can set visibility and check that we really did.
    hash.clear();
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotReconfigure", Hash("visibility", static_cast<int> (karabo::util::Schema::ADMIN)))
                            .timeout(timeoutInMs).receive());
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotGetConfiguration").timeout(timeoutInMs).receive(hash));
    CPPUNIT_ASSERT_EQUAL(static_cast<int> (karabo::util::Schema::ADMIN), hash.get<int>("visibility"));

    // But we cannot set archive since not reconfigurable.
    CPPUNIT_ASSERT_THROW(m_deviceServer->request(deviceId, "slotReconfigure", Hash("archive", false))
                         .timeout(timeoutInMs).receive(), karabo::util::RemoteException);
}


void Device_Test::testUpdateState() {
    const std::string deviceId("TestDevice");

    // Check initial state of test device
    const State state(m_deviceClient->get<State>(deviceId, "state"));
    CPPUNIT_ASSERT_MESSAGE("State is " + state.name(), state == State::UNKNOWN);
    CPPUNIT_ASSERT_EQUAL(0u, m_deviceClient->get<unsigned int>(deviceId, "countStateToggles"));
    CPPUNIT_ASSERT(std::abs(-1. - m_deviceClient->get<double>(deviceId, "valueWithAlarm")) > 1.e-7);

    // Prepare Hash argument to slotToggleState with two different time stamps
    const Epochstamp stampToggle(1575296000ull, 1111ull);
    const Epochstamp stampState(1575297000ull, 2222ull);
    Hash msg;
    stampToggle.toHashAttributes(msg.set("stampCountToggles", 0).getAttributes());
    stampState.toHashAttributes(msg.set("stampState", 0).getAttributes());

    // Send state update request and...
    // ... test its (implicit) reply value,
    std::string reply;
    CPPUNIT_ASSERT_NO_THROW(m_deviceServer->request(deviceId, "slotToggleState", msg).timeout(5000).receive(reply));
    CPPUNIT_ASSERT_EQUAL(std::string("NORMAL"), reply);

    // ... test that the state was switched,
    const State stateNew(m_deviceClient->get<State>(deviceId, "state"));
    CPPUNIT_ASSERT_MESSAGE("State is " + stateNew.name(), stateNew == State::NORMAL);

    // ... test that other values updated as well,
    CPPUNIT_ASSERT_EQUAL(1u, m_deviceClient->get<unsigned int>(deviceId, "countStateToggles"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-1., m_deviceClient->get<double>(deviceId, "valueWithAlarm"), 1.e-7);

    // ... and finally test the desired timestamps:
    //     * state and valueWithAlarm get the same as given explicitly to updateState
    //     * countStateToggles gets the one mingled into the 'other' Hash
    const auto atto = karabo::util::ATTOSEC;
    const Hash cfg(m_deviceClient->get(deviceId));
    const Epochstamp stampStateNew(Epochstamp::fromHashAttributes(cfg.getAttributes("state")));
    CPPUNIT_ASSERT_MESSAGE(stampStateNew.toIso8601(atto) += " != " + stampState.toIso8601(atto),
                           stampStateNew == stampState);

    const Epochstamp stampValue(Epochstamp::fromHashAttributes(cfg.getAttributes("valueWithAlarm")));
    CPPUNIT_ASSERT_MESSAGE(stampValue.toIso8601(atto) += " != " + stampState.toIso8601(atto),
                           stampValue == stampState);

    const Epochstamp stampToggleNew(Epochstamp::fromHashAttributes(cfg.getAttributes("countStateToggles")));
    CPPUNIT_ASSERT_MESSAGE(stampToggleNew.toIso8601(atto) += " != " + stampToggle.toIso8601(atto),
                           stampToggleNew == stampToggle);



}


void Device_Test::testBadInit() {

    // HACK against topology caching in DeviceClient:
    // If we do not call getDevices() here, but run this as the last test within appTestRunner() (if it is the first, it's fine!!!),
    // the getDevices("<serverId>") below in the test case 3 waiting condition is fooled and returns an empty list when
    // called the first time. The log tells us
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
    auto requestor = m_deviceServer->request("", "slotStartDevice",
                                             Hash("classId", "TestDeviceBadInit",
                                                  "deviceId", devId,
                                                  "configuration", Hash("initProblem", "delay",
                                                                        "delay", delayInSec))
                                             ).timeout(2000); // starting a device takes at least one second...
    // Although initialization sleeps delayInSec, no timeout within the 2 seconds we allow for that
    bool ok = false;
    std::string dummy;
    CPPUNIT_ASSERT_NO_THROW(requestor.receive(ok, dummy));
    CPPUNIT_ASSERT(ok);

    // After instantiation, state switches to INIT, as soon as initialisation method runs.
    State devState(State::UNKNOWN);
    bool waitOk = waitForCondition([this, devId, &devState]() {
        devState = m_deviceClient->get<State>(devId, "state");
                                   return (devState == State::INIT);
    }, 2000);
    CPPUNIT_ASSERT_MESSAGE(devState.name(), waitOk);

    // At end of initialization, state changes to NORMAL - wait for it...
    waitOk = waitForCondition([this, devId, &devState]() {
        devState = m_deviceClient->get<State>(devId, "state");
                              return (devState == State::NORMAL);
    }, (delayInSec + 2) * 1000); // wait longer than delaying sleep
    CPPUNIT_ASSERT_MESSAGE(devState.name(), waitOk);

    m_deviceClient->killDeviceNoWait(devId);

    //
    // Case 2: The initialization method fails with an exception:
    //
    devId.back() = '2'; // let's take a new id to avoid delays until the previous device is down
    requestor = m_deviceServer->request("", "slotStartDevice",
                                        Hash("classId", "TestDeviceBadInit",
                                             "deviceId", devId,
                                             "configuration", Hash("initProblem", "throw"))
                                        ).timeout(2000); // starting a device takes at least one second...
    // Despite the failing initialization, the device instantiates successfully, no timeout:
    ok = false;
    CPPUNIT_ASSERT_NO_THROW(requestor.receive(ok, dummy));
    CPPUNIT_ASSERT(ok);

    // After instantiation, state switches to INIT, as soon as that method runs  (and will stay like that forever...)
    waitOk = waitForCondition([this, devId, &devState]() {
        devState = m_deviceClient->get<State>(devId, "state");
                              return (devState == State::INIT);
    }, 2000);
    CPPUNIT_ASSERT_MESSAGE(devState.name(), waitOk);

    // ..., but the "status" field will tell us about the exception:
    std::string status;
    waitOk = waitForCondition([this, devId, &status]() {
        status = m_deviceClient->get<std::string>(devId, "status");
                              return (status.find("Initialization failed: ") == 0ul);
    }, 2500);
    CPPUNIT_ASSERT_MESSAGE(status, waitOk);
    CPPUNIT_ASSERT_MESSAGE(status, status.find("Throw during initialization - for test purposes!") != std::string::npos);
    // State stays INIT
    CPPUNIT_ASSERT_MESSAGE(devState.name(), devState == State::INIT);

    m_deviceClient->killDeviceNoWait(devId);

    //
    // Case 3: A very long lasting initialization method (as case 1), with a try to shutdown while initialization:
    //
    devId.back() += 1; // another id again, see above ('+= 1' for Alessandro... )
    requestor = m_deviceServer->request("", "slotStartDevice",
                                        Hash("classId", "TestDeviceBadInit",
                                             "deviceId", devId,
                                             "configuration", Hash("initProblem", "delay",
                                                                   "delay", delayInSec))
                                        ).timeout(2000); // starting a device takes at least one second...
    // Although initialization sleeps 'delayInSec', no timeout within the 2 seconds we allow for that
    ok = false;
    CPPUNIT_ASSERT_NO_THROW(requestor.receive(ok, dummy));
    CPPUNIT_ASSERT(ok);

    // After instantiation, state switches to INIT, as soon as initialize method runs
    waitOk = waitForCondition([this, devId, &devState]() {
        devState = m_deviceClient->get<State>(devId, "state");
                              return (devState == State::INIT);
    }, 2000);
    const Epochstamp initStartedTime;
    CPPUNIT_ASSERT_MESSAGE(devState.name(), waitOk);

    // We kill the device that is still initializing: It will not die immediately (only once initialization is done),
    // but preDestruction is called.
    // Do not use client->killDevice(devId): that waits until device is really gone (not only that slotKillDevice is finished).
    // Neither use m_deviceServer to request slotKillDevice - see Device::slotKillDevice
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(devId, "slotKillDevice"));
    CPPUNIT_ASSERT_EQUAL(std::string("preDestruction called"), m_deviceClient->get<std::string>(devId, "status"));

    // Now wait until device is gone - will take until initialize method has finished!
    std::vector<std::string> devs;
    waitOk = waitForCondition([this, devId, &devs]() {
        devs = m_deviceClient->getDevices(m_deviceServer->getInstanceId());
                     return (std::find(devs.begin(), devs.end(), devId) == devs.end());
    }, (delayInSec + 2) * 1000); // Longer than the delay in initialize()
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

bool Device_Test::waitForCondition(boost::function<bool() > checker, unsigned int timeoutMillis) {
    constexpr unsigned int sleepIntervalMillis = 5;
    unsigned int numOfWaits = 0;
    const unsigned int maxNumOfWaits = static_cast<unsigned int> (std::ceil(timeoutMillis / sleepIntervalMillis));
    while (numOfWaits < maxNumOfWaits && !checker()) {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(sleepIntervalMillis));
        numOfWaits++;
    }
    return (numOfWaits < maxNumOfWaits);
}
