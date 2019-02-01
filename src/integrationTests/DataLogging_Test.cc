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

DataLogging_Test::DataLogging_Test(): m_server("DataLoggingTestServer"), m_deviceId("PropertyTestDevice"){
}


DataLogging_Test::~DataLogging_Test() {
}


void DataLogging_Test::setUp() {
    // Uncomment to run with a local broker:
    //setenv("KARABO_BROKER", "tcp://localhost:7777", true);

    // Start central event-loop
    m_eventLoopThread = boost::thread(boost::bind(&EventLoop::work));

    // Create and start server
    Hash config("serverId", m_server, "scanPlugins", false, "Logger.priority", "ERROR");
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
}


void DataLogging_Test::testAllInstantiated() {
    std::clog << "Testing deviceInstantiation... " << std::flush;
    int timeout = 1500; // milliseconds
    vector<string> devices;
    devices.push_back(karabo::util::DATALOGGER_PREFIX + m_deviceId);
    devices.push_back(karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server));
    devices.push_back(karabo::util::DATALOGREADER_PREFIX + ("1-" + m_server));
    while (timeout > 0) {
        const Hash& topo = m_deviceClient->getSystemTopology();
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
    CPPUNIT_ASSERT_MESSAGE("History size different than expected", max_set == history.size());
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
}


void DataLogging_Test::testVectorString() {
    auto lambda = [] (int i) -> vector<string> {
        vector<string> v = {"abc" + karabo::util::toString(i), "xy|z" + karabo::util::toString(i)};
        return v;
    };
    testHistory<vector < string >> ("vectors.stringProperty", lambda, false);
}


void DataLogging_Test::testTable() {
    auto lambda = [] (int i) -> vector<Hash> {
        vector<Hash> t = {
                          Hash("e1", "abc" + karabo::util::toString(i), "e2", ((i % 2) == 0),
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


