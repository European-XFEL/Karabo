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
 * File:   BaseLogging_Test.cc
 *
 * Created on October 20, 2020, 3:30 PM
 */

#include "BaseLogging_Test.hh"
#define NO_VECTOR_HASH_ASSERTION_TRAITS
// Various CppUnit test helpers, but no special treatment for vector<Hash>.
// That needs even more special treatment below
#include "CppUnitMacroExtension.hh"
#undef NO_VECTOR_HASH_ASSERTION_TRAITS

#include <cstdlib>
#include <filesystem>
#include <karabo/net/EventLoop.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include <karabo/util/StringTools.hh>
#include <sstream>

#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/Version.hh"

USING_KARABO_NAMESPACES;
using karabo::util::Epochstamp;
using karabo::util::INT32_ELEMENT;
using karabo::util::OVERWRITE_ELEMENT;
using karabo::util::State;
using karabo::util::Timestamp;
using karabo::util::toString;
using karabo::xms::SLOT_ELEMENT;
using std::string;
using std::vector;

// Test timing parameters. Setup for Influx running in a non Telegfraf based
// environment. Can be overwritten by the Telegraf test suite.
int BaseLogging_Test::KRB_TEST_MAX_TIMEOUT = 10; // in seconds
int BaseLogging_Test::SLOT_REQUEST_TIMEOUT_MILLIS = 10000;
int BaseLogging_Test::FLUSH_REQUEST_TIMEOUT_MILLIS = 60000;
int BaseLogging_Test::PAUSE_BEFORE_RETRY_MILLIS = 300;
int BaseLogging_Test::NUM_RETRY = 400;
int BaseLogging_Test::FLUSH_INTERVAL_SEC = 1;
int BaseLogging_Test::WAIT_WRITES = 4000;
const char* BaseLogging_Test::DEFAULT_TEST_LOG_PRIORITY = "ERROR";

static Epochstamp threeDaysBack = Epochstamp() - TimeDuration(3, 0, 0, 0, 0);

const unsigned int maxVectorSize = 2000u; // smaller than default to test that setting it works.

class DataLogTestDevice : public karabo::core::Device<> {
   public:
    KARABO_CLASSINFO(DataLogTestDevice, "DataLogTestDevice", "integrationTests-" + karabo::util::Version::getVersion())

    static void expectedParameters(karabo::util::Schema& expected) {
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

    DataLogTestDevice(const karabo::util::Hash& input) : karabo::core::Device<>(input) {
        KARABO_SLOT(slotIncreaseValue);
        KARABO_SLOT(slotUpdateSchema, const karabo::util::Schema);
        // NOTE: this is a terrible idea. Never do this in the field.
        KARABO_SLOT(slotUpdateConfigGeneric, const karabo::util::Hash);
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
KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, DataLogTestDevice)


// A device with float and double properties without limits to be able to set inf and nan.
// Otherwise copy PropertyTest behaviour as needed for testNans().
class NanTestDevice : public karabo::core::Device<> {
   public:
    KARABO_CLASSINFO(NanTestDevice, "NanTestDevice", "integrationTests-" + karabo::util::Version::getVersion())

    static void expectedParameters(karabo::util::Schema& expected) {
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


    NanTestDevice(const karabo::util::Hash& input) : karabo::core::Device<>(input) {}


    void preReconfigure(Hash& incomingReconfiguration) {
        if (incomingReconfiguration.has("doubleProperty")) {
            set("doublePropertyReadOnly", incomingReconfiguration.get<double>("doubleProperty"));
        }
    }
};

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, NanTestDevice)


// Very special vector<Hash> treatment for CppUnit, ignoring differences in attributes.
// Add here what has been excluded from CppUnitMacroExtension.hh by '#define NO_VECTOR_HASH_ASSERTION_TRAITS' above.
// Note: Removing this code here would still compile, but then vector<Hash> comparison would fall back to the
//       vector<T> template in CppUnitMacroExtension.hh, i.e. using Hash::operator==(Hash) which unfortunately only
//       checks similarity (i.e. checks only for same paths).
namespace CppUnit {

    template <>
    struct assertion_traits<std::vector<karabo::util::Hash>> {
        static bool equal(const std::vector<karabo::util::Hash>& a, const std::vector<karabo::util::Hash>& b) {
            // using karabo::util::similar() here is not OK. the attributes of the hashes are dropped in one
            // of the serialization steps
            if (a.size() != b.size()) {
                return false;
            }
            for (size_t i = 0; i < a.size(); i++) {
                const karabo::util::Hash& a_i = a[i];
                const karabo::util::Hash& b_i = b[i];
                if (b_i.size() != a_i.size()) {
                    return false;
                }
                std::vector<std::string> paths;
                a_i.getPaths(paths);
                for (const std::string& path : paths) {
                    // most of the saving is serialized into text, this is why this helper
                    // checks the equality between values only passed as strings.
                    if (a_i.getAs<string>(path) != b_i.getAs<string>(path)) {
                        return false;
                    }
                }
            }
            return true;
        }


        static std::string toString(const std::vector<karabo::util::Hash>& p) {
            std::ostringstream o;
            o << "(" << std::endl;
            for (const karabo::util::Hash& e : p) {
                o << e << "," << std::endl;
            }
            o << ")";
            return o.str();
        }
    };
} // namespace CppUnit


BaseLogging_Test::BaseLogging_Test()
    : m_server("DataLoggingTestServer"),
      // Use platform-dependent name for the device: concurrent tests in CI operate
      // on the same InfluxDB database ...
      m_deviceId(getDeviceIdPrefix() + "PropertyTestDevice"),
      m_fileLoggerDirectory(getDeviceIdPrefix() + "dataLoggingTest"),
      m_changedPath(false),
      m_oldPath() {}


BaseLogging_Test::~BaseLogging_Test() {}


std::string BaseLogging_Test::getDeviceIdPrefix() {
    // Avoid test collision on CI by specifying a unique prefix.
    return !getenv("KARABO_BROKER_TOPIC") ? "" : getenv("KARABO_BROKER_TOPIC");
}


void BaseLogging_Test::setUp() {
    // There are indications for rare hanging between tests, see
    // https://git.xfel.eu/Karabo/Framework/-/jobs/101484 So debug print when this happens.
    const Epochstamp start;
    std::clog << "\nStart setUp " << start.toIso8601Ext() << std::endl;

    // Uncomment to run with a local broker:
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);

    // Start central event-loop
    auto work = []() {
        try {
            EventLoop::work();
        } catch (const karabo::util::TimeoutException& e) {
            // Looks like thread joining fails sometimes...
            std::clog << "Timeout from EventLoop::work(): " << e << std::endl;
        }
    };
    m_eventLoopThread = boost::thread(work);

    // Create and start server
    Hash config("serverId", m_server, "scanPlugins", false, "Logger.priority", DEFAULT_TEST_LOG_PRIORITY);
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
    std::clog << "End setUp " << stop.toIso8601Ext() << std::endl;
}


void BaseLogging_Test::tearDown() {
    // So debug print for in between tests, see setUp()
    const Epochstamp start;
    std::clog << "Start tearDown " << start.toIso8601Ext() << std::endl;

    m_sigSlot.reset();
    m_deviceClient.reset();
    m_deviceServer.reset();

    EventLoop::stop();
    if (m_eventLoopThread.joinable()) {
        m_eventLoopThread.join();
    }

    // Clean up directory - you may want to comment out these lines for debugging
    std::filesystem::remove("loggermap.xml");
    if (!m_keepLoggerDirectory) {
        std::filesystem::remove_all(m_fileLoggerDirectory);
    }

    if (m_changedPath) {
        if (m_oldPath.empty()) {
            unsetenv("PATH");
        } else {
            setenv("PATH", m_oldPath.data(), 1);
        }
    }

    // So debug print for in between tests, see setUp()
    const Epochstamp stop;
    std::clog << "End tearDown " << stop.toIso8601Ext() << std::endl;
}


bool BaseLogging_Test::waitForCondition(std::function<bool()> checker, unsigned int timeoutMillis,
                                        unsigned int sleepIntervalMillis) {
    unsigned int numOfWaits = 0;
    const unsigned int maxNumOfWaits = static_cast<unsigned int>(std::ceil(timeoutMillis / sleepIntervalMillis));
    while (numOfWaits < maxNumOfWaits && !checker()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepIntervalMillis));
        numOfWaits++;
    }
    return (numOfWaits < maxNumOfWaits);
}


void BaseLogging_Test::waitUntilLogged(const std::string& deviceId, const std::string& textForFailure) {
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

    CPPUNIT_ASSERT_MESSAGE(
          textForFailure + ": '" + deviceId + "' not logged, loggerCfg: " + toString(m_deviceClient->get(loggerId)),
          isLogged);
}


