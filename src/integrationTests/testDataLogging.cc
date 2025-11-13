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
 * File:   tesstDataLogging.cc
 * Author: silenzi
 *
 * Created on Nov 12, 2018, 6:07:07 PM
 */
#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <karabo/net/EventLoop.hh>
#include <karabo/net/InfluxDbClientUtils.hh>
#include <karabo/util/DataLogUtils.hh>
#include <nlohmann/json.hpp>
#include <sstream>

#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/data/types/StringTools.hh"
#include "karabo/karabo.hpp"


class TestDataLogging : public ::testing::Test {
   protected:
    TestDataLogging();
    ~TestDataLogging() override;
    void SetUp() override;
    void TearDown() override;

    static int KRB_TEST_MAX_TIMEOUT;
    static int SLOT_REQUEST_TIMEOUT_MILLIS;
    static int FLUSH_REQUEST_TIMEOUT_MILLIS;
    static int FLUSH_INTERVAL_SEC;
    static int NUM_RETRY;
    static int PAUSE_BEFORE_RETRY_MILLIS;
    static int WAIT_WRITES;
    static const char* DEFAULT_TEST_LOG_PRIORITY;

    void testLoggerMapProperty();
    // void influxAllTestRunner();

    void testAllInstantiated(bool waitForLoggerReady = true);

    void testInt(bool testPastConf = true);
    void testUInt64(bool testPastConf = false);
    void testFloat(bool testPastConf = false);
    void testString(bool testPastConf = false);
    void testVectorString(bool testPastConf = false);
    void testVectorChar(bool testPastConf = false);
    void testVectorSignedChar(bool testPastConf = false);
    void testVectorUnsignedChar(bool testPastConf = false);
    void testVectorShort(bool testPastConf = false);
    void testVectorUnsignedShort(bool testPastConf = false);
    void testVectorInt(bool testPastConf = false);
    void testVectorUnsignedInt(bool testPastConf = false);
    void testVectorLongLong(bool testPastConf = false);
    void testVectorUnsignedLongLong(bool testPastConf = false);

    void testVectorBool(bool testPastConf = false);
    void testTable(bool testPastConf = false);
    void testChar(bool testPastConf = true);

    void testLastKnownConfiguration();
    void testCfgFromPastRestart(bool pastStampStaysPast);

    /**
     * Checks that the DataLoggers handle NaN floats and doubles.
     */
    void testNans();

    template <class T>
    void testHistory(const std::string& key, const std::function<T(int)>& f, const bool testConf);

    std::pair<bool, std::string> startDataLoggerManager(
          const std::string& loggerType, bool useInvalidInfluxUrl = false, bool useInvalidDbName = false,
          unsigned int maxPerDevicePropLogRate = 5 * 1024, unsigned int propLogRatePeriod = 5,
          unsigned int maxSchemaLogRate = 15 * 1024, unsigned int schemaLogRatePeriod = 5,
          unsigned int maxStringLength = karabo::util::MAX_INFLUX_VALUE_LENGTH, double safeSchemaRetentionPeriod = 2.0);

    /**
     * Checks that slotGetPropertyHistory logging works when a
     * Schema evolution changes the device schema at some timepoint
     * within the requested history interval.
     */
    void testSchemaEvolution();

    /**
     * Checks that the InfluxLogReader doesn't accept out of range
     * values for the 'maxNumData' parameter in calls to
     * 'slotGetPropertyHistory'.
     */
    void testMaxNumDataRange();

    /**
     * Checks that the InfluxLogReader is properly enforcing the
     * 'maxNumData' parameter in calls to 'slotGetPropertyHistory'.
     * Histories with up to 'maxNumData' entries should return
     * 'maxNumData' property values as they were written. Histories
     * with more than 'maxNumData' entries should return 'maxNumData'
     * property values samples.
     */
    void testMaxNumDataHistory();

    /**
     * Checks that the InfluxLogger is properly dropping values
     * too far ahead in the future.
     */
    void testDropBadData();

    /**
     * Sets PropertyTestDevice Schema
     *
     * circumvent min/max limits and vector size specification
     */
    void setPropertyTestSchema();

    /**
     * @brief Checks that getConfigurationFromPast does not retrieve properties
     * with no default value that have not been set during the instantiation
     * of the device that is closest to the requested timepoint.
     *
     * "Instantiation of the device that is closest to the requested timepoint"
     * means either the last instantiation of the device before the requested
     * timepoint, if the device was not active at the timepoint, or the
     * instantiation of the device that was active at the timepoint.
     */
    void testUnchangedNoDefaultProperties();

    /**
     * Test that manager goes to ERROR if server list and loggermap.xml are inconsistent
     *
     * Requires that a loggermap.xml is present from previous manager
     */
    void testFailingManager();


    std::string getDeviceIdPrefix();

    bool waitForCondition(std::function<bool()> checker, unsigned int timeoutMillis,
                          unsigned int sleepIntervalMillis = 5u);

    /**
     * Waits until logger has started to log device, assert otherwise.
     * Timeout is TestDataLogging::KRB_TEST_MAX_TIMEOUT)
     *
     * @param deviceId device that should be logged
     * @param textForFailure string prepended to assertion failure message if not logged
     */
    void waitUntilLogged(const std::string& deviceId, const std::string& textForFailure);


    const std::string m_server;
    const std::string m_deviceId;

    karabo::core::DeviceServer::Pointer m_deviceServer;
    std::jthread m_eventLoopThread;
    karabo::xms::SignalSlotable::Pointer m_sigSlot;
    karabo::core::DeviceClient::Pointer m_deviceClient;

    bool m_changedPath;
    std::string m_oldPath;
};


using namespace std::chrono;
using namespace std::literals::chrono_literals;
USING_KARABO_NAMESPACES;
using karabo::data::Epochstamp;
using karabo::data::INT32_ELEMENT;
using karabo::data::OVERWRITE_ELEMENT;
using karabo::data::State;
using karabo::data::Timestamp;
using karabo::data::toString;
using karabo::xms::SLOT_ELEMENT;
using std::string;
using std::vector;

int TestDataLogging::KRB_TEST_MAX_TIMEOUT = 10; // in seconds
int TestDataLogging::SLOT_REQUEST_TIMEOUT_MILLIS = 10'000;
int TestDataLogging::FLUSH_REQUEST_TIMEOUT_MILLIS = 60'000;
int TestDataLogging::PAUSE_BEFORE_RETRY_MILLIS = 300;
int TestDataLogging::NUM_RETRY = 400;
int TestDataLogging::FLUSH_INTERVAL_SEC = 1;
int TestDataLogging::WAIT_WRITES = 4'000;
const char* TestDataLogging::DEFAULT_TEST_LOG_PRIORITY = "ERROR";

static Epochstamp threeDaysBack = Epochstamp() - TimeDuration(3, 0, 0, 0, 0);

const unsigned int maxVectorSize = 2'000u; // smaller than default - verify that it can be set.


class DataLogTestDevice : public karabo::core::Device {
   public:
    KARABO_CLASSINFO(DataLogTestDevice, "DataLogTestDevice", "integrationTests-" + karabo::util::Version::getVersion())

    static void expectedParameters(karabo::data::Schema& expected) {
        OVERWRITE_ELEMENT(expected)
              .key("state")
              .setNewOptions(State::INIT, State::ON)
              .setNewDefaultValue(State::INIT)
              .commit();

        INT32_ELEMENT(expected).key("oldValue").readOnly().initialValue(-1).commit();

        INT32_ELEMENT(expected).key("value").readOnly().initialValue(0).commit();

        VECTOR_INT32_ELEMENT(expected).key("vector").readOnly().initialValue({}).commit();

        INT32_ELEMENT(expected)
              .key("int32Property")
              .displayedName("Int32 property")
              .reconfigurable()
              .assignmentOptional()
              .defaultValue(32000000)
              .commit();

        INT32_ELEMENT(expected)
              .key("Int32NoDefault")
              .displayedName("Int32 without default")
              .reconfigurable()
              .assignmentOptional()
              .noDefaultValue()
              .commit();

        STRING_ELEMENT(expected)
              .key("stringProperty")
              .displayedName("String property")
              .description("A string property")
              .readOnly()
              .commit();

        SLOT_ELEMENT(expected).key("slotIncreaseValue").commit();

        SLOT_ELEMENT(expected).key("slotUpdateSchema").commit();
    }

    DataLogTestDevice(const karabo::data::Hash& input) : karabo::core::Device(input) {
        KARABO_SLOT(slotIncreaseValue);
        KARABO_SLOT(slotUpdateSchema, const karabo::data::Schema);
        // NOTE: this is a terrible idea. Never do this in the field.
        KARABO_SLOT(slotUpdateConfigGeneric, const karabo::data::Hash);
        KARABO_INITIAL_FUNCTION(initialize);
    }

    virtual ~DataLogTestDevice() {}

   private:
    void initialize() {
        // Set oldValue with time stamp from past - now - 3 days
        set("oldValue", 99, Timestamp(threeDaysBack, 0ull));

        updateState(State::ON);
    }

    void slotIncreaseValue() {
        set("value", get<int>("value") + 1);
    }

    void slotUpdateConfigGeneric(const Hash conf) {
        // this is a terrible idea, but is helpful in this test.
        // Do NOT use this pattern in any production system!
        set(conf);
    }

    void slotUpdateSchema(const Schema sch) {
        updateSchema(sch);
    }
};

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::Device, DataLogTestDevice)


// A device with float and double properties without limits to be able to set inf and nan.
// Otherwise copy PropertyTest behaviour as needed for testNans().
class NanTestDevice : public karabo::core::Device {
   public:
    KARABO_CLASSINFO(NanTestDevice, "NanTestDevice", "integrationTests-" + karabo::util::Version::getVersion())

    static void expectedParameters(karabo::data::Schema& expected) {
        INT32_ELEMENT(expected).key("int32Property").reconfigurable().assignmentOptional().defaultValue(3).commit();

        FLOAT_ELEMENT(expected)
              .key("floatProperty")
              .reconfigurable()
              .assignmentOptional()
              .defaultValue(3.141596f)
              .commit();

        DOUBLE_ELEMENT(expected)
              .key("doubleProperty")
              .reconfigurable()
              .assignmentOptional()
              .defaultValue(3.1415967773331)
              .commit();

        DOUBLE_ELEMENT(expected).key("doublePropertyReadOnly").readOnly().initialValue(3.1415967773331).commit();
    }

    NanTestDevice(const karabo::data::Hash& input) : karabo::core::Device(input) {}

    void preReconfigure(Hash& incomingReconfiguration) {
        if (incomingReconfiguration.has("doubleProperty")) {
            set("doublePropertyReadOnly", incomingReconfiguration.get<double>("doubleProperty"));
        }
    }
};

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::Device, NanTestDevice)


void TestDataLogging::testLoggerMapProperty() {
    std::clog << "Testing table of data loggers... " << std::flush;

    const auto& devices = m_deviceClient->getDevices();

    // We make sure all the devices in the system have an entry in the loggerMap table, and
    // that they have the same data logger
    const auto mapEntries = m_deviceClient->get<std::vector<karabo::data::Hash>>("loggerManager", "loggerMap");
    ASSERT_LT(0ULL, static_cast<unsigned long long>(mapEntries.size()));

    const auto& data_logger = mapEntries[0].get<std::string>("dataLogger");

    for (const Hash& entry : mapEntries) {
        ASSERT_EQ(data_logger, entry.get<std::string>("dataLogger"));
    }

    for (const auto& device : devices) {
        auto found = std::find_if(mapEntries.begin(), mapEntries.end(),
                                  [device](const Hash& entry) { return device == entry.get<std::string>("device"); });

        ASSERT_TRUE(found != mapEntries.end()) << device << " not in loggers map";
    }

    std::clog << "OK" << std::endl;
}


TestDataLogging::TestDataLogging()
    : m_server("DataLoggingTestServer"),
      // Use platform-dependent name for the device: concurrent tests in CI operate
      // on the same InfluxDB database ...
      m_deviceId(getDeviceIdPrefix() + "PropertyTestDevice"),
      m_changedPath(false),
      m_oldPath() {}


TestDataLogging::~TestDataLogging() {}


std::string TestDataLogging::getDeviceIdPrefix() {
    // Avoid test collision on CI by specifying a unique prefix.
    return !getenv("KARABO_BROKER_TOPIC") ? "" : getenv("KARABO_BROKER_TOPIC");
}


void TestDataLogging::SetUp() {
    // There are indications for rare hanging between tests, see
    // https://git.xfel.eu/Karabo/Framework/-/jobs/101484 So debug print when this happens.
    const Epochstamp start;
    std::clog << "\nStart SetUp " << start.toIso8601Ext() << std::endl;

    // Uncomment to run with a local broker:
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);

    // Start central event-loop
    auto work = [](std::stop_token stoken) {
        try {
            EventLoop::work();
        } catch (const karabo::data::TimeoutException& e) {
            // Looks like thread joining fails sometimes...
            std::clog << "Timeout from EventLoop::work(): " << e << std::endl;
        }
    };
    m_eventLoopThread = std::jthread(work);

    // Create and start server
    Hash config("serverId", m_server, "log.level", DEFAULT_TEST_LOG_PRIORITY);
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();

    // Create client
    m_deviceClient = std::make_shared<DeviceClient>(std::string(), false);
    m_deviceClient->initialize();
    m_deviceClient->setInternalTimeout(SLOT_REQUEST_TIMEOUT_MILLIS);

    m_sigSlot = std::make_shared<SignalSlotable>("sigSlot");
    m_sigSlot->start();

    // There are indications for rare hanging between tests, see
    // https://git.xfel.eu/Karabo/Framework/-/jobs/101484 So debug print when this happens.
    const Epochstamp stop;
    std::clog << "End SetUp " << stop.toIso8601Ext() << std::endl;
}


void TestDataLogging::TearDown() {
    // So debug print for in between tests, see setUp()
    const Epochstamp start;
    std::clog << "Start TearDown " << start.toIso8601Ext() << std::endl;

    m_sigSlot.reset();
    m_deviceClient.reset();
    m_deviceServer.reset();
    std::this_thread::sleep_for(milliseconds(200));
    EventLoop::stop();
    if (m_eventLoopThread.joinable()) m_eventLoopThread.join();

    // Clean up directory - you may want to comment out these lines for debugging
    std::filesystem::remove("loggermap.xml");

    if (m_changedPath) {
        if (m_oldPath.empty()) {
            unsetenv("PATH");
        } else {
            setenv("PATH", m_oldPath.data(), 1);
        }
    }

    // So debug print for in between tests, see setUp()
    const Epochstamp stop;
    std::clog << "End TearDown " << stop.toIso8601Ext() << std::endl;
}


bool TestDataLogging::waitForCondition(std::function<bool()> checker, unsigned int timeoutMillis,
                                       unsigned int sleepIntervalMillis) {
    unsigned int numOfWaits = 0;
    const unsigned int maxNumOfWaits = static_cast<unsigned int>(std::ceil(timeoutMillis / sleepIntervalMillis));
    while (numOfWaits < maxNumOfWaits && !checker()) {
        std::this_thread::sleep_for(milliseconds(sleepIntervalMillis));
        numOfWaits++;
    }
    return (numOfWaits < maxNumOfWaits);
}


void TestDataLogging::waitUntilLogged(const std::string& deviceId, const std::string& textForFailure) {
    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const bool isLogged = waitForCondition(
          [this, &loggerId, &deviceId]() {
              auto toLogIds = m_deviceClient->get<std::vector<std::string>>(loggerId, "devicesToBeLogged");
              if (std::find(toLogIds.begin(), toLogIds.end(), deviceId) == toLogIds.end()) {
                  // Logger manager did not (yet?) tell the logger to log the device
                  return false;
              }
              auto notLoggedIds = m_deviceClient->get<std::vector<std::string>>(loggerId, "devicesNotLogged");
              const bool inNotLogged =
                    (std::find(notLoggedIds.begin(), notLoggedIds.end(), deviceId) != notLoggedIds.end());

              return !inNotLogged;
          },
          KRB_TEST_MAX_TIMEOUT * 1000);

    ASSERT_TRUE(isLogged) << textForFailure << ": '" << deviceId
                          << "' not logged, loggerCfg: " << toString(m_deviceClient->get(loggerId));
}

void TestDataLogging::setPropertyTestSchema() {
    Hash response;
    m_sigSlot->request(m_deviceId, "useLoggingTestSchema").timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(response);
    ASSERT_TRUE(response.get<bool>("success")) << "Could not update schema";
}

