/*
 * File:   DataLogging_Test.cc
 * Author: silenzi
 *
 * Created on Nov 12, 2018, 6:07:07 PM
 */

#include "DataLogging_Test.hh"
#include <karabo/net/EventLoop.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include "karabo/util/DataLogUtils.hh"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <cstdlib>
#include <sstream>

USING_KARABO_NAMESPACES;
using std::vector;
using std::string;
using karabo::util::toString;
using karabo::util::Epochstamp;
using karabo::util::Timestamp;
using karabo::util::INT32_ELEMENT;
using karabo::util::OVERWRITE_ELEMENT;
using karabo::util::State;
using karabo::xms::SLOT_ELEMENT;

#define KRB_TEST_MAX_TIMEOUT 10

/* Timeout, in milliseconds, for a slot request. */
#define SLOT_REQUEST_TIMEOUT_MILLIS 2500

#define PAUSE_BEFORE_RETRY_MILLIS 150

class DataLogTestDevice : public karabo::core::Device<> {


public:
    KARABO_CLASSINFO(DataLogTestDevice, "DataLogTestDevice", "2.8")

    static void expectedParameters(karabo::util::Schema& expected) {

        OVERWRITE_ELEMENT(expected).key("state")
                .setNewOptions(State::INIT, State::ON)
                .setNewDefaultValue(State::INIT)
                .commit();

        INT32_ELEMENT(expected).key("oldValue")
                .readOnly()
                .initialValue(-1)
                .commit();

        INT32_ELEMENT(expected).key("value")
                .readOnly()
                .initialValue(0)
                .commit();

        SLOT_ELEMENT(expected).key("slotIncreaseValue")
                .commit();
    }

    DataLogTestDevice(const karabo::util::Hash& input) : karabo::core::Device<>(input) {
        KARABO_SLOT(slotIncreaseValue);
        KARABO_INITIAL_FUNCTION(initialize);
    }


    virtual ~DataLogTestDevice() {
    }

private:

    void initialize() {
        // Set oldValue with time stamp from past - noon January 1st, 1999
        const Epochstamp stamp("19990101T120000.000000Z");
        set("oldValue", 99, Timestamp(stamp, 0ull));

        updateState(State::ON);
    }


    void slotIncreaseValue() {
        set("value", get<int>("value") + 1);
    }
};
KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device<>, DataLogTestDevice)


bool waitForCondition(boost::function<bool() > checker, unsigned int timeoutMillis) {
    const unsigned int sleepIntervalMillis = 5;
    unsigned int numOfWaits = 0;
    const unsigned int maxNumOfWaits = static_cast<unsigned int> (std::ceil(timeoutMillis / sleepIntervalMillis));
    while (numOfWaits < maxNumOfWaits && !checker()) {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(sleepIntervalMillis));
        numOfWaits++;
    }
    return (numOfWaits < maxNumOfWaits);
}

// adding vector<Hash>, Hash, and vector<string> helpers for CppUnit
namespace CppUnit{
    template <>
    struct assertion_traits<karabo::util::Hash>{
        static bool equal(const karabo::util::Hash &a, const karabo::util::Hash &b){
            if (b.size() != a.size()){
                return false;
            }
            std::vector<std::string> paths;
            a.getPaths(paths);
            for (const std::string & path : paths) {
                // most of the saving is serialized into text, this is why this helper
                // checks the equality between values only passed as strings.

                if (a.getAs<string>(path) != b.getAs<string>(path)) {
                    return false;
                }
            }
            return true;
        }
        static std::string toString(const karabo::util::Hash &p){
            std::ostringstream o;
            o << p << std::endl;
            return o.str();
        }
    };

    template <>
    struct assertion_traits<std::vector< karabo::util::Hash>>{
        static bool equal(const std::vector< karabo::util::Hash> &a, const std::vector< karabo::util::Hash> &b){
            // using karabo::util::similar() here is not OK. the attributes of the hashes are dropped in one
            // of the serialization steps
            if (a.size() != b.size()){
                return false;
            }
            for (size_t i = 0; i < a.size(); i++){
                const karabo::util::Hash& a_i = a[i];
                const karabo::util::Hash& b_i = b[i];
                if (b_i.size() != a_i.size()){
                    return false;
                }
                std::vector<std::string> paths;
                a_i.getPaths(paths);
                for (const std::string & path : paths) {
                    // most of the saving is serialized into text, this is why this helper
                    // checks the equality between values only passed as strings.
                    if (a_i.getAs<string>(path) != b_i.getAs<string>(path)) {
                        return false;
                    }
                }
            }
            return true;
        }