void BaseLogging_Test::setPropertyTestSchema() {
    std::vector<Hash> updates;
    updates.push_back(Hash("path", "floatProperty", "attribute", KARABO_SCHEMA_MIN_INC, "value",
                           -1.f * std::numeric_limits<float>::infinity()));
    updates.push_back(Hash("path", "floatProperty", "attribute", KARABO_SCHEMA_MAX_INC, "value",
                           std::numeric_limits<float>::infinity()));
    updates.push_back(Hash("path", "doubleProperty", "attribute", KARABO_SCHEMA_MIN_INC, "value",
                           -1. * std::numeric_limits<double>::infinity()));
    updates.push_back(Hash("path", "doubleProperty", "attribute", KARABO_SCHEMA_MAX_INC, "value",
                           std::numeric_limits<double>::infinity()));
    updates.push_back(Hash("path", "vectors.int8Property", "attribute", KARABO_SCHEMA_MIN_SIZE, "value", 0));
    updates.push_back(Hash("path", "vectors.uint8Property", "attribute", KARABO_SCHEMA_MIN_SIZE, "value", 0));
    updates.push_back(Hash("path", "vectors.stringProperty", "attribute", KARABO_SCHEMA_MIN_SIZE, "value", 0));
    updates.push_back(Hash("path", "vectors.boolProperty", "attribute", KARABO_SCHEMA_MIN_SIZE, "value", 0));
    updates.push_back(Hash("path", "vectors.int16Property", "attribute", KARABO_SCHEMA_MIN_SIZE, "value", 0));
    updates.push_back(Hash("path", "vectors.uint16Property", "attribute", KARABO_SCHEMA_MIN_SIZE, "value", 0));
    updates.push_back(Hash("path", "vectors.int32Property", "attribute", KARABO_SCHEMA_MIN_SIZE, "value", 0));
    updates.push_back(Hash("path", "vectors.uint32Property", "attribute", KARABO_SCHEMA_MIN_SIZE, "value", 0));
    updates.push_back(Hash("path", "vectors.int64Property", "attribute", KARABO_SCHEMA_MIN_SIZE, "value", 0));
    updates.push_back(Hash("path", "vectors.uint64Property", "attribute", KARABO_SCHEMA_MIN_SIZE, "value", 0));

    Hash response;
    m_sigSlot->request(m_deviceId, "slotUpdateSchemaAttributes", updates)
          .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
          .receive(response);
    CPPUNIT_ASSERT_MESSAGE("Could not update schema", response.get<bool>("success"));
}


std::pair<bool, std::string> BaseLogging_Test::startDataLoggerManager(
      const std::string& loggerType, bool useInvalidInfluxUrl, bool useInvalidDbName,
      unsigned int maxPerDevicePropLogRate, unsigned int propLogRatePeriod, unsigned int maxSchemaLogRate,
      unsigned int schemaLogRatePeriod, unsigned int maxStringLength, double safeSchemaRetentionPeriod) {
    Hash manager_conf;
    manager_conf.set("deviceId", "loggerManager");
    manager_conf.set("flushInterval", FLUSH_INTERVAL_SEC);
    manager_conf.set<vector<string>>("serverList", {m_server});
    manager_conf.set("logger", loggerType);

    if (loggerType == "FileDataLogger") {
        manager_conf.set("fileDataLogger.directory",
                         (m_fileLoggerDirectory.empty() ? "" : m_fileLoggerDirectory + "/") + "karaboHistory");
    } else if (loggerType == "InfluxDataLogger") {
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
        CPPUNIT_FAIL("Unknown logger type '" + loggerType + "'");
    }

    std::clog << "\n***** DataLoggerManager configuration *****\n" << manager_conf << std::endl;
    return m_deviceClient->instantiate(m_server, "DataLoggerManager", manager_conf, KRB_TEST_MAX_TIMEOUT);
}


void BaseLogging_Test::testMaxNumDataRange() {
    std::clog << "Check if InfluxLogReader is validating range for 'maxNumData' for slot 'getPropertyHistory' ... ";

    const std::string dlReader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
    waitUntilLogged(dlReader0, "testMaxNumDataRange");
    const std::string outOfRangeErrMsg1("Requested maximum number of data points ('maxNumData') is");
    const std::string outOfRangeErrMsg2("which surpasses the limit of");

    const int readerMaxHistSize = m_deviceClient->get<int>(dlReader0, "maxHistorySize");

    Epochstamp refEpoch;

    Hash params;
    params.set<string>("from", (refEpoch - TimeDuration(30, 0)).toIso8601());
    params.set<string>("to", refEpoch.toIso8601());
    params.set<int>("maxNumData", readerMaxHistSize + 1);

    vector<Hash> history;
    std::string replyDevice, replyProperty;

    // Values past InfluxLogReader::maxHistorySize must be rejected.
    try {
        m_sigSlot->request(dlReader0, "slotGetPropertyHistory", dlReader0, "url", params)
              .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
              .receive(replyDevice, replyProperty, history);
        throw KARABO_LOGIC_EXCEPTION("Wrong arguments to slotGetPropertyHistory did not let it throw");
    } catch (karabo::util::RemoteException& e) {
        const std::string& errMsg = e.userFriendlyMsg(true);
        CPPUNIT_ASSERT_MESSAGE(errMsg, errMsg.find(outOfRangeErrMsg1) != std::string::npos);
        CPPUNIT_ASSERT_MESSAGE(errMsg, errMsg.find(outOfRangeErrMsg2) != std::string::npos);
    } catch (const std::exception& e) {
        CPPUNIT_ASSERT_MESSAGE(std::string("Unexpected exception: ") += e.what(), false);
    }

    // Negative values must be rejected.
    params.set<int>("maxNumData", -1);
    try {
        m_sigSlot->request(dlReader0, "slotGetPropertyHistory", dlReader0, "url", params)
              .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
              .receive(replyDevice, replyProperty, history);
        throw KARABO_LOGIC_EXCEPTION("Wrong arguments to slotGetPropertyHistory did not let it throw");
    } catch (karabo::util::RemoteException& e) {
        const std::string& errMsg = e.userFriendlyMsg(true);
        CPPUNIT_ASSERT(errMsg.find(outOfRangeErrMsg1) != std::string::npos);
        CPPUNIT_ASSERT(errMsg.find(outOfRangeErrMsg2) != std::string::npos);
    } catch (const std::exception& e) {
        CPPUNIT_ASSERT_MESSAGE(std::string("Unexpected exception: ") += e.what(), false);
    }

    // 0 must be accepted - it as if InfluxLogReader::maxHistorySize has been used.
    params.set<int>("maxNumData", 0);
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(dlReader0, "slotGetPropertyHistory", dlReader0, "url", params)
                                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                  .receive(replyDevice, replyProperty, history));

    // InfluxLogReader::maxHistorySize must be accepted.
    params.set<int>("maxNumData", readerMaxHistSize);
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(dlReader0, "slotGetPropertyHistory", dlReader0, "url", params)
                                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                  .receive(replyDevice, replyProperty, history));

    std::clog << "OK" << std::endl;
}


void BaseLogging_Test::testMaxNumDataHistory() {
    std::clog << "Check if InfluxLogReader is properly enforcing the 'maxNumData' parameter for slot "
                 "'getPropertyHistory' ..."
              << std::endl;

    // Instantiates a DataLogTestDevice for performing the check.
    const std::string deviceId(getDeviceIdPrefix() + "MaxNumDataHistory");
    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const auto res =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE("Failed to instantiate testing device '" + deviceId + "':" + res.second, res.first);

    waitUntilLogged(deviceId, "testMaxNumDataHistory");

    // Writing sequence - write a sequence of increasing values.
    Epochstamp beforeWrites;
    const int numOfWrites = 40;
    for (int i = 0; i < numOfWrites; i++) {
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set<int>(deviceId, "int32Property", i));
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
    }
    // Captures the timepoint after the writes.
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
    Epochstamp afterWrites;

    // Makes sure all the writes are done before retrieval.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());

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
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);

    // History retrieval may take more than one attempt.
    auto historyChecker = [this, maxNumDataFull, &deviceId, &dlreader0, &params, &replyDevice, &replyProperty,
                           &history]() {
        try {
            m_sigSlot->request(dlreader0, "slotGetPropertyHistory", deviceId, "int32Property", params)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(replyDevice, replyProperty, history);
            return history.size() == static_cast<std::size_t>(maxNumDataFull);
        } catch (const karabo::util::TimeoutException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
            return false;
        } catch (const karabo::util::RemoteException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
            return false;
        }
    };

    waitForCondition(historyChecker, 90'000u, 1'000u);

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Size for full history different from expected.", static_cast<size_t>(maxNumDataFull),
                                 history.size());
    for (int i = 0; i < maxNumDataFull; i++) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE(
              "Value at history entry #" + karabo::util::toString(i) + " different from expected.", i,
              history[i].get<int>("v"));
    }

    // Sampled history retrieval - must retrieve all values sampled.
    // The specified 'maxNumData' is half the size of the full history size.
    const int maxNumDataSampled = numOfWrites / 2;
    params.set<int>("maxNumData", maxNumDataSampled);

    history.clear();

    // Sample history retrieval may take more than one attempt.
    auto sampleHistoryChecker = [this, maxNumDataSampled, &deviceId, &dlreader0, &params, &replyDevice, &replyProperty,
                                 &history]() {
        try {
            m_sigSlot->request(dlreader0, "slotGetPropertyHistory", deviceId, "int32Property", params)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(replyDevice, replyProperty, history);
            return history.size() >= (maxNumDataSampled / 2ul);
        } catch (const karabo::util::TimeoutException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
            return false;
        } catch (const karabo::util::RemoteException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
            return false;
        }
    };

    bool succeeded = waitForCondition(sampleHistoryChecker, 96'000u, 1'000u);

    size_t historySize = history.size();
    CPPUNIT_ASSERT_MESSAGE("Timeout on requesting history", succeeded);
    CPPUNIT_ASSERT_MESSAGE("Size of the down-sampled history larger than request sample",
                           historySize <= static_cast<size_t>(maxNumDataSampled));
    CPPUNIT_ASSERT_MESSAGE(
          "Size of the down-sampled history smaller than minimum expected (at least half in this case)",
          historySize >= (maxNumDataSampled / 2ul));
    // Makes sure that the maxNumDataSampled values retrieved are distributed across the
    // whole set of maxNumDataFull values. A deviation margin is tolerated to accomodate
    // different timings involved in the writing sequence phase.
    const int deviationMargin = 8;
    Hash& lastHistoryEntry = history[historySize - 1];
    CPPUNIT_ASSERT_MESSAGE("Value at history entry #0 is outside the expected range: should be between 0 and " +
                                 karabo::util::toString(deviationMargin) + ", got " +
                                 karabo::util::toString(history[0].get<int>("v")) + ".",
                           history[0].get<int>("v") >= 0 && history[0].get<int>("v") <= deviationMargin);
    CPPUNIT_ASSERT_MESSAGE(
          "Value at history entry #" + karabo::util::toString(lastHistoryEntry) +
                " is outside the expected range: should be between " + karabo::util::toString(40 - deviationMargin) +
                " and 40, got " + karabo::util::toString(lastHistoryEntry.get<int>("v")) + ".",
          lastHistoryEntry.get<int>("v") >= 40 - deviationMargin && lastHistoryEntry.get<int>("v") <= 40);
    std::clog << "... OK" << std::endl;
}