std::pair<bool, std::string> TestDataLogging::startDataLoggerManager(
      const std::string& loggerType, bool useInvalidInfluxUrl, bool useInvalidDbName,
      unsigned int maxPerDevicePropLogRate, unsigned int propLogRatePeriod, unsigned int maxSchemaLogRate,
      unsigned int schemaLogRatePeriod, unsigned int maxStringLength, double safeSchemaRetentionPeriod) {
    Hash manager_conf;
    manager_conf.set("deviceId", "loggerManager");
    manager_conf.set("flushInterval", FLUSH_INTERVAL_SEC);
    manager_conf.set<vector<string>>("serverList", {m_server});

    if (loggerType == "InfluxDataLogger") {
        std::string influxUrlWrite;

        if (getenv("KARABO_INFLUXDB_WRITE_URL")) {
            influxUrlWrite = getenv("KARABO_INFLUXDB_WRITE_URL");
        } else {
            influxUrlWrite = "tcp://localhost:8086";
        }

        std::string influxUrlRead;
        if (getenv("KARABO_INFLUXDB_QUERY_URL")) {
            influxUrlRead = getenv("KARABO_INFLUXDB_QUERY_URL");
        } else {
            influxUrlRead = "tcp://localhost:8086";
        }

        const char* envDbName = getenv("KARABO_INFLUXDB_DBNAME");
        std::string dbName(envDbName ? envDbName : ""); // without environment variable, use empty string
        if (useInvalidDbName) {
            dbName += "_invalid";
        }

        if (useInvalidInfluxUrl) {
            if (getenv("KARABO_TEST_INFLUXDB_HOST")) {
                std::string testHost = getenv("KARABO_TEST_INFLUXDB_HOST");
                influxUrlWrite = "tcp://" + testHost + ":8088";
            } else {
                influxUrlWrite = "tcp://localhost:8088";
            }
            influxUrlRead = influxUrlWrite;
        }

        manager_conf.set("influxDataLogger.urlWrite", influxUrlWrite);
        manager_conf.set("influxDataLogger.urlRead", influxUrlRead);
        manager_conf.set("influxDataLogger.dbname", dbName);
        manager_conf.set("influxDataLogger.maxVectorSize", maxVectorSize);
        manager_conf.set("influxDataLogger.maxPerDevicePropLogRate", maxPerDevicePropLogRate);
        manager_conf.set("influxDataLogger.propLogRatePeriod", propLogRatePeriod);
        manager_conf.set("influxDataLogger.maxSchemaLogRate", maxSchemaLogRate);
        manager_conf.set("influxDataLogger.schemaLogRatePeriod", schemaLogRatePeriod);
        manager_conf.set("influxDataLogger.maxValueStringSize", maxStringLength);
        manager_conf.set("influxDataLogger.safeSchemaRetentionPeriod", safeSchemaRetentionPeriod);

    } else {
        return std::make_pair<bool, std::string>(false, "Unknown logger type '" + loggerType + "'");
    }

    // std::clog << "\n***** DataLoggerManager configuration *****\n" << manager_conf << std::endl;
    return m_deviceClient->instantiate(m_server, "DataLoggerManager", manager_conf, KRB_TEST_MAX_TIMEOUT);
}


void TestDataLogging::testMaxNumDataRange() {
    std::clog << "Check if InfluxLogReader is validating range for 'maxNumData' for slot 'getPropertyHistory' ... ";

    const std::string dlReader = karabo::util::DATALOGREADER_PREFIX + m_server;
    waitUntilLogged(dlReader, "testMaxNumDataRange");
    const std::string outOfRangeErrMsg1("Requested maximum number of data points ('maxNumData') is");
    const std::string outOfRangeErrMsg2("which surpasses the limit of");

    const int readerMaxHistSize = m_deviceClient->get<int>(dlReader, "maxHistorySize");

    Epochstamp refEpoch;

    Hash params;
    params.set<string>("from", (refEpoch - TimeDuration(30, 0)).toIso8601());
    params.set<string>("to", refEpoch.toIso8601());
    params.set<int>("maxNumData", readerMaxHistSize + 1);

    vector<Hash> history;
    std::string replyDevice, replyProperty;

    // Values past InfluxLogReader::maxHistorySize must be rejected.
    try {
        m_sigSlot->request(dlReader, "slotGetPropertyHistory", dlReader, "url", params)
              .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
              .receive(replyDevice, replyProperty, history);
        throw KARABO_LOGIC_EXCEPTION("Wrong arguments to slotGetPropertyHistory did not let it throw");
    } catch (karabo::data::RemoteException& e) {
        const std::string& errMsg = e.userFriendlyMsg(true);
        ASSERT_TRUE(errMsg.find(outOfRangeErrMsg1) != std::string::npos) << errMsg;
        ASSERT_TRUE(errMsg.find(outOfRangeErrMsg2) != std::string::npos) << errMsg;
    } catch (const std::exception& e) {
        ASSERT_TRUE(false) << "Unexpected exception: " << e.what();
    }

    // Negative values must be rejected.
    params.set<int>("maxNumData", -1);
    try {
        m_sigSlot->request(dlReader, "slotGetPropertyHistory", dlReader, "url", params)
              .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
              .receive(replyDevice, replyProperty, history);
        throw KARABO_LOGIC_EXCEPTION("Wrong arguments to slotGetPropertyHistory did not let it throw");
    } catch (karabo::data::RemoteException& e) {
        const std::string& errMsg = e.userFriendlyMsg(true);
        ASSERT_TRUE(errMsg.find(outOfRangeErrMsg1) != std::string::npos);
        ASSERT_TRUE(errMsg.find(outOfRangeErrMsg2) != std::string::npos);
    } catch (const std::exception& e) {
        ASSERT_TRUE(false) << "Unexpected exception: " << e.what();
    }

    // 0 must be accepted - it as if InfluxLogReader::maxHistorySize has been used.
    params.set<int>("maxNumData", 0);
    ASSERT_NO_THROW(m_sigSlot->request(dlReader, "slotGetPropertyHistory", dlReader, "url", params)
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive(replyDevice, replyProperty, history));

    // InfluxLogReader::maxHistorySize must be accepted.
    params.set<int>("maxNumData", readerMaxHistSize);
    ASSERT_NO_THROW(m_sigSlot->request(dlReader, "slotGetPropertyHistory", dlReader, "url", params)
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive(replyDevice, replyProperty, history));

    std::clog << "OK" << std::endl;
}


void TestDataLogging::testMaxNumDataHistory() {
    std::clog << "Check if InfluxLogReader is properly enforcing the 'maxNumData' parameter for slot "
                 "'getPropertyHistory' ..."
              << std::endl;

    // Instantiates a DataLogTestDevice for performing the check.
    const std::string deviceId(getDeviceIdPrefix() + "MaxNumDataHistory");
    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const auto res =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(res.first) << "Failed to instantiate testing device '" << deviceId << "':" << res.second;

    waitUntilLogged(deviceId, "testMaxNumDataHistory");

    // Writing sequence - write a sequence of increasing values.
    Epochstamp beforeWrites;
    const int numOfWrites = 40;
    for (int i = 0; i < numOfWrites; i++) {
        ASSERT_NO_THROW(m_deviceClient->set<int>(deviceId, "int32Property", i));
        std::this_thread::sleep_for(10ms);
    }
    // Captures the timepoint after the writes.
    std::this_thread::sleep_for(100ms);
    Epochstamp afterWrites;

    // Makes sure all the writes are done before retrieval.
    ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());

    // Full history retrieval - must retrieve all values exactly as they were
    // written.
    Hash params;
    params.set<string>("from", beforeWrites.toIso8601());
    params.set<string>("to", afterWrites.toIso8601());
    const int maxNumDataFull = numOfWrites;
    params.set<int>("maxNumData", maxNumDataFull);

    vector<Hash> history;
    std::string replyDevice;
    std::string replyProperty;
    const std::string dlReader = karabo::util::DATALOGREADER_PREFIX + m_server;

    // History retrieval may take more than one attempt.
    auto historyChecker = [this, maxNumDataFull, &deviceId, &dlReader, &params, &replyDevice, &replyProperty,
                           &history]() {
        try {
            m_sigSlot->request(dlReader, "slotGetPropertyHistory", deviceId, "int32Property", params)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(replyDevice, replyProperty, history);
            return history.size() == static_cast<std::size_t>(maxNumDataFull);
        } catch (const karabo::data::TimeoutException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
            return false;
        } catch (const karabo::data::RemoteException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
            return false;
        }
    };

    waitForCondition(historyChecker, 90'000u, 1'000u);

    ASSERT_EQ(static_cast<size_t>(maxNumDataFull), history.size()) << "Size for full history different from expected.";
    for (int i = 0; i < maxNumDataFull; i++) {
        ASSERT_EQ(i, history[i].get<int>("v"))
              << "Value at history entry #" << karabo::data::toString(i) << " different from expected.";
    }

    // Sampled history retrieval - must retrieve all values sampled.
    // The specified 'maxNumData' is half the size of the full history size.
    const int maxNumDataSampled = numOfWrites / 2;
    params.set<int>("maxNumData", maxNumDataSampled);

    history.clear();

    // Sample history retrieval may take more than one attempt.
    auto sampleHistoryChecker = [this, maxNumDataSampled, &deviceId, &dlReader, &params, &replyDevice, &replyProperty,
                                 &history]() {
        try {
            m_sigSlot->request(dlReader, "slotGetPropertyHistory", deviceId, "int32Property", params)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(replyDevice, replyProperty, history);
            return history.size() >= (maxNumDataSampled / 2ul);
        } catch (const karabo::data::TimeoutException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
            return false;
        } catch (const karabo::data::RemoteException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
            return false;
        }
    };

    bool succeeded = waitForCondition(sampleHistoryChecker, 96'000u, 1'000u);

    size_t historySize = history.size();
    ASSERT_TRUE(succeeded) << "Timeout on requesting history";
    ASSERT_TRUE(historySize <= static_cast<size_t>(maxNumDataSampled))
          << "Size of the down-sampled history larger than request sample";
    ASSERT_TRUE(historySize >= (maxNumDataSampled / 2ul))
          << "Size of the down-sampled history smaller than minimum expected (at least half in this case)";
    // Makes sure that the maxNumDataSampled values retrieved are distributed across the
    // whole set of maxNumDataFull values. A deviation margin is tolerated to accomodate
    // different timings involved in the writing sequence phase.
    const int deviationMargin = 8;
    Hash& lastHistoryEntry = history[historySize - 1];
    ASSERT_TRUE(history[0].get<int>("v") >= 0 && history[0].get<int>("v") <= deviationMargin)
          << "Value at history entry #0 is outside the expected range: should be between 0 and " << deviationMargin
          << ", got " << history[0].get<int>("v") << ".";
    ASSERT_TRUE(lastHistoryEntry.get<int>("v") >= 40 - deviationMargin && lastHistoryEntry.get<int>("v") <= 40)
          << "Value at history entry #" << toString(lastHistoryEntry)
          << " is outside the expected range: should be between " << (40 - deviationMargin) << " and 40, got "
          << lastHistoryEntry.get<int>("v") << ".";
    std::clog << "... OK" << std::endl;
}

void TestDataLogging::testDropBadData() {
    std::clog << "Testing that the logger drops bad data ... " << std::flush;

    const std::string deviceId(getDeviceIdPrefix() + "deviceWithFutureStamp");
    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    auto success =
          m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    waitUntilLogged(deviceId, "testDropBadData");

    const std::string dlReader = karabo::util::DATALOGREADER_PREFIX + m_server;
    unsigned int numCycles = 5;
    Epochstamp before;
    const Epochstamp inAlmostAFortnite = before + TimeDuration(13, 0, 0, 0, 0);
    const Epochstamp inAFortnite = inAlmostAFortnite + TimeDuration(1, 0, 0, 0, 0);

    // Getting original timestamp of 'value' to compare with later
    // We first change the property once - otherwise, what we get back from influx is the start of logging time
    // and not the timestamp on device
    ASSERT_NO_THROW(m_deviceClient->execute(deviceId, "slotIncreaseValue", KRB_TEST_MAX_TIMEOUT));
    Hash cfg;
    ASSERT_NO_THROW(m_deviceClient->get(deviceId, cfg));
    ASSERT_TRUE(cfg.has("value")) << "'value' is missing from the configuration";
    const Epochstamp originalEpoch = Epochstamp::fromHashAttributes(cfg.getAttributes("value"));
    const int originalValue = cfg.get<int>("value");

    for (unsigned int i = 0; i < numCycles; ++i) {
        Hash update("value", 10000 + i);
        const Timestamp muchLater(inAlmostAFortnite, TimeId());
        muchLater.toHashAttributes(update.getAttributes("value"));
        // Call slotUpdateConfigGeneric from m_deviceClient so that m_deviceClient->get is in sync for sure
        ASSERT_NO_THROW(m_deviceClient->execute(deviceId, "slotUpdateConfigGeneric", KRB_TEST_MAX_TIMEOUT, update));
        // Get configuration, check expected values, check (static) time stamp of "oldValue" and store stamp of
        // "value"
        ASSERT_NO_THROW(m_deviceClient->get(deviceId, cfg));
        ASSERT_TRUE(cfg.has("value")) << "'value' is missing from the configuration";
        ASSERT_TRUE(cfg.has("vector")) << "'vector' is missing from the configuration";
        ASSERT_EQ(static_cast<int>(i) + 10000, cfg.get<int>("value"));
        ASSERT_EQ(std::vector<int>(), cfg.get<vector<int>>("vector"));
        const Epochstamp stamp = Epochstamp::fromHashAttributes(cfg.getAttributes("value"));

        ASSERT_TRUE(stamp == inAlmostAFortnite) << "'value' has wrong time stamp: " << stamp.toIso8601()
                                                << " instead of " << inAlmostAFortnite.toIso8601();

        // Flush Data
        ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());

        // Get config back - but it may take a while till data is store, so try a few times
        // (some CI failed with fixed 250 ms of sleep)
        Schema schema;
        bool configAtTimepoint;
        std::string configTimepoint;
        cfg.clear();
        int maxTime = 2000;
        do {
            std::this_thread::sleep_for(50ms);
            maxTime -= 50;
            ASSERT_NO_THROW(
                  m_sigSlot->request(dlReader, "slotGetConfigurationFromPast", deviceId, inAFortnite.toIso8601())
                        .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                        .receive(cfg, schema, configAtTimepoint, configTimepoint));
        } while (!cfg.has("value") && maxTime >= 0);
        ASSERT_TRUE(cfg.has("value")) << "'value' is missing from configuration: " << toString(cfg);
        const Epochstamp received = Epochstamp::fromHashAttributes(cfg.getAttributes("value"));
        // the data is stored in the influxDB and has lower resolution (microsecond) what Epochstamp offers
        // (attosecond). We therefore compare the time difference. TimeDuration will always be positive (sic).
        const double dt = received - originalEpoch;
        ASSERT_TRUE(dt < 1e-6) << "'value' has wrong time stamp: " << received.toIso8601()
                               << " - difference is : " << dt;
        ASSERT_EQ(cfg.get<int>("value"), originalValue);
    }

    // Now check that we can get back info about bad data - but first add other bad data items,
    // first a too long vector and then a mixture of too long vector, far future, and decent data
    const unsigned int vectorSize = maxVectorSize + 1u;
    Hash updates("vector", std::vector<int>(vectorSize, 42)); // one longer than logger tolerates
    Timestamp vectorUpdateTime1;
    vectorUpdateTime1.toHashAttributes(updates.getAttributes("vector")); // Add stamp to test full cycle
    ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotUpdateConfigGeneric", updates)
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive());
    // second (i.e. mixed) update
    Timestamp vectorUpdateTime2;
    vectorUpdateTime2.toHashAttributes(updates.getAttributes("vector")); // Overwrite with new stamp
    Hash::Node& valueNode = updates.set("value", 42);
    Timestamp(inAlmostAFortnite, TimeId()).toHashAttributes(valueNode.getAttributes());
    updates.set("oldValue", -42); // no timestamp from here, will get injected automatically in device
    ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotUpdateConfigGeneric", updates)
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive());

    ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());

    // Get back bad data
    // vectorUpdateTime2 is to early, future data gets timestamp after it, using inAFortnite might create
    // interference between different test runs, so create a new stamp:
    const Epochstamp whenFlushed;
    Hash badDataAllDevices;
    int maxTime = 3'000; // 2'000 still failed on a loaded CI
    while (maxTime >= 0) {
        std::this_thread::sleep_for(100ms);
        maxTime -= 100;
        ASSERT_NO_THROW(m_sigSlot
                              ->request(dlReader, "slotGetBadData", before.toIso8601Ext(), whenFlushed.toIso8601Ext())
                              //(inAFortnite + TimeDuration(1, 0, 0, 0, 0)).toIso8601Ext())
                              .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                              .receive(badDataAllDevices));
        if (badDataAllDevices.has(deviceId) &&
            badDataAllDevices.get<std::vector<Hash>>(deviceId).size() == numCycles + 3ul) {
            break;
        }
    }

    ASSERT_EQ(1ul, badDataAllDevices.size()); // Just our test device is a bad guy...
    ASSERT_TRUE(badDataAllDevices.has(deviceId));
    const std::vector<Hash>& badData = badDataAllDevices.get<std::vector<Hash>>(deviceId);
    // numCycles plus 3: 1st vector and then "2nd vector and future value" split into two due to different
    // timestamps
    ASSERT_EQ(numCycles + 3ul, badData.size()) << toString(badDataAllDevices);

    // Test the bad data from far future
    Epochstamp last = before;
    for (size_t i = 0; i < numCycles; ++i) {
        const Hash& futureHash = badData[i];
        ASSERT_TRUE(futureHash.has("info"));
        const std::string& info = futureHash.get<std::string>("info");
        ASSERT_TRUE(info.find("log metric(s) for device '" + deviceId + "'") != std::string::npos) << info;
        ASSERT_TRUE(info.find("'value' (from far future " + inAlmostAFortnite.toIso8601Ext() += ")") !=
                    std::string::npos)
              << info;
        ASSERT_TRUE(futureHash.has("time"));
        const std::string& timeStr = futureHash.get<std::string>("time");
        const Epochstamp timeEpoch = Epochstamp::fromHashAttributes(futureHash.getAttributes("time"));
        ASSERT_EQ(timeStr, timeEpoch.toIso8601Ext());
        // Timestamp is defined inside logger (since not 'believing' the far future one),
        // so we cannot be exactly sure for when this is logged (but in order)
        ASSERT_TRUE(last < timeEpoch);
        ASSERT_TRUE(Epochstamp() > timeEpoch);
        last = timeEpoch;
    }
    // Test the bad data from 1st too long vector
    const Hash& vectorHash = badData[badData.size() - 3];
    ASSERT_TRUE(vectorHash.has("info"));
    const std::string& info = vectorHash.get<std::string>("info");
    ASSERT_TRUE(info.find("log metric(s) for device '" + deviceId + "'") != std::string::npos) << info;
    ASSERT_TRUE(info.find("'vector' (vector of size " + toString(vectorSize) += ")") != std::string::npos) << info;
    ASSERT_TRUE(vectorHash.has("time"));
    const std::string& timeStr = vectorHash.get<std::string>("time");
    const Epochstamp timeEpoch = Epochstamp::fromHashAttributes(vectorHash.getAttributes("time"));
    ASSERT_EQ(timeStr, timeEpoch.toIso8601Ext());
    const double dt2 = timeEpoch - vectorUpdateTime1.getEpochstamp();
    ASSERT_GE(1.e-6, dt2); // we store only microsecond precision in DB

    // Test the bad data from mixture of too long vector, future data and OK data
    // First of these comes vector
    const Hash& mixtureHash1 = badData[badData.size() - 2];
    ASSERT_TRUE(mixtureHash1.has("info"));
    const std::string& info2 = mixtureHash1.get<std::string>("info");
    ASSERT_TRUE(info2.find("log metric(s) for device '" + deviceId + "'") != std::string::npos) << info2;
    ASSERT_TRUE(info2.find("'vector' (vector of size " + toString(vectorSize) += ")") != std::string::npos) << info2;
    ASSERT_TRUE(mixtureHash1.has("time"));
    const std::string& timeStr2 = mixtureHash1.get<std::string>("time");
    const Epochstamp timeEpoch2 = Epochstamp::fromHashAttributes(mixtureHash1.getAttributes("time"));
    ASSERT_EQ(timeStr2, timeEpoch2.toIso8601Ext());
    const double dt3 = timeEpoch2 - vectorUpdateTime2.getEpochstamp();
    ASSERT_GE(1.e-6, dt3); // only microsecond precision in DB

    // Finally future data from mixture
    const Hash& mixtureHash2 = badData[badData.size() - 1];
    ASSERT_TRUE(mixtureHash2.has("info"));
    const std::string& info3 = mixtureHash2.get<std::string>("info");
    ASSERT_TRUE(info3.find("log metric(s) for device '" + deviceId + "'") != std::string::npos) << info3;
    ASSERT_TRUE(info3.find("'value' (from far future " + inAlmostAFortnite.toIso8601Ext() += ")") != std::string::npos)
          << info3;
    ASSERT_TRUE(mixtureHash2.has("time"));
    const std::string& timeStr3 = mixtureHash2.get<std::string>("time");
    const Epochstamp timeEpoch3 = Epochstamp::fromHashAttributes(mixtureHash2.getAttributes("time"));
    ASSERT_EQ(timeStr3, timeEpoch3.toIso8601Ext());
    // Timestamp is defined in logger since future stamp seen as unreliable:
    // between "before setting the data mixture" and "now"
    ASSERT_TRUE(vectorUpdateTime2.getEpochstamp() < timeEpoch3);
    ASSERT_TRUE(Epochstamp() > timeEpoch3);

    // Check that the "decent" data from same "signalChanged" as long vector and future value is properly logged
    bool configAtTimepoint = false;
    cfg.clear();
    Schema dummySchema;
    std::string dummyConfigTimepoint;
    ASSERT_NO_THROW(m_sigSlot->request(dlReader, "slotGetConfigurationFromPast", deviceId, Timestamp().toIso8601())
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive(cfg, dummySchema, configAtTimepoint, dummyConfigTimepoint));
    ASSERT_TRUE(configAtTimepoint);
    ASSERT_TRUE(cfg.has("oldValue"));
    ASSERT_EQ(-42, cfg.get<int>("oldValue"));
    const Epochstamp oldValueStamp(Epochstamp::fromHashAttributes(cfg.getAttributes("oldValue")));
    // This timestamp is defined by device when setting it,
    // but again this is between "before setting the data mixture" and "now"
    ASSERT_TRUE(vectorUpdateTime2.getEpochstamp() < oldValueStamp);
    ASSERT_TRUE(Epochstamp() > oldValueStamp);

    // Now test that slotGetBadData correctly returns nothing for a decent period (here: future)
    ASSERT_NO_THROW(m_sigSlot
                          ->request(dlReader, "slotGetBadData", inAFortnite.toIso8601Ext(),
                                    (inAFortnite + TimeDuration(1, 0, 0, 0, 0)).toIso8601Ext())
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive(badDataAllDevices));
    ASSERT_TRUE(badDataAllDevices.empty()) << toString(badDataAllDevices);

    std::clog << "... OK" << std::endl;
}