        static std::string toString(const std::vector< karabo::util::Hash> &p){
            std::ostringstream o;
            o << "(" << std::endl;
            for (const karabo::util::Hash& e : p){
                o << e << "," << std::endl;
            }
            o << ")";
            return o.str();
        }
    };

    template <>
    struct assertion_traits<std::vector<std::string>>{
        static bool equal(const std::vector<std::string> &a, const std::vector<std::string> &b){
            return a == b;
        }
        static std::string toString(const std::vector<std::string> &p){
            std::ostringstream o;
            o << "(" << std::endl;
            for (const std::string& e : p){
                o << e << "," << std::endl;
            }
            o << ")";
            return o.str();
        }
    };


    void assertEquals(const float& expected, const float& actual, const SourceLine& s, const std::string& message) {
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message, expected, actual, expected * 1.e-6);
    }
}

CPPUNIT_TEST_SUITE_REGISTRATION(DataLogging_Test);

const unsigned int DataLogging_Test::m_flushIntervalSec = 1u;


DataLogging_Test::DataLogging_Test()
    : m_server("DataLoggingTestServer"), m_deviceId("PropertyTestDevice"), m_fileLoggerDirectory("dataLoggingTest"),
    m_changedPath(false), m_oldPath() {

}


DataLogging_Test::~DataLogging_Test() {
}


void DataLogging_Test::setUp() {
    // Uncomment to run with a local broker:
    //setenv("KARABO_BROKER", "tcp://localhost:7777", true);

    if (!getenv("KARABO")) {
        // We are likely running this test from inside NetBeans without an activated Karabo.
        // So we extend PATH such that "karabo-idxbuild", triggered by the DataLogReader, is found:
        std::string newPath(karabo::util::Version::getPathToKaraboInstallation() + "/bin");
        const char* oldPath = getenv("PATH");
        if (oldPath) {
            m_oldPath = oldPath;
            (newPath += ":") += oldPath;
        }
        setenv("PATH", newPath.data(), 1);
        m_changedPath = true;
    }

    // Start central event-loop
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));

    // Create and start server
    Hash config("serverId", m_server, "scanPlugins", false, "Logger.priority", "FATAL");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();
    // Create client
    m_deviceClient = boost::make_shared<DeviceClient>();
    m_sigSlot = boost::make_shared<SignalSlotable>("sigSlot");
    m_sigSlot->start();
}


std::pair<bool, std::string> DataLogging_Test::startLoggers(const std::string& loggerType,
                                                            bool useInvalidInfluxUrl) {

    Hash manager_conf;
    manager_conf.set("deviceId", "loggerManager");
    manager_conf.set("flushInterval", m_flushIntervalSec);
    manager_conf.set<vector < string >> ("serverList",{m_server});
    manager_conf.set("logger", loggerType);

    if (loggerType == "FileDataLogger") {
        manager_conf.set("logger.FileDataLogger.directory",
                         (m_fileLoggerDirectory.empty() ? "" : m_fileLoggerDirectory + "/") + "karaboHistory");
    } else if (loggerType == "InfluxDataLogger") {
        std::ostringstream influxUrl;
        influxUrl << "tcp://";

        if (getenv("KARABO_TEST_INFLUXDB_HOST")) {
            influxUrl << getenv("KARABO_TEST_INFLUXDB_HOST");
        } else {
            influxUrl << "localhost";
        }
        influxUrl << ":";
        if (useInvalidInfluxUrl) {
            influxUrl << "8088";
        } else if (getenv("KARABO_TEST_INFLUXDB_PORT")) {
            influxUrl << getenv("KARABO_TEST_INFLUXDB_PORT");
        } else {
            influxUrl << "8086";
        }
        manager_conf.set("logger.InfluxDataLogger.url", influxUrl.str());
    } else {
        CPPUNIT_FAIL("Unknown logger type '" + loggerType + "'");
    }

    return m_deviceClient->instantiate(m_server,
                                       "DataLoggerManager", manager_conf, KRB_TEST_MAX_TIMEOUT);
}

void DataLogging_Test::tearDown() {
    m_deviceClient.reset();
    m_deviceServer.reset();
    m_sigSlot.reset();
    EventLoop::stop();
    m_eventLoopThread.join();

    // Clean up directory - you may want to comment out these lines for debugging
    boost::filesystem::remove("loggermap.xml");
    boost::filesystem::remove_all(m_fileLoggerDirectory);

    if (m_changedPath) {
        if (m_oldPath.empty()) {
            unsetenv("PATH");
        } else {
            setenv("PATH", m_oldPath.data(), 1);
        }
    }
}