void BaseLogging_Test::testDropBadData() {
    std::clog << "Testing that the logger drops bad data ... " << std::flush;

    const std::string deviceId(getDeviceIdPrefix() + "deviceWithFutureStamp");
    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    auto success =
          m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    waitUntilLogged(deviceId, "testDropBadData");

    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
    unsigned int numCycles = 5;
    Epochstamp before;
    const Epochstamp inAlmostAFortnite = before + TimeDuration(13, 0, 0, 0, 0);
    const Epochstamp inAFortnite = inAlmostAFortnite + TimeDuration(1, 0, 0, 0, 0);

    // Getting original timestamp of 'value' to compare with later
    // We first change the property once - otherwise, what we get back from influx is the start of logging time
    // and not the timestamp on device
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(deviceId, "slotIncreaseValue", KRB_TEST_MAX_TIMEOUT));
    Hash cfg;
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(deviceId, cfg));
    CPPUNIT_ASSERT_MESSAGE("'value' is missing from the configuration", cfg.has("value"));
    const Epochstamp originalEpoch = Epochstamp::fromHashAttributes(cfg.getAttributes("value"));
    const int originalValue = cfg.get<int>("value");

    for (unsigned int i = 0; i < numCycles; ++i) {
        Hash update("value", 10000 + i);
        const Timestamp muchLater(inAlmostAFortnite, Trainstamp());
        muchLater.toHashAttributes(update.getAttributes("value"));
        // Call slotUpdateConfigGeneric from m_deviceClient so that m_deviceClient->get is in sync for sure
        CPPUNIT_ASSERT_NO_THROW(
              m_deviceClient->execute(deviceId, "slotUpdateConfigGeneric", KRB_TEST_MAX_TIMEOUT, update));
        // Get configuration, check expected values, check (static) time stamp of "oldValue" and store stamp of
        // "value"
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(deviceId, cfg));
        CPPUNIT_ASSERT_MESSAGE("'value' is missing from the configuration", cfg.has("value"));
        CPPUNIT_ASSERT_MESSAGE("'vector' is missing from the configuration", cfg.has("vector"));
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(i) + 10000, cfg.get<int>("value"));
        CPPUNIT_ASSERT_EQUAL(std::vector<int>(), cfg.get<vector<int>>("vector"));
        const Epochstamp stamp = Epochstamp::fromHashAttributes(cfg.getAttributes("value"));

        CPPUNIT_ASSERT_MESSAGE(
              "'value' has wrong time stamp: " + stamp.toIso8601() + " instead of " + inAlmostAFortnite.toIso8601(),
              stamp == inAlmostAFortnite);

        // Flush Data
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());

        // Get config back - but it may take a while till data is store, so try a few times
        // (some CI failed with fixed 250 ms of sleep)
        Schema schema;
        bool configAtTimepoint;
        std::string configTimepoint;
        cfg.clear();
        int maxTime = 2000;
        do {
            boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
            maxTime -= 50;
            CPPUNIT_ASSERT_NO_THROW(
                  m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", deviceId, inAFortnite.toIso8601())
                        .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                        .receive(cfg, schema, configAtTimepoint, configTimepoint));
        } while (!cfg.has("value") && maxTime >= 0);
        CPPUNIT_ASSERT_MESSAGE("'value' is missing from configuration: " + toString(cfg), cfg.has("value"));
        const Epochstamp received = Epochstamp::fromHashAttributes(cfg.getAttributes("value"));
        // the data is stored in the influxDB and has lower resolution (microsecond) what Epochstamp offers
        // (attosecond). We therefore compare the time difference. TimeDuration will always be positive (sic).
        const double dt = received - originalEpoch;
        CPPUNIT_ASSERT_MESSAGE(
              "'value' has wrong time stamp: " + received.toIso8601() + " - difference is : " + toString(dt),
              dt < 1e-6);
        CPPUNIT_ASSERT_EQUAL(cfg.get<int>("value"), originalValue);
    }

    // Now check that we can get back info about bad data - but first add other bad data items,
    // first a too long vector and then a mixture of too long vector, far future, and decent data
    const unsigned int vectorSize = maxVectorSize + 1u;
    Hash updates("vector", std::vector<int>(vectorSize, 42)); // one longer than logger tolerates
    Timestamp vectorUpdateTime1;
    vectorUpdateTime1.toHashAttributes(updates.getAttributes("vector")); // Add stamp to test full cycle
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotUpdateConfigGeneric", updates)
                                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                  .receive());
    // second (i.e. mixed) update
    Timestamp vectorUpdateTime2;
    vectorUpdateTime2.toHashAttributes(updates.getAttributes("vector")); // Overwrite with new stamp
    Hash::Node& valueNode = updates.set("value", 42);
    Timestamp(inAlmostAFortnite, Trainstamp()).toHashAttributes(valueNode.getAttributes());
    updates.set("oldValue", -42); // no timestamp from here, will get injected automatically in device
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotUpdateConfigGeneric", updates)
                                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                  .receive());

    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());

    // Get back bad data
    // vectorUpdateTime2 is to early, future data gets timestamp after it, using inAFortnite might create
    // interference between different test runs, so create a new stamp:
    const Epochstamp whenFlushed;
    Hash badDataAllDevices;
    int maxTime = 3'000; // 2'000 still failed on a loaded CI
    while (maxTime >= 0) {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
        maxTime -= 100;
        CPPUNIT_ASSERT_NO_THROW(
              m_sigSlot
                    ->request(dlreader0, "slotGetBadData", before.toIso8601Ext(), whenFlushed.toIso8601Ext())
                    //(inAFortnite + TimeDuration(1, 0, 0, 0, 0)).toIso8601Ext())
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                    .receive(badDataAllDevices));
        if (badDataAllDevices.has(deviceId) &&
            badDataAllDevices.get<std::vector<Hash>>(deviceId).size() == numCycles + 3ul) {
            break;
        }
    }

    CPPUNIT_ASSERT_EQUAL(1ul, badDataAllDevices.size()); // Just our test device is a bad guy...
    CPPUNIT_ASSERT(badDataAllDevices.has(deviceId));
    const std::vector<Hash>& badData = badDataAllDevices.get<std::vector<Hash>>(deviceId);
    // numCycles plus 3: 1st vector and then "2nd vector and future value" split into two due to different
    // timestamps
    CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(badDataAllDevices), numCycles + 3ul, badData.size());

    // Test the bad data from far future
    Epochstamp last = before;
    for (size_t i = 0; i < numCycles; ++i) {
        const Hash& futureHash = badData[i];
        CPPUNIT_ASSERT(futureHash.has("info"));
        const std::string& info = futureHash.get<std::string>("info");
        CPPUNIT_ASSERT_MESSAGE(info, info.find("log metric(s) for device '" + deviceId + "'") != std::string::npos);
        CPPUNIT_ASSERT_MESSAGE(info, info.find("'value' (from far future " + inAlmostAFortnite.toIso8601Ext() += ")") !=
                                           std::string::npos);
        CPPUNIT_ASSERT(futureHash.has("time"));
        const std::string& timeStr = futureHash.get<std::string>("time");
        const Epochstamp timeEpoch = Epochstamp::fromHashAttributes(futureHash.getAttributes("time"));
        CPPUNIT_ASSERT_EQUAL(timeStr, timeEpoch.toIso8601Ext());
        // Timestamp is defined inside logger (since not 'believing' the far future one),
        // so we cannot be exactly sure for when this is logged (but in order)
        CPPUNIT_ASSERT(last < timeEpoch);
        CPPUNIT_ASSERT(Epochstamp() > timeEpoch);
        last = timeEpoch;
    }
    // Test the bad data from 1st too long vector
    const Hash& vectorHash = badData[badData.size() - 3];
    CPPUNIT_ASSERT(vectorHash.has("info"));
    const std::string& info = vectorHash.get<std::string>("info");
    CPPUNIT_ASSERT_MESSAGE(info, info.find("log metric(s) for device '" + deviceId + "'") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE(info,
                           info.find("'vector' (vector of size " + toString(vectorSize) += ")") != std::string::npos);
    CPPUNIT_ASSERT(vectorHash.has("time"));
    const std::string& timeStr = vectorHash.get<std::string>("time");
    const Epochstamp timeEpoch = Epochstamp::fromHashAttributes(vectorHash.getAttributes("time"));
    CPPUNIT_ASSERT_EQUAL(timeStr, timeEpoch.toIso8601Ext());
    const double dt2 = timeEpoch - vectorUpdateTime1.getEpochstamp();
    CPPUNIT_ASSERT_LESSEQUAL(1.e-6, dt2); // we store only microsecond precision in DB

    // Test the bad data from mixture of too long vector, future data and OK data
    // First of these comes vector
    const Hash& mixtureHash1 = badData[badData.size() - 2];
    CPPUNIT_ASSERT(mixtureHash1.has("info"));
    const std::string& info2 = mixtureHash1.get<std::string>("info");
    CPPUNIT_ASSERT_MESSAGE(info2, info2.find("log metric(s) for device '" + deviceId + "'") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE(info2,
                           info2.find("'vector' (vector of size " + toString(vectorSize) += ")") != std::string::npos);
    CPPUNIT_ASSERT(mixtureHash1.has("time"));
    const std::string& timeStr2 = mixtureHash1.get<std::string>("time");
    const Epochstamp timeEpoch2 = Epochstamp::fromHashAttributes(mixtureHash1.getAttributes("time"));
    CPPUNIT_ASSERT_EQUAL(timeStr2, timeEpoch2.toIso8601Ext());
    const double dt3 = timeEpoch2 - vectorUpdateTime2.getEpochstamp();
    CPPUNIT_ASSERT_LESSEQUAL(1.e-6, dt3); // only microsecond precision in DB

    // Finally future data from mixture
    const Hash& mixtureHash2 = badData[badData.size() - 1];
    CPPUNIT_ASSERT(mixtureHash2.has("info"));
    const std::string& info3 = mixtureHash2.get<std::string>("info");
    CPPUNIT_ASSERT_MESSAGE(info3, info3.find("log metric(s) for device '" + deviceId + "'") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE(info3, info3.find("'value' (from far future " + inAlmostAFortnite.toIso8601Ext() += ")") !=
                                        std::string::npos);
    CPPUNIT_ASSERT(mixtureHash2.has("time"));
    const std::string& timeStr3 = mixtureHash2.get<std::string>("time");
    const Epochstamp timeEpoch3 = Epochstamp::fromHashAttributes(mixtureHash2.getAttributes("time"));
    CPPUNIT_ASSERT_EQUAL(timeStr3, timeEpoch3.toIso8601Ext());
    // Timestamp is defined in logger since future stamp seen as unreliable:
    // between "before setting the data mixture" and "now"
    CPPUNIT_ASSERT(vectorUpdateTime2.getEpochstamp() < timeEpoch3);
    CPPUNIT_ASSERT(Epochstamp() > timeEpoch3);

    // Check that the "decent" data from same "signalChanged" as long vector and future value is properly logged
    bool configAtTimepoint = false;
    cfg.clear();
    Schema dummySchema;
    std::string dummyConfigTimepoint;
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", deviceId, Timestamp().toIso8601())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(cfg, dummySchema, configAtTimepoint, dummyConfigTimepoint));
    CPPUNIT_ASSERT(configAtTimepoint);
    CPPUNIT_ASSERT(cfg.has("oldValue"));
    CPPUNIT_ASSERT_EQUAL(-42, cfg.get<int>("oldValue"));
    const Epochstamp oldValueStamp(Epochstamp::fromHashAttributes(cfg.getAttributes("oldValue")));
    // This timestamp is defined by device when setting it,
    // but again this is between "before setting the data mixture" and "now"
    CPPUNIT_ASSERT(vectorUpdateTime2.getEpochstamp() < oldValueStamp);
    CPPUNIT_ASSERT(Epochstamp() > oldValueStamp);

    // Now test that slotGetBadData correctly returns nothing for a decent period (here: future)
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot
                                  ->request(dlreader0, "slotGetBadData", inAFortnite.toIso8601Ext(),
                                            (inAFortnite + TimeDuration(1, 0, 0, 0, 0)).toIso8601Ext())
                                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                  .receive(badDataAllDevices));
    CPPUNIT_ASSERT_MESSAGE(toString(badDataAllDevices), badDataAllDevices.empty());

    std::clog << "... OK" << std::endl;
}

void BaseLogging_Test::testAllInstantiated(bool waitForLoggerReady) {
    std::clog << "Testing logger and readers instantiations ... " << std::flush;

    const vector<string> devices({karabo::util::DATALOGGER_PREFIX + m_server,
                                  karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server),
                                  karabo::util::DATALOGREADER_PREFIX + ("1-" + m_server)});

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
    CPPUNIT_ASSERT_MESSAGE(
          "Timeout looking for logger and readers instances. Up are only " + toString(m_deviceClient->getDevices()),
          succeeded);

    if (waitForLoggerReady) {
        // Makes sure that the DataLogger has reached ON state before proceeding.
        // Any call to the Flush slot while the DataLogger is in a different state will trigger an exception.
        // For the Influx Logger case, this initialization time can be quite long - if the db does not exist
        // yet, the DataLogger must create it before reaching the ON state.

        karabo::util::State loggerState = karabo::util::State::UNKNOWN;
        const std::string& dataLoggerId = karabo::util::DATALOGGER_PREFIX + m_server;

        waitForCondition(
              [this, &loggerState, &dataLoggerId]() {
                  loggerState = m_deviceClient->get<karabo::util::State>(dataLoggerId, "state");
                  return loggerState == karabo::util::State::ON;
              },
              60 * KRB_TEST_MAX_TIMEOUT * 1000u, 100u);

        CPPUNIT_ASSERT_EQUAL_MESSAGE("Timeout while waiting for DataLogger '" + dataLoggerId + "' to reach ON state.",
                                     karabo::util::State::ON, loggerState);
    }

    std::clog << "Ok" << std::endl;
}