void TestDataLogging::testAllInstantiated(bool waitForLoggerReady) {
    std::clog << "Testing logger and readers instantiations ... " << std::flush;

    const vector<string> devices(
          {karabo::util::DATALOGGER_PREFIX + m_server, karabo::util::DATALOGREADER_PREFIX + m_server});

    bool succeeded = waitForCondition(
          [this, &devices]() {
              auto inClientTopo = [&devices](const Hash& topo) {
                  const Hash& device = topo.get<Hash>("device");
                  bool allUp = true;
                  for (const string& deviceId : devices) {
                      allUp = allUp && device.has(deviceId);
                  }
                  return allUp;
              };
              bool allInTopo = inClientTopo(m_deviceClient->getSystemTopology());
              if (!allInTopo) {
                  // Use bad exists interface (request/reply with 200 ms timeout) to workaround unreliable client cache
                  bool missing = false;
                  for (const std::string& device : devices) {
                      if (!m_deviceServer->exists(device).first) missing = true;
                  }
                  // Check topology again to avoid blaming when just later than exists(..) check
                  if (!missing && !inClientTopo(m_deviceClient->getSystemTopology())) {
                      std::clog << "Note: All up, but client cache did not see them!" << std::endl;
                  }
                  return !missing;
              }
              return allInTopo;
          },
          60 * KRB_TEST_MAX_TIMEOUT * 1'000u // Increased tolerance: instantiation can be quite slow on a busy CI...
          ,
          100u);
    if (!succeeded) { // Debuggig output
        for (const std::string& device : devices) {
            bool isThere = m_deviceServer->exists(device).first;
            std::clog << device << ": " << (isThere ? "online" : "offline") << std::endl;
        }
    }
    ASSERT_TRUE(succeeded) << "Timeout looking for logger and readers instances. Up are only "
                           << toString(m_deviceClient->getDevices());

    if (waitForLoggerReady) {
        // Makes sure that the DataLogger has reached ON state before proceeding.
        // Any call to the Flush slot while the DataLogger is in a different state will trigger an exception.
        // For the Influx Logger case, this initialization time can be quite long - if the db does not exist
        // yet, the DataLogger must create it before reaching the ON state.

        State loggerState = State::UNKNOWN;
        const std::string& dataLoggerId = karabo::util::DATALOGGER_PREFIX + m_server;

        waitForCondition(
              [this, &loggerState, &dataLoggerId]() {
                  loggerState = m_deviceClient->get<karabo::data::State>(dataLoggerId, "state");
                  return loggerState == karabo::data::State::ON;
              },
              60 * KRB_TEST_MAX_TIMEOUT * 1000u, 100u);

        ASSERT_EQ(State::ON, loggerState)
              << "Timeout while waiting for DataLogger '" << dataLoggerId << "' to reach ON state.";
    }

    std::clog << "Ok" << std::endl;
}


void TestDataLogging::testLastKnownConfiguration() {
    // Timestamp for test start - used to print test duration at the end.
    Epochstamp testCaseStart;

    // Last value set in previous test cases for property 'int32Property'.
    const int kLastValueSet = 99;

    Epochstamp beforeAnything(0, 0);

    std::clog << "Testing last known configuration at specific timepoints ..." << std::endl;

    const std::string dlReader = karabo::util::DATALOGREADER_PREFIX + m_server;

    Schema schema;
    Hash conf;
    bool configAtTimepoint;
    std::string configTimepoint;

    std::clog << "... before any logging activity (at " << beforeAnything.toIso8601() << ") ...";
    // At the beforeAnything timepoint no known configuration existed, so an exception is expected.
    bool remoteExcept = false;
    try {
        m_sigSlot->request(dlReader, "slotGetConfigurationFromPast", m_deviceId, beforeAnything.toIso8601())
              .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
              .receive(conf, schema, configAtTimepoint, configTimepoint);
    } catch (const RemoteException& re) {
        const std::string fileLoggerMsg(
              "Requested time point for device configuration is earlier than anything logged");
        const std::string influxLoggerMsg("No active schema could be found for device at (or before) timepoint.");
        ASSERT_TRUE((re.detailedMsg().find(fileLoggerMsg) != string::npos ||
                     re.detailedMsg().find(influxLoggerMsg) != string::npos))
              << "Exception message: " << re.detailedMsg();
        remoteExcept = true;
    } catch (const std::exception& e) {
        ASSERT_TRUE(false) << "Unexpected exception: " << e.what();
    }
    ASSERT_TRUE(remoteExcept) << "Expected exception, received " << toString(conf);

    std::clog << "\n... Ok (no configuration retrieved)." << std::endl;

    karabo::data::Epochstamp rightBeforeDeviceGone;
    std::clog << "... right before killing device being logged (at " << rightBeforeDeviceGone.toIso8601() << ") ...";
    // At the rightBeforeDeviceGone timepoint, a last known configuration should be obtained with the last value set
    // in the  previous test cases for the 'int32Property' - even after the device being logged is gone.
    ASSERT_NO_THROW(
          m_sigSlot->request(dlReader, "slotGetConfigurationFromPast", m_deviceId, rightBeforeDeviceGone.toIso8601())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(conf, schema, configAtTimepoint, configTimepoint));

    ASSERT_EQ(99, conf.get<int>("int32Property"));

    ASSERT_EQ("with\nnewline99", conf.get<std::string>("stringProperty"));

    ASSERT_EQ(std::vector<std::string>({"abc99", "xy|z99", "A\nB99"}),
              conf.get<std::vector<std::string>>("vectors.stringProperty"));
    ASSERT_EQ(
          std::vector<Hash>({Hash("e1", "ab\nc99", "e2", false, "e3", 12 * 99, "e4", 0.9837F * 99, "e5", 1.2345 * 99),
                             Hash("e1", "xy|z99", "e2", true, "e3", 42 * 99, "e4", 2.33333F * 99, "e5", 7.77777 * 99)}),
          conf.get<std::vector<Hash>>("table"));
    std::clog << "\n... Ok (retrieved configuration with last known value for 'int32Property', 'stringProperty', "
              << "'vectors.stringProperty', and 'table')." << std::endl;

    ASSERT_EQ(kLastValueSet, conf.get<int>("int32Property"));
    ASSERT_EQ(true, configAtTimepoint);

    std::vector<std::string> confKeys;
    conf.getKeys(confKeys);
    Epochstamp latestTimestamp(0, 0);
    for (const std::string& path : confKeys) {
        const Hash::Node& propNode = conf.getNode(path);
        if (propNode.hasAttribute("sec") && propNode.hasAttribute("frac")) {
            auto propSec = propNode.getAttribute<unsigned long long>("sec");
            auto propFrac = propNode.getAttribute<unsigned long long>("frac");
            Epochstamp propTimestamp(propSec, propFrac);
            if (propTimestamp > latestTimestamp) {
                latestTimestamp = propTimestamp;
            }
        }
    }
    ASSERT_EQ(latestTimestamp.toIso8601Ext(), configTimepoint);
    std::clog << "\n... "
              << "Ok (retrieved configuration with last known value for 'int32Property' while the device was being "
                 "logged)."
              << std::endl;

    // killDevice waits for the device to be killed (or throws an exception in case of failure).
    ASSERT_NO_THROW(m_deviceClient->killDevice(m_deviceId, KRB_TEST_MAX_TIMEOUT));

    // Assures that the logger in charge of the device is not logging it anymore by testing that m_deviceId is not
    // among the rows of the "lastUpdatesUtc" property of the logger. The "flush" slot guarantees that the property
    // "lastUpdatesUtc" is in sync with devices being logged.
    ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                          .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                          .receive());
    const auto lastUpdates =
          m_deviceClient->get<std::vector<Hash>>(karabo::util::DATALOGGER_PREFIX + m_server, "lastUpdatesUtc");
    bool deviceIdFound = false;
    for (const Hash& entry : lastUpdates) {
        if (entry.get<std::string>("deviceId") == m_deviceId) {
            deviceIdFound = true;
            break;
        }
    }
    ASSERT_EQ(false, deviceIdFound);

    // There is an interval between the device being killed and the event that it is gone reaching the logger.
    // But we need to be sure that the timepoint used in the request for configuration from past is after the
    // timestamp associated to the device shutdown event.
    // In rare CI cases this sleep seems not to be enough, therefore the loop below that even postpones the
    // requested timepoint.
    std::this_thread::sleep_for(1000ms);

    Epochstamp afterDeviceGone;
    std::clog << "... after device being logged is gone (requested config at " << afterDeviceGone.toIso8601()
              << " or later) ...";
    int nTries = NUM_RETRY;
    unsigned int numChecks = 0;
    conf.clear();
    configAtTimepoint = true;
    while ((!conf.has("int32Property") || kLastValueSet != conf.get<int>("int32Property") || configAtTimepoint) &&
           nTries-- > 0) {
        std::this_thread::sleep_for(milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
        afterDeviceGone.now();

        // At the afterDeviceGone timepoint, a last known configuration should be obtained with the last value set
        // in the previous test cases for the 'int32Property' - even after the device being logged is gone.
        ASSERT_NO_THROW(
              m_sigSlot->request(dlReader, "slotGetConfigurationFromPast", m_deviceId, afterDeviceGone.toIso8601())
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                    .receive(conf, schema, configAtTimepoint, configTimepoint));
        ++numChecks;
    }
    const std::string msg("Failed after " + toString(numChecks) + " attempts\nconf: " + toString(conf) +=
                          "\nconfigAtTimePoint: " + toString(configAtTimepoint) +=
                          "\nconfigTimepoint: " + configTimepoint);
    ASSERT_EQ(kLastValueSet, conf.get<int>("int32Property")) << msg;
    ASSERT_EQ(false, configAtTimepoint) << msg;
    karabo::data::Epochstamp configStamp(configTimepoint);
    // if data migration happened the data is younger than the file based logging data
    ASSERT_TRUE(configStamp > beforeAnything) << msg;
    ASSERT_TRUE(configStamp < afterDeviceGone) << msg;
    std::clog << "\n... " << "Timestamp of retrieved configuration: " << configTimepoint << "\n "
              << "Ok (retrieved configuration with last known value for 'int32Property' while the device was not being "
                 "logged)."
              << std::endl;


    const karabo::data::TimeDuration testDuration = testCaseStart.elapsed();

    const std::streamsize currPrecision{std::clog.precision()};
    std::clog << "(testLastKnownConfiguration took " << std::setprecision(4) << static_cast<double>(testDuration)
              << " sec. to execute)" << std::setprecision(currPrecision) << std::endl;

    std::clog << ("OK") << std::endl;
}


