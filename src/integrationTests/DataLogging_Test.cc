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
#include <boost/filesystem.hpp>
#include <sstream>

USING_KARABO_NAMESPACES;
using std::vector;
using std::string;
using karabo::util::toString;

#define KRB_TEST_MAX_TIMEOUT 10

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


DataLogging_Test::DataLogging_Test() : m_server("DataLoggingTestServer"), m_deviceId("PropertyTestDevice"),
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
    m_deviceClient = boost::shared_ptr<DeviceClient>(new DeviceClient());
    m_sigSlot = boost::make_shared<SignalSlotable>("sigSlot");
    m_sigSlot->start();
}


void DataLogging_Test::tearDown() {
    m_deviceClient.reset();
    m_deviceServer.reset();
    m_sigSlot.reset();
    EventLoop::stop();
    m_eventLoopThread.join();

    // Clean up directory - you may want to comment out these lines for debugging
    boost::filesystem::remove("loggermap.xml");
    boost::filesystem::remove_all("dataLoggingTest");

    if (m_changedPath) {
        if (m_oldPath.empty()) {
            unsetenv("PATH");
        } else {
            setenv("PATH", m_oldPath.data(), 1);
        }
    }
}


void DataLogging_Test::allTestRunner() {
    std::pair<bool, std::string> success = m_deviceClient->instantiate(m_server, "PropertyTest",
                                                                       Hash("deviceId", m_deviceId),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    Hash manager_conf;
    manager_conf.set("deviceId", "loggerManager");
    manager_conf.set("flushInterval", m_flushIntervalSec);
    manager_conf.set("directory", "dataLoggingTest/karaboHistory");
    manager_conf.set<vector<string>>("serverList", {m_server});
    success = m_deviceClient->instantiate(m_server,
                                          "DataLoggerManager", manager_conf, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();
    testInt();
    testFloat();
    testString();
    testVectorString();
    testTable();
    testHistoryAfterChanges();
    // This must be the last test case - it stops the device being logged to make sure that the
    // last known configuration can be successfully retrieved after the device is gone.
    testLastKnownConfiguration();
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
        for (const string & deviceId : devices){
            allUp = allUp && device.has(deviceId);
        }
        if (allUp) break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(50));
        timeout -= 50;
    }
    CPPUNIT_ASSERT_MESSAGE("Timeout while waiting for datalogging to be instantiated", timeout>0);
    std::clog << "Ok" << std::endl;
}


void DataLogging_Test::testHistoryAfterChanges() {

    const string propertyName("int32Property");
    const string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
    const int max_set = 100;

    std::clog << "Testing Property History retrieval after changes for '" << propertyName << "'... " << std::flush;

    // write a bunch of times
    for (int i = 0; i < max_set; i++) {
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set<int>(m_deviceId, propertyName, i));
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }

    Epochstamp es_after;
    string after = es_after.toIso8601();
    Epochstamp es_wayAfter(es_after.getSeconds() + 60, es_after.getFractionalSeconds());
    string wayAfter = es_wayAfter.toIso8601();

    boost::this_thread::sleep(boost::posix_time::milliseconds(m_flushIntervalSec * 1500)); // wait more than the flush time

    // placeholders, could be skipped but they are here for future expansions of the tests
    string device;
    string property;
    vector<Hash> history;
    Hash params;
    params.set<string>("from", after);
    params.set<string>("to", wayAfter);
    params.set<int>("maxNumData", max_set * 2);

    // FIXME: refactor this once indexing is properly handled.
    // the history retrieval might take more than one try, it could have to index the files.
    int timeout = 10000;
    unsigned int numTimeouts = 0;
    while (timeout >= 0 && history.size() < 1) {
        try {
            m_sigSlot->request(dlreader0, "slotGetPropertyHistory", m_deviceId, propertyName, params)
                    .timeout(500).receive(device, property, history);
        } catch (const karabo::util::TimeoutException& e) {
            karabo::util::Exception::clearTrace();
            ++numTimeouts;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        timeout -= 1000;
    }
    CPPUNIT_ASSERT_EQUAL_MESSAGE("History size should be 1, got " + karabo::util::toString(history.size()) + ".",
                                 1, history.size());
    CPPUNIT_ASSERT_MESSAGE("Timeout while getting property history " + toString(numTimeouts), timeout >= 0);
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

    const int kRequestTimeoutMs = 15000;

    // Last value set in previous test cases for property 'int32Property'.
    const int kLastValueSet = 99;

    std::clog << "Testing last known configuration at specific timepoints ..." << std::endl;

    const string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);

    Schema schema;
    Hash conf;
    bool configAtTimepoint;

    std::clog << "... before any logging activity (at " << m_beforeAnything.toIso8601() << ") ...";
    // At the m_beforeAnything timepoint no known configuration existed, so an
    // empty configuration is expected.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast",
                                               m_deviceId, m_beforeAnything.toIso8601())
                            .timeout(kRequestTimeoutMs).receive(conf, schema, configAtTimepoint));

    CPPUNIT_ASSERT_MESSAGE("At timepoint BeforeAnything no last known configuration is expected.", conf.empty());
    CPPUNIT_ASSERT_EQUAL(false, configAtTimepoint);
    std::clog << "\n... Ok (no configuration retrieved)." << std::endl;

    Epochstamp rightBeforeDeviceGone;
    std::clog << "... right before killing device being logged (at " << rightBeforeDeviceGone.toIso8601() << ") ...";
    // At the rightBeforeDeviceGone timepoint, a last known configuration should be obtained with the last value set in
    // the  previous test cases for the 'int32Property' - even after the device being logged is gone.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast",
                                               m_deviceId, rightBeforeDeviceGone.toIso8601())
                            .timeout(kRequestTimeoutMs).receive(conf, schema, configAtTimepoint));

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
    std::clog << "\n... "
            << "Ok (retrieved configuration with last known value for 'int32Property' while the device was being logged)."
            << std::endl;

    // killDevice waits for the device to be killed (or throws an exception in case of failure).
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->killDevice(m_deviceId, KRB_TEST_MAX_TIMEOUT));

    // Assures that the logger in charge of the device is not logging it anymore by testing that m_deviceId is not
    // among the rows of the "lastUpdatesUtc" property of the logger. The "flush" slot guarantees that the property
    // "lastUpdatesUtc" is in sync with devices being logged.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                            .timeout(kRequestTimeoutMs).receive());
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

    Epochstamp afterDeviceGone;
    std::clog << "... after device being logged is gone (at " << afterDeviceGone.toIso8601() << ") ...";
    // At the afterDeviceGone timepoint, a last known configuration should be obtained with the last value set in the
    // previous test cases for the 'int32Property' - even after the device being logged is gone.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast",
                                               m_deviceId, afterDeviceGone.toIso8601())
                            .timeout(kRequestTimeoutMs).receive(conf, schema, configAtTimepoint));

    CPPUNIT_ASSERT_EQUAL(kLastValueSet, conf.get<int>("int32Property"));
    CPPUNIT_ASSERT_EQUAL(false, configAtTimepoint);
    std::clog << "\n... "
            << "Ok (retrieved configuration with last known value for 'int32Property' while the device was not being logged)."
            << std::endl;

}