void BaseLogging_Test::testLastKnownConfiguration(karabo::util::Epochstamp fileMigratedDataEndsBefore,
                                                  bool dataWasMigrated) {
    // Timestamp for test start - used to print test duration at the end.
    Epochstamp testCaseStart;

    // Last value set in previous test cases for property 'int32Property'.
    const int kLastValueSet = 99;

    Epochstamp beforeAnything(0, 0);

    std::clog << "Testing last known configuration at specific timepoints ..." << std::endl;

    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);

    Schema schema;
    Hash conf;
    bool configAtTimepoint;
    std::string configTimepoint;

    std::clog << "... before any logging activity (at " << beforeAnything.toIso8601() << ") ...";
    // At the beforeAnything timepoint no known configuration existed, so an exception is expected.
    bool remoteExcept = false;
    try {
        m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", m_deviceId, beforeAnything.toIso8601())
              .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
              .receive(conf, schema, configAtTimepoint, configTimepoint);
    } catch (const RemoteException& re) {
        const std::string fileLoggerMsg(
              "Requested time point for device configuration is earlier than anything logged");
        const std::string influxLoggerMsg("No active schema could be found for device at (or before) timepoint.");
        CPPUNIT_ASSERT_MESSAGE("Exception message: " + re.detailedMsg(),
                               (re.detailedMsg().find(fileLoggerMsg) != string::npos ||
                                re.detailedMsg().find(influxLoggerMsg) != string::npos));
        remoteExcept = true;
    } catch (const std::exception& e) {
        CPPUNIT_ASSERT_MESSAGE(string("Unexpected exception: ") += e.what(), false);
    }
    CPPUNIT_ASSERT_MESSAGE("Expected exception, received " + toString(conf), remoteExcept);

    std::clog << "\n... Ok (no configuration retrieved)." << std::endl;

    karabo::util::Epochstamp rightBeforeDeviceGone;
    std::clog << "... right before killing device being logged (at " << rightBeforeDeviceGone.toIso8601() << ") ...";
    // At the rightBeforeDeviceGone timepoint, a last known configuration should be obtained with the last value set
    // in the  previous test cases for the 'int32Property' - even after the device being logged is gone.
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", m_deviceId, rightBeforeDeviceGone.toIso8601())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(conf, schema, configAtTimepoint, configTimepoint));

    CPPUNIT_ASSERT_EQUAL(99, conf.get<int>("int32Property"));

    CPPUNIT_ASSERT_EQUAL(std::string("with\nnewline99"), conf.get<std::string>("stringProperty"));

    CPPUNIT_ASSERT_EQUAL(std::vector<std::string>({"abc99", "xy|z99", "A\nB99"}),
                         conf.get<std::vector<std::string>>("vectors.stringProperty"));
    CPPUNIT_ASSERT_EQUAL(
          std::vector<Hash>({Hash("e1", "ab\nc99", "e2", false, "e3", 12 * 99, "e4", 0.9837F * 99, "e5", 1.2345 * 99),
                             Hash("e1", "xy|z99", "e2", true, "e3", 42 * 99, "e4", 2.33333F * 99, "e5", 7.77777 * 99)}),
          conf.get<std::vector<Hash>>("table"));
    std::clog << "\n... Ok (retrieved configuration with last known value for 'int32Property', 'stringProperty', "
              << "'vectors.stringProperty', and 'table')." << std::endl;

    CPPUNIT_ASSERT_EQUAL(kLastValueSet, conf.get<int>("int32Property"));
    CPPUNIT_ASSERT_EQUAL(true, configAtTimepoint);

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
    CPPUNIT_ASSERT_EQUAL(latestTimestamp.toIso8601Ext(), configTimepoint);
    std::clog << "\n... "
              << "Ok (retrieved configuration with last known value for 'int32Property' while the device was being "
                 "logged)."
              << std::endl;

    // killDevice waits for the device to be killed (or throws an exception in case of failure).
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->killDevice(m_deviceId, KRB_TEST_MAX_TIMEOUT));

    // Assures that the logger in charge of the device is not logging it anymore by testing that m_deviceId is not
    // among the rows of the "lastUpdatesUtc" property of the logger. The "flush" slot guarantees that the property
    // "lastUpdatesUtc" is in sync with devices being logged.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
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
    CPPUNIT_ASSERT_EQUAL(false, deviceIdFound);

    // There is an interval between the device being killed and the event that it is gone reaching the logger.
    // But we need to be sure that the timepoint used in the request for configuration from past is after the
    // timestamp associated to the device shutdown event.
    // In rare CI cases this sleep seems not to be enough, therefore the loop below that even postpones the
    // requested timepoint.
    boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));

    Epochstamp afterDeviceGone;
    std::clog << "... after device being logged is gone (requested config at " << afterDeviceGone.toIso8601()
              << " or later) ...";
    int nTries = NUM_RETRY;
    unsigned int numChecks = 0;
    conf.clear();
    configAtTimepoint = true;
    while ((!conf.has("int32Property") || kLastValueSet != conf.get<int>("int32Property") || configAtTimepoint) &&
           nTries-- > 0) {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
        afterDeviceGone.now();

        // At the afterDeviceGone timepoint, a last known configuration should be obtained with the last value set
        // in the previous test cases for the 'int32Property' - even after the device being logged is gone.
        CPPUNIT_ASSERT_NO_THROW(
              m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", m_deviceId, afterDeviceGone.toIso8601())
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                    .receive(conf, schema, configAtTimepoint, configTimepoint));
        ++numChecks;
    }
    const std::string msg("Failed after " + toString(numChecks) + " attempts\nconf: " + toString(conf) +=
                          "\nconfigAtTimePoint: " + toString(configAtTimepoint) +=
                          "\nconfigTimepoint: " + configTimepoint);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, kLastValueSet, conf.get<int>("int32Property"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, false, configAtTimepoint);
    karabo::util::Epochstamp configStamp(configTimepoint);
    // if data migration happened the data is younger than the file based logging data
    CPPUNIT_ASSERT_MESSAGE(msg, configStamp > (dataWasMigrated ? fileMigratedDataEndsBefore : beforeAnything));
    CPPUNIT_ASSERT_MESSAGE(msg, configStamp < afterDeviceGone);
    std::clog << "\n... " << "Timestamp of retrieved configuration: " << configTimepoint << "\n "
              << "Ok (retrieved configuration with last known value for 'int32Property' while the device was not being "
                 "logged)."
              << std::endl;


    if (dataWasMigrated) {
        // check for the migrated data
        std::clog << "\n... from migrated data (requested config at " << fileMigratedDataEndsBefore.toIso8601()
                  << ") ...";
        // At the afterDeviceGone timepoint, a last known configuration should be obtained with the last value set
        // in the previous test cases for the 'int32Property' - even after the device being logged is gone.
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot
                                      ->request(dlreader0, "slotGetConfigurationFromPast", m_deviceId,
                                                fileMigratedDataEndsBefore.toIso8601())
                                      .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                      .receive(conf, schema, configAtTimepoint, configTimepoint));

        CPPUNIT_ASSERT_EQUAL(kLastValueSet, conf.get<int>("int32Property"));
        CPPUNIT_ASSERT_EQUAL(false, configAtTimepoint);
        karabo::util::Epochstamp configStamp(configTimepoint);
        CPPUNIT_ASSERT(configStamp > beforeAnything);
        // if migration failed or was incompatible we would not get a timestamp matching this condition
        // there wouldn't be any data in the DB before this.
        CPPUNIT_ASSERT(configStamp < fileMigratedDataEndsBefore);
        std::clog << "\n... " << "Timestamp of retrieved configuration: " << configTimepoint << "\n "
                  << "Ok (retrieved configuration with last known value for 'int32Property' from file logger migrated "
                     "data)."
                  << std::endl;
    }

    const karabo::util::TimeDuration testDuration = testCaseStart.elapsed();

    const std::streamsize currPrecision{std::clog.precision()};
    std::clog << "(testLastKnownConfiguration took " << std::setprecision(4) << static_cast<double>(testDuration)
              << " sec. to execute)" << std::setprecision(currPrecision) << std::endl;

    std::clog << ("OK") << std::endl;
}