void TestDataLogging::testCfgFromPastRestart(bool pastConfigStaysPast) {
    std::clog << "Testing past configuration retrieval with stamp older than device..." << std::endl;

    Epochstamp testCaseStart;

    // Start device and take care that the logger is ready for it
    // Use platform-dependent name for the device: concurrent tests in CI operate
    // on the same InfluxDB database ...
    const std::string deviceId(getDeviceIdPrefix() + "deviceWithOldStamp");
    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    auto success =
          m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    waitUntilLogged(deviceId, "testCfgFromPastRestart");

    // few cycles: increase value, stop and restart logging
    const unsigned int numCycles = 5;
    std::vector<Epochstamp> stampsAfter;        // stamps after increasing value
    std::vector<Epochstamp> valueStamps;        // stamps of the updated values
    std::vector<Epochstamp> stampsAfterRestart; // stamps after restart logging
    const Epochstamp oldStamp = threeDaysBack;
    for (unsigned int i = 0; i < numCycles; ++i) {
        // Increase "variable" value and store after increasing it
        ASSERT_NO_THROW(m_deviceClient->execute(deviceId, "slotIncreaseValue", KRB_TEST_MAX_TIMEOUT));
        std::this_thread::sleep_for(1ms); // ensure timestamp is after setting
        stampsAfter.push_back(Epochstamp());

        // Get configuration, check expected values, check (static) time stamp of "oldValue" and store stamp of
        // "value"
        Hash cfg;
        ASSERT_NO_THROW(m_deviceClient->get(deviceId, cfg));
        ASSERT_EQ(static_cast<int>(i) + 1, cfg.get<int>("value"));

        ASSERT_EQ(99, cfg.get<int>("oldValue"));
        const Epochstamp stamp = Epochstamp::fromHashAttributes(cfg.getAttributes("oldValue"));
        ASSERT_TRUE(stamp == oldStamp) << "'oldValue' has wrong time stamp: " << stamp.toIso8601();
        valueStamps.push_back(Epochstamp::fromHashAttributes(cfg.getAttributes("value")));

        // Stop logging our device and check that it is not logged anymore.
        // Sleep needed before flush to ensure that - for file logger - the output stream has actually seen the data
        std::this_thread::sleep_for(250ms); // locally 100 was always enough
        ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                              .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                              .receive());
        ASSERT_NO_THROW(m_sigSlot->request(loggerId, "slotTagDeviceToBeDiscontinued", "D", deviceId)
                              .timeout(KRB_TEST_MAX_TIMEOUT * 1000)
                              .receive());
        ASSERT_TRUE(waitForCondition(
              [this, &loggerId, &deviceId]() {
                  auto loggedIds = m_deviceClient->get<std::vector<std::string>>(loggerId, "devicesToBeLogged");
                  // NOT in there anymore
                  return (std::find(loggedIds.begin(), loggedIds.end(), deviceId) == loggedIds.end());
              },
              KRB_TEST_MAX_TIMEOUT * 1000))
              << "Test device still present in 'devicesToBeLogged' :"
              << toString(m_deviceClient->get<std::vector<std::string>>(loggerId, "devicesToBeLogged"));

        // Restart again (and validate it is logging) - file based logger will gather the complete config again on
        // disk
        ASSERT_NO_THROW(m_sigSlot->request(loggerId, "slotAddDevicesToBeLogged", vector<string>(1, deviceId))
                              .timeout(KRB_TEST_MAX_TIMEOUT * 1000)
                              .receive());

        waitUntilLogged(deviceId, "testCfgFromPastRestart, cycle " + toString(numCycles));

        stampsAfterRestart.push_back(Epochstamp());
    }

    // Again flush - at the end of the last cycle we started logging again and archive_index.txt and archive_<N>.txt
    // might be out of sync otherwise - nevertheless for file based logging we need the repeated retries below for
    // the same reason as the sleeps above. :-(
    ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                          .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                          .receive());

    std::this_thread::sleep_for(250ms);

    // Now check that for all stored stamps, the stamps gathered for the reader are correct
    const std::string dlReader = karabo::util::DATALOGREADER_PREFIX + m_server;
    int failedCycles = 0;
    for (unsigned int i = 0; i < numCycles; ++i) {
        // Time stamp after increasing value
        const Epochstamp& stampAfter = stampsAfter[i];

        // Gather full configuration (repeat until success, see above)
        Hash conf;
        Schema schema;
        int nTries = NUM_RETRY;
        int nChecks = 0;
        int nRemoteExceptions = 0;

        // Still, conf.empty() check needed here although any non-throwing slotGetConfigurationFromPast should
        // be trustworthy now! But
        // - for file logger, data might not have reached the streams when flush was called
        // - for influx logger there is a period between the DB has confirmed arrival of data and that the data is
        //   ready for reading.
        while (nTries > 0 && (conf.empty() || static_cast<int>(i + 1) != conf.get<int>("value"))) {
            try {
                nChecks++;
                m_sigSlot->request(dlReader, "slotGetConfigurationFromPast", deviceId, stampAfter.toIso8601())
                      .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                      .receive(conf, schema);
            } catch (const RemoteException& re) {
                ++nRemoteExceptions;
                // The data might not yet be available for the reader - despite the flush and the long sleep above!
                // File and influx logger will reply then with exceptions, but their text is different. Here we
                // allow for these expected exceptions (and go on with next try), but bail out for any other remote
                // exception.
                const std::string fileLoggerMsg(
                      "Requested time point for device configuration is earlier than anything logged");
                const std::string influxLoggerMsg( // see InfluxLogReader::onLastSchemaDigestBeforeTime
                      "No active schema could be found for device");
                ASSERT_TRUE((re.detailedMsg().find(fileLoggerMsg) != std::string::npos ||
                             re.detailedMsg().find(influxLoggerMsg) != std::string::npos))
                      << "Unexpected RemoteException received: " << re.what();
            } catch (const TimeoutException& te) {
                // Also timeouts are allowed...
                karabo::data::Exception::clearTrace();
            }
            std::this_thread::sleep_for(milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
            nTries--;
        }

        std::string msg("\nFailed to retrieve expected configuration for device '" + m_deviceId + "' after " +
                        toString(nChecks) + " attempts - " + toString(nRemoteExceptions) +
                        " remote exceptions among them.\n");

        if (conf.size() > 0) {
            msg = msg + "On cycle '" + toString(i) + "' of  '" + toString(numCycles) + "', 'value' is '" +
                  toString(conf.get<int>("value")) + "'; the expected value is '" + toString(static_cast<int>(i + 1)) +
                  "'.\nThe retrieved 'value' timestamp is '" +
                  Epochstamp::fromHashAttributes(conf.getAttributes("value")).toIso8601() +
                  "'. The timepoint used by getConfigurationFromPast is '" + stampAfter.toIso8601() + "'.\n";
        }

        ASSERT_TRUE(conf.size() > 0) << msg;
        ASSERT_EQ(99, conf.get<int>("oldValue"));

        if (conf.get<int>("value") != static_cast<int>(i + 1)) {
            std::clog << msg << std::endl;
            failedCycles++;
            continue;
        }

        // Check received stamps: For "value" be aware that we store with
        // microsec precision only: we might be 1 off since we cut off digits instead of rounding
        const Epochstamp stampValueFromPast = Epochstamp::fromHashAttributes(conf.getAttributes("value"));
        ASSERT_TRUE((stampValueFromPast - valueStamps[i]).getFractions(TIME_UNITS::MICROSEC) <= 1ull)
              << stampValueFromPast.toIso8601() << " vs " << valueStamps[i].toIso8601();
        // The stamp for "oldValue" differs bewteen backends
        const Epochstamp stampOldFromPast = Epochstamp::fromHashAttributes(conf.getAttributes("oldValue"));
        std::string oldFromPastStr = stampOldFromPast.toIso8601(); // convert to microsecond precision
        if (pastConfigStaysPast) {
            // The stamp of "oldValue" is always the same
            ASSERT_EQ(oldStamp.toIso8601(), oldFromPastStr) << "'oldValue' from past has wrong time stamp";
        } else {
            // Timestamps older than start of device logging are stored as stamp "start of device logging".
            // Stamp stampsAfterRestart[i] is after we are sure that logging runs again, so that is after
            // "start of device logging" - but we cannot really be sure how far.
            ASSERT_TRUE(stampOldFromPast < stampsAfterRestart[i])
                  << "received '" << stampOldFromPast.toIso8601() << "', after restart '"
                  << stampsAfterRestart[i].toIso8601();
            const double dt =
                  stampOldFromPast - stampsAfterRestart[i]; // Has no sign due to the intermediate TimeDuration object
            ASSERT_TRUE(dt < 10.) << "'oldValue' has wrong time stamp: " << stampOldFromPast.toIso8601()
                                  << " - difference is : "
                                  << toString(
                                           dt); // seen 2.95 (!) in https://git.xfel.eu/Karabo/Framework/-/jobs/290211
        }
    }

    std::ostringstream valueHist;
    if (failedCycles > 0) {
        // Gather the value history during the test period to ease troubleshooting a test failure.
        const std::string fromEpochStr{testCaseStart.toIso8601Ext()};
        const std::string toEpochStr{Epochstamp().toIso8601Ext()};
        valueHist << "History of property 'value' of device '" << deviceId << "' between '" << fromEpochStr << "' and '"
                  << toEpochStr << "':\n";
        Hash params;
        params.set("from", fromEpochStr);
        params.set("to", toEpochStr);
        params.set("maxNumData", static_cast<int>(numCycles * 2));
        vector<Hash> history;
        std::string histDevice, histProperty;
        ASSERT_NO_THROW(m_sigSlot->request(dlReader, "slotGetPropertyHistory", deviceId, "value", params)
                              .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                              .receive(histDevice, histProperty, history));
        for (const Hash& histEntry : history) {
            valueHist << Epochstamp::fromHashAttributes(histEntry.getAttributes("v")).toIso8601Ext() << " - "
                      << histEntry.get<int>("v") << "\n";
        }
        valueHist << "\n";
    }

    ASSERT_EQ(0, failedCycles) << failedCycles << " out of " << numCycles << " cycles failed!\n" << valueHist.str();

    const karabo::data::TimeDuration testDuration = testCaseStart.elapsed();

    const std::streamsize currPrecision{std::clog.precision()};
    std::clog << "(testCfgFromPastRestart took " << std::setprecision(4) << static_cast<double>(testDuration)
              << " sec. to execute)" << std::setprecision(currPrecision) << std::endl;

    std::clog << "OK" << std::endl;
}


void TestDataLogging::testUnchangedNoDefaultProperties() {
    using namespace std;
    clog << "Testing past config retrieval of properties with no default value ..." << flush;

    const string noDefaultProp = "Int32NoDefault";
    const string dlReader = karabo::util::DATALOGREADER_PREFIX + m_server;

    // Start test device and take care that the logger is ready for it
    // Use platform-dependent name for the device: concurrent tests in CI
    // operate on the same InfluxDB database ...
    const string deviceId(getDeviceIdPrefix() + "deviceWithNoDefaultValueProp");
    const string logId = karabo::util::DATALOGGER_PREFIX + m_server;
    auto success =
          m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    waitUntilLogged(deviceId, "testUnchangedNoDefaultProperties");

    // Set the value of the test device's property with no default value.
    ASSERT_NO_THROW(m_deviceClient->set(deviceId, noDefaultProp, 12));
    std::this_thread::sleep_for(100ms);

    // Save this instant as a iso string
    Epochstamp es_afterPropSet;
    string afterPropSet = es_afterPropSet.toIso8601();

    // Flushes the datalogger to guarantee that setting of the property will be
    // in the log.
    ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                          .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                          .receive());
    std::this_thread::sleep_for(150ms);

    // Assert that getConfigurationFromPast retrieves the non default value
    // when given a timepoint right after the setting.
    Hash conf;
    Schema schema;
    bool configAtTimepoint;
    string configTimepoint;

    int nTries = NUM_RETRY;
    while (!conf.has(noDefaultProp) && nTries > 0) {
        try {
            m_sigSlot->request(dlReader, "slotGetConfigurationFromPast", deviceId, afterPropSet)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(conf, schema, configAtTimepoint, configTimepoint);
        } catch (const RemoteException& e) {
            const std::string errMsg = e.detailedMsg();
            // Tolerate only exception with following text that may come if file logger's disk IO is pretty slow
            auto errExpect = "Requested time point for device configuration is earlier than anything logged.";
            ASSERT_TRUE(errMsg.find(errExpect) != std::string::npos) << errMsg;
            std::this_thread::sleep_for(milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
            m_sigSlot->call(karabo::util::DATALOGGER_PREFIX + m_server, "flush");
            --nTries;
        } catch (const std::exception& e) {
            ASSERT_TRUE(false) << e.what(); // do not tolerate any other exception
        }
    }
    ASSERT_TRUE(conf.has(noDefaultProp)) << toString(conf);
    ASSERT_EQ(12, conf.get<int>(noDefaultProp)) << "Value for '" << noDefaultProp << "' differs from expected.";

    // Shutdown the PropertyTest device
    success = m_deviceClient->killDevice(deviceId);
    ASSERT_TRUE(success.first) << success.second;

    // Reinstantiate the PropertyTest device, but this time do not set its
    // no default value property.
    success =
          m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    waitUntilLogged(deviceId, "testUnchangedNoDefaultProperties_2");

    // Flush the data logger to make sure there's no logging pendency
    ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                          .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                          .receive());
    // Wait some time for the data just flushed to be available for reading.
    std::this_thread::sleep_for(150ms);

    // Save this instant as a iso string
    Epochstamp es_afterDeviceReinst;
    string afterDeviceReinst = es_afterDeviceReinst.toIso8601();

    // Assert that getConfigurationFromPast for a timepoint after the second
    // instantiation of the PropertyTest device does not have the no default
    // value property.
    ASSERT_NO_THROW(m_sigSlot->request(dlReader, "slotGetConfigurationFromPast", deviceId, afterDeviceReinst)
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive(conf, schema, configAtTimepoint, configTimepoint));
    ASSERT_TRUE(!conf.has(noDefaultProp))
          << "Property '" << noDefaultProp << "' should not be in retrieved configuration.";

    // Shutdown the testing device.
    success = m_deviceClient->killDevice(deviceId);
    ASSERT_TRUE(success.first) << success.second;

    clog << "OK" << endl;
}


template <class T>
void isEqualMessage(const std::string& message, const T& expected, const T& actual,
                    const std::vector<karabo::data::Hash>& fullHistory) {
    ASSERT_EQ(expected, actual) << message << ":\n" << fullHistory;
}


template <>
void isEqualMessage(const std::string& message, const std::vector<karabo::data::Hash>& expected,
                    const std::vector<karabo::data::Hash>& actual, const std::vector<karabo::data::Hash>& fullHistory) {
    ASSERT_TRUE(karabo::data::fullyEquals(expected, actual)) << message << ":\n" << fullHistory;
}


template <>
void isEqualMessage(const std::string& message, const karabo::data::Hash& expected, const karabo::data::Hash& actual,
                    const std::vector<karabo::data::Hash>& fullHistory) {
    ASSERT_TRUE(expected.fullyEquals(actual, true)) << message << ":\n" << fullHistory;
}


template <>
void isEqualMessage(const std::string& message, const float& expected, const float& actual,
                    const std::vector<karabo::data::Hash>& fullHistory) {
    ASSERT_NEAR(expected, actual, expected * 1.e-6) << message << ":\n" << fullHistory;
}


