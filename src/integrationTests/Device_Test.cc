/*
 * File:   Device_Test.cc
 * Author: gero.flucke@xfel.eu
 *
 */

#include "Device_Test.hh"

#include <karabo/core/Device.hh>
#include <karabo/net/EventLoop.hh>
#include <karabo/xms/SignalSlotable.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/Epochstamp.hh>
#include <karabo/util/Schema.hh>
#include <karabo/util/Timestamp.hh>

#define KRB_TEST_MAX_TIMEOUT 5

using karabo::net::EventLoop;
using karabo::util::Hash;
using karabo::util::Epochstamp;
using karabo::util::Schema;
using karabo::util::Timestamp;
using karabo::util::DOUBLE_ELEMENT;
using karabo::util::INT32_ELEMENT;
using karabo::util::STRING_ELEMENT;
using karabo::util::TABLE_ELEMENT;
using karabo::core::DeviceServer;
using karabo::core::DeviceClient;
using karabo::xms::SignalSlotable;

class TestDevice : public karabo::core::Device<> {

public:

    KARABO_CLASSINFO(TestDevice, "TestDevice", "2.1")

    static const int ALARM_HIGH = 1000.0;

    static void expectedParameters(karabo::util::Schema& expected) {
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
                .setNodeSchema(rowSchema)
                .assignmentOptional().defaultValue({Hash("type", "INT", "name", "firstLine"),
                                                   Hash("type", "BOOL", "name", "secondLine")})
                .reconfigurable()
                .commit();

        DOUBLE_ELEMENT(expected).key("valueWithAlarm")
                .readOnly()
                .alarmHigh(TestDevice::ALARM_HIGH).needsAcknowledging(false)
                .observerAccess()
                .commit();
    }


    TestDevice(const karabo::util::Hash& input) : karabo::core::Device<>(input) {
        // Bind to a slot what now is called from deviceServer:
        KARABO_SLOT(slotTimeTick, unsigned long long /*id*/, unsigned long long /*sec*/, unsigned long long /*frac*/, unsigned long long /*period*/)

        KARABO_SLOT(slotIdOfEpochstamp, unsigned long long /*sec*/, unsigned long long /*frac*/)

        KARABO_SLOT(slotAppendSchema, const karabo::util::Schema);

        KARABO_SLOT(slotUpdateSchema, const karabo::util::Schema);
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

};

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, TestDevice)


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
}


void Device_Test::testGetTimestamp() {
    // This tests the extrapolations done in Device::getTimestamp(Epochstamp& epoch)
    // and Device::slotGetTime().

    // Setup a communication helper
    auto sigSlotA = boost::make_shared<SignalSlotable>("sigSlotA");
    sigSlotA->start();

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
    auto sigSlotA = boost::make_shared<SignalSlotable>("sigSlotA");
    sigSlotA->start();

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
}


void Device_Test::testSchemaWithAttrUpdate() {

    // Setup a communication helper
    auto sigSlotA = boost::make_shared<SignalSlotable>("sigSlotA");
    sigSlotA->start();

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
}


void Device_Test::testSchemaWithAttrAppend() {
    // Setup a communication helper
    auto sigSlotA = boost::make_shared<SignalSlotable>("sigSlotA");
    sigSlotA->start();

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