void BaseLogging_Test::testCfgFromPastRestart(bool pastConfigStaysPast) {
    std::clog << "Testing past configuration retrieval with stamp older than device..." << std::endl;

    Epochstamp testCaseStart;

    // Start device and take care that the logger is ready for it
    // Use platform-dependent name for the device: concurrent tests in CI operate
    // on the same InfluxDB database ...
    const std::string deviceId(getDeviceIdPrefix() + "deviceWithOldStamp");
    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    auto success =
          m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    waitUntilLogged(deviceId, "testCfgFromPastRestart");

    // few cycles: increase value, stop and restart logging
    const unsigned int numCycles = 5;
    std::vector<Epochstamp> stampsAfter;        // stamps after increasing value
    std::vector<Epochstamp> valueStamps;        // stamps of the updated values
    std::vector<Epochstamp> stampsAfterRestart; // stamps after restart logging
    const Epochstamp oldStamp = threeDaysBack;
    for (unsigned int i = 0; i < numCycles; ++i) {
        // Increase "variable" value and store after increasing it
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(deviceId, "slotIncreaseValue", KRB_TEST_MAX_TIMEOUT));
        boost::this_thread::sleep_for(boost::chrono::milliseconds(1)); // ensure timestamp is after setting
        stampsAfter.push_back(Epochstamp());

        // Get configuration, check expected values, check (static) time stamp of "oldValue" and store stamp of
        // "value"
        Hash cfg;
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(deviceId, cfg));
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(i) + 1, cfg.get<int>("value"));

        CPPUNIT_ASSERT_EQUAL(99, cfg.get<int>("oldValue"));
        const Epochstamp stamp = Epochstamp::fromHashAttributes(cfg.getAttributes("oldValue"));
        CPPUNIT_ASSERT_MESSAGE("'oldValue' has wrong time stamp: " + stamp.toIso8601(), stamp == oldStamp);
        valueStamps.push_back(Epochstamp::fromHashAttributes(cfg.getAttributes("value")));

        // Stop logging our device and check that it is not logged anymore.
        // Sleep needed before flush to ensure that - for file logger - the output stream has actually seen the data
        boost::this_thread::sleep_for(boost::chrono::milliseconds(250)); // locally 100 was always enough
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                                      .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                                      .receive());
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(loggerId, "slotTagDeviceToBeDiscontinued", "D", deviceId)
                                      .timeout(KRB_TEST_MAX_TIMEOUT * 1000)
                                      .receive());
        CPPUNIT_ASSERT_MESSAGE(
              "Test device still present in 'devicesToBeLogged' :" +
                    toString(m_deviceClient->get<std::vector<std::string>>(loggerId, "devicesToBeLogged")),
              waitForCondition(
                    [this, &loggerId, &deviceId]() {
                        auto loggedIds = m_deviceClient->get<std::vector<std::string>>(loggerId, "devicesToBeLogged");
                        // NOT in there anymore
                        return (std::find(loggedIds.begin(), loggedIds.end(), deviceId) == loggedIds.end());
                    },
                    KRB_TEST_MAX_TIMEOUT * 1000));

        // Restart again (and validate it is logging) - file based logger will gather the complete config again on
        // disk
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(loggerId, "slotAddDevicesToBeLogged", vector<string>(1, deviceId))
                                      .timeout(KRB_TEST_MAX_TIMEOUT * 1000)
                                      .receive());

        waitUntilLogged(deviceId, "testCfgFromPastRestart, cycle " + toString(numCycles));

        stampsAfterRestart.push_back(Epochstamp());
    }

    // Again flush - at the end of the last cycle we started logging again and archive_index.txt and archive_<N>.txt
    // might be out of sync otherwise - nevertheless for file based logging we need the repeated retries below for
    // the same reason as the sleeps above. :-(
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                                  .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                                  .receive());

    boost::this_thread::sleep_for(boost::chrono::milliseconds(250));

    // Now check that for all stored stamps, the stamps gathered for the reader are correct
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
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
                m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", deviceId, stampAfter.toIso8601())
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
                CPPUNIT_ASSERT_MESSAGE("Unexpected RemoteException received: " + std::string(re.what()),
                                       (re.detailedMsg().find(fileLoggerMsg) != std::string::npos ||
                                        re.detailedMsg().find(influxLoggerMsg) != std::string::npos));
            } catch (const TimeoutException& te) {
                // Also timeouts are allowed...
                karabo::util::Exception::clearTrace();
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
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

        CPPUNIT_ASSERT_MESSAGE(msg, conf.size() > 0);
        CPPUNIT_ASSERT_EQUAL(99, conf.get<int>("oldValue"));

        if (conf.get<int>("value") != static_cast<int>(i + 1)) {
            std::clog << msg << std::endl;
            failedCycles++;
            continue;
        }

        // Check received stamps: For "value" be aware that we store with
        // microsec precision only: we might be 1 off since we cut off digits instead of rounding
        const Epochstamp stampValueFromPast = Epochstamp::fromHashAttributes(conf.getAttributes("value"));
        CPPUNIT_ASSERT_MESSAGE(stampValueFromPast.toIso8601() + " vs " + valueStamps[i].toIso8601(),
                               (stampValueFromPast - valueStamps[i]).getFractions(TIME_UNITS::MICROSEC) <= 1ull);
        // The stamp for "oldValue" differs bewteen backends
        const Epochstamp stampOldFromPast = Epochstamp::fromHashAttributes(conf.getAttributes("oldValue"));
        std::string oldFromPastStr = stampOldFromPast.toIso8601(); // convert to microsecond precision
        if (pastConfigStaysPast) {
            // The stamp of "oldValue" is always the same
            CPPUNIT_ASSERT_EQUAL_MESSAGE("'oldValue' from past has wrong time stamp", oldStamp.toIso8601(),
                                         oldFromPastStr);
        } else {
            // Timestamps older than start of device logging are stored as stamp "start of device logging".
            // Stamp stampsAfterRestart[i] is after we are sure that logging runs again, so that is after
            // "start of device logging" - but we cannot really be sure how far.
            CPPUNIT_ASSERT_MESSAGE("received '" + stampOldFromPast.toIso8601() +=
                                   "', after restart '" + stampsAfterRestart[i].toIso8601(),
                                   stampOldFromPast < stampsAfterRestart[i]);
            const double dt =
                  stampOldFromPast - stampsAfterRestart[i]; // Has no sign due to the intermediate TimeDuration object
            CPPUNIT_ASSERT_MESSAGE("'oldValue' has wrong time stamp: " + stampOldFromPast.toIso8601() +=
                                   " - difference is : " + toString(dt),
                                   dt < 10.); // seen 2.95 (!) in https://git.xfel.eu/Karabo/Framework/-/jobs/290211
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
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(dlreader0, "slotGetPropertyHistory", deviceId, "value", params)
                                      .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                      .receive(histDevice, histProperty, history));
        for (const Hash& histEntry : history) {
            valueHist << Epochstamp::fromHashAttributes(histEntry.getAttributes("v")).toIso8601Ext() << " - "
                      << histEntry.get<int>("v") << "\n";
        }
        valueHist << "\n";
    }

    CPPUNIT_ASSERT_EQUAL_MESSAGE(
          toString(failedCycles) + " out of " + toString(numCycles) + " cycles failed!\n" + valueHist.str(), 0,
          failedCycles);

    const karabo::util::TimeDuration testDuration = testCaseStart.elapsed();

    const std::streamsize currPrecision{std::clog.precision()};
    std::clog << "(testCfgFromPastRestart took " << std::setprecision(4) << static_cast<double>(testDuration)
              << " sec. to execute)" << std::setprecision(currPrecision) << std::endl;

    std::clog << "OK" << std::endl;
}