template <class T>
void TestDataLogging::testHistory(const std::string& key, const std::function<T(int)>& f, const bool testConf) {
    const std::string dlReader = karabo::util::DATALOGREADER_PREFIX + m_server;
    const int max_set = 100;
    std::clog << "Testing Property History retrieval for '" << key << "'... " << std::flush;

    // get configuration for later checks
    Hash beforeWritesCfg;
    ASSERT_NO_THROW((m_deviceClient->get(m_deviceId, beforeWritesCfg)));

    std::this_thread::sleep_for(150ms);

    // save this instant as a iso string
    Epochstamp es_beforeWrites;
    std::string beforeWrites = es_beforeWrites.toIso8601();

    // write a bunch of times
    for (int i = 0; i < max_set; i++) {
        ASSERT_NO_THROW(m_deviceClient->set<T>(m_deviceId, key, f(i)));
        std::this_thread::sleep_for(10ms);
    }

    Hash afterWritesCfg;
    ASSERT_NO_THROW(m_deviceClient->get(m_deviceId, afterWritesCfg));

    // save this instant as a iso string
    Epochstamp es_afterWrites;
    std::string afterWrites = es_afterWrites.toIso8601();

    ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                          .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                          .receive());

    // place holders, could be skipped but they are here for future expansions of the tests
    std::string device;
    std::string property;
    vector<Hash> history;
    Hash params;
    params.set<string>("from", beforeWrites);
    params.set<string>("to", afterWrites);
    params.set<int>("maxNumData", max_set * 2);
    // the history retrieval might take more than one try, it could have to index the files (or wait
    // for the records to be available for reading in the Influx case).

    const unsigned int numGetPropHist = m_deviceClient->get<unsigned int>(dlReader, "numGetPropertyHistory");
    std::vector<std::string> exceptionsMsgs;

    int nTries = NUM_RETRY;
    unsigned int numExceptions = 0;
    unsigned int numChecks = 0;
    while (nTries >= 0 && history.size() != max_set) {
        try {
            numChecks++;
            // TODO: use the deviceClient to retrieve the property history
            // history = m_deviceClient->getPropertyHistory(m_deviceId, key, before, after, max_set * 2);
            m_sigSlot->request(dlReader, "slotGetPropertyHistory", m_deviceId, key, params)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(device, property, history);
        } catch (const karabo::data::TimeoutException& e) {
            karabo::data::Exception::clearTrace();
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
        } catch (const karabo::data::RemoteException& e) {
            karabo::data::Exception::clearTrace();
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
        }
        std::this_thread::sleep_for(milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
        nTries--;
    }

    ASSERT_EQ(static_cast<size_t>(max_set), history.size())
          << "History size different than expected after " << numChecks << " checks:\n\tdeviceId: " << m_deviceId
          << "\n\tkey: " << key << "\n\tparam.from: " << beforeWrites << "\n\tparam.to: " << afterWrites
          << "\n\tparam.maxNumData: " << (max_set * 2) << "\n\thistory.size(): " << history.size()
          << "\n\tNumber of Exceptions: " << toString(numExceptions) << "\n\tExceptions:\n"
          << boost::algorithm::join(exceptionsMsgs, "\n") << "\nhistory\t" << toString(history);

    ASSERT_EQ(numGetPropHist + numChecks, m_deviceClient->get<unsigned int>(dlReader, "numGetPropertyHistory"));

    for (int i = 0; i < max_set; i++) {
        // checking values and timestamps
        isEqualMessage("Wrong value in history " + toString(i), f(i), history[i].get<T>("v"), history);
        Epochstamp current = Epochstamp::fromHashAttributes(history[i].getAttributes("v"));
        ASSERT_TRUE(current <= es_afterWrites) << "Timestamp later than the requested window";
        ASSERT_TRUE(current >= es_beforeWrites) << "Timestamp earlier than the requested window";
        if (i > 0) {
            Epochstamp previous = Epochstamp::fromHashAttributes(history[i - 1].getAttributes("v"));
            ASSERT_TRUE(current > previous) << "Timestamp earlier than the requested window";
        }
    }
    std::clog << "Ok" << std::endl;

    // skip the configuration retrieval
    if (!testConf) return;

    std::clog << "Testing past configuration retrieval for '" << key << "'... " << std::flush;

    exceptionsMsgs.clear();

    ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                          .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                          .receive());

    const unsigned int numGetCfgFromPast = m_deviceClient->get<unsigned int>(dlReader, "numGetConfigurationFromPast");
    nTries = NUM_RETRY;
    numExceptions = 0;
    numChecks = 0;
    // place holder schema, could be checked in future tests
    Schema schema;
    Hash conf;
    while (nTries >= 0) { // '&& !conf.empty()' check not needed as in TestDataLogging::testCfgFromPastRestart:
        //                   The 'history.size() != max_set' check in the loop above already ensures that all data
        //                   available

        // TODO: use the deviceClient to retrieve the configuration from past
        // auto pair = m_deviceClient->getConfigurationFromPast(m_deviceId, before);
        // conf = pair.first;
        bool excepted = false;
        try {
            numChecks++;
            m_sigSlot->request(dlReader, "slotGetConfigurationFromPast", m_deviceId, beforeWrites)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(conf, schema);
        } catch (const karabo::data::TimeoutException& e) {
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
            excepted = true;
        } catch (const karabo::data::RemoteException& e) {
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
            excepted = true;
        }
        if (!excepted) break; // Any result should be trustworthy!
        std::this_thread::sleep_for(milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
        nTries--;
    }

    ASSERT_TRUE(conf.size() > 0) << "Configuration size from slotGetConfigurationFromPast still 0 after " << numChecks
                                 << " checks.\n\tdeviceId: " << m_deviceId << "\n\tparam.before: " << beforeWrites
                                 << "\n\tconf.size(): " << conf.size() << "\n\tNumber of Exceptions: " << numExceptions
                                 << "\n\tExceptions:\n"
                                 << boost::algorithm::join(exceptionsMsgs, "\n");
    ASSERT_EQ(numGetCfgFromPast + numChecks,
              m_deviceClient->get<unsigned int>(dlReader, "numGetConfigurationFromPast"));

    // One needs to check only the content here, therefore only the leaves are examined
    std::vector<std::string> leaves;
    getLeaves(beforeWritesCfg, schema, leaves, '.');
    std::vector<std::string> confLeaves;
    getLeaves(conf, schema, confLeaves, '.');
    std::string missingKeysFromPast;
    for (const std::string& leaf : leaves) {
        if (std::find(confLeaves.begin(), confLeaves.end(), leaf) != confLeaves.end()) {
            // Leaf is in the configuration retrieved from past - check its value against the
            // one in the configuration snapshot obtained directly from the device.
            ASSERT_EQ(beforeWritesCfg.getAs<std::string>(leaf), conf.getAs<std::string>(leaf))
                  << "Wrong configuration from past (before writes) for key :" << leaf;
        } else {
            // Configuration from past is only allowed to miss non-archived leaves. Checks that the
            // missing leaf has NO_ARCHIVING set for its ARCHIVE_POLICY attribute.
            if (!schema.hasArchivePolicy(leaf) || schema.getArchivePolicy(leaf) != Schema::NO_ARCHIVING) {
                missingKeysFromPast += leaf + " : ";
            }
        }
    }

    // Check that all keys are logged.
    ASSERT_EQ(0ul, missingKeysFromPast.size()) << "Missing keys in configuration from past (before writes):\n"
                                               << missingKeysFromPast;

    nTries = NUM_RETRY;
    numExceptions = 0;
    numChecks = 0;
    conf.clear();
    while (nTries >= 0) {
        bool excepted = false;
        try {
            // TODO: use the deviceClient to retrieve the configuration from past
            // auto pair = m_deviceClient->getConfigurationFromPast(m_deviceId, before);
            // conf = pair.first
            numChecks++;
            m_sigSlot->request(dlReader, "slotGetConfigurationFromPast", m_deviceId, afterWrites)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(conf, schema);
        } catch (const karabo::data::TimeoutException& e) {
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
            excepted = true;
        } catch (const karabo::data::RemoteException& e) {
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
            excepted = true;
        }
        if (!excepted) break; // Any result should be trustworthy!
        std::this_thread::sleep_for(milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
        nTries--;
    }

    ASSERT_TRUE(conf.size() > 0) << "Configuration still not retrieved after  " << numChecks
                                 << " checks.\n\tdeviceId: " << m_deviceId << "\n\tparam.before: " << beforeWrites
                                 << "\n\tconf.size(): " << conf.size() << "\n\tNumber of Exceptions: " << numExceptions
                                 << "\n\tRemote Errors:\n"
                                 << boost::algorithm::join(exceptionsMsgs, "\n");
    // One needs to check only the content here, therefore only the leaves are examined.
    leaves.clear();
    getLeaves(afterWritesCfg, schema, leaves, '.');
    confLeaves.clear();
    getLeaves(conf, schema, confLeaves, '.');
    missingKeysFromPast.clear();
    for (const std::string& leaf : leaves) {
        if (std::find(confLeaves.begin(), confLeaves.end(), leaf) != confLeaves.end()) {
            // Leaf is in the configuration retrieved from past - check its value against the
            // one in the configuration snapshot obtained directly from the device.
            ASSERT_EQ(afterWritesCfg.getAs<std::string>(leaf), conf.getAs<std::string>(leaf))
                  << "Wrong configuration from past (after writes) for key :" << leaf;
        } else {
            // Configuration from past is only allowed to miss non-archived leaves. Checks that the
            // missing leaf has NO_ARCHIVING set for its ARCHIVE_POLICY attribute.
            if (!schema.hasArchivePolicy(leaf) || schema.getArchivePolicy(leaf) != Schema::NO_ARCHIVING) {
                missingKeysFromPast += leaf + " : ";
            }
        }
    }

    // TODO: Uncomment the following assert as soon as all the missing keys cases are fixed.
    /*
    ASSERT_EQ(0ul, missingKeysFromPast.size())
          << "Missing keys in configuration from past (after writes):\n" << missingKeysFromPast;
     */
    // TODO: Remove the following conditional logging once the assert above is activated.
    if (!missingKeysFromPast.empty()) {
        std::clog << "Missing keys in configuration from past (after writes):\n" << missingKeysFromPast << std::endl;
    }

    std::clog << "Ok" << std::endl;
}


void TestDataLogging::testInt(bool testPastConf) {
    auto lambda = [](int i) -> int { return i; };
    testHistory<int>("int32Property", lambda, testPastConf);
}


void TestDataLogging::testUInt64(bool testPastConf) {
    auto lambda = [](int i) -> unsigned long long { return (unsigned long long)i - 1; };
    testHistory<unsigned long long>("uint64Property", lambda, testPastConf);
}


void TestDataLogging::testFloat(bool testPastConf) {
    auto lambda = [](int i) -> float { return 2.5e-8f * i; };
    testHistory<float>("floatProperty", lambda, testPastConf);
}


void TestDataLogging::testString(bool testPastConf) {
    auto lambda = [](int i) -> string { return ((i % 2) ? string() : "(1|2|" + karabo::data::toString(i)) + ")"; };
    testHistory<string>("stringProperty", lambda, testPastConf);

    // Also test a string with a new line character
    auto lambda2 = [](int i) -> string { return "with\nnewline" + karabo::data::toString(i); };
    testHistory<string>("stringProperty", lambda2, testPastConf);
}


void TestDataLogging::testVectorString(bool testPastConf) {
    auto lambdaMixed = [](int i) -> vector<string> {
        vector<string> v = {"abc" + toString(i), "xy|z" + toString(i), string(), "A\nB" + toString(i)};
        // rotate the vector to check all positions for the empty string
        std::rotate(v.begin(), v.begin() + (i % v.size()), v.end());
        return (i % 5 == 0) ? vector<string>() : v;
    };
    testHistory<vector<string>>("vectors.stringProperty", lambdaMixed, false);

    auto lambda = [](int i) -> vector<string> {
        // Also test pipe '|' (the separator in our text files) and new line '\n'
        vector<string> v = {"abc" + toString(i), "xy|z" + toString(i), "A\nB" + toString(i)};
        return v;
    };
    testHistory<vector<string>>("vectors.stringProperty", lambda, testPastConf);
}


void TestDataLogging::testVectorChar(bool testPastConf) {
    auto lambda = [](int i) -> vector<char> {
        const char c = static_cast<char>(i & 0xFF);
        return {c, c, c, c, c, 0};
    };
    testHistory<vector<char>>("vectors.charProperty", lambda, testPastConf);
}


void TestDataLogging::testVectorSignedChar(bool testPastConf) {
    auto lambda = [](int i) -> vector<signed char> {
        std::vector<signed char> result;
        if ((i % 3) != 0) { // every third is empty
            result = {2, -4, 8, -16, 32};
            if ((i % 2) == 0) result.push_back(std::numeric_limits<signed char>::lowest());
            if ((i % 5) == 0) result.push_back(std::numeric_limits<signed char>::max());
        }
        return result;
    };
    testHistory<vector<signed char>>("vectors.int8Property", lambda, testPastConf);
}


void TestDataLogging::testVectorUnsignedChar(bool testPastConf) {
    auto lambda = [](int i) -> vector<unsigned char> {
        std::vector<unsigned char> result;
        if ((i % 3) != 0) { // every third is empty
            result = {2, 4, 8, 16, 32};
            if ((i % 2) == 0) result.push_back(0);
            if ((i % 5) == 0) result.push_back(255);
        }
        return result;
    };
    testHistory<vector<unsigned char>>("vectors.uint8Property", lambda, testPastConf);
}


void TestDataLogging::testVectorBool(bool testPastConf) {
    auto lambda = [](int i) -> vector<bool> {
        if (i % 13 == 0) {
            return vector<bool>();
        } else if (i % 11) {
            return vector<bool>(1, (i % 2 == 0));
        } else {
            return vector<bool>{(i % 2 == 0), (i % 3 == 0), (i % 5 == 0), (i % 7 == 0)};
        }
    };
    testHistory<vector<bool>>("vectors.boolProperty", lambda, testPastConf);
}


void TestDataLogging::testVectorShort(bool testPastConf) {
    auto lambda = [](int i) -> vector<short> {
        std::vector<short> result;
        if ((i % 3) != 0) { // every third is empty
            result = {-2, 4, 0, 16, -5000};
            if ((i % 2) == 0) result.push_back(std::numeric_limits<short>::lowest());
            if ((i % 5) == 0) result.push_back(std::numeric_limits<short>::max());
        }
        return result;
    };
    testHistory<vector<short>>("vectors.int16Property", lambda, testPastConf);
}


void TestDataLogging::testVectorUnsignedShort(bool testPastConf) {
    auto lambda = [](int i) -> vector<unsigned short> {
        std::vector<unsigned short> result;
        if ((i % 3) != 0) { // every third is empty
            result = {4, static_cast<unsigned short>(2 * i), 8, 16, 5000};
            if ((i % 2) == 0) result.push_back(std::numeric_limits<unsigned short>::lowest());
            if ((i % 5) == 0) result.push_back(std::numeric_limits<unsigned short>::max());
        }
        return result;
    };
    testHistory<vector<unsigned short>>("vectors.uint16Property", lambda, testPastConf);
}


void TestDataLogging::testVectorInt(bool testPastConf) {
    auto lambda = [](int i) -> vector<int> {
        std::vector<int> result;
        if ((i % 3) != 0) { // every third is empty
            result = {2, -4 * i, 8 * i, 16, -5000};
            if ((i % 2) == 0) result.push_back(std::numeric_limits<int>::lowest());
            if ((i % 5) == 0) result.push_back(std::numeric_limits<int>::max());
        }
        return result;
    };
    testHistory<vector<int>>("vectors.int32Property", lambda, testPastConf);
}


void TestDataLogging::testVectorUnsignedInt(bool testPastConf) {
    auto lambda = [](int i) -> vector<unsigned int> {
        std::vector<unsigned int> result;
        if ((i % 3) != 0) { // every third is empty
            result = {2, static_cast<unsigned int>(4 * i), 8, 16, 5000};
            if ((i % 2) == 0) result.push_back(std::numeric_limits<unsigned int>::lowest());
            if ((i % 5) == 0) result.push_back(std::numeric_limits<unsigned int>::max());
        }
        return result;
    };
    testHistory<vector<unsigned int>>("vectors.uint32Property", lambda, testPastConf);
}


void TestDataLogging::testVectorLongLong(bool testPastConf) {
    auto lambda = [](int i) -> vector<long long> {
        std::vector<long long> result;
        if ((i % 3) != 0) { // every third is empty
            result = {2ll, -4ll * i, 8ll, 16ll * i, -500055ll};
            if ((i % 2) == 0) result.push_back(std::numeric_limits<long long>::lowest());
            if ((i % 5) == 0) result.push_back(std::numeric_limits<long long>::max());
        }
        return result;
    };
    testHistory<vector<long long>>("vectors.int64Property", lambda, testPastConf);
}


void TestDataLogging::testVectorUnsignedLongLong(bool testPastConf) {
    auto lambda = [](int i) -> vector<unsigned long long> {
        std::vector<unsigned long long> result;
        if ((i % 3) != 0) { // every third is empty
            result = {2ull, 4ull, 8ull, 16ull, 500055ull * i};
            if ((i % 2) == 0) result.push_back(std::numeric_limits<unsigned long long>::lowest());
            if ((i % 5) == 0) result.push_back(std::numeric_limits<unsigned long long>::max());
        }
        return result;
    };
    testHistory<vector<unsigned long long>>("vectors.uint64Property", lambda, testPastConf);
}


void TestDataLogging::testTable(bool testPastConf) {
    auto lambda = [](int i) -> vector<Hash> {
        vector<Hash> t = {// For strings, test also pipe '|' (the separator in our text files) and newline '\n'.
                          Hash("e1", "ab\nc" + karabo::data::toString(i), "e2", ((i % 2) == 0), "e3", 12 * i, "e4",
                               0.9837F * i, "e5", 1.2345 * i),
                          Hash("e1", "xy|z" + karabo::data::toString(i), "e2", ((i % 2) == 1), "e3", 42 * i, "e4",
                               2.33333F * i, "e5", 7.77777 * i)};
        return t;
    };
    testHistory<vector<Hash>>("table", lambda, testPastConf);
}


void TestDataLogging::testChar(bool testPastConf) {
    auto lambda = [](int i) -> char { return static_cast<char>(i & 0xff); };
    testHistory<char>("charProperty", lambda, testPastConf);
}


void TestDataLogging::testNans() {
    std::clog << "Test handling of NaNs for getPropertyHistory and getConfigurationFromPast ..." << std::endl;

    Epochstamp testCaseStart;

    const std::string deviceId(m_deviceId + "forNan");
    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "NanTestDevice", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    waitUntilLogged(deviceId, "testNans");

    const std::string dlReader = karabo::util::DATALOGREADER_PREFIX + m_server;
    const size_t max_set = 100ul;
    const size_t full_return_size = max_set + 1ul;
    std::clog << "Testing NaN and infinity are treated by Loggers " << std::flush;

    // define some bad floating points to test against
    const vector<float> bad_floats = {
          std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::signaling_NaN(),
          std::numeric_limits<float>::infinity(), -1.f * std::numeric_limits<float>::infinity()};
    const vector<double> bad_doubles = {
          std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::signaling_NaN(),
          std::numeric_limits<double>::infinity(), -1. * std::numeric_limits<double>::infinity()};

    // save this instant as a iso string
    Epochstamp es_beforeWrites;
    std::string beforeWrites = es_beforeWrites.toIso8601();
    // In this test we do not care about problems that the file data logger has with the first history request nor
    // about any potential interference created by indexing files on-the-fly when we continue to write that
    // property. Therefore we trigger direct index creation for the needed properties before they get updated.
    for (const std::string& property : std::vector<std::string>({"int32Property", "floatProperty", "doubleProperty"})) {
        const Hash params("from", beforeWrites, "to", Epochstamp().toIso8601(), "maxNumData",
                          static_cast<int>(max_set * 2));
        m_sigSlot->call(dlReader, "slotGetPropertyHistory", deviceId, property, params); // fire-and-forget...
    }

    // Collect stamps for when each bad floating point has been set (once) - to later test
    // slotGetConfigurationFromPast. Use std::min with max_set as protection (max_set _should_ always be larger...)
    std::vector<Epochstamp> vec_es_afterWrites(std::min(max_set, bad_floats.size()), Epochstamp(0ull, 0ull));
    // Also collect stamps of most recent update stamp at the above points in time
    std::vector<Epochstamp> vec_es_updateStamps(vec_es_afterWrites);
    // write a bunch of times and record the timestamps of the updated properties
    std::vector<Epochstamp> updateStamps;
    for (size_t i = 0; i < max_set; i++) {
        Hash new_conf;
        new_conf.set("int32Property", static_cast<int>(i));
        new_conf.set("floatProperty", bad_floats[i % bad_floats.size()]);
        new_conf.set("doubleProperty", bad_doubles[i % bad_doubles.size()]);

        ASSERT_NO_THROW(m_deviceClient->set(deviceId, new_conf));
        const Hash cfg = m_deviceClient->get(deviceId);
        updateStamps.push_back(Epochstamp::fromHashAttributes(cfg.getAttributes("doubleProperty")));
        std::this_thread::sleep_for(10ms);
        if (i < vec_es_afterWrites.size()) {
            vec_es_afterWrites[i].now();
            // Looks like doublePropertyReadOnly is updated later than doubleProperty:
            vec_es_updateStamps[i] = Epochstamp::fromHashAttributes(cfg.getAttributes("doublePropertyReadOnly"));
        }
    }

    // set one last time a valid value.
    Hash end_conf;
    end_conf.set("int32Property", static_cast<int>(max_set));
    end_conf.set("floatProperty", (1.f * max_set));
    end_conf.set("doubleProperty", (1. * max_set));
    ASSERT_NO_THROW(m_deviceClient->set(deviceId, end_conf));
    updateStamps.push_back(
          Epochstamp::fromHashAttributes(m_deviceClient->get(deviceId).getAttributes("doubleProperty")));
    // The sleep interval below had to be increased because of the Telegraf environment - the time required to save
    // is higher. If es_afterWrites captured after the sleep instruction refers to a time point that comes before
    // the time Telegraf + Influx are done writing the data, the property history will not be of the expected size
    // and the test will fail.
    std::this_thread::sleep_for(milliseconds(WAIT_WRITES));

    // save this instant as a iso string
    Epochstamp es_afterWrites;
    std::string afterWrites = es_afterWrites.toIso8601();

    ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                          .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                          .receive());

    Hash params;
    params.set<string>("from", beforeWrites);
    params.set<string>("to", afterWrites);
    params.set<int>("maxNumData", max_set * 2);
    std::vector<std::string> exceptionsMsgs;

    // Check the length of the history for the properties injected.
    const std::map<std::string, size_t> properties = {std::make_pair(std::string("int32Property"), full_return_size),
                                                      std::make_pair(std::string("floatProperty"), full_return_size),
                                                      std::make_pair(std::string("doubleProperty"), full_return_size)};

    for (const auto& property_pair : properties) {
        int nTries = NUM_RETRY;
        unsigned int numChecks = 0;
        unsigned int numExceptions = 0;
        vector<Hash> history;
        // TODO: Remove beforeFistsCheck, afterLastCheck and the printout of the statistics for obtaining history
        // a.s.a.
        //       load on exflserv10 gets normal.
        Epochstamp beforeFirstCheck;
        while (nTries >= 0 && history.size() != property_pair.second) {
            std::string device, property;
            try {
                numChecks++;
                // TODO: use the deviceClient to retrieve the property history
                // history = m_deviceClient->getPropertyHistory(deviceId, key, before, after, max_set * 2);
                m_sigSlot->request(dlReader, "slotGetPropertyHistory", deviceId, property_pair.first, params)
                      .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                      .receive(device, property, history);
            } catch (const karabo::data::TimeoutException& e) {
                karabo::data::Exception::clearTrace();
                exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
                ++numExceptions;
            } catch (const karabo::data::RemoteException& e) {
                karabo::data::Exception::clearTrace();
                exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
                ++numExceptions;
            }
            std::this_thread::sleep_for(milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
            nTries--;
        }

        Epochstamp afterLastCheck;
        if (property_pair.second == history.size()) {
            std::clog << "\ntestNans: History size check for property '" << property_pair.first << "' succeeded after "
                      << numChecks << " attempt(s) ranging from " << beforeFirstCheck.toIso8601() << " to "
                      << afterLastCheck.toIso8601() << " ("
                      << afterLastCheck.elapsed(beforeFirstCheck).getTotalSeconds() << " secs)." << std::endl;
        }

        ASSERT_EQ(static_cast<size_t>(property_pair.second), history.size())
              << "History size different than expected after " << toString(numChecks)
              << " checks:\n\tdeviceId: " << deviceId << "\n\tproperty : " << property_pair.first
              << "\n\tparam.from: " << beforeWrites << "\n\tparam.to: " << afterWrites
              << "\n\tparam.maxNumData: " << (max_set * 2) << "\n\thistory.size(): " << history.size()
              << "\n\tNumber of Exceptions: " << numExceptions << "\n\tExceptions:\n"
              << boost::algorithm::join(exceptionsMsgs, "\n");

        // Test that the return values match, incl. timestamps
        for (size_t i = 0; i <= max_set; ++i) {
            // First check timestamp - to microsecond precision
            const Epochstamp historyStamp = Epochstamp::fromHashAttributes(history[i].getAttributes("v"));
            const TimeDuration diff = historyStamp.elapsed(updateStamps[i]);
            ASSERT_TRUE(diff < TimeDuration(0ull, 1000000000000ull)) << toString(diff); // 1e12 attosec, i.e. 1 microsec

            if (property_pair.first == "floatProperty") {
                const float floatInput = (i == max_set ? max_set : bad_floats[i % bad_floats.size()]);
                const float historyFloat = history[i].get<float>("v");
                if (std::isnan(floatInput)) {
                    // comparison with nan is always false
                    ASSERT_TRUE(std::isnan(historyFloat)) << i;
                } else {
                    // comparison with +/-inf works
                    ASSERT_EQ(floatInput, historyFloat) << i;
                }
            } else if (property_pair.first == "doubleProperty") {
                const double doubleInput = (i == max_set ? max_set : bad_doubles[i % bad_doubles.size()]);
                const double historyDouble = history[i].get<double>("v");
                if (std::isnan(doubleInput)) {
                    // comparison with nan is always false
                    ASSERT_TRUE(std::isnan(historyDouble)) << i;
                } else {
                    // comparison with +/-inf works
                    ASSERT_EQ(doubleInput, historyDouble) << i;
                }
            } else if (property_pair.first == "int32Property") {
                ASSERT_EQ(static_cast<int>(i), history[i].get<int>("v")) << i;
            }
        }
    }

    // Now test slotGetConfigurationFromPast with infinite values
    for (size_t i = 0; i < vec_es_afterWrites.size(); ++i) {
        Hash conf;
        Schema schema;
        bool configAtTimepoint = false;
        std::string configTimepoint;
        ASSERT_NO_THROW(
              m_sigSlot->request(dlReader, "slotGetConfigurationFromPast", deviceId, vec_es_afterWrites[i].toIso8601())
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                    .receive(conf, schema, configAtTimepoint, configTimepoint));

        ASSERT_TRUE(configAtTimepoint);
        // This equality check relies on the fact that the string representation implicitly rounds to micro second
        // precision, i.e. the precision in the data base. So if the test fails here, do like above with the
        // TimeDuration.
        ASSERT_EQ(vec_es_updateStamps[i].toIso8601Ext(), configTimepoint);
        const double theD = conf.get<double>("doubleProperty");
        const float theF = conf.get<float>("floatProperty");
        if (std::isnan(bad_floats[i])) {
            // assuming same order of nan/inf for both bad_floats and bad_doubles
            ASSERT_TRUE(std::isnan(theF)) << i << ": theF = " << theF;
            ASSERT_TRUE(std::isnan(theD)) << i << ": theD = " << theD;
        } else {
            ASSERT_EQ(bad_floats[i], theF) << i;
            ASSERT_EQ(bad_doubles[i], theD) << i;
        }
    }

    // Clean-up
    success = m_deviceClient->killDevice(deviceId);
    ASSERT_TRUE(success.first) << success.second;

    const karabo::data::TimeDuration testDuration = testCaseStart.elapsed();

    const std::streamsize currPrecision{std::clog.precision()};
    std::clog << "(testNans took " << std::setprecision(4) << static_cast<double>(testDuration) << " sec. to execute)"
              << std::setprecision(currPrecision) << std::endl;

    std::clog << "Ok" << std::endl;
}


void TestDataLogging::testSchemaEvolution() {
    std::clog << "Testing property history retrieval when schema evolution happens ..." << std::endl;

    // Instantiates a DataLogTestDevice to use for the schema evolution test.
    // "m_deviceIdPrefix" allows concurrent Influx tests on the different platform CI runners.
    const std::string deviceId(getDeviceIdPrefix() + "SchemaEvolutionDevice");
    auto success =
          m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    waitUntilLogged(deviceId, "testSchemaEvolution");

    // Captures the timepoint before any property modification.
    Epochstamp fromTimePoint;

    // "reconfigurableValue" as string
    Schema schemaStr;
    STRING_ELEMENT(schemaStr)
          .key("reconfigurableValue")
          .assignmentOptional()
          .defaultValue("")
          .reconfigurable()
          .commit();
    ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStr).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotReconfigure", Hash("reconfigurableValue", "Non empty str"))
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive());

    // "reconfigurableValue" as vector of strings
    ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", Schema()).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    Schema schemaVecStr;
    VECTOR_STRING_ELEMENT(schemaVecStr)
          .key("reconfigurableValue")
          .assignmentOptional()
          .defaultValue(std::vector<std::string>({"a"}))
          .reconfigurable()
          .commit();
    ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotUpdateSchema", schemaVecStr)
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive());
    const std::vector<std::string> strVector{"a", "", "b", "c"};
    Hash strVecValueCfg{"reconfigurableValue", strVector};
    ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotReconfigure", strVecValueCfg)
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive());


    // "reconfigurableValue" as int32
    ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", Schema()).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    Schema schemaInt32;
    INT32_ELEMENT(schemaInt32)
          .key("reconfigurableValue")
          .assignmentOptional()
          .defaultValue(0)
          .reconfigurable()
          .commit();
    ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaInt32).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotReconfigure", Hash("reconfigurableValue", 10))
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive());

    // Makes sure all the writes are done before retrieval.
    ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                          .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                          .receive());
    // The sleep interval below had to be added because of the Telegraf environment - the time required to save is
    // higher. If toTimePoint captured after the sleep instruction refers to a time point that comes before the time
    // Telegraf + Influx are done writing the data, the property history will not be of the expected size and the
    // test will fail.
    std::this_thread::sleep_for(milliseconds(WAIT_WRITES));

    // Checks that all the property values set with the expected types can be retrieved.
    Epochstamp toTimePoint;

    Hash params;
    params.set<string>("from", fromTimePoint.toIso8601());
    params.set<string>("to", toTimePoint.toIso8601());
    const int maxNumData = 10;
    params.set<int>("maxNumData", maxNumData);

    vector<Hash> history;
    std::string replyDevice;
    std::string replyProperty;

    const std::string dlReader = karabo::util::DATALOGREADER_PREFIX + m_server;

    // the history retrieval might take more than one try, it could have to index the files (or wait
    // for the records to be available for reading in the Influx case).
    std::vector<std::string> exceptionsMsgs;

    int nTries = NUM_RETRY;
    unsigned int numExceptions = 0;
    unsigned int numChecks = 0;
    while (nTries >= 0 && history.size() != 6) {
        try {
            numChecks++;
            m_sigSlot->request(dlReader, "slotGetPropertyHistory", deviceId, "reconfigurableValue", params)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(replyDevice, replyProperty, history);
        } catch (const karabo::data::TimeoutException& e) {
            karabo::data::Exception::clearTrace();
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
        } catch (const karabo::data::RemoteException& e) {
            karabo::data::Exception::clearTrace();
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
        }
        std::this_thread::sleep_for(milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
        nTries--;
    }

    ASSERT_EQ(6, static_cast<int>(history.size()))
          << "History size different than expected after " << numChecks << " checks:\n\tdeviceId: " << deviceId
          << "\n\tproperty: \"value\"" << "\n\tparam.from: " << fromTimePoint.toIso8601()
          << "\n\tparam.to: " << toTimePoint.toIso8601() << "\n\tparam.maxNumData: " << maxNumData
          << "\n\thistory.size(): " << history.size() << "\n\tNumber of Exceptions: " << numExceptions
          << "\n\tExceptions:\n"
          << boost::algorithm::join(exceptionsMsgs, "\n");

    // Checks the first two expected values - of string type.
    // The first is the empty string that is the default value.
    // The second is an explicitily set non empty string value.
    ASSERT_EQ("", history[0].get<std::string>("v")) << "First string value different from expected for history entry";
    ASSERT_EQ("Non empty str", history[1].get<std::string>("v"))
          << "Second string value different from expected for history entry";

    // Checks the next two expected values - of vector of strings type.
    // The first is the empty vector that is the default value.
    // The second is an explicitily set non empty vector.

    ASSERT_EQ(1ul, history[2].get<std::vector<std::string>>("v").size())
          << "First vector of strings in history is not of the expected size";
    ASSERT_EQ("a", history[2].get<std::vector<std::string>>("v")[0])
          << "Value of sole element of first vector of strings in history different from exepected";

    const auto& strVectorValue = history[3].get<std::vector<std::string>>("v");
    ASSERT_EQ(strVector.size(), strVectorValue.size())
          << "Size of second vector of strings in history different from expected";
    for (size_t i = 0; i < strVectorValue.size(); i++) {
        ASSERT_EQ(strVector[i], strVectorValue[i])
              << "Value at index " << i << " of second vector of strings in history different from expected";
    }
    // Checks the last expected values - of int type.
    // The first is the default value of 0.
    // The second is an explicitily set int value.
    ASSERT_EQ(0, history[4].get<int>("v")) << "First int value different from expected for history entry";
    ASSERT_EQ(10, history[5].get<int>("v")) << "Second int value different from expected for history entry";

    std::clog << "Ok" << std::endl;
}