template <class T>
void DataLogging_Test::testHistory(const string& key, const std::function<T(int)> &f, const bool testConf) {
    const string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
    const string dlreader1 = karabo::util::DATALOGREADER_PREFIX + ("1-" + m_server);
    const int max_set = 100;
    std::clog << "Testing Property History retrieval for '" << key << "'... " << std::flush;
    // get configuration for later checks
    Hash beforeConf;
    CPPUNIT_ASSERT_NO_THROW((m_deviceClient->get(m_deviceId, beforeConf)));
    // save this instant as a iso string
    Epochstamp es_before;
    string before = es_before.toIso8601();

    // write a bunch of times
    for (int i = 0; i < max_set; i++) {
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set<T>(m_deviceId, key, f(i)));
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }

    Hash afterConf;
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(m_deviceId, afterConf));
    Epochstamp es_after;
    string after = es_after.toIso8601();

    // save this instant as a iso string

    boost::this_thread::sleep(boost::posix_time::milliseconds(m_flushIntervalSec * 1500)); // wait more than the flush time

    // place holders, could be skipped but they are here for future expansions of the tests
    string device;
    string property;
    vector<Hash> history;
    Hash params;
    params.set<string>("from", before);
    params.set<string>("to", after);
    params.set<int>("maxNumData", max_set * 2);
    // the history retrieval might take more than one try, it could have to index the files.
    int timeout = 20000;
    unsigned int numTimeouts = 0;
    while (timeout >= 0 && history.size() == 0) {
        try {
            // TODO: use the deviceClient to retrieve the property history
            //history = m_deviceClient->getPropertyHistory(m_deviceId, key, before, after, max_set * 2);
            m_sigSlot->request(dlreader0, "slotGetPropertyHistory", m_deviceId, key, params)
                    .timeout(500).receive(device, property, history);
        } catch (const karabo::util::TimeoutException& e) {
            karabo::util::Exception::clearTrace();
            ++numTimeouts;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(200));
        timeout -= 200;
    }
    CPPUNIT_ASSERT_MESSAGE("Timeout while getting property history " + toString(numTimeouts), timeout >= 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("History size different than expected", max_set, history.size());
    for (int i = 0; i < max_set; i++) {
        // checking values and timestamps
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong value in history", f(i), history[i].get<T>("v"));
        Epochstamp current = Epochstamp::fromHashAttributes(history[i].getAttributes("v"));
        CPPUNIT_ASSERT_MESSAGE("Timestamp later than the requested window", current <= es_after);
        CPPUNIT_ASSERT_MESSAGE("Timestamp earlier than the requested window", current >= es_before);
        if (i>0) {
            Epochstamp previous = Epochstamp::fromHashAttributes(history[i - 1].getAttributes("v"));
            CPPUNIT_ASSERT_MESSAGE("Timestamp earlier than the requested window", current > previous);
        }
    }
    std::clog << "Ok" << std::endl;

    // skip the configuration retrieval
    if (!testConf) return;
    std::clog << "Testing past configuration retrieval for '" << key << "'... " << std::flush;

    timeout = 20000;
    numTimeouts = 0;
    // place holder schema, could be checked in future tests
    Schema schema;
    Hash conf;
    while (timeout >= 0 && conf.size() == 0) {
        // TODO: use the deviceClient to retrieve the configuration from past
        // auto pair = m_deviceClient->getConfigurationFromPast(m_deviceId, before);
        // conf = pair.first;
        conf.clear();
        try {
            m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", m_deviceId, before)
                    .timeout(500).receive(conf, schema);
        } catch (const karabo::util::TimeoutException& e) {
            karabo::util::Exception::clearTrace();
            ++numTimeouts;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(200));
        timeout -= 200;
    }
    CPPUNIT_ASSERT_MESSAGE("Timeout while getting configuration from past " + toString(numTimeouts), timeout >= 0);
    // One needs to check only the content here, therefore only the leaves are examined
    std::vector<std::string> leaves;
    getLeaves(conf, schema, leaves, '.');
    for (const std::string & leaf : leaves) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong configuration from past (before) for key :" + leaf,
                                     beforeConf.getAs<string>(leaf),
                                     conf.getAs<string>(leaf));
    }

    timeout = 20000;
    numTimeouts = 0;
    conf.clear();
    while (timeout >= 0 && conf.size() == 0) {
        try {
            // TODO: use the deviceClient to retrieve the configuration from past
            // auto pair = m_deviceClient->getConfigurationFromPast(m_deviceId, before);
            // conf = pair.first;
            m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", m_deviceId, after)
                    .timeout(500).receive(conf, schema);
        } catch (const karabo::util::TimeoutException& e) {
            karabo::util::Exception::clearTrace();
            ++numTimeouts;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(200));
        timeout -= 200;
    }
    CPPUNIT_ASSERT_MESSAGE("Timeout while getting configuration from past after settings " + toString(numTimeouts), timeout >= 0);
    // One needs to check only the content here, therefore only the leaves are examined
    leaves.clear();
    getLeaves(conf, schema, leaves, '.');
    for (const std::string & leaf : leaves) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong configuration from past (after) for key :" + leaf,
                                     afterConf.getAs<string>(leaf),
                                     conf.getAs<string>(leaf));
    }

    std::clog << "Ok" << std::endl;
}