void DataLogging_Test::fileAllTestRunner() {
    std::pair<bool, std::string> success = m_deviceClient->instantiate(m_server, "PropertyTest",
                                                                       Hash("deviceId", m_deviceId),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    std::clog << "==== Starting sequence of File Logging tests ====" << std::endl;
    success = startLoggers("FileDataLogger");
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();
    testInt();
    testFloat();
    testString();
    testVectorString();
    testTable();
    testHistoryAfterChanges();
    // This must be the last test case before killing the DataLoggerManager - it stops the
    // device being logged to make sure that the last known configuration can be successfully
    // retrieved after the device is gone.
    testLastKnownConfiguration();

    // This deals with its own device, so comment above about being last is not applicable
    testCfgFromPastRestart();
}


void DataLogging_Test::influxAllTestRunner() {
    std::pair<bool, std::string> success = m_deviceClient->instantiate(m_server, "PropertyTest",
                                                                       Hash("deviceId", m_deviceId),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Starts the same set of tests with InfluxDb logging instead of text-file based logging
    std::clog << "\n==== Starting sequence of Influx Logging tests ====" << std::endl;
    success = startLoggers("InfluxDataLogger");
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();
    testInt(true);
    testFloat(false);
    testString(false);
    testVectorString(false);
    testTable(false);

    // NOTE:
    // "testHistoryAfterChanges" is not being called for the Influx based logging: it tests a behavior
    // of including the last known value of a property if no change had occurred to that property within
    // the time range passed to slotGetPropertyHistory. As the GUI is not depending on that change and
    // it would require an extra query to InfluxDb, this behavior of slotGetPropertyHistory hadn't been
    // migrated to InfluxDb based logging.

    testLastKnownConfiguration();

    testCfgFromPastRestart();
}


void DataLogging_Test::testAllInstantiated() {
    std::clog << "Testing deviceInstantiation... " << std::flush;
    int timeout = 1500; // milliseconds
    vector<string> devices;
    devices.push_back(karabo::util::DATALOGGER_PREFIX + m_server);
    devices.push_back(karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server));
    devices.push_back(karabo::util::DATALOGREADER_PREFIX + ("1-" + m_server));
    while (timeout > 0) {
        const Hash topo(m_deviceClient->getSystemTopology());
        CPPUNIT_ASSERT(topo.has("device"));
        const Hash& device = topo.get<Hash>("device");
        bool allUp = true;
        for (const string & deviceId : devices) {
            allUp = allUp && device.has(deviceId);
        }
        if (allUp) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(50));
        timeout -= 50;
    }
    CPPUNIT_ASSERT_MESSAGE("Timeout while waiting for datalogging to be instantiated", timeout > 0);
    std::clog << "Ok" << std::endl;
}


void DataLogging_Test::testHistoryAfterChanges() {

    const std::string propertyName("int32Property");
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
    const int max_set = 100;

    std::clog << "Testing Property History retrieval after changes for '" << propertyName << "'... " << std::flush;

    // write a bunch of times
    for (int i = 0; i < max_set; i++) {
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set<int>(m_deviceId, propertyName, i));
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }

    Epochstamp es_after;
    std::string after = es_after.toIso8601();
    Epochstamp es_wayAfter(es_after.getSeconds() + 60, es_after.getFractionalSeconds());
    std::string wayAfter = es_wayAfter.toIso8601();

    // wait more than the flush time
    boost::this_thread::sleep(boost::posix_time::milliseconds(m_flushIntervalSec * 1000 + 250));

    // placeholders, could be skipped but they are here for future expansions of the tests
    std::string device;
    std::string property;
    vector<Hash> history;
    Hash params;
    params.set<std::string>("from", after);
    params.set<std::string>("to", wayAfter);
    params.set<int>("maxNumData", max_set * 2);

    // FIXME: refactor this once indexing is properly handled.
    // the history retrieval might take more than one try, it could have to index the files.
    int nTries = 100;
    while (nTries >= 0 && history.size() < 1) {
        try {
            m_sigSlot->request(dlreader0, "slotGetPropertyHistory", m_deviceId, propertyName, params)
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(device, property, history);
        } catch (const karabo::util::TimeoutException &e) {
            karabo::util::Exception::clearTrace();
        } catch (const karabo::util::RemoteException &e) {
            karabo::util::Exception::clearTrace();
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(SLOT_REQUEST_TIMEOUT_MILLIS));
        nTries--;
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE("History size should be 1, got " + karabo::util::toString(history.size()) + ".",
                                 1, history.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Device name on reply, '" + device + "', differs from expected, '" + m_deviceId + "'.",
                                 m_deviceId, device);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Property name on reply, '" + property + "', differs from expected, '" + propertyName + "'.",
                                 propertyName, property);

    // checking values and timestamps
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong value in history", 99, history[0].get<int>("v"));
    Epochstamp current = Epochstamp::fromHashAttributes(history[0].getAttributes("v"));
    CPPUNIT_ASSERT_MESSAGE("Timestamp later than the requested window", current <= es_wayAfter);

    std::clog << "Ok" << std::endl;
}