// TODO: implement testVectorFloat() (include vectors of different length and empty vector);
// TODO: implement test and FIX for a vector of strings with an empty string as its only element


TEST_F(TestDataLogging, influxAllTestRunner) {
    std::this_thread::sleep_for(1000ms);

    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", m_deviceId), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    setPropertyTestSchema();

    std::clog << "\n==== Starting sequence of Influx Logging tests on \"" << m_deviceId << "\" ====" << std::endl;
    success = startDataLoggerManager("InfluxDataLogger");
    ASSERT_TRUE(success.first) << success.second;

    testAllInstantiated();

    testMaxNumDataRange();
    testMaxNumDataHistory();
    testDropBadData();

    testLoggerMapProperty();

    // Following tests use device m_deviceId, so ensure it is logged
    waitUntilLogged(m_deviceId, "influxAllTestRunner");
    testInt(true);
    testUInt64(false);
    testFloat(false);
    testString(false);
    testChar(false);
    testVectorString(false);
    testVectorChar(false);
    testVectorSignedChar(false);
    testVectorUnsignedChar(false);
    testVectorBool(false);
    testVectorShort(false);
    testVectorUnsignedShort(false);
    testVectorInt(false);
    testVectorUnsignedInt(false);
    testVectorLongLong(false);
    testVectorUnsignedLongLong(false);
    testTable(false);

    testUnchangedNoDefaultProperties();

    // This must be the last test case that relies on the device in m_deviceId (the logged
    // PropertyTest instance) being available at the start of the test case.
    // 'testLastKnownConfiguration' stops the device being logged to make sure that the
    // last known configuration can be successfully retrieved after the device is gone.
    testLastKnownConfiguration();

    // These deal with their own devices, so comment above about using the PropertyTest instance
    // in m_deviceId is not applicable.
    testCfgFromPastRestart(
          false); // in influx logging, old, past device incarnation stamps are logged as start of device logging
    testSchemaEvolution();
    testNans();
}


/**
 * @brief Checks that the maximum per device schema logging rate for Influx is being properly enforced.
 *
 * This test instantiates its own Influx DataLoggerManager with a much lower threshold for the maximum schema
 * logging rate allowed. The smaller value is needed to make it easier to violate the threshold. It has to be
 * run in isolation because a smaller threshold could easily interfere with the other tests.
 */
