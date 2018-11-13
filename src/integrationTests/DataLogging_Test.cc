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
#include <sstream>

USING_KARABO_NAMESPACES;
using std::vector;
using std::string;

#define KRB_TEST_MAX_TIMEOUT 10

// adding vector<Hash>, Hash, and vector<string> helpers for CppUnit
namespace CppUnit{
    template <>
    struct assertion_traits<std::vector<karabo::util::Hash>>{
        static bool equal(const std::vector<karabo::util::Hash> &a, const std::vector<karabo::util::Hash> &b){
            return karabo::util::similar(a, b);
        }
        static std::string toString(const std::vector<karabo::util::Hash> &p){
            std::ostringstream o;
            o << "(" << std::endl;
            for (const karabo::util::Hash& h : p){
                o << h << "," << std::endl;
            }
            o << ")";
            return o.str();
        }
    };

    template <>
    struct assertion_traits<karabo::util::Hash>{
        static bool equal(const karabo::util::Hash &a, const karabo::util::Hash &b){
            return karabo::util::similar(a, b);
        }
        static std::string toString(const karabo::util::Hash &p){
            std::ostringstream o;
            o << p << std::endl;
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
}


CPPUNIT_TEST_SUITE_REGISTRATION(DataLogging_Test);

DataLogging_Test::DataLogging_Test(): m_server("DataLoggingTestServer"), m_deviceId("PropertyTest"){
}


DataLogging_Test::~DataLogging_Test() {
}


void DataLogging_Test::setUp() {
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

    EventLoop::stop();
    m_eventLoopThread.join();
}


void DataLogging_Test::allTestRunner() {
    std::pair<bool, std::string> success = m_deviceClient->instantiate(m_server, "PropertyTest",
                                                                       Hash("deviceId", m_deviceId),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    Hash manager_conf;
    manager_conf.set("deviceId", "loggerManager");
    manager_conf.set<unsigned int>("flushInterval", 1);
    manager_conf.set<string>("directory", "karaboHistory");
    manager_conf.set<vector<string>>("serverList", {m_server});
    success = m_deviceClient->instantiate(m_server, "DataLoggerManager",
                                                                       manager_conf,
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    testAllInstantiated();
    testInt();
    testString();
    testVectorString();
    testTable();
    testFloat();
}


void DataLogging_Test::testAllInstantiated() {
    std::clog << "Testing deviceInstantiation... ";
    int timeout = 1500; // milliseconds
    vector<string> devices;
    devices.push_back("DataLogger-" + m_deviceId);
    devices.push_back("DataLogReader1-" + m_server);
    devices.push_back("DataLogReader0-" + m_server);
    while (timeout > 0){
        const Hash& topo = m_deviceClient->getSystemTopology();
        CPPUNIT_ASSERT(topo.has("device"));
        const Hash& device = topo.get<Hash>("device");
        bool allUp = true;
        for (const string & deviceId : devices){
            allUp = allUp && device.has(deviceId);
        }
        if (allUp) break;
    }
    CPPUNIT_ASSERT_MESSAGE("Timeout while waiting for datalogging to be instantiated", timeout>0);
    std::clog << "Ok" << std::endl;
}

template <class T>
void DataLogging_Test::testHistory(const string& key, const std::function<T(int)> &f){
    std::clog << "Testing Property History retrieval for '" << key << "' ...";
    // get configuration for later checks
    Hash beforeConf;
    m_deviceClient->get(m_deviceId, beforeConf);
    // get this timepoint as a iso string
    Epochstamp es;
    es.now();
    string before = es.toIso8601();

    // write a bunch of times
    for (int i = 0; i< 100 ;  i++ ){
        m_deviceClient->set<T>(m_deviceId, key, f(i));
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }

    boost::this_thread::sleep(boost::posix_time::milliseconds(1500)); // more than the flush time
    Hash afterConf;
    m_deviceClient->get(m_deviceId, afterConf);
    string deviceId;
    string property;
    vector<Hash> history;
    es.now();
    string after = es.toIso8601();
    Hash params;
    params.set("from", before);
    params.set("to", after);
    params.set<int>("maxNumData", 1000); // more than 100!
    int timeout = 2000;
    while (timeout >0 && history.size()==0) {
        m_sigSlot->request("DataLogReader1-" + m_server, "slotGetPropertyHistory", m_deviceId, key ,params)
                .timeout(200).receive<string, string, vector<Hash>>(deviceId, property, history);
        boost::this_thread::sleep(boost::posix_time::milliseconds(200));
        timeout -= 200;
    }
    CPPUNIT_ASSERT_MESSAGE("Timeout while getting property history", timeout > 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Not able to retrieve history", history.size(), 100ul);
    for (int i = 0; i< 100 ;  i++ ){
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong value in history", f(i), history.at(i).get<T>("v"));
    }
    std::clog << "Ok" << std::endl;
    //return;
    std::clog << "Testing past configuration retrieval for '" << key << "' ...";

    timeout = 2000;
    Schema schema;
    Hash conf;
    vector<string> paths;
    while (timeout >0 && paths.size()==0) {
        conf.clear();
        m_sigSlot->request("DataLogReader0-" + m_server, "slotGetConfigurationFromPast", m_deviceId, before)
                .timeout(200).receive<Hash, Schema>(conf, schema);
        boost::this_thread::sleep(boost::posix_time::milliseconds(200));
        timeout -= 200;
        conf.getPaths(paths);
    }
    CPPUNIT_ASSERT_MESSAGE("Timeout while getting configuration from past", timeout > 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong configuration from past", beforeConf, conf);

    timeout = 2000;
    while (timeout >0 && !conf.empty()) {
        conf.clear();
        m_sigSlot->request("DataLogReader0-" + m_server, "slotGetConfigurationFromPast", m_deviceId, after)
                .timeout(200).receive<Hash, Schema>(conf, schema);
        boost::this_thread::sleep(boost::posix_time::milliseconds(200));
        timeout -= 200;
    }
    CPPUNIT_ASSERT_MESSAGE("Timeout while getting configuration from past", timeout > 0);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong configuration from past", afterConf, conf);
    std::clog << "Ok" << std::endl;
}


void DataLogging_Test::testInt(){
    std::function<int(int)> lambda = [] (int i) {return i;};
    testHistory<int>("int32Property", lambda);
}


void DataLogging_Test::testFloat(){
    std::function<float(int)>  lambda = [] (int i) {return 2.5*i;};
    testHistory<float>("floatProperty", lambda);
}


void DataLogging_Test::testString(){
    std::function<string(int)>  lambda = [] (int i) {
        return "ab|c"+karabo::util::toString(i);
    };
    testHistory<string>("stringProperty", lambda);
}


void DataLogging_Test::testVectorString(){
    std::function<vector<string>(int)>  lambda = [] (int i) {
        vector<string> v = {"abc"+karabo::util::toString(i), "xy|z"+karabo::util::toString(i)};
        return v;
    };
    testHistory<vector<string>>("vectors.stringProperty", lambda);
}


void DataLogging_Test::testTable(){
    std::function<vector<Hash>(int)>  lambda = [] (int i) {
        vector<Hash> t = {
            Hash("e1", "abc"+karabo::util::toString(i),  "e2", ((i % 2) == 0),
                 "e3", 12*i, "e4", 0.9837F * i, "e5", 1.2345 * i),
            Hash("e1", "xy|z"+karabo::util::toString(i), "e2", ((i % 2) == 1),
                 "e3", 42*i, "e4", 2.33333F * i, "e5", 7.77777 * i)};
        return t;
    };
    testHistory<vector<Hash>>("table", lambda);
}