void DataLogging_Test::testLastKnownConfiguration() {

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
        m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast",
                           m_deviceId, beforeAnything.toIso8601())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(conf, schema, configAtTimepoint, configTimepoint);
    } catch (const RemoteException& re) {
        const string fileLoggerMsg("Requested time point for device configuration is earlier than anything logged");
        const string influxLoggerMsg("Failed to query schema digest");
        CPPUNIT_ASSERT_MESSAGE("Exception message: " + re.detailedMsg(),
                               (re.detailedMsg().find(fileLoggerMsg) != string::npos
                                || re.detailedMsg().find(influxLoggerMsg) != string::npos));
        remoteExcept = true;
    } catch (const std::exception& e) {
        CPPUNIT_ASSERT_MESSAGE(string("Unexpected exception: ") += e.what(), false);
    }
    CPPUNIT_ASSERT_MESSAGE("Expected exception, received " + toString(conf), remoteExcept);

    std::clog << "\n... Ok (no configuration retrieved)." << std::endl;

    karabo::util::Epochstamp rightBeforeDeviceGone;
    std::clog << "... right before killing device being logged (at " << rightBeforeDeviceGone.toIso8601() << ") ...";
    // At the rightBeforeDeviceGone timepoint, a last known configuration should be obtained with the last value set in
    // the  previous test cases for the 'int32Property' - even after the device being logged is gone.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast",
                                               m_deviceId, rightBeforeDeviceGone.toIso8601())
                            .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                            .receive(conf, schema, configAtTimepoint, configTimepoint));

    CPPUNIT_ASSERT_EQUAL(99, conf.get<int>("int32Property"));

    CPPUNIT_ASSERT_EQUAL(std::string("with\nnewline99"), conf.get<std::string>("stringProperty"));

    CPPUNIT_ASSERT_EQUAL(std::vector<std::string>({"abc99", "xy|z99", "A\nB99"}),
                         conf.get<std::vector<std::string> >("vectors.stringProperty"));
    CPPUNIT_ASSERT_EQUAL(std::vector<Hash>({Hash("e1", "ab\nc99", "e2", false, "e3", 12 * 99, "e4", 0.9837F * 99, "e5", 1.2345 * 99),
                                           Hash("e1", "xy|z99", "e2", true, "e3", 42 * 99, "e4", 2.33333F * 99, "e5", 7.77777 * 99)}),
                         conf.get<std::vector<Hash> >("table"));
    std::clog << "\n... Ok (retrieved configuration with last known value for 'int32Property', 'stringProperty', "
            << "'vectors.stringProperty', and 'table')." << std::endl;

    CPPUNIT_ASSERT_EQUAL(kLastValueSet, conf.get<int>("int32Property"));
    CPPUNIT_ASSERT_EQUAL(true, configAtTimepoint);

    std::vector<std::string> confKeys;
    conf.getKeys(confKeys);
    Epochstamp latestTimestamp(0, 0);
    for (const auto path : confKeys) {
        const Hash::Node &propNode = conf.getNode(path);
        if (propNode.hasAttribute("sec") && propNode.hasAttribute("frac")) {
            auto propSec = propNode.getAttribute<unsigned long long>("sec");
            auto propFrac = propNode.getAttribute<unsigned long long>("frac");
            Epochstamp propTimestamp(propSec, propFrac);
            if (propTimestamp > latestTimestamp) {
                latestTimestamp = propTimestamp;
            }
        }
    }
    CPPUNIT_ASSERT_EQUAL(latestTimestamp.toIso8601(), configTimepoint);
    std::clog << "\n... "
            << "Ok (retrieved configuration with last known value for 'int32Property' while the device was being logged)."
            << std::endl;

    // killDevice waits for the device to be killed (or throws an exception in case of failure).
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->killDevice(m_deviceId, KRB_TEST_MAX_TIMEOUT));

    // Assures that the logger in charge of the device is not logging it anymore by testing that m_deviceId is not
    // among the rows of the "lastUpdatesUtc" property of the logger. The "flush" slot guarantees that the property
    // "lastUpdatesUtc" is in sync with devices being logged.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                            .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    const auto lastUpdates =
            m_deviceClient->get<std::vector < Hash >> (karabo::util::DATALOGGER_PREFIX + m_server, "lastUpdatesUtc");
    bool deviceIdFound = false;
    for (const Hash& entry : lastUpdates) {
        if (entry.get<std::string>("deviceId") == m_deviceId) {
            deviceIdFound = true;
            break;
        }
    }
    CPPUNIT_ASSERT_EQUAL(false, deviceIdFound);

    // Waits a while before retrieving the configuration for a timepoint where the device is guaranteed to be offline.
    // There is an interval between the device being killed and the event that it is gone reaching the logger - the
    // delay decreases the chances of the timepoint used in the request for configuration from past to precede the
    // timestamp associated to the device shutdown event.
    boost::this_thread::sleep(boost::posix_time::milliseconds(1250));

    Epochstamp afterDeviceGone;
    std::clog << "... after device being logged is gone (requested config at " << afterDeviceGone.toIso8601() << ") ...";
    // At the afterDeviceGone timepoint, a last known configuration should be obtained with the last value set in the
    // previous test cases for the 'int32Property' - even after the device being logged is gone.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast",
                                               m_deviceId, afterDeviceGone.toIso8601())
                            .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(conf, schema, configAtTimepoint, configTimepoint));

    CPPUNIT_ASSERT_EQUAL(kLastValueSet, conf.get<int>("int32Property"));
    CPPUNIT_ASSERT_EQUAL(false, configAtTimepoint);
    karabo::util::Epochstamp configStamp(configTimepoint);
    CPPUNIT_ASSERT(configStamp > beforeAnything);
    CPPUNIT_ASSERT(configStamp < afterDeviceGone);
    std::clog << "\n... "
            << "Timestamp of retrieved configuration: " << configTimepoint << "\n "
            << "Ok (retrieved configuration with last known value for 'int32Property' while the device was not being logged)."
            << std::endl;

}