TEST_F(TestDataLogging, testInfluxMaxSchemaLogRate) {
    std::clog << "Testing enforcing of max schema logging rate limit for Influx ..." << std::endl;

    const unsigned int rateWinSecs = 1u;
    const unsigned int afterFlushWait = 1'000u;

    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const std::string logReaderId = karabo::util::DATALOGREADER_PREFIX + m_server;
    const std::string deviceId(getDeviceIdPrefix() + "SchemaLogRateDevice");

    // defValueSuffix guarantees uniqueness of the schema - the test doesn't
    // assume that the database is clear of its previous runs.
    const std::string defValueSuffix = toString(Epochstamp().getTime());

    // Schema injections to be used throughout the test.
    Schema schemaStrA;
    STRING_ELEMENT(schemaStrA)
          .key("stringProperty")
          .assignmentOptional()
          .defaultValue("A_" + defValueSuffix)
          .reconfigurable()
          .commit();
    Schema schemaStrB;
    STRING_ELEMENT(schemaStrB)
          .key("stringProperty")
          .assignmentOptional()
          .defaultValue("B_" + defValueSuffix)
          .reconfigurable()
          .commit();
    Schema schemaStrC;
    STRING_ELEMENT(schemaStrC)
          .key("stringProperty")
          .assignmentOptional()
          .defaultValue("C_" + defValueSuffix)
          .reconfigurable()
          .commit();
    Schema schemaStrD;
    STRING_ELEMENT(schemaStrD)
          .key("stringPropertyD")
          .assignmentOptional()
          .defaultValue("D_" + defValueSuffix)
          .reconfigurable()
          .commit();

    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    // Starts the logger and readers with a lower max schema rate threshold - 18 kb/s - over a rateWinSecs seconds
    // rating window. The base64 encoded schema of the DataLogTestDevice is 12,516 bytes (before schema update),
    // so with rateWinSecs == 1, a single schema can be logged in that period, but two cannot.
    success = startDataLoggerManager("InfluxDataLogger", false, false, 32, rateWinSecs, 18, rateWinSecs);
    ASSERT_TRUE(success.first) << success.second;

    testAllInstantiated();
    waitUntilLogged(deviceId, "testInfluxMaxSchemaLogRate");

    // Wait some time to isolate the schema update bursts.
    std::this_thread::sleep_for(milliseconds(rateWinSecs * 1000 + 1));

    ///////  Checks that a schema update within the rating limit is accepted.
    Epochstamp beforeFirstBurst;
    ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStrA).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    // Makes sure that data has been received by logger and written to Influx.
    std::this_thread::sleep_for(500ms);
    ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", FLUSH_REQUEST_TIMEOUT_MILLIS / 1000));
    std::this_thread::sleep_for(milliseconds(afterFlushWait));
    Epochstamp afterFirstBurst;

    // Checks that the schema update has not been flagged as bad data.
    Hash badDataAllDevices;
    ASSERT_NO_THROW(m_sigSlot
                          ->request(logReaderId, "slotGetBadData", beforeFirstBurst.toIso8601Ext(),
                                    afterFirstBurst.toIso8601Ext())
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive(badDataAllDevices));
    ASSERT_EQ(0ul, badDataAllDevices.size()) << toString(badDataAllDevices);

    // Wait some time to isolate the schema update bursts.
    std::this_thread::sleep_for(milliseconds(rateWinSecs * 1000 + 1));

    ////////  Checks that two schema updates in a fast succession would go above the
    ////////  threshold and one of the updates (the second) would be rejected.
    Epochstamp beforeSecondBurst;
    ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStrB).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStrC).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    // Makes sure that data has been received by logger and written to Influx.
    std::this_thread::sleep_for(500ms);
    ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", FLUSH_REQUEST_TIMEOUT_MILLIS / 1000));
    std::this_thread::sleep_for(milliseconds(afterFlushWait));
    Epochstamp afterSecondBurst;

    // Checks that one of the schema updates failed.
    badDataAllDevices.clear();
    waitForCondition(
          [this, &badDataAllDevices, &logReaderId, &beforeSecondBurst, &afterSecondBurst]() {
              try {
                  m_sigSlot
                        ->request(logReaderId, "slotGetBadData", beforeSecondBurst.toIso8601Ext(),
                                  afterSecondBurst.toIso8601Ext())
                        .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                        .receive(badDataAllDevices);
                  return badDataAllDevices.size() == 1ul;
              } catch (const std::exception& e) {
                  std::clog << "ERROR trying to retrieve BadData for all devices: " << e.what() << std::flush;
                  return false;
              }
          },
          KRB_TEST_MAX_TIMEOUT * 1'000u, 200u);
    ASSERT_EQ(1ul, badDataAllDevices.size());
    ASSERT_TRUE(badDataAllDevices.has(deviceId));
    const auto& badDataEntries = badDataAllDevices.get<std::vector<Hash>>(deviceId);
    ASSERT_EQ(1ul, badDataEntries.size());
    const std::string& badDataInfo = badDataEntries[0].get<std::string>("info");
    ASSERT_TRUE(badDataInfo.find(deviceId + "::schema") != std::string::npos)
          << "Expected pattern, '" << deviceId << "::schema', not found in bad data description:\n'" << badDataInfo
          << "'";

    // Wait some time to isolate the schema update bursts.
    std::this_thread::sleep_for(milliseconds(rateWinSecs * 1000 + 1));

    //////  Checks that after the updates have settled down for a while, schemas
    //////  can be logged again.
    Epochstamp beforeThirdBurst;
    ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStrD).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    // Makes sure that data has been received by logger and written to Influx.
    std::this_thread::sleep_for(500ms);
    ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", FLUSH_REQUEST_TIMEOUT_MILLIS / 1000));
    std::this_thread::sleep_for(milliseconds(afterFlushWait));
    Epochstamp afterThirdBurst;
    // Checks that the schema update succeeded.
    badDataAllDevices.clear();
    ASSERT_NO_THROW(m_sigSlot
                          ->request(logReaderId, "slotGetBadData", beforeThirdBurst.toIso8601Ext(),
                                    afterThirdBurst.toIso8601Ext())
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive(badDataAllDevices));
    ASSERT_EQ(0ul, badDataAllDevices.size());

    // Checks that the latest version of the schema, and by consequence, the past configuration are retrieved correctly.
    // This check fails for schemas saved by versions of the InfluxDataLogger prior to the fixes in
    // https://git.xfel.eu/Karabo/Framework/-/merge_requests/6470 and retrieved with versions of the InfluxLogReader
    // prior to the modifications in https://git.xfel.eu/Karabo/Framework/-/merge_requests/6478.
    Schema schema;
    Hash pastCfg;
    bool cfgAtTime;
    std::string cfgTime;
    int nTries = NUM_RETRY;
    do {
        ASSERT_NO_THROW(
              m_sigSlot->request(logReaderId, "slotGetConfigurationFromPast", deviceId, afterThirdBurst.toIso8601())
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                    .receive(pastCfg, schema, cfgAtTime, cfgTime));
        if (schema.has("stringPropertyD")) break;
        std::this_thread::sleep_for(milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
    } while (nTries-- > 0);
    ASSERT_TRUE(schema.has("stringPropertyD")) << "Schema lacks expected key, \"stringPropertyD\"";
    ASSERT_EQ(Types::STRING, schema.getValueType("stringPropertyD"));
    ASSERT_EQ(pastCfg.get<std::string>("stringPropertyD"), "D_" + defValueSuffix);

    std::clog << "OK" << std::endl;
}


/**
 * @brief Checks that the maximum length allowed for a string value to be saved on Influx is being enforced and that
 * all violations are being properly logged.
 */
TEST_F(TestDataLogging, testInfluxMaxStringLength) {
    std::clog << "Testing enforcing of max string value length for Influx ..." << std::endl;

    const unsigned int maxStringLength = 8'192u;
    const std::string belowLimitStr(maxStringLength / 2, 'B');
    const std::string atLimitStr(maxStringLength, '@');
    const std::string aboveLimitStr(maxStringLength * 2, 'A');

    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const std::string logReaderId = karabo::util::DATALOGREADER_PREFIX + m_server;
    // A device exclusive for this test case is used to guarantee that its
    // schema will be partitioned into multiple chunks. The default schema for
    // the PropertyTest device has around 78 Kb and for this test the
    // value used for the Influx logger "maxStringValueLength" property is 8 kb.
    const std::string propTestDevice = m_deviceId + "__MAX__STRING";

    const unsigned int afterFlushWait = 1'000u;

    Epochstamp beforeServerInstantiation;

    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", propTestDevice), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    success = startDataLoggerManager("InfluxDataLogger", false, false, 5120u, 5u, 15'360u, 5u, maxStringLength);
    ASSERT_TRUE(success.first) << success.second;

    testAllInstantiated();
    waitUntilLogged(propTestDevice, "testInfluxMaxStringLength");

    ///////  Checks that a string below the length limit is accepted
    Epochstamp beforeBelowLimit;
    m_deviceClient->set(propTestDevice, "stringProperty", belowLimitStr);
    // Makes sure the data has been written to Influx
    ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());
    std::this_thread::sleep_for(milliseconds(afterFlushWait));
    Epochstamp afterBelowLimit;

    Hash badDataAllDevices;
    ASSERT_NO_THROW(m_sigSlot
                          ->request(logReaderId, "slotGetBadData", beforeBelowLimit.toIso8601Ext(),
                                    afterBelowLimit.toIso8601Ext())
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive(badDataAllDevices));
    ASSERT_EQ(0ul, badDataAllDevices.size());

    ///////  Checks that a string whose length is exactly at the limit is accepted.
    Epochstamp beforeAtLimit;
    m_deviceClient->set(propTestDevice, "stringProperty", atLimitStr);
    // Makes sure the data has been written to Influx.
    ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());
    std::this_thread::sleep_for(milliseconds(afterFlushWait));
    Epochstamp afterAtLimit;

    badDataAllDevices.clear();
    ASSERT_NO_THROW(
          m_sigSlot->request(logReaderId, "slotGetBadData", beforeAtLimit.toIso8601Ext(), afterAtLimit.toIso8601Ext())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(badDataAllDevices));
    ASSERT_EQ(0ul, badDataAllDevices.size());


    ///////  Checks that a string above the length limit is rejected with the proper code.
    Epochstamp beforeAboveLimit;
    m_deviceClient->set(propTestDevice, "stringProperty", aboveLimitStr);
    // Makes sure the data has been written to Influx.
    ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());
    std::this_thread::sleep_for(milliseconds(afterFlushWait));
    Epochstamp afterAboveLimit;

    badDataAllDevices.clear();
    ASSERT_NO_THROW(m_sigSlot
                          ->request(logReaderId, "slotGetBadData", beforeAboveLimit.toIso8601Ext(),
                                    afterAboveLimit.toIso8601Ext())
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive(badDataAllDevices));
    ASSERT_EQ(1ul, badDataAllDevices.size());
    const auto deviceBadData = badDataAllDevices.get<std::vector<Hash>>(propTestDevice);
    const std::string badDataInfo = deviceBadData[0].get<std::string>("info");

    // [1] is the code for string metric values longer than the Influx limit.
    ASSERT_TRUE(badDataInfo.find(">> [1] 'stringProperty") != std::string::npos)
          << "Expected pattern, \">> [1] 'stringProperty'\", not found in bad data description:\n'" << badDataInfo
          << "'";

    ///////  Checks that the PropertyTest device with the appended prefix had its schema properly chunked by
    ///////  asserting that the past configuration with a string logged exactly at the limit can be retrieved.
    Schema schema;
    Hash pastCfg;
    bool cfgAtTime;
    std::string cfgTime;
    ASSERT_NO_THROW(
          m_sigSlot->request(logReaderId, "slotGetConfigurationFromPast", propTestDevice, afterAtLimit.toIso8601())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(pastCfg, schema, cfgAtTime, cfgTime));
    ASSERT_TRUE(schema.has("stringProperty")) << "Schema lacks expected key, \"stringProperty\"";
    ASSERT_EQ(Types::STRING, schema.getValueType("stringProperty"));
    ASSERT_EQ(pastCfg.get<std::string>("stringProperty"), atLimitStr);

    std::clog << "OK" << std::endl;
}


/**
 * @brief Checks that the maximum per device property logging rate for Influx is being properly enforced.
 *
 * This test instantiates its own Influx DataLoggerManager with a much lower threshold for the maximum logging rate
 * allowed. The smaller value is needed to make it easier to violate the threshold. It has to be run in isolation
 * because a smaller threshold could easily interfere with the other tests.
 */
TEST_F(TestDataLogging, testInfluxMaxPerDevicePropLogRate) {
    std::clog << "Testing enforcing of max per device property logging rate limit for Influx ..." << std::endl;

    // CAVEAT - to avoid long sleeps between its parts, this test sets the timestamp properties as part of the
    // property updates calls. If the difference between the properties timestamps and the local system time becomes
    // greater than a given interval (currently 120 seconds), the Influx logger will stop using the property timestamps
    // as the reference to calculate the rates and will start using the local system time.
    // For this test to work, the whole time spam of its execution, using property timestamps as the time reference,
    // must be less than the clock difference tolerated by the Influx logger (currently 120 seconds).

    const unsigned int rateWinSecs = 2u; // Size, in seconds, of the rating window to be used during the test. Limited
                                         // by the current maximum allowed value for the property "propLogRatePeriod" of
                                         // the InfluxDataLogger and by the test specific caveat above. Values above 10
                                         // are not recommended for this test the size of the property histories
                                         // retrieved can become large and drain resources on the CI machines.

    const int maxPropHistSize = rateWinSecs * 8; // 8 is the maximum number of times a property is written per iteration
                                                 // during the write bursts of the tests.

    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const std::string logReaderId = karabo::util::DATALOGREADER_PREFIX + m_server;

    const std::string str32Kb(32768ul, 'A');
    const std::string str8Kb(8192ul, 'B');

    const TimeValue millisecInAtto = 1'000'000'000'000'000ull; // Resolution of fractional seconds is AttoSec (10^-18).
    const unsigned int afterFlushWait = 1'500u;

    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          m_server, "DataLogTestDevice", Hash("deviceId", m_deviceId), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    // Starts the logger and readers with a lower max rate threshold - 32 kb/s - over a rateWinSecs seconds rating
    // window.
    success = startDataLoggerManager("InfluxDataLogger", false, false, 32, rateWinSecs);
    ASSERT_TRUE(success.first) << success.second;

    testAllInstantiated();
    waitUntilLogged(m_deviceId, "testInfluxMaxPerDevicePropLogRate");

    // Checks that writing 32Kb of data is within the log rate tolerance.
    Epochstamp before32KbWrite;
    for (unsigned int i = 0; i < 4 * rateWinSecs; i++) {
        Hash updateProp = Hash("stringProperty", str8Kb);
        Epochstamp updateEpoch(before32KbWrite + TimeDuration(0, (i + 1) * millisecInAtto));
        Timestamp updateTime(updateEpoch, TimeId());
        updateTime.toHashAttributes(updateProp.getAttributes("stringProperty"));
        ASSERT_NO_THROW(m_sigSlot->request(m_deviceId, "slotUpdateConfigGeneric", updateProp)
                              .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                              .receive());
    }
    // after32KbWrite is set to be the timestamp of the last write performed in the
    // previous loop plus a safety margin.
    Epochstamp after32KbWrite(before32KbWrite + TimeDuration(0, 5 * rateWinSecs * millisecInAtto));
    // Make sure that data has been written to Influx.
    ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", FLUSH_REQUEST_TIMEOUT_MILLIS / 1000));
    std::this_thread::sleep_for(milliseconds(afterFlushWait));

    // Checks that the 32Kb strings have not been flagged as bad data.
    Hash badDataAllDevices;
    ASSERT_NO_THROW(
          m_sigSlot
                ->request(logReaderId, "slotGetBadData", before32KbWrite.toIso8601Ext(), after32KbWrite.toIso8601Ext())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(badDataAllDevices));
    ASSERT_EQ(0ul, badDataAllDevices.size());
    // Checks that the 8Kb strings have been successfully logged.
    std::vector<Hash> history;
    Hash historyParams{
          "from", before32KbWrite.toIso8601Ext(), "to", after32KbWrite.toIso8601Ext(), "maxNumData", maxPropHistSize};
    std::string replyDevice;
    std::string replyProperty;
    ASSERT_NO_THROW(
          m_sigSlot->request(logReaderId, "slotGetPropertyHistory", m_deviceId, "stringProperty", historyParams)
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(replyDevice, replyProperty, history));
    ASSERT_EQ(4ul * rateWinSecs, history.size()) << "stringProperty history size different from expected.";
    for (unsigned int i = 0; i < 4 * rateWinSecs; i++) {
        std::string& historyStr8Kb = history[i].get<std::string>("v");
        ASSERT_EQ(8192ul, historyStr8Kb.size()) << "stringProperty value doesn't have expected size.";
        ASSERT_EQ(str8Kb.at(0ul), historyStr8Kb.at(0ul)) << "stringProperty value doesn't have expected characters.";
    }

    // Checks that updating a string property constantly above the rate will cause data to be rejected.
    // Use ratesWinSecs seconds after the time of the most recent write plus a safety margin of 4 milliseconds
    // as the starting time to guarantee that we have a complete independent rating window for the upcoming
    // burst.
    Epochstamp before64KbWrite(after32KbWrite + TimeDuration(rateWinSecs, 4 * millisecInAtto));
    for (unsigned int i = 0; i < 8 * rateWinSecs; i++) {
        Hash updateProps = Hash("stringProperty", str8Kb, "int32Property", 10);
        Epochstamp updateEpoch(before64KbWrite + TimeDuration(0, (i + 1) * millisecInAtto));
        Timestamp updateTime(updateEpoch, TimeId());
        updateTime.toHashAttributes(updateProps.getAttributes("stringProperty"));
        updateTime.toHashAttributes(updateProps.getAttributes("int32Property"));
        ASSERT_NO_THROW(m_sigSlot->request(m_deviceId, "slotUpdateConfigGeneric", updateProps)
                              .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                              .receive());
    }
    Epochstamp after64KbWrite(before64KbWrite + TimeDuration(0, 9 * rateWinSecs * millisecInAtto));
    // Make sure that data has been written to Influx.
    ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", FLUSH_REQUEST_TIMEOUT_MILLIS / 1000));
    std::this_thread::sleep_for(milliseconds(afterFlushWait));

    // Checks that the half of the stringProperty updates has exceeded the max log rate and has been rated as bad data.
    badDataAllDevices.clear();
    ASSERT_NO_THROW(
          m_sigSlot
                ->request(logReaderId, "slotGetBadData", before64KbWrite.toIso8601Ext(), after64KbWrite.toIso8601Ext())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(badDataAllDevices));
    ASSERT_EQ(1ul,
              badDataAllDevices.size()); // 1 is because the bad data is grouped under a single deviceId.
    ASSERT_EQ(4ul * rateWinSecs, badDataAllDevices.get<std::vector<karabo::data::Hash>>(m_deviceId).size());
    // Checks that half of the 8Kb strings written have been successfully set as property values.
    history.clear();
    historyParams.set<std::string>("from", before64KbWrite.toIso8601Ext());
    historyParams.set<std::string>("to", after64KbWrite.toIso8601Ext());
    historyParams.set<int>("maxNumData", maxPropHistSize);
    replyDevice.clear();
    replyProperty.clear();
    ASSERT_NO_THROW(
          m_sigSlot->request(logReaderId, "slotGetPropertyHistory", m_deviceId, "stringProperty", historyParams)
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(replyDevice, replyProperty, history));
    ASSERT_EQ(4ul * rateWinSecs, history.size()) << "stringProperty history size different from expected.";
    for (unsigned int i = 0; i < 4 * rateWinSecs; i++) {
        const std::string historyStr8Kb = history[i].get<std::string>("v");
        ASSERT_EQ(8192ul, historyStr8Kb.size()) << "stringProperty value doesn't have expected size.";
        ASSERT_EQ(str8Kb.at(0ul), historyStr8Kb.at(0ul)) << "stringProperty value doesn't have expected characters.";
    }

    // Checks that the int32Property updates were successfully logged even though the stringProperty was blocked.
    history.clear();
    historyParams.set<std::string>("from", before64KbWrite.toIso8601Ext());
    historyParams.set<std::string>("to", after64KbWrite.toIso8601Ext());
    historyParams.set<int>("maxNumData", maxPropHistSize);
    replyDevice.clear();
    replyProperty.clear();
    ASSERT_NO_THROW(
          m_sigSlot->request(logReaderId, "slotGetPropertyHistory", m_deviceId, "int32Property", historyParams)
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(replyDevice, replyProperty, history));
    ASSERT_EQ(8ul * rateWinSecs, history.size()) << "int32Property history size different from expected.";
    for (unsigned int i = 0; i < 8 * rateWinSecs; i++) {
        ASSERT_EQ(10, history[i].get<int>("v")) << "int32Property value differs from expected.";
    }

    // Updating a string property with a 32 Kb string should be accepted again after enough
    // time has passed since the previous max rate threshold reached condition.
    Epochstamp beforeSingle32KbWrite(after64KbWrite + TimeDuration(1 * rateWinSecs, 4 * millisecInAtto));
    Hash updateStr32Kb("stringProperty", str32Kb);
    Epochstamp updateEpoch(beforeSingle32KbWrite + TimeDuration(0, 6 * millisecInAtto));
    Timestamp updateTime(updateEpoch, TimeId());
    updateTime.toHashAttributes(updateStr32Kb.getAttributes("stringProperty"));
    ASSERT_NO_THROW(m_sigSlot->request(m_deviceId, "slotUpdateConfigGeneric", updateStr32Kb)
                          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                          .receive());
    Epochstamp afterSingle32KbWrite(beforeSingle32KbWrite + TimeDuration(0, 8 * millisecInAtto));
    // Make sure that data has been written to Influx.
    ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", FLUSH_REQUEST_TIMEOUT_MILLIS / 1000));
    std::this_thread::sleep_for(milliseconds(afterFlushWait));

    // Checks that the 32 Kb string has been successfully set as property values.
    history.clear();
    historyParams.set<std::string>("from", beforeSingle32KbWrite.toIso8601Ext());
    historyParams.set<std::string>("to", afterSingle32KbWrite.toIso8601Ext());
    historyParams.set<int>("maxNumData", maxPropHistSize);
    replyDevice.clear();
    replyProperty.clear();
    ASSERT_NO_THROW(
          m_sigSlot->request(logReaderId, "slotGetPropertyHistory", m_deviceId, "stringProperty", historyParams)
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(replyDevice, replyProperty, history));
    ASSERT_EQ(1ul, history.size()) << "stringProperty history size different from expected.";
    const std::string historySingleStr32kb = history[0].get<std::string>("v");
    ASSERT_EQ(32768ul, historySingleStr32kb.size()) << "stringProperty value doesn't have expected size.";
    ASSERT_EQ(str32Kb.at(0ul), historySingleStr32kb.at(0ul))
          << "stringProperty value doesn't have expected characters.";

    std::clog << "OK" << std::endl;
}