void BaseLogging_Test::testUnchangedNoDefaultProperties() {
    using namespace std;
    clog << "Testing past config retrieval of properties with no default value ..." << flush;

    const string noDefaultProp = "Int32NoDefault";
    const string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);

    // Start test device and take care that the logger is ready for it
    // Use platform-dependent name for the device: concurrent tests in CI
    // operate on the same InfluxDB database ...
    const string deviceId(getDeviceIdPrefix() + "deviceWithNoDefaultValueProp");
    const string logId = karabo::util::DATALOGGER_PREFIX + m_server;
    auto success =
          m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    waitUntilLogged(deviceId, "testUnchangedNoDefaultProperties");

    // Set the value of the test device's property with no default value.
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set(deviceId, noDefaultProp, 12));
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));

    // Save this instant as a iso string
    Epochstamp es_afterPropSet;
    string afterPropSet = es_afterPropSet.toIso8601();

    // Flushes the datalogger to guarantee that setting of the property will be
    // in the log.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                                  .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                                  .receive());
    boost::this_thread::sleep_for(boost::chrono::milliseconds(150));

    // Assert that getConfigurationFromPast retrieves the non default value
    // when given a timepoint right after the setting.
    Hash conf;
    Schema schema;
    bool configAtTimepoint;
    string configTimepoint;

    int nTries = NUM_RETRY;
    while (!conf.has(noDefaultProp) && nTries > 0) {
        try {
            m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", deviceId, afterPropSet)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(conf, schema, configAtTimepoint, configTimepoint);
        } catch (const RemoteException& e) {
            const std::string errMsg = e.detailedMsg();
            // Tolerate only exception with following text that may come if file logger's disk IO is pretty slow
            auto errExpect = "Requested time point for device configuration is earlier than anything logged.";
            CPPUNIT_ASSERT_MESSAGE(errMsg, errMsg.find(errExpect) != std::string::npos);
            boost::this_thread::sleep_for(boost::chrono::milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
            m_sigSlot->call(karabo::util::DATALOGGER_PREFIX + m_server, "flush");
            --nTries;
        } catch (const std::exception& e) {
            CPPUNIT_ASSERT_MESSAGE(e.what(), false); // do not tolerate any other exception
        }
    }
    CPPUNIT_ASSERT_MESSAGE(toString(conf), conf.has(noDefaultProp));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Value for '" + noDefaultProp + "' differs from expected.", 12,
                                 conf.get<int>(noDefaultProp));

    // Shutdown the PropertyTest device
    success = m_deviceClient->killDevice(deviceId);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Reinstantiate the PropertyTest device, but this time do not set its
    // no default value property.
    success =
          m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    waitUntilLogged(deviceId, "testUnchangedNoDefaultProperties_2");

    // Flush the data logger to make sure there's no logging pendency
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                                  .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                                  .receive());
    // Wait some time for the data just flushed to be available for reading.
    boost::this_thread::sleep_for(boost::chrono::milliseconds(150));

    // Save this instant as a iso string
    Epochstamp es_afterDeviceReinst;
    string afterDeviceReinst = es_afterDeviceReinst.toIso8601();

    // Assert that getConfigurationFromPast for a timepoint after the second
    // instantiation of the PropertyTest device does not have the no default
    // value property.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", deviceId, afterDeviceReinst)
                                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                  .receive(conf, schema, configAtTimepoint, configTimepoint));
    CPPUNIT_ASSERT_MESSAGE("Property '" + noDefaultProp + "' should not be in retrieved configuration.",
                           !conf.has(noDefaultProp));

    // Shutdown the testing device.
    success = m_deviceClient->killDevice(deviceId);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    clog << "OK" << endl;
}


template <class T>
void isEqualMessage(const std::string& message, const T& expected, const T& actual,
                    const std::vector<karabo::util::Hash>& fullHistory) {
    std::string msg(message);
    if (expected != actual) {
        (msg += ": ") += toString(fullHistory);
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, expected, actual);
}


template <>
void isEqualMessage(const std::string& message, const std::vector<bool>& expected, const std::vector<bool>& actual,
                    const std::vector<karabo::util::Hash>& fullHistory) {
    std::string msg(message);
    if (expected != actual) {
        (msg += ": ") += toString(fullHistory);
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE(msg, toString(expected), toString(actual));
}

template <>
void isEqualMessage(const std::string& message, const float& expected, const float& actual,
                    const std::vector<karabo::util::Hash>& fullHistory) {
    std::string msg(message);
    if (expected != actual) {
        (msg += ": ") += toString(fullHistory);
    }
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg, expected, actual, expected * 1.e-6);
}