void DataLogging_Test::testCfgFromPastRestart() {
    std::clog << "Testing past configuration retrieval with stamp older than device..." << std::flush;

    // Start device and take care that the logger is ready for it
    const std::string deviceId("deviceWithOldStamp");
    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    auto success = m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId),
                                               KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    CPPUNIT_ASSERT_MESSAGE(toString(m_deviceClient->get<std::vector < std::string >> (loggerId, "devicesToBeLogged")),
                           waitForCondition([this, &loggerId, &deviceId]() {
                               auto loggedIds = m_deviceClient->get<std::vector < std::string >> (loggerId, "devicesToBeLogged");
                                            return (std::find(loggedIds.begin(), loggedIds.end(), deviceId) != loggedIds.end());
                           },
                                            KRB_TEST_MAX_TIMEOUT * 1000)
                           );

    // few cycles: increase value, stop and increase logging
    const unsigned int numCycles = 5;
    std::vector<Epochstamp> stampsAfter; // stamps after increasing value
    std::vector<Epochstamp> valueStamps; // stamps of the updated values
    const Epochstamp oldStamp("19990101T120000.000000Z");
    for (unsigned int i = 0; i < numCycles; ++i) {

        // Increase "variable" value and store after increasing it
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(deviceId, "slotIncreaseValue", KRB_TEST_MAX_TIMEOUT));
        stampsAfter.push_back(Epochstamp());

        // Get configuration, check expected values, check (static) time stamp of "oldValue" and store stamp of "value"
        Hash cfg;
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(deviceId, cfg));
        CPPUNIT_ASSERT_EQUAL(i + 1, cfg.get<int>("value"));

        CPPUNIT_ASSERT_EQUAL(99, cfg.get<int>("oldValue"));
        const Epochstamp stamp = Epochstamp::fromHashAttributes(cfg.getAttributes("oldValue"));
        CPPUNIT_ASSERT_MESSAGE("'oldValue' has wrong time stamp: " + stamp.toIso8601(), stamp == oldStamp);
        valueStamps.push_back(Epochstamp::fromHashAttributes(cfg.getAttributes("value")));

        // Stop logging our device and check that it is not logged anymore.
        // Sleep needed before flush to ensure that - for file logger - the output stream has actually seen the data
        boost::this_thread::sleep(boost::posix_time::milliseconds(250)); // locally 100 was always enough
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                                .timeout(KRB_TEST_MAX_TIMEOUT * 1000).receive());
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(loggerId, "slotTagDeviceToBeDiscontinued", "D", deviceId)
                                .timeout(KRB_TEST_MAX_TIMEOUT * 1000).receive());
        CPPUNIT_ASSERT_MESSAGE(toString(m_deviceClient->get<std::vector < std::string >> (loggerId, "devicesToBeLogged")),
                               waitForCondition([this, &loggerId, &deviceId]() {
                                   auto loggedIds = m_deviceClient->get<std::vector < std::string >> (loggerId, "devicesToBeLogged");
                                                // NOT in there anymore
                                                return (std::find(loggedIds.begin(), loggedIds.end(), deviceId) == loggedIds.end());
                               },
                                                KRB_TEST_MAX_TIMEOUT * 1000)
                               );

        // Restart again (and validate it is logging) - file based logger will gather the complete config again on disk
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(loggerId, "slotAddDevicesToBeLogged", vector<string>(1, deviceId))
                                .timeout(KRB_TEST_MAX_TIMEOUT * 1000).receive());
        CPPUNIT_ASSERT_MESSAGE(toString(m_deviceClient->get<std::vector < std::string >> (loggerId, "devicesToBeLogged")),
                               waitForCondition([this, &loggerId, &deviceId]() {
                                   auto loggedIds = m_deviceClient->get<std::vector < std::string >> (loggerId, "devicesToBeLogged");
                                                return (std::find(loggedIds.begin(), loggedIds.end(), deviceId) != loggedIds.end());
                               },
                                                KRB_TEST_MAX_TIMEOUT * 1000)
                               );
    }

    // Again flush - at the end of the last cycle we started logging again and archive_index.txt and archive_<N>.txt
    // might be out of sync otherwise - nevertheless for file based logging we need the repeated retries below for the
    // same reason as the sleeps above. :-(
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                            .timeout(KRB_TEST_MAX_TIMEOUT * 1000).receive());

    // Now check that for all stored stamps, the stamps gathered for the reader are correct
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
    for (unsigned int i = 0; i < numCycles; ++i) {
        // Time stamp after increasing value
        const Epochstamp& stampAfter = stampsAfter[i];

        // Gather full configuration (repeat until success, see above)
        Hash conf;
        Schema schema;
        int nTries = 100;
        int nChecks = 0;

        while (nTries >= 0 && conf.size() == 0) {
            try {
                nChecks++;
                m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast",
                                   deviceId, stampAfter.toIso8601())
                        .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                        .receive(conf, schema);
            } catch (const RemoteException& re) {
                CPPUNIT_ASSERT_MESSAGE("Unexpected RemoteException received: " + std::string(re.what()),
                                       re.detailedMsg().find("earlier than anything logged") != std::string::npos);
            } catch (const TimeoutException &te) {
                karabo::util::Exception::clearTrace();
            }
            boost::this_thread::sleep(boost::posix_time::milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
            nTries--;
        }

        CPPUNIT_ASSERT_MESSAGE("Failed to retrieve a non-empty configuration for device '" + m_deviceId +
                               "' after " + toString(nChecks) + " attempts.",
                               conf.size() > 0);
        CPPUNIT_ASSERT_EQUAL(99, conf.get<int>("oldValue"));
        CPPUNIT_ASSERT_EQUAL(i + 1, conf.get<int>("value")); // +1: stamp is after update

        // Check received stamps: The one of "oldValue is always the same, for "value" be aware that we store with
        // microsec precision only: we might be 1 off since we cut off digits instead of rounding
        const Epochstamp stampOldFromPast = Epochstamp::fromHashAttributes(conf.getAttributes("oldValue"));
        CPPUNIT_ASSERT_MESSAGE("'oldValue' from past has wrong time stamp: " + stampOldFromPast.toIso8601(), stampOldFromPast == oldStamp);
        const Epochstamp stampValueFromPast = Epochstamp::fromHashAttributes(conf.getAttributes("value"));
        CPPUNIT_ASSERT_MESSAGE(stampValueFromPast.toIso8601() + " vs " + valueStamps[i].toIso8601(),
                               (stampValueFromPast - valueStamps[i]).getFractions(TIME_UNITS::MICROSEC) <= 1ull);

    }

    std::clog << "OK" << std::endl;
}