/**
 * @brief Checks that schemas older than the safeSchemaRetentionPeriod are copied.
 */
TEST_F(TestDataLogging, testInfluxSafeSchemaRetentionPeriod) {
    namespace nl = nlohmann;

    std::clog << "Testing that schemas older than safeSchemaRetentionPeriod are preserved ..." << std::endl;

    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const std::string logReaderId = karabo::util::DATALOGREADER_PREFIX + m_server;
    const std::string propTestDevice = m_deviceId + "__SCHEMA_RETENTION_PERIOD";

    const unsigned int afterFlushWait = 500u;
    const double halfSecInYears = 0.5 / (365 * 24 * 60 * 60);

    Epochstamp testStartEpoch;

    std::pair<bool, std::string> success = startDataLoggerManager(
          "InfluxDataLogger", /* useInvalidInfluxUrl */ false, /* useInvalidDbName */ false,
          /* maxPerDevicePropLogRate */ 5120u, 5u, /* maxSchemaLogRate */ 15'360u, /* schemaLogRatePeriod */ 5u,
          /* maxStringLength */ 921'600u, /* safeSchemaRetentionPeriod */ halfSecInYears);
    ASSERT_TRUE(success.first) << success.second;

    testAllInstantiated();

    success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", propTestDevice), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;
    waitUntilLogged(propTestDevice, "testInfluxSafeSchemaRetentionPeriod - 1");

    // Restart the PropertyTest device under test - this will trigger a new attempt to save the device schema with the
    // same digest, since no change happened to the schema between the two instantiations.
    success = m_deviceClient->killDevice(propTestDevice, KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;
    // Waits for an interval long enough to guarantee that any other schema saving attempt will happen after the
    // one saved for the previous PropertyTest device under test has gone outside the safe retention window.
    std::this_thread::sleep_for(1600ms);
    success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", propTestDevice), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;
    waitUntilLogged(propTestDevice, "testInfluxSafeSchemaRetentionPeriod - 2");

    // Makes sure all the data has been saved in Influx.
    ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());
    std::this_thread::sleep_for(milliseconds(afterFlushWait));

    Epochstamp afterWritesEpoch;

    // Checks that since of the start of this test, two schemas with the same digest have been inserted into the Influx
    // measurement - one for each start of the PropertyTest device under test.
    const InfluxDbClient::Pointer influxClient = buildInfluxReadClient();
    std::string firstDigest;
    std::string secondDigest;
    std::ostringstream oss;
    // Note: InfluxQL requires the returning of at least one field in the query results for the query to work.
    //       To comply with that, the query also asks for the schema_size, given that the digest is a tag, not a field.
    oss << "SELECT digest, schema_size FROM \"" << propTestDevice << "__SCHEMAS\" "
        << "WHERE time >= " << epochAsMicrosecString(testStartEpoch) << toInfluxDurationUnit(TIME_UNITS::MICROSEC)
        << " AND time <= " << epochAsMicrosecString(afterWritesEpoch) << toInfluxDurationUnit(TIME_UNITS::MICROSEC);
    waitForCondition(
          [&influxClient, &oss, &firstDigest, &secondDigest]() {
              auto prom = std::make_shared<std::promise<HttpResponse>>();
              std::future<HttpResponse> fut = prom->get_future();

              influxClient->queryDb(oss.str(), [prom](const HttpResponse& resp) { prom->set_value(resp); });
              std::future_status status = fut.wait_for(std::chrono::seconds(KRB_TEST_MAX_TIMEOUT));
              if (status != std::future_status::ready) {
                  return false;
              }
              HttpResponse resp = fut.get();
              if (resp.code != 200) {
                  std::clog << "ERROR querying for schemas:\nquery: " << oss.str() << "\nresponse: " << resp
                            << std::flush;
                  return false;
              }
              const std::string& respBody = resp.payload;
              nl::json respObj;
              try {
                  respObj = nl::json::parse(respBody);
              } catch (const std::exception& e) {
                  std::clog << "ERROR: Invalid JSON object in Influx response body:\n"
                            << respBody << "\n"
                            << std::flush;
                  return false;
              }
              const auto& schemas = respObj["results"][0]["series"][0]["values"];
              if (schemas.is_null()) {
                  return false;
              }
              if (schemas.size() == 2ul) {
                  firstDigest = schemas[0][1].get<std::string>();
                  secondDigest = schemas[1][1].get<std::string>();
                  return true;
              }
              return false;
          },
          20'000u, 500u); // seen timeout with 10,000 ms

    ASSERT_TRUE(!firstDigest.empty()) << "Didn't find the first expected schema";
    ASSERT_TRUE(!secondDigest.empty()) << "Didn't find the second expected schema";
    ASSERT_EQ(firstDigest, secondDigest) << "Schemas in Influx response don't have the same digest.";

    /* -- Sample of response body expected for the query for schemas
    {
      "results": [
        {
          "statement_id": 0,
          "series": [
            {
              "name": "PropertyTestDevice__SCHEMA_RETENTION_PERIOD__SCHEMAS",
              "columns": [
                "time",
                "digest",
                "schema_size"
              ],
              "values": [
                [
                  1694638751807275,
                  "\"29daf991ab26b3fe99a391397cb2fa1f5db5d99e\"",
                  68316
                ],
                [
                  1694638755032238,
                  "\"29daf991ab26b3fe99a391397cb2fa1f5db5d99e\"",
                  68316
                ]
              ]
            }
          ]
        }
      ]
    }
    --- */
    std::clog << "OK" << std::endl;
}


/**
 * Checks that the Influx logger and reader fail as soon as
 * possible when there's no Influx server available. Uses an
 * invalid url configuration for simulating the scenario of
 * the Influx server not being available.
 *
 * Note: During the test run in the CI machine, the docker
 * command is not available (the CI test is already executed
 * in a container), so the karabo-startinfluxdb and
 * karabo-stopinfluxdb commands cannot be run. That's the
 * reason behind the forced invalid configuration.
 */
TEST_F(TestDataLogging, testNoInfluxServerHandling) {
    std::clog << "Testing handling of no Influx Server available scenarios ..." << std::endl;

    // Temporarily set the logging level to FATAL to avoid spamming the logs of the device server (and the
    // test output) with connection errors.
    ASSERT_NO_THROW(m_deviceClient->execute(m_server, "slotLoggerLevel", KRB_TEST_MAX_TIMEOUT, "FATAL"))
          << "Error setting the logger level to 'FATAL'";

    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", m_deviceId), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    // Starts the logger and readers with invalid InfluxDB (or Telegraf) URLs.
    success = startDataLoggerManager("InfluxDataLogger", true);
    ASSERT_TRUE(success.first) << success.second;

    testAllInstantiated(false);

    // The DataLogger should be in ERROR state.
    karabo::data::State loggerState = karabo::data::State::UNKNOWN;
    std::string loggerStatus;
    const std::string& dataLoggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    waitForCondition(
          [this, &loggerState, &loggerStatus, &dataLoggerId]() {
              loggerState = m_deviceClient->get<karabo::data::State>(dataLoggerId, "state");
              loggerStatus = m_deviceClient->get<std::string>(dataLoggerId, "status");
              return (loggerState == karabo::data::State::ERROR);
          },
          KRB_TEST_MAX_TIMEOUT * 1000);

    ASSERT_TRUE(loggerState == State::ERROR)
          << "Timeout while waiting for DataLogger '" << dataLoggerId << "' to reach ERROR state.";

    std::clog << "... Influx logger in ERROR state, as expected, with status '" << loggerStatus << "'" << std::endl;

    // The LogReader should be still in ON state - it only goes to error after failing to connect to the Influx
    // instance.
    const std::string logReaderId = karabo::util::DATALOGREADER_PREFIX + m_server;
    karabo::data::State readerState = karabo::data::State::UNKNOWN;
    waitForCondition(
          [this, &readerState, &logReaderId]() {
              readerState = m_deviceClient->get<karabo::data::State>(logReaderId, "state");
              return (readerState == karabo::data::State::ON);
          },
          KRB_TEST_MAX_TIMEOUT * 1000);

    ASSERT_TRUE(readerState == State::ON)
          << "Timeout while waiting for LogReader '" << logReaderId << "' to reach ON state.";

    // Any attempt to recover a configuration from Influx should fail when the Influx Server is not
    // available.
    Epochstamp withNoServer;
    std::clog << "Requested config at '" << withNoServer.toIso8601() << "' with an invalid server url ... "
              << std::endl;

    Schema schema;
    Hash conf;
    bool cfgAtTime;
    std::string cfgTime;
    bool remoteExceptionCaught = false;
    try {
        m_sigSlot->request(logReaderId, "slotGetConfigurationFromPast", m_deviceId, withNoServer.toIso8601())
              .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
              .receive(conf, schema, cfgAtTime, cfgTime);
    } catch (const karabo::data::RemoteException& exc) {
        bool condition = (exc.detailedMsg().find("Could not connect to InfluxDb at") != std::string::npos) ||
                         (exc.detailedMsg().find("Reading from InfluxDB failed") != std::string::npos) ||
                         (exc.detailedMsg().find("Connection reset by peer"));
        ASSERT_TRUE(condition) << "Unexpected RemoteException while handling no Influx server:\n'" << exc.detailedMsg()
                               << "'\n";
        remoteExceptionCaught = true;
    }

    ASSERT_TRUE(remoteExceptionCaught);

    // At this point the LogReader will have tried to access Influx and failed. It should now be in ERROR.
    readerState = karabo::data::State::UNKNOWN;
    waitForCondition(
          [this, &readerState, &logReaderId]() {
              readerState = m_deviceClient->get<karabo::data::State>(logReaderId, "state");
              return (readerState == karabo::data::State::ERROR);
          },
          KRB_TEST_MAX_TIMEOUT * 1000);

    ASSERT_TRUE(readerState == State::ERROR)
          << "Timeout while waiting for LogReader '" << logReaderId << "' to reach ERROR state.";

    std::clog << "... request to retrieve past configuration failed with RemoteException as expected." << std::endl;

    // By simply starting the devices related to Influx logging, some logging writing activity takes place.
    // If this point of the test is reached with invalid urls configured for both reading and writing to the
    // Influx (or Telegraf) server, it is safe to conclude that the Influx Logger doesn't get compromissed by a
    // server not available condition - the host of the Influx logger is the same process that runs this test.

    // Restore the logger level of the device server that hosts the logger to the WARN level
    ASSERT_NO_THROW(
          m_deviceClient->execute(m_server, "slotLoggerLevel", KRB_TEST_MAX_TIMEOUT, DEFAULT_TEST_LOG_PRIORITY))
          << "Error setting the logger level level back to 'WARN'";

    std::clog << "OK" << std::endl;
}


/**
 * @brief Checks that the InfluxLogReader does averaging properly during the execution of slotPropertyHistory when
 * the number of data points available is larger than the maximum number of data points to be retrieved.
 *
 * This test is motivated by the bug fixed in https://git.xfel.eu/Karabo/Framework/-/merge_requests/6805.
 */
TEST_F(TestDataLogging, testInfluxPropHistoryAveraging) {
    std::clog << "Testing InfluxLogReader averaging when the requested Property History has too many points ..."
              << std::flush;

    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const std::string logReaderId = karabo::util::DATALOGREADER_PREFIX + m_server;
    const int maxPropHistorySize = 40;
    const int numWrites = maxPropHistorySize + 20;

    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", m_deviceId), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    success = startDataLoggerManager("InfluxDataLogger");
    ASSERT_TRUE(success.first) << success.second;

    testAllInstantiated();
    waitUntilLogged(m_deviceId, "testInfluxPropHistoryAveraging");

    Epochstamp beforePropWrites;
    for (size_t i = 0; i < numWrites; i++) {
        double propValue = i * 2.0;
        if (i % 9 == 0) {
            // Insert some NaNs values - that, along with the number of data points in the history being above the
            // maxNumData parameter, were the trigger for the bug fixed in
            // https://git.xfel.eu/Karabo/Framework/-/merge_requests/6805.
            propValue = std::numeric_limits<double>::signaling_NaN();
        }
        ASSERT_NO_THROW(m_deviceClient->set<double>(m_deviceId, "doubleProperty", propValue));
    }
    Epochstamp afterPropWrites;

    // Make sure that data has been written to Influx.
    ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", FLUSH_REQUEST_TIMEOUT_MILLIS / 1000));
    std::this_thread::sleep_for(1500ms);

    // Checks that slotGetPropertyHistory gets the averages consistently - the same number of data points and the same
    // values - when invoked multiple times with the same parameters. This test systematically fails if the fix
    // submitted in https://git.xfel.eu/Karabo/Framework/-/merge_requests/6805 is not present.
    std::vector<Hash> hist1, hist2;
    std::string replyDevice, replyProperty;
    Hash historyParams("from", beforePropWrites.toIso8601Ext(), "to", afterPropWrites.toIso8601Ext(), "maxNumData",
                       maxPropHistorySize);

    ASSERT_NO_THROW(
          m_sigSlot->request(logReaderId, "slotGetPropertyHistory", m_deviceId, "doubleProperty", historyParams)
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(replyDevice, replyProperty, hist1));

    ASSERT_NO_THROW(
          m_sigSlot->request(logReaderId, "slotGetPropertyHistory", m_deviceId, "doubleProperty", historyParams)
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(replyDevice, replyProperty, hist2));

    ASSERT_EQ(hist1.size(), hist2.size());
    for (size_t i = 0; i < hist1.size(); i++) {
        ASSERT_TRUE(hist1[i].fullyEquals(hist2[i])) << "History items at position " << i << " differ.";
    }

    std::clog << "OK" << std::endl;
}


void TestDataLogging::testFailingManager() {
    std::clog << "Testing logger manager goes to ERROR with inconsistent config ..." << std::flush;
    const std::string dataLogManagerId("loggerManager");
    std::pair<bool, std::string> success = m_deviceClient->killDevice(dataLogManagerId, KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    const Hash conf("deviceId", dataLogManagerId,
                    // Place list that is inconsistent with existing loggermap.xml (i.e. server in loggerMap is
                    // missing), this will be noticed by the logger and bring it to ERROR.
                    "serverList", std::vector<std::string>{"garbageServer"});

    success = m_deviceClient->instantiate(m_server, "DataLoggerManager", conf, KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    karabo::data::State loggerState = karabo::data::State::UNKNOWN;
    waitForCondition(
          [this, &loggerState, &dataLogManagerId]() {
              loggerState = m_deviceClient->get<karabo::data::State>(dataLogManagerId, "state");
              return loggerState == karabo::data::State::ERROR;
          },
          KRB_TEST_MAX_TIMEOUT * 1000);

    const std::string status = m_deviceClient->get<std::string>(dataLogManagerId, "status");
    ASSERT_EQ(State::ERROR, loggerState) << "Missed ERROR state - status: " << status;
    ASSERT_TRUE(status.find("Failure in initialize(), likely a restart is needed:") != std::string::npos) << status;
    ASSERT_TRUE(status.find("Inconsistent 'loggermap.xml' and 'serverList' configuration:") != std::string::npos)
          << status;
    ASSERT_TRUE(status.find("'DataLoggingTestServer' is in map, but not in list.") != std::string::npos) << status;
}