template <class T>
void BaseLogging_Test::testHistory(const std::string& key, const std::function<T(int)>& f, const bool testConf) {
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
    const int max_set = 100;
    std::clog << "Testing Property History retrieval for '" << key << "'... " << std::flush;

    // get configuration for later checks
    Hash beforeWritesCfg;
    CPPUNIT_ASSERT_NO_THROW((m_deviceClient->get(m_deviceId, beforeWritesCfg)));

    boost::this_thread::sleep_for(boost::chrono::milliseconds(150));

    // save this instant as a iso string
    Epochstamp es_beforeWrites;
    std::string beforeWrites = es_beforeWrites.toIso8601();

    // write a bunch of times
    for (int i = 0; i < max_set; i++) {
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set<T>(m_deviceId, key, f(i)));
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
    }

    Hash afterWritesCfg;
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(m_deviceId, afterWritesCfg));

    // save this instant as a iso string
    Epochstamp es_afterWrites;
    std::string afterWrites = es_afterWrites.toIso8601();

    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
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

    const unsigned int numGetPropHist = m_deviceClient->get<unsigned int>(dlreader0, "numGetPropertyHistory");
    std::vector<std::string> exceptionsMsgs;

    int nTries = NUM_RETRY;
    unsigned int numExceptions = 0;
    unsigned int numChecks = 0;
    while (nTries >= 0 && history.size() != max_set) {
        try {
            numChecks++;
            // TODO: use the deviceClient to retrieve the property history
            // history = m_deviceClient->getPropertyHistory(m_deviceId, key, before, after, max_set * 2);
            m_sigSlot->request(dlreader0, "slotGetPropertyHistory", m_deviceId, key, params)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(device, property, history);
        } catch (const karabo::util::TimeoutException& e) {
            karabo::util::Exception::clearTrace();
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
        } catch (const karabo::util::RemoteException& e) {
            karabo::util::Exception::clearTrace();
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
        }
        boost::this_thread::sleep_for(boost::chrono::milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
        nTries--;
    }

    CPPUNIT_ASSERT_EQUAL_MESSAGE(
          "History size different than expected after " + toString(numChecks) + " checks:\n\tdeviceId: " + m_deviceId +
                "\n\tkey: " + key + "\n\tparam.from: " + beforeWrites + "\n\tparam.to: " + afterWrites +
                "\n\tparam.maxNumData: " + toString(max_set * 2) + "\n\thistory.size(): " + toString(history.size()) +
                "\n\tNumber of Exceptions: " + toString(numExceptions) + "\n\tExceptions:\n" +
                boost::algorithm::join(exceptionsMsgs, "\n") + "\nhistory\t" + toString(history),
          static_cast<size_t>(max_set), history.size());

    CPPUNIT_ASSERT_EQUAL(numGetPropHist + numChecks,
                         m_deviceClient->get<unsigned int>(dlreader0, "numGetPropertyHistory"));

    for (int i = 0; i < max_set; i++) {
        // checking values and timestamps
        isEqualMessage("Wrong value in history " + toString(i), f(i), history[i].get<T>("v"), history);
        Epochstamp current = Epochstamp::fromHashAttributes(history[i].getAttributes("v"));
        CPPUNIT_ASSERT_MESSAGE("Timestamp later than the requested window", current <= es_afterWrites);
        CPPUNIT_ASSERT_MESSAGE("Timestamp earlier than the requested window", current >= es_beforeWrites);
        if (i > 0) {
            Epochstamp previous = Epochstamp::fromHashAttributes(history[i - 1].getAttributes("v"));
            CPPUNIT_ASSERT_MESSAGE("Timestamp earlier than the requested window", current > previous);
        }
    }
    std::clog << "Ok" << std::endl;

    // skip the configuration retrieval
    if (!testConf) return;

    std::clog << "Testing past configuration retrieval for '" << key << "'... " << std::flush;

    exceptionsMsgs.clear();

    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                                  .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                                  .receive());

    const unsigned int numGetCfgFromPast = m_deviceClient->get<unsigned int>(dlreader0, "numGetConfigurationFromPast");
    nTries = NUM_RETRY;
    numExceptions = 0;
    numChecks = 0;
    // place holder schema, could be checked in future tests
    Schema schema;
    Hash conf;
    while (nTries >= 0) { // '&& !conf.empty()' check not needed as in DataLogging_Test::testCfgFromPastRestart:
        //                   The 'history.size() != max_set' check in the loop above already ensures that all data
        //                   available

        // TODO: use the deviceClient to retrieve the configuration from past
        // auto pair = m_deviceClient->getConfigurationFromPast(m_deviceId, before);
        // conf = pair.first;
        bool excepted = false;
        try {
            numChecks++;
            m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", m_deviceId, beforeWrites)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(conf, schema);
        } catch (const karabo::util::TimeoutException& e) {
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
            excepted = true;
        } catch (const karabo::util::RemoteException& e) {
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
            excepted = true;
        }
        if (!excepted) break; // Any result should be trustworthy!
        boost::this_thread::sleep_for(boost::chrono::milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
        nTries--;
    }

    CPPUNIT_ASSERT_MESSAGE("Configuration size from slotGetConfigurationFromPast still 0 after " + toString(numChecks) +
                                 " checks.\n\tdeviceId: " + m_deviceId + "\n\tparam.before: " + beforeWrites +
                                 "\n\tconf.size(): " + toString(conf.size()) +
                                 "\n\tNumber of Exceptions: " + toString(numExceptions) + "\n\tExceptions:\n" +
                                 boost::algorithm::join(exceptionsMsgs, "\n"),
                           conf.size() > 0);
    CPPUNIT_ASSERT_EQUAL(numGetCfgFromPast + numChecks,
                         m_deviceClient->get<unsigned int>(dlreader0, "numGetConfigurationFromPast"));

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
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong configuration from past (before writes) for key :" + leaf,
                                         beforeWritesCfg.getAs<std::string>(leaf), conf.getAs<std::string>(leaf));
        } else {
            // Configuration from past is only allowed to miss non-archived leaves. Checks that the
            // missing leaf has NO_ARCHIVING set for its ARCHIVE_POLICY attribute.
            if (!schema.hasArchivePolicy(leaf) || schema.getArchivePolicy(leaf) != Schema::NO_ARCHIVING) {
                missingKeysFromPast += leaf + " : ";
            }
        }
    }

    // Check that all keys are logged.
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Missing keys in configuration from past (before writes):\n" + missingKeysFromPast,
                                 0ul, missingKeysFromPast.size());

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
            m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", m_deviceId, afterWrites)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(conf, schema);
        } catch (const karabo::util::TimeoutException& e) {
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
            excepted = true;
        } catch (const karabo::util::RemoteException& e) {
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
            excepted = true;
        }
        if (!excepted) break; // Any result should be trustworthy!
        boost::this_thread::sleep_for(boost::chrono::milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
        nTries--;
    }

    CPPUNIT_ASSERT_MESSAGE("Configuration still not retrieved after  " + toString(numChecks) +
                                 " checks.\n\tdeviceId: " + m_deviceId + "\n\tparam.before: " + beforeWrites +
                                 "\n\tconf.size(): " + toString(conf.size()) +
                                 "\n\tNumber of Exceptions: " + toString(numExceptions) + "\n\tRemote Errors:\n" +
                                 boost::algorithm::join(exceptionsMsgs, "\n"),
                           conf.size() > 0);
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
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong configuration from past (after writes) for key :" + leaf,
                                         afterWritesCfg.getAs<std::string>(leaf), conf.getAs<std::string>(leaf));
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
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Missing keys in configuration from past (after writes):\n" + missingKeysFromPast,
                                 0ul,
                                 missingKeysFromPast.size());
     */
    // TODO: Remove the following conditional logging once the assert above is activated.
    if (!missingKeysFromPast.empty()) {
        std::clog << "Missing keys in configuration from past (after writes):\n" << missingKeysFromPast << std::endl;
    }

    std::clog << "Ok" << std::endl;
}


void BaseLogging_Test::testInt(bool testPastConf) {
    auto lambda = [](int i) -> int { return i; };
    testHistory<int>("int32Property", lambda, testPastConf);
}


void BaseLogging_Test::testUInt64(bool testPastConf) {
    auto lambda = [](int i) -> unsigned long long { return (unsigned long long)i - 1; };
    testHistory<unsigned long long>("uint64Property", lambda, testPastConf);
}


void BaseLogging_Test::testFloat(bool testPastConf) {
    auto lambda = [](int i) -> float { return 2.5e-8f * i; };
    testHistory<float>("floatProperty", lambda, testPastConf);
}


void BaseLogging_Test::testString(bool testPastConf) {
    auto lambda = [](int i) -> string { return ((i % 2) ? string() : "(1|2|" + karabo::util::toString(i)) + ")"; };
    testHistory<string>("stringProperty", lambda, testPastConf);

    // Also test a string with a new line character
    auto lambda2 = [](int i) -> string { return "with\nnewline" + karabo::util::toString(i); };
    testHistory<string>("stringProperty", lambda2, testPastConf);
}