void DataLogging_Test::testNoInfluxServerHandling() {

    std::clog << "Testing handling of no Influx Server available scenarios ..." << std::endl;

    std::pair<bool, std::string> success = m_deviceClient->instantiate(m_server, "PropertyTest",
                                                                       Hash("deviceId", m_deviceId),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Starts the loggers with an invalid InfluxDB Server url.
    success = startLoggers("InfluxDataLogger", true);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();

    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);

    // Any attempt to recover a configuration from Influx should fail if the Influx Server is not
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
        m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast",
                           m_deviceId, withNoServer.toIso8601())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(conf, schema, cfgAtTime, cfgTime);
    } catch (const karabo::util::RemoteException &exc) {
        CPPUNIT_ASSERT(exc.detailedMsg().find("No connection to InfluxDb server available") != std::string::npos);
        remoteExceptionCaught = true;
    }

    CPPUNIT_ASSERT(remoteExceptionCaught);

    std::clog << "... request failed with RemoteException as expected." << std::endl;

    // By simply starting the devices related to Influx logging, some logging writing activity takes place.
    // If this point of the test is reached with an invalid url configured for the Influx Server, it is safe
    // to conclude that the Influx Logger doesn't get stuck when no server is available.

    std::clog << "OK" << std::endl;
}