void DataLogging_Test::testInt() {
    auto lambda = [] (int i) -> int {
        return i;
    };
    testHistory<int>("int32Property", lambda, true);
}


void DataLogging_Test::testFloat() {
    auto lambda = [] (int i) -> float {
        return 2.5e-8f * i;
    };
    testHistory<float>("floatProperty", lambda, false);
}


void DataLogging_Test::testString() {
    auto lambda = [] (int i) -> string {
        return "ab|c" + karabo::util::toString(i);
    };
    testHistory<string>("stringProperty", lambda, false);

    // Also test a string with a new line character
    auto lambda2 = [] (int i) -> string {
        return "with\nnewline" + karabo::util::toString(i);
    };
    testHistory<string>("stringProperty", lambda2, false);
}


void DataLogging_Test::testVectorString() {
    auto lambda = [] (int i) -> vector<string> {
        // Also test pipe '|' (the separator in our text files) and new line '\n'
        vector<string> v = {"abc" + toString(i), "xy|z" + toString(i), "A\nB" + toString(i)};
        return v;
    };
    testHistory<vector < string >> ("vectors.stringProperty", lambda, false);
}


void DataLogging_Test::testTable() {
    auto lambda = [] (int i) -> vector<Hash> {
        vector<Hash> t = {// For strings, test also pipe '|' (the separator in our text files) and newline '\n'.
                          Hash("e1", "ab\nc" + karabo::util::toString(i), "e2", ((i % 2) == 0),
                               "e3", 12 * i, "e4", 0.9837F * i, "e5", 1.2345 * i),
                          Hash("e1", "xy|z" + karabo::util::toString(i), "e2", ((i % 2) == 1),
                               "e3", 42 * i, "e4", 2.33333F * i, "e5", 7.77777 * i)
        };
        return t;
    };
    testHistory<vector<Hash> >("table", lambda, false);
}

// TODO: ideally, all properties of the PropertyTest device should be implemented,
//       to add them one should add a method per property and add the proper cppunit helpers at the beginning of this
//       file