void BaseLogging_Test::testVectorString(bool testPastConf) {
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


void BaseLogging_Test::testVectorChar(bool testPastConf) {
    auto lambda = [](int i) -> vector<char> {
        const char c = static_cast<char>(i & 0xFF);
        return {c, c, c, c, c, 0};
    };
    testHistory<vector<char>>("vectors.charProperty", lambda, testPastConf);
}


void BaseLogging_Test::testVectorSignedChar(bool testPastConf) {
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


void BaseLogging_Test::testVectorUnsignedChar(bool testPastConf) {
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


void BaseLogging_Test::testVectorBool(bool testPastConf) {
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


void BaseLogging_Test::testVectorShort(bool testPastConf) {
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


void BaseLogging_Test::testVectorUnsignedShort(bool testPastConf) {
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


void BaseLogging_Test::testVectorInt(bool testPastConf) {
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


void BaseLogging_Test::testVectorUnsignedInt(bool testPastConf) {
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


void BaseLogging_Test::testVectorLongLong(bool testPastConf) {
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


void BaseLogging_Test::testVectorUnsignedLongLong(bool testPastConf) {
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


void BaseLogging_Test::testTable(bool testPastConf) {
    auto lambda = [](int i) -> vector<Hash> {
        vector<Hash> t = {// For strings, test also pipe '|' (the separator in our text files) and newline '\n'.
                          Hash("e1", "ab\nc" + karabo::util::toString(i), "e2", ((i % 2) == 0), "e3", 12 * i, "e4",
                               0.9837F * i, "e5", 1.2345 * i),
                          Hash("e1", "xy|z" + karabo::util::toString(i), "e2", ((i % 2) == 1), "e3", 42 * i, "e4",
                               2.33333F * i, "e5", 7.77777 * i)};
        return t;
    };
    testHistory<vector<Hash>>("table", lambda, testPastConf);
}


void BaseLogging_Test::testChar(bool testPastConf) {
    auto lambda = [](int i) -> char { return static_cast<char>(i & 0xff); };
    testHistory<char>("charProperty", lambda, testPastConf);
}


void BaseLogging_Test::testNans() {
    std::clog << "Test handling of NaNs for getPropertyHistory and getConfigurationFromPast ..." << std::endl;

    Epochstamp testCaseStart;

    const std::string deviceId(m_deviceId + "forNan");
    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "NanTestDevice", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    waitUntilLogged(deviceId, "testNans");

    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
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
        m_sigSlot->call(dlreader0, "slotGetPropertyHistory", deviceId, property, params); // fire-and-forget...
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

        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set(deviceId, new_conf));
        const Hash cfg = m_deviceClient->get(deviceId);
        updateStamps.push_back(Epochstamp::fromHashAttributes(cfg.getAttributes("doubleProperty")));
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
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
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set(deviceId, end_conf));
    updateStamps.push_back(
          Epochstamp::fromHashAttributes(m_deviceClient->get(deviceId).getAttributes("doubleProperty")));
    // The sleep interval below had to be increased because of the Telegraf environment - the time required to save
    // is higher. If es_afterWrites captured after the sleep instruction refers to a time point that comes before
    // the time Telegraf + Influx are done writing the data, the property history will not be of the expected size
    // and the test will fail.
    boost::this_thread::sleep_for(boost::chrono::milliseconds(WAIT_WRITES));

    // save this instant as a iso string
    Epochstamp es_afterWrites;
    std::string afterWrites = es_afterWrites.toIso8601();

    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
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
                m_sigSlot->request(dlreader0, "slotGetPropertyHistory", deviceId, property_pair.first, params)
                      .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                      .receive(device, property, history);
            } catch (const karabo::util::TimeoutException& e) {
                karabo::util::Exception::clearTrace();
                exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
                ++numExceptions;
            } catch (const karabo::util::RemoteException& e) {
                karabo::util::Exception::clearTrace();
                exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
                ++numExceptions;
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
            nTries--;
        }

        Epochstamp afterLastCheck;
        if (property_pair.second == history.size()) {
            std::clog << "\ntestNans: History size check for property '" << property_pair.first << "' succeeded after "
                      << numChecks << " attempt(s) ranging from " << beforeFirstCheck.toIso8601() << " to "
                      << afterLastCheck.toIso8601() << " ("
                      << afterLastCheck.elapsed(beforeFirstCheck).getTotalSeconds() << " secs)." << std::endl;
        }

        CPPUNIT_ASSERT_EQUAL_MESSAGE(
              "History size different than expected after " + toString(numChecks) + " checks:\n\tdeviceId: " +
                    deviceId + "\n\tproperty : " + property_pair.first + "\n\tparam.from: " + beforeWrites +
                    "\n\tparam.to: " + afterWrites + "\n\tparam.maxNumData: " + toString(max_set * 2) +
                    "\n\thistory.size(): " + toString(history.size()) + "\n\tNumber of Exceptions: " +
                    toString(numExceptions) + "\n\tExceptions:\n" + boost::algorithm::join(exceptionsMsgs, "\n"),
              static_cast<size_t>(property_pair.second), history.size());

        // Test that the return values match, incl. timestamps
        for (size_t i = 0; i <= max_set; ++i) {
            // First check timestamp - to microsecond precision
            const Epochstamp historyStamp = Epochstamp::fromHashAttributes(history[i].getAttributes("v"));
            const TimeDuration diff = historyStamp.elapsed(updateStamps[i]);
            CPPUNIT_ASSERT_MESSAGE(toString(diff),
                                   diff < TimeDuration(0ull, 1000000000000ull)); // 1e12 attosec, i.e. 1 microsec

            if (property_pair.first == "floatProperty") {
                const float floatInput = (i == max_set ? max_set : bad_floats[i % bad_floats.size()]);
                const float historyFloat = history[i].get<float>("v");
                if (std::isnan(floatInput)) {
                    // comparison with nan is always false
                    CPPUNIT_ASSERT_MESSAGE(toString(i), std::isnan(historyFloat));
                } else {
                    // comparison with +/-inf works
                    CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(i), floatInput, historyFloat);
                }
            } else if (property_pair.first == "doubleProperty") {
                const double doubleInput = (i == max_set ? max_set : bad_doubles[i % bad_doubles.size()]);
                const double historyDouble = history[i].get<double>("v");
                if (std::isnan(doubleInput)) {
                    // comparison with nan is always false
                    CPPUNIT_ASSERT_MESSAGE(toString(i), std::isnan(historyDouble));
                } else {
                    // comparison with +/-inf works
                    CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(i), doubleInput, historyDouble);
                }
            } else if (property_pair.first == "int32Property") {
                CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(i), static_cast<int>(i), history[i].get<int>("v"));
            }
        }
    }

    // Now test slotGetConfigurationFromPast with infinite values
    for (size_t i = 0; i < vec_es_afterWrites.size(); ++i) {
        Hash conf;
        Schema schema;
        bool configAtTimepoint = false;
        std::string configTimepoint;
        CPPUNIT_ASSERT_NO_THROW(
              m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", deviceId, vec_es_afterWrites[i].toIso8601())
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                    .receive(conf, schema, configAtTimepoint, configTimepoint));

        CPPUNIT_ASSERT(configAtTimepoint);
        // This equality check relies on the fact that the string representation implicitly rounds to micro second
        // precision, i.e. the precision in the data base. So if the test fails here, do like above with the
        // TimeDuration.
        CPPUNIT_ASSERT_EQUAL(vec_es_updateStamps[i].toIso8601Ext(), configTimepoint);
        const double theD = conf.get<double>("doubleProperty");
        const float theF = conf.get<float>("floatProperty");
        if (std::isnan(bad_floats[i])) {
            // assuming same order of nan/inf for both bad_floats and bad_doubles
            CPPUNIT_ASSERT_MESSAGE(toString(i) + ": theF = " + toString(theF), std::isnan(theF));
            CPPUNIT_ASSERT_MESSAGE(toString(i) + ": theD = " + toString(theD), std::isnan(theD));
        } else {
            CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(i), bad_floats[i], theF);
            CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(i), bad_doubles[i], theD);
        }
    }

    // Clean-up
    success = m_deviceClient->killDevice(deviceId);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    const karabo::util::TimeDuration testDuration = testCaseStart.elapsed();

    const std::streamsize currPrecision{std::clog.precision()};
    std::clog << "(testNans took " << std::setprecision(4) << static_cast<double>(testDuration) << " sec. to execute)"
              << std::setprecision(currPrecision) << std::endl;

    std::clog << "Ok" << std::endl;
}


void BaseLogging_Test::testSchemaEvolution() {
    std::clog << "Testing property history retrieval when schema evolution happens ..." << std::endl;

    // Instantiates a DataLogTestDevice to use for the schema evolution test.
    // "m_deviceIdPrefix" allows concurrent Influx tests on the different platform CI runners.
    const std::string deviceId(getDeviceIdPrefix() + "SchemaEvolutionDevice");
    auto success =
          m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

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
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStr).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotReconfigure", Hash("reconfigurableValue", "Non empty str"))
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive());

    // "reconfigurableValue" as vector of strings
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", Schema()).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    Schema schemaVecStr;
    VECTOR_STRING_ELEMENT(schemaVecStr)
          .key("reconfigurableValue")
          .assignmentOptional()
          .defaultValue(std::vector<std::string>({"a"}))
          .reconfigurable()
          .commit();
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotUpdateSchema", schemaVecStr)
                                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                  .receive());
    const std::vector<std::string> strVector{"a", "", "b", "c"};
    Hash strVecValueCfg{"reconfigurableValue", strVector};
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotReconfigure", strVecValueCfg)
                                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                  .receive());


    // "reconfigurableValue" as int32
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", Schema()).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    Schema schemaInt32;
    INT32_ELEMENT(schemaInt32)
          .key("reconfigurableValue")
          .assignmentOptional()
          .defaultValue(0)
          .reconfigurable()
          .commit();
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaInt32).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotReconfigure", Hash("reconfigurableValue", 10))
                                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                  .receive());

    // Makes sure all the writes are done before retrieval.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                                  .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                                  .receive());
    // The sleep interval below had to be added because of the Telegraf environment - the time required to save is
    // higher. If toTimePoint captured after the sleep instruction refers to a time point that comes before the time
    // Telegraf + Influx are done writing the data, the property history will not be of the expected size and the
    // test will fail.
    boost::this_thread::sleep_for(boost::chrono::milliseconds(WAIT_WRITES));

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

    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);

    // the history retrieval might take more than one try, it could have to index the files (or wait
    // for the records to be available for reading in the Influx case).
    std::vector<std::string> exceptionsMsgs;

    int nTries = NUM_RETRY;
    unsigned int numExceptions = 0;
    unsigned int numChecks = 0;
    while (nTries >= 0 && history.size() != 6) {
        try {
            numChecks++;
            m_sigSlot->request(dlreader0, "slotGetPropertyHistory", deviceId, "reconfigurableValue", params)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(replyDevice, replyProperty, history);
        } catch (const karabo::util::TimeoutException& e) {
            karabo::util::Exception::clearTrace();
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
        } catch (const karabo::util::RemoteException& e) {
            karabo::util::Exception::clearTrace();
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
        }
        boost::this_thread::sleep_for(boost::chrono::milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
        nTries--;
    }

    CPPUNIT_ASSERT_EQUAL_MESSAGE(
          "History size different than expected after " + toString(numChecks) + " checks:\n\tdeviceId: " + deviceId +
                "\n\tproperty: \"value\"" + "\n\tparam.from: " + fromTimePoint.toIso8601() +
                "\n\tparam.to: " + toTimePoint.toIso8601() + "\n\tparam.maxNumData: " + toString(maxNumData) +
                "\n\thistory.size(): " + toString(history.size()) + "\n\tNumber of Exceptions: " +
                toString(numExceptions) + "\n\tExceptions:\n" + boost::algorithm::join(exceptionsMsgs, "\n"),
          6, static_cast<int>(history.size()));

    // Checks the first two expected values - of string type.
    // The first is the empty string that is the default value.
    // The second is an explicitily set non empty string value.
    CPPUNIT_ASSERT_EQUAL_MESSAGE("First string value different from expected for history entry", std::string(""),
                                 history[0].get<std::string>("v"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Second string value different from expected for history entry",
                                 std::string("Non empty str"), history[1].get<std::string>("v"));
    // Checks the next two expected values - of vector of strings type.
    // The first is the empty vector that is the default value.
    // The second is an explicitily set non empty vector.

    CPPUNIT_ASSERT_EQUAL_MESSAGE("First vector of strings in history is not of the expected size", 1ul,
                                 history[2].get<std::vector<std::string>>("v").size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Value of sole element of first vector of strings in history different from exepected",
                                 std::string("a"), history[2].get<std::vector<std::string>>("v")[0]);

    const auto& strVectorValue = history[3].get<std::vector<std::string>>("v");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Size of second vector of strings in history different from expected",
                                 strVector.size(), strVectorValue.size());
    for (size_t i = 0; i < strVectorValue.size(); i++) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE(
              "Value at index " + toString(i) + " of second vector of strings in history different from expected",
              strVector[i], strVectorValue[i]);
    }
    // Checks the last expected values - of int type.
    // The first is the default value of 0.
    // The second is an explicitily set int value.
    CPPUNIT_ASSERT_EQUAL_MESSAGE("First int value different from expected for history entry", 0,
                                 history[4].get<int>("v"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Second int value different from expected for history entry", 10,
                                 history[5].get<int>("v"));

    std::clog << "Ok" << std::endl;
}

// TODO: implement testVectorFloat() (include vectors of different length and empty vector);
// TODO: implement test and FIX for a vector of strings with an empty string as its only element