template <class T>
void DataLogging_Test::testHistory(const std::string& key, const std::function<T(int)> &f,
                                   const bool testConf) {
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
    const int max_set = 100;
    std::clog << "Testing Property History retrieval for '" << key << "'... " << std::flush;

    // get configuration for later checks
    Hash beforeWritesCfg;
    CPPUNIT_ASSERT_NO_THROW((m_deviceClient->get(m_deviceId, beforeWritesCfg)));

    boost::this_thread::sleep(boost::posix_time::milliseconds(150));

    // save this instant as a iso string
    Epochstamp es_beforeWrites;
    std::string beforeWrites = es_beforeWrites.toIso8601();

    // write a bunch of times
    for (int i = 0; i < max_set; i++) {
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set<T>(m_deviceId, key, f(i)));
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }

    Hash afterWritesCfg;
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(m_deviceId, afterWritesCfg));

    // save this instant as a iso string
    Epochstamp es_afterWrites;
    std::string afterWrites = es_afterWrites.toIso8601();

    // waits a little for the logged changes to be available for reading - this is specially
    // important for the InfluxDb case. If there's not enough time, the assertions below on
    // the number of entries in the response to slotGetPropertyHistory won't match.
    boost::this_thread::sleep(boost::posix_time::milliseconds(m_flushIntervalSec*1000 + 1500));

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

    std::vector<std::string> exceptionsMsgs;

    int nTries = 100;
    unsigned int numExceptions = 0;
    unsigned int numChecks = 0;
    while (nTries >= 0 && history.size() != max_set) {
        try {
            numChecks++;
            // TODO: use the deviceClient to retrieve the property history
            //history = m_deviceClient->getPropertyHistory(m_deviceId, key, before, after, max_set * 2);
            m_sigSlot->request(dlreader0, "slotGetPropertyHistory", m_deviceId, key, params)
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(device, property, history);
        } catch (const karabo::util::TimeoutException& e) {
            karabo::util::Exception::clearTrace();
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
        } catch (const karabo::util::RemoteException& e) {
            karabo::util::Exception::clearTrace();
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
        nTries--;
    }

    CPPUNIT_ASSERT_EQUAL_MESSAGE("History size different than expected after " + toString(numChecks) +
                                 " checks:\n\tdeviceId: " + m_deviceId + "\n\tparam.from: " + beforeWrites +
                                 "\n\tparam.to: " + afterWrites + "\n\tparam.maxNumData: " + toString(max_set * 2) +
                                 "\n\thistory.size(): " + toString(history.size()) +
                                 "\n\tNumber of Exceptions: " + toString(numExceptions) +
                                 "\n\tExceptions:\n" + boost::algorithm::join(exceptionsMsgs, "\n"),
                                 static_cast<size_t> (max_set), history.size());

    for (int i = 0; i < max_set; i++) {
        // checking values and timestamps
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong value in history", f(i), history[i].get<T>("v"));
        Epochstamp current = Epochstamp::fromHashAttributes(history[i].getAttributes("v"));
        CPPUNIT_ASSERT_MESSAGE("Timestamp later than the requested window", current <= es_afterWrites);
        CPPUNIT_ASSERT_MESSAGE("Timestamp earlier than the requested window", current >= es_beforeWrites);
        if (i>0) {
            Epochstamp previous = Epochstamp::fromHashAttributes(history[i - 1].getAttributes("v"));
            CPPUNIT_ASSERT_MESSAGE("Timestamp earlier than the requested window", current > previous);
        }
    }
    std::clog << "Ok" << std::endl;

    // skip the configuration retrieval
    if (!testConf) return;

    std::clog << "Testing past configuration retrieval for '" << key << "'... " << std::flush;

    exceptionsMsgs.clear();

    nTries = 100;
    numExceptions = 0;
    numChecks = 0;
    // place holder schema, could be checked in future tests
    Schema schema;
    Hash conf;
    while (nTries >= 0 && conf.size() == 0) {
        // TODO: use the deviceClient to retrieve the configuration from past
        // auto pair = m_deviceClient->getConfigurationFromPast(m_deviceId, before);
        // conf = pair.first;
        conf.clear();
        try {
            numChecks++;
            m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", m_deviceId, beforeWrites)
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(conf, schema);
        } catch (const karabo::util::TimeoutException &e) {
            karabo::util::Exception::clearTrace();
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
        } catch (const karabo::util::RemoteException &e) {
            karabo::util::Exception::clearTrace();
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
        nTries--;
    }

    CPPUNIT_ASSERT_MESSAGE("Configuration size from slotGetConfigurationFromPast still 0 after " + toString(numChecks) +
                           " checks.\n\tdeviceId: " + m_deviceId + "\n\tparam.before: " + beforeWrites +
                           "\n\tconf.size(): " + toString(conf.size()) +
                           "\n\tNumber of Exceptions: " + toString(numExceptions) +
                           "\n\tExceptions:\n" + boost::algorithm::join(exceptionsMsgs, "\n"),
                           conf.size() > 0);
    // One needs to check only the content here, therefore only the leaves are examined
    std::vector<std::string> leaves;
    getLeaves(conf, schema, leaves, '.');
    for (const std::string & leaf : leaves) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong configuration from past (before writes) for key :" + leaf,
                                     beforeWritesCfg.getAs<std::string>(leaf),
                                     conf.getAs<std::string>(leaf));
    }

    nTries = 100;
    numExceptions = 0;
    numChecks = 0;
    conf.clear();
    while (nTries >= 0 && conf.size() == 0) {
        try {
            // TODO: use the deviceClient to retrieve the configuration from past
            // auto pair = m_deviceClient->getConfigurationFromPast(m_deviceId, before);
            // conf = pair.first
            numChecks++;
            m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", m_deviceId, afterWrites)
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(conf, schema);
        } catch (const karabo::util::TimeoutException& e) {
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
        } catch (const karabo::util::RemoteException &e) {
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
        nTries--;
    }
    CPPUNIT_ASSERT_MESSAGE("Configuration still not retrieved after  " + toString(numChecks) +
                           " checks.\n\tdeviceId: " + m_deviceId + "\n\tparam.before: " + beforeWrites +
                           "\n\tconf.size(): " + toString(conf.size()) +
                           "\n\tNumber of Exceptions: " + toString(numExceptions) +
                           "\n\tRemote Errors:\n" + boost::algorithm::join(exceptionsMsgs, "\n"),
                           conf.size() > 0);
    // One needs to check only the content here, therefore only the leaves are examined
    leaves.clear();
    getLeaves(conf, schema, leaves, '.');
    for (const std::string & leaf : leaves) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong configuration from past (after) for key :" + leaf,
                                     afterWritesCfg.getAs<std::string>(leaf),
                                     conf.getAs<std::string>(leaf));
    }

    std::clog << "Ok" << std::endl;
}


void DataLogging_Test::testInt(bool testPastConf) {
    auto lambda = [] (int i) -> int {
        return i;
    };
    testHistory<int>("int32Property", lambda, testPastConf);
}


void DataLogging_Test::testFloat(bool testPastConf) {
    auto lambda = [] (int i) -> float {
        return 2.5e-8f * i;
    };
    testHistory<float>("floatProperty", lambda, testPastConf);
}


void DataLogging_Test::testString(bool testPastConf) {
    auto lambda = [] (int i) -> string {
        return "ab|c" + karabo::util::toString(i);
    };
    testHistory<string>("stringProperty", lambda, testPastConf);

    // Also test a string with a new line character
    auto lambda2 = [] (int i) -> string {
        return "with\nnewline" + karabo::util::toString(i);
    };
    testHistory<string>("stringProperty", lambda2, testPastConf);
}


void DataLogging_Test::testVectorString(bool testPastConf) {
    auto lambda = [] (int i) -> vector<string> {
        // Also test pipe '|' (the separator in our text files) and new line '\n'
        vector<string> v = {"abc" + toString(i), "xy|z" + toString(i), "A\nB" + toString(i)};
        return v;
    };
    testHistory<vector < string >> ("vectors.stringProperty", lambda, testPastConf);
}


void DataLogging_Test::testTable(bool testPastConf) {
    auto lambda = [] (int i) -> vector<Hash> {
        vector<Hash> t = {// For strings, test also pipe '|' (the separator in our text files) and newline '\n'.
                          Hash("e1", "ab\nc" + karabo::util::toString(i), "e2", ((i % 2) == 0),
                               "e3", 12 * i, "e4", 0.9837F * i, "e5", 1.2345 * i),
                          Hash("e1", "xy|z" + karabo::util::toString(i), "e2", ((i % 2) == 1),
                               "e3", 42 * i, "e4", 2.33333F * i, "e5", 7.77777 * i)
        };
        return t;
    };
    testHistory<vector<Hash> >("table", lambda, testPastConf);
}

// TODO: ideally, all properties of the PropertyTest device should be implemented,
//       to add them one should add a method per property and add the proper cppunit helpers at the beginning of this
//       file


