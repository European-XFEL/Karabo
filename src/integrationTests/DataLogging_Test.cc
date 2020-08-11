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
#include <cstdlib>
#include <sstream>
#include <karabo/util/StringTools.hh>

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

#define KRB_TEST_MAX_TIMEOUT 10 // seconds

/* Timeout, in milliseconds, for a slot request. */
#define SLOT_REQUEST_TIMEOUT_MILLIS 5000
#define FLUSH_REQUEST_TIMEOUT_MILLIS 20000

/* As the Telegraf environment has a higher request/response roundtrip time, use
   larger interval between retries. */
#define PAUSE_BEFORE_RETRY_INFLUX 300
#define PAUSE_BEFORE_RETRY_TELEGRAF 1000
#define PAUSE_BEFORE_RETRY_MILLIS (m_switchedToTelegrafEnv ? PAUSE_BEFORE_RETRY_TELEGRAF : PAUSE_BEFORE_RETRY_INFLUX)

#define NUM_RETRY_INFLUX 400
#define NUM_RETRY_TELEGRAF 1200  // TODO: Reduce this to 600 (10 minutes) once load on exflserv10 gets normal.
#define NUM_RETRY (m_switchedToTelegrafEnv ? NUM_RETRY_TELEGRAF : NUM_RETRY_INFLUX)

#define FLUSH_INTERVAL_SEC_INFLUX 1u
#define FLUSH_INTERVAL_SEC_TELEGRAF 20u
#define FLUSH_INTERVAL_SEC (m_switchedToTelegrafEnv ? FLUSH_INTERVAL_SEC_TELEGRAF : FLUSH_INTERVAL_SEC_INFLUX)

/* Interval, in milliseconds, to wait for a sequence of writes to complete before
   starting checking the write results - this is required to get an appropriate
   timestamp for the upper limit for the reading range. */
#define WAIT_WRITES_INFLUX 4000
#define WAIT_WRITES_TELEGRAF 8000
#define WAIT_WRITES (m_switchedToTelegrafEnv ? WAIT_WRITES_TELEGRAF : WAIT_WRITES_INFLUX)

static Epochstamp threeDaysBack = Epochstamp() - TimeDuration(3,0,0,0,0);

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

        SLOT_ELEMENT(expected).key("slotUpdateSchema")
                .commit();
    }

    DataLogTestDevice(const karabo::util::Hash& input) : karabo::core::Device<>(input) {
        KARABO_SLOT(slotIncreaseValue);
        KARABO_SLOT(slotUpdateSchema, const karabo::util::Schema);
        KARABO_INITIAL_FUNCTION(initialize);
    }


    virtual ~DataLogTestDevice() {
    }

private:

    void initialize() {
        // Set oldValue with time stamp from past - now - 3 days
        set("oldValue", 99, Timestamp(threeDaysBack, 0ull));

        updateState(State::ON);
    }

    void slotIncreaseValue() {
        set("value", get<int>("value") + 1);
    }


    void slotUpdateSchema(const Schema sch) {
        updateSchema(sch);
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
    struct assertion_traits<std::vector<unsigned char>>
    {


        static bool equal(const std::vector<unsigned char> &a, const std::vector<unsigned char> &b) {
            return a == b;
        }


        static std::string toString(const std::vector<unsigned char> &p) {
            // Cannot use 'return karabo::util::toString(p)' since that uses base64 encoding
            std::ostringstream o;
            o << "'";
            for (const unsigned char& e : p) {
                o << static_cast<unsigned int> (e) << ',';
            }
            o << "'";
            return o.str();
        }
    };

    template<>
    template < typename T>
    struct assertion_traits<std::vector<T>>
    {


        static bool equal(const std::vector<T>& a, const std::vector<T>& b) {
            return a == b;
        }


        static std::string toString(const std::vector<T>& p) {
            return karabo::util::toString(p);
        }
    };
}


CPPUNIT_TEST_SUITE_REGISTRATION(DataLogging_Test);

// Avoid test collision on CI by specifying a unique prefix.
static const std::string deviceIdPrefix = !getenv("KARABO_BROKER_TOPIC") ? "" : getenv("KARABO_BROKER_TOPIC");

DataLogging_Test::DataLogging_Test()
    : m_server("DataLoggingTestServer"),
    // Use platform-dependent name for the device: concurrent tests in CI operate
    // on the same InfluxDB database ...
    m_deviceId(deviceIdPrefix + "PropertyTestDevice"),
    m_fileLoggerDirectory("dataLoggingTest"),
    m_changedPath(false), m_oldPath() {

}


DataLogging_Test::~DataLogging_Test() {
}


void DataLogging_Test::setUp() {

    // There are indications for rare hanging between tests, see https://git.xfel.eu/gitlab/Karabo/Framework/-/jobs/101484
    // So debug print when this happens.
    const Epochstamp start;
    std::clog << "\nStart setUp " << start.toIso8601Ext() << std::endl;

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

    // There are indications for rare hanging between tests, see https://git.xfel.eu/gitlab/Karabo/Framework/-/jobs/101484
    // So debug print when this happens.
    const Epochstamp stop;
    std::clog << "End setUp " << stop.toIso8601Ext() << std::endl;
}


void DataLogging_Test::switchToTelegrafEnv() {
    m_influxDb_dbName = ::getenv("KARABO_INFLUXDB_DBNAME");
    m_influxDb_query_user = ::getenv("KARABO_INFLUXDB_QUERY_USER");
    m_influxDb_query_password = ::getenv("KARABO_INFLUXDB_QUERY_PASSWORD");
    m_influxDb_query_url = ::getenv("KARABO_INFLUXDB_QUERY_URL");
    m_influxDb_write_user = ::getenv("KARABO_INFLUXDB_WRITE_USER");
    m_influxDb_write_password = ::getenv("KARABO_INFLUXDB_WRITE_PASSWORD");
    m_influxDb_write_url = ::getenv("KARABO_INFLUXDB_WRITE_URL");

    //   InfluxDB cluster with telegraf front-end and 2 InfluxDB cpus as a backend
    ::setenv("KARABO_INFLUXDB_DBNAME", ::getenv("KARABO_TEST_TELEGRAF_DBNAME"), 1);
    ::setenv("KARABO_INFLUXDB_QUERY_USER", ::getenv("KARABO_TEST_TELEGRAF_QUERY_USER"), 1);
    ::setenv("KARABO_INFLUXDB_QUERY_PASSWORD", ::getenv("KARABO_TEST_TELEGRAF_QUERY_PASSWORD"), 1);
    ::setenv("KARABO_INFLUXDB_QUERY_URL", ::getenv("KARABO_TEST_TELEGRAF_QUERY_URL"), 1);
    ::setenv("KARABO_INFLUXDB_WRITE_USER", ::getenv("KARABO_TEST_TELEGRAF_WRITE_USER"), 1);
    ::setenv("KARABO_INFLUXDB_WRITE_PASSWORD", ::getenv("KARABO_TEST_TELEGRAF_WRITE_PASSWORD"), 1);
    ::setenv("KARABO_INFLUXDB_WRITE_URL", ::getenv("KARABO_TEST_TELEGRAF_WRITE_URL"), 1);

    m_switchedToTelegrafEnv = true;
}


void DataLogging_Test::switchFromTelegrafEnv() {
    ::setenv("KARABO_INFLUXDB_DBNAME", m_influxDb_dbName.c_str(), 1);
    ::setenv("KARABO_INFLUXDB_QUERY_USER", m_influxDb_query_user.c_str(), 1);
    ::setenv("KARABO_INFLUXDB_QUERY_PASSWORD", m_influxDb_query_password.c_str(), 1);
    ::setenv("KARABO_INFLUXDB_QUERY_URL", m_influxDb_query_url.c_str(), 1);
    ::setenv("KARABO_INFLUXDB_WRITE_USER", m_influxDb_write_user.c_str(), 1);
    ::setenv("KARABO_INFLUXDB_WRITE_PASSWORD", m_influxDb_write_password.c_str(), 1);
    ::setenv("KARABO_INFLUXDB_WRITE_URL", m_influxDb_write_url.c_str(), 1);

    m_switchedToTelegrafEnv = false;
}

void DataLogging_Test::setPropertyTestSchema() {
    std::vector<Hash> updates;
    updates.push_back(Hash("path", "floatProperty", "attribute", KARABO_SCHEMA_MIN_INC, "value", -1.f * std::numeric_limits<float>::infinity()));
    updates.push_back(Hash("path", "floatProperty", "attribute", KARABO_SCHEMA_MAX_INC, "value", std::numeric_limits<float>::infinity()));
    updates.push_back(Hash("path", "doubleProperty", "attribute", KARABO_SCHEMA_MIN_INC, "value", -1. * std::numeric_limits<double>::infinity()));
    updates.push_back(Hash("path", "doubleProperty", "attribute", KARABO_SCHEMA_MAX_INC, "value", std::numeric_limits<double>::infinity()));
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
            .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(response);
    CPPUNIT_ASSERT_MESSAGE("Could not update schema", response.get<bool>("success"));

}

std::pair<bool, std::string> DataLogging_Test::startLoggers(const std::string& loggerType,
                                                            bool useInvalidInfluxUrl,
                                                            bool useInvalidDbName) {
    Hash manager_conf;
    manager_conf.set("deviceId", "loggerManager");
    manager_conf.set("flushInterval", FLUSH_INTERVAL_SEC);
    manager_conf.set<vector < string >> ("serverList",{m_server});
    manager_conf.set("logger", loggerType);

    if (loggerType == "FileDataLogger") {
        manager_conf.set("logger.FileDataLogger.directory",
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

        manager_conf.set("logger.InfluxDataLogger.urlWrite", influxUrlWrite);
        manager_conf.set("logger.InfluxDataLogger.urlRead", influxUrlRead);
        manager_conf.set("logger.InfluxDataLogger.dbname", dbName);

    } else {
        CPPUNIT_FAIL("Unknown logger type '" + loggerType + "'");
    }

    std::clog << "\n***** DataLoggerManager configuration *****\n" << manager_conf << std::endl;
    return m_deviceClient->instantiate(m_server,
                                       "DataLoggerManager", manager_conf, KRB_TEST_MAX_TIMEOUT);
}


void DataLogging_Test::tearDown() {
    // So debug print for in between tests, see setUp()
    const Epochstamp start;
    std::clog << "Start tearDown " << start.toIso8601Ext() << std::endl;

    m_deviceClient.reset();
    m_deviceServer.reset();
    m_sigSlot.reset();
    EventLoop::stop();
    m_eventLoopThread.join();

    // Clean up directory - you may want to comment out these lines for debugging
    boost::filesystem::remove("loggermap.xml");
    if (!m_keepLoggerDirectory) {
        boost::filesystem::remove_all(m_fileLoggerDirectory);
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

    // If the InfluxDb has been switched to use Telegraf, but hasn't been restored (e.g. the test that made the switch
    // didn't run until its end), do so in here.
    if (m_switchedToTelegrafEnv) {
        switchFromTelegrafEnv();
    }
}


void DataLogging_Test::fileAllTestRunner() {
    std::pair<bool, std::string> success = m_deviceClient->instantiate(m_server, "PropertyTest",
                                                                       Hash("deviceId", m_deviceId),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    setPropertyTestSchema();

    std::clog << "\n==== Starting sequence of File Logging tests ====" << std::endl;
    success = startLoggers("FileDataLogger");
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();
    // The "testNans" test gives repeatedly some timeout that results in
    // assertion and for the following test.  The reason is still unclear.
    // testNans();
    testInt();
    testUInt64();
    testFloat();
    testString();
    // TODO: port base64 encoding to the FileDataLogger/FileLogReader
    // testChar(false);
    testVectorString();
    testVectorChar();
    testVectorSignedChar();
    testVectorUnsignedChar();
    testVectorBool();
    testVectorShort();
    testVectorUnsignedShort();
    testVectorInt();
    testVectorUnsignedInt();
    testVectorLongLong();
    testVectorUnsignedLongLong();
    testTable();
    // This must be the last test case that relies on the device in m_deviceId (the logged
    // PropertyTest instance) being available at the start of the test case.
    // 'testLastKnownConfiguration' stops the device being logged to make sure that the
    // last known configuration can be successfully retrieved after the device is gone.
    testLastKnownConfiguration();

    // These deal with their own devices, so comment above about using the PropertyTest instance
    // in m_deviceId is not applicable.
    testCfgFromPastRestart();

    // TODO: Uncomment test below as soon as FileLogReader::slotGetPropertyHistoryImpl is fixed.
    //       Currently it is failing to retrieve all the logged entries (see comment on discussions of
    //       https://git.xfel.eu/gitlab/Karabo/Framework/merge_requests/4455).
    //testSchemaEvolution();
}

void DataLogging_Test::testMigrateFileLoggerData() {

    // launch the migration script onto the logged path

    const std::string influxUrlWrite = (getenv("KARABO_INFLUXDB_WRITE_URL") ? getenv("KARABO_INFLUXDB_WRITE_URL") : "http://localhost:8086");
    const std::string influxUrlRead = (getenv("KARABO_INFLUXDB_QUERY_URL") ? getenv("KARABO_INFLUXDB_QUERY_URL") : "http://localhost:8086");
    const std::string influxDbName = (getenv("KARABO_INFLUXDB_DBNAME") ? getenv("KARABO_INFLUXDB_DBNAME") : (getenv("KARABO_BROKER_TOPIC") ? getenv("KARABO_BROKER_TOPIC") : getenv("USER")));
    const std::string influxUserWrite = (getenv("KARABO_INFLUXDB_WRITE_USER") ? getenv("KARABO_INFLUXDB_WRITE_USER") : std::string("infadm"));
    const std::string influxPwdWrite = (getenv("KARABO_INFLUXDB_WRITE_PASSWORD") ? getenv("KARABO_INFLUXDB_WRITE_PASSWORD") : std::string("admpwd"));
    const std::string influxUserRead = (getenv("KARABO_INFLUXDB_QUERY_USER") ? getenv("KARABO_INFLUXDB_QUERY_USER") : influxUserWrite);
    const std::string influxPwdRead = (getenv("KARABO_INFLUXDB_QUERY_PASSWORD") ? getenv("KARABO_INFLUXDB_QUERY_PASSWORD") : influxPwdWrite);
    const std::string absLoggerPath =  boost::filesystem::absolute("./"  + m_fileLoggerDirectory).string();
    const std::string migrationResultsPath = absLoggerPath + std::string("/migrationresults");
    std::ostringstream cmd;
    cmd << "cd ../../../src/pythonKarabo; ../../karabo/extern/bin/python3 ";
    cmd << "karabo/influxdb/dl_migrator.py ";

    cmd << influxDbName << " " << absLoggerPath << "/karaboHistory/" << " " << migrationResultsPath << " ";
    cmd << "--write-url " << boost::algorithm::replace_first_copy(influxUrlWrite, "tcp://", "http://") << " ";
    cmd << "--write-user " << influxUserWrite << " ";
    cmd << "--write-pwd " << influxPwdWrite << " ";
    cmd << "--read-url " << boost::algorithm::replace_first_copy(influxUrlRead, "tcp://", "http://") << " ";
    cmd << "--read-user " << influxUserRead << " ";
    cmd << "--read-pwd " << influxPwdRead << " ";
    cmd << "--lines-per-write 200 --write-timeout 50 --concurrent-tasks 2";

    const int ret = system(cmd.str().c_str());
    CPPUNIT_ASSERT_EQUAL(0, ret);

    boost::filesystem::path p(migrationResultsPath + "/processed/"+m_deviceId+"/");
    if(boost::filesystem::is_directory(p)) {
        for(auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(p), {})) {
            std::ostringstream msg;
            msg << "Check if " << entry << " was migrated OK: "<<boost::filesystem::extension(entry);
            std::clog<<msg.str()<<std::endl;
            CPPUNIT_ASSERT_MESSAGE(msg.str(), boost::filesystem::extension(entry) == ".ok");

        }
    }

    unsigned int errorCount = 0;
    boost::filesystem::path perr(migrationResultsPath + "/part_processed/"+m_deviceId+"/");
    if(boost::filesystem::is_directory(perr)) {
        for(auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(perr), {})) {
            // print out the error
            std::ostringstream cmd;
            cmd << "cat "<<entry;
            system(cmd.str().c_str());
            errorCount++;

        }
    }

    CPPUNIT_ASSERT_MESSAGE("Check that no errors occurred in migration. See logs above if they did!", errorCount == 0);

    m_dataWasMigrated = true;
    // remove migration results
    boost::filesystem::remove_all(migrationResultsPath);

}


void DataLogging_Test::testMaxNumDataRange() {

    std::clog << "Check if InfluxLogReader is validating range for 'maxNumData' for slot 'getPropertyHistory' ... ";

    const std::string dlReader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
    const std::string outOfRangeErrMsg("'maxNumData' parameter is intentionally limited to a maximum of");

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
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(replyDevice, replyProperty, history);

    } catch (karabo::util::RemoteException& e) {
        const std::string& errMsg = e.userFriendlyMsg();
        CPPUNIT_ASSERT(errMsg.find(outOfRangeErrMsg) != std::string::npos);
    }

    // Negative values must be rejected.
    params.set<int>("maxNumData", -1);
    try {
        m_sigSlot->request(dlReader0, "slotGetPropertyHistory", dlReader0, "url", params)
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(replyDevice, replyProperty, history);

    } catch (karabo::util::RemoteException& e) {
        const std::string& errMsg = e.userFriendlyMsg();
        CPPUNIT_ASSERT(errMsg.find(outOfRangeErrMsg) != std::string::npos);
    }

    // 0 must be accepted - it as if InfluxLogReader::maxHistorySize has been used.
    params.set<int>("maxNumData", 0);
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(dlReader0, "slotGetPropertyHistory", dlReader0, "url", params)
                            .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(replyDevice, replyProperty, history));

    // InfluxLogReader::maxHistorySize must be accepted.
    params.set<int>("maxNumData", readerMaxHistSize);
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(dlReader0, "slotGetPropertyHistory", dlReader0, "url", params)
                            .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(replyDevice, replyProperty, history));

    std::clog << "OK" << std::endl;
}


void DataLogging_Test::testMaxNumDataHistory() {

    std::clog
            << "Check if InfluxLogReader is properly enforcing the 'maxNumData' parameter for slot 'getPropertyHistory' ..."
            << std::endl;

    bool envResponsive = false;

    // Instantiates a DataLogTestDevice for performing the check.
    const std::string deviceId(deviceIdPrefix + "MaxNumDataHistory");
    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const auto res = m_deviceClient->instantiate(m_server, "PropertyTest",
                                                 Hash("deviceId", deviceId),
                                                 KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE("Failed to instantiate testing device '" + deviceId
                           + "':" + res.second,
                           res.first);

    // Checks that the testing device is being logged.
    bool isLogged = waitForCondition(
        [this, &loggerId, &deviceId]() {
            auto loggedIds =
                 m_deviceClient->get<std::vector < std::string >> (loggerId, "devicesToBeLogged");
            return (std::find(loggedIds.begin(), loggedIds.end(), deviceId) != loggedIds.end());
        },
        KRB_TEST_MAX_TIMEOUT * 1000
    );

    CPPUNIT_ASSERT_MESSAGE("Failed to start logging of testing device '" + deviceId + ".",
                           isLogged);

    // Writing sequence - write a sequence of increasing values.
    Epochstamp beforeWrites;
    const int numOfWrites = 40;
    for (int i = 0; i < numOfWrites; i++) {
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set<int>(deviceId, "int32Property", i));
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
    }
    // Captures the timepoint after the writes.
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    Epochstamp afterWrites;

    // Makes sure all the writes are done before retrieval.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush")
                                       .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                                       .receive());

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
    const int timeoutSecs = 90;
    int nTries = timeoutSecs; // number of attempts spaced by 1 sec.
    while (nTries >= 0 && history.size() != maxNumDataFull) {
        try {
            m_sigSlot->request(dlreader0, "slotGetPropertyHistory", deviceId, "int32Property", params)
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(replyDevice, replyProperty, history);
        } catch (const karabo::util::TimeoutException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
        } catch (const karabo::util::RemoteException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        nTries--;
    }

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Size for full history different from expected.",
                                 static_cast<size_t>(maxNumDataFull),
                                 history.size());
    for (int i = 0; i < maxNumDataFull; i++) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Value at history entry #"
                                     + karabo::util::toString(i)
                                     + " different from expected.",
                                     i,
                                     history[i].get<int>("v"));
    }

    // Sampled history retrieval - must retrieve all values sampled.
    // The specified 'maxNumData' is half the size of the full history size.
    const int maxNumDataSampled = numOfWrites/2;
    params.set<int>("maxNumData", maxNumDataSampled);

    history.clear();

    // History retrieval may take more than one attempt.
    nTries = timeoutSecs; // number of attempts spaced by 1 sec.
    while (nTries >= 0 && history.size() != maxNumDataSampled) {
        try {
            m_sigSlot->request(dlreader0, "slotGetPropertyHistory", deviceId, "int32Property", params)
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(replyDevice, replyProperty, history);
        } catch (const karabo::util::TimeoutException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
        } catch (const karabo::util::RemoteException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        nTries--;
    }

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Size for sampled history different from expected.",
                                 static_cast<size_t>(maxNumDataSampled),
                                 history.size());
    // Makes sure that the maxNumDataSampled values retrieved are distributed across the
    // whole set of maxNumDataFull values. A deviation margin is tolerated to accomodate
    // different timings involved in the writing sequence phase.
    const int deviationMargin = 8;
    CPPUNIT_ASSERT_MESSAGE("Value at history entry #0 is outside the expected range: should be between 0 and "
                           + karabo::util::toString(deviationMargin)
                           + ", got "
                           + karabo::util::toString(history[0].get<int>("v")) + ".",
                           history[0].get<int>("v") >= 0 && history[0].get<int>("v") <= deviationMargin);
    CPPUNIT_ASSERT_MESSAGE("Value at history entry #"
                           + karabo::util::toString(history[maxNumDataSampled-1])
                           + " is outside the expected range: should be between "
                           + karabo::util::toString(40 - deviationMargin)
                           + " and 40, got "
                           + karabo::util::toString(history[maxNumDataSampled-1].get<int>("v")) + ".",
                           history[maxNumDataSampled - 1].get<int>("v") >= 40 - deviationMargin
                           && history[maxNumDataSampled - 1].get<int>("v") <= 40);

    std::clog << "... OK" << std::endl;
}


void DataLogging_Test::influxAllTestRunner() {
    // and epoch stamp certainly before the next round of influx logging
    m_fileMigratedDataEndsBefore = Epochstamp();
    boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));

    std::pair<bool, std::string> success = m_deviceClient->instantiate(m_server, "PropertyTest",
                                                                       Hash("deviceId", m_deviceId),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    setPropertyTestSchema();

    // Starts the same set of tests with InfluxDb logging instead of text-file based logging
    std::clog << "\n==== Starting sequence of Influx Logging tests on \""
            << m_deviceId << "\" ====" << std::endl;
    success = startLoggers("InfluxDataLogger");
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();

    testMaxNumDataRange();

    // When the Telegraf environment is not responsive, skips the Telegraf tests.
    if (m_switchedToTelegrafEnv) {
        if (!isTelegrafEnvResponsive()) {
            return;
        }
    }

    if (!m_switchedToTelegrafEnv) {
        // migration tests are skipped on Telegraf
        // migrate the logger data produced so far into InfluxDB - do at this point so that a DB is for sure created
        testMigrateFileLoggerData();
    }

    testMaxNumDataHistory();
    testNans();
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

    // This must be the last test case that relies on the device in m_deviceId (the logged
    // PropertyTest instance) being available at the start of the test case.
    // 'testLastKnownConfiguration' stops the device being logged to make sure that the
    // last known configuration can be successfully retrieved after the device is gone.
    testLastKnownConfiguration();

    // These deal with their own devices, so comment above about using the PropertyTest instance
    // in m_deviceId is not applicable.
    testCfgFromPastRestart();
    testSchemaEvolution();
}


bool DataLogging_Test::isTelegrafEnvResponsive() {
    const int timeoutSecs = 90;

    std::clog
            << "Check if Telegraf environment is responsive (updates retrieved within "
            << karabo::util::toString(timeoutSecs) << " secs.) ... "
            << std::endl;

    bool envResponsive = false;

    // Instantiates a DataLogTestDevice for performing the check: a single property update
    // followed by a property history retrieval will be used as the "probing" operation.
    const std::string deviceId(deviceIdPrefix + "TelegrafEnvProbe");
    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    auto success = m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId),
                                               KRB_TEST_MAX_TIMEOUT);

    if (!success.first) {
        return false;
    }

    // Checks that the probing device is being logged.
    bool isLogged = waitForCondition(
        [this, &loggerId, &deviceId]() {
            auto loggedIds =
                 m_deviceClient->get<std::vector < std::string >> (loggerId, "devicesToBeLogged");
            return (std::find(loggedIds.begin(), loggedIds.end(), deviceId) != loggedIds.end());
        },
        KRB_TEST_MAX_TIMEOUT * 1000
    );

    if (!isLogged) {
        return false;
    }

    // Probing sequence
    Epochstamp beforePropUpdate;

    // Updates a property
    try {
        m_deviceClient->execute(deviceId, "slotIncreaseValue", KRB_TEST_MAX_TIMEOUT);
    } catch (std::exception& e) {
        std::clog << "... (not responsive).\nError during property update: "
                << e.what() << std::endl;
        return false;
    }


    // Captures the timepoint after updating the property
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    Epochstamp afterPropUpdate;

    // Makes sure all the writes are done before retrieval.
    try {
        m_sigSlot->request(loggerId, "flush")
                   .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS)
                   .receive();
    } catch(std::exception &e) {
        std::clog << "... (not responsive).\nError during flush "
                  << e.what() << std::endl;
        return false;
    }

    // Tries to obtain the property update from the Influx node.
    Hash params;
    params.set<string>("from", beforePropUpdate.toIso8601());
    params.set<string>("to", afterPropUpdate.toIso8601());
    const int maxNumData = 10;
    params.set<int>("maxNumData", maxNumData);

    vector<Hash> history;
    std::string replyDevice;
    std::string replyProperty;
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);

    // History retrieval may take more than one try; if the retrieval happens
    // within the timeout limit, the environment is considered responsive.
    int nTries = timeoutSecs; // number of attempts spaced by 1 sec.
    while (nTries >= 0 && history.size() != 1) {
        try {
            m_sigSlot->request(dlreader0, "slotGetPropertyHistory", deviceId, "value", params)
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(replyDevice, replyProperty, history);
        } catch (const karabo::util::TimeoutException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
        } catch (const karabo::util::RemoteException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        nTries--;
    }

    if (history.size() == 1) {
        envResponsive = true;
    }

    std::clog << "... (" << (envResponsive ? "" : "not") << " responsive)." << std::endl;

    return envResponsive;
}


void DataLogging_Test::influxAllTestRunnerWithTelegraf() {

    // delete logger directory after this test
    m_keepLoggerDirectory = false;

    if (!::getenv("KARABO_TEST_TELEGRAF")) {
        std::clog << "==== Skip sequence of Telegraf Logging tests ====" << std::endl;
        return;
    }

    std::clog << "\n==== Repeat test with Telegraf setup ====" << std::endl;

    // Run influxAllTestRunner with "telegraf" environment:
    switchToTelegrafEnv();

    influxAllTestRunner();

    // Restores Influx environment for upcoming tests.
    switchFromTelegrafEnv();

    std::clog << "==== Telegraf Influx Logging test finished ====" << std::endl;
}


void DataLogging_Test::testAllInstantiated(bool waitForLoggerReady) {
    std::clog << "Testing deviceInstantiation... " << std::flush;
    int timeout = KRB_TEST_MAX_TIMEOUT * 1000; // milliseconds
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

    if (waitForLoggerReady) {
        // Makes sure that the DataLogger has reached NORMAL state before proceeding.
        // Any call to the Flush slot while the DataLogger is in a different state will trigger an exception.
        // For the Influx Logger case, this initialization time can be quite long - if the db does not exist
        // yet, the DataLogger must create it before reaching the NORMAL state.
        int timeout = 10 * KRB_TEST_MAX_TIMEOUT * 1000; // milliseconds

        karabo::util::State loggerState = karabo::util::State::UNKNOWN;
        const std::string &dataLoggerId = karabo::util::DATALOGGER_PREFIX + m_server;
        while (timeout > 0) {
            loggerState = m_deviceClient->get<karabo::util::State>(dataLoggerId, "state");
            if (loggerState == karabo::util::State::NORMAL) {
                break;
            }
            boost::this_thread::sleep(boost::posix_time::milliseconds(50));
            timeout -= 50;
        }

        CPPUNIT_ASSERT_MESSAGE("Timeout while waiting for DataLogger '" + dataLoggerId + "' to reach NORMAL state.",
                               loggerState == karabo::util::State::NORMAL);
    }

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
        const std::string fileLoggerMsg("Requested time point for device configuration is earlier than anything logged");
        const std::string influxLoggerMsg("Failed to query schema digest");
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
    CPPUNIT_ASSERT_EQUAL(latestTimestamp.toIso8601Ext(), configTimepoint);
    std::clog << "\n... "
            << "Ok (retrieved configuration with last known value for 'int32Property' while the device was being logged)."
            << std::endl;

    // killDevice waits for the device to be killed (or throws an exception in case of failure).
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->killDevice(m_deviceId, KRB_TEST_MAX_TIMEOUT));

    // Assures that the logger in charge of the device is not logging it anymore by testing that m_deviceId is not
    // among the rows of the "lastUpdatesUtc" property of the logger. The "flush" slot guarantees that the property
    // "lastUpdatesUtc" is in sync with devices being logged.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                            .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());
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
    boost::this_thread::sleep(boost::posix_time::milliseconds(15250));

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
    // if data migration happened the data is younger than the file based logging data
    CPPUNIT_ASSERT(configStamp > (m_dataWasMigrated ? m_fileMigratedDataEndsBefore : beforeAnything));
    CPPUNIT_ASSERT(configStamp < afterDeviceGone);
    std::clog << "\n... "
            << "Timestamp of retrieved configuration: " << configTimepoint << "\n "
            << "Ok (retrieved configuration with last known value for 'int32Property' while the device was not being logged)."
            << std::endl;


    if (m_dataWasMigrated) {
        // check for the migrated data
        std::clog << "\n... from migrated data (requested config at " << m_fileMigratedDataEndsBefore.toIso8601() << ") ...";
        // At the afterDeviceGone timepoint, a last known configuration should be obtained with the last value set in the
        // previous test cases for the 'int32Property' - even after the device being logged is gone.
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast",
                                                   m_deviceId, m_fileMigratedDataEndsBefore.toIso8601())
                                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(conf, schema, configAtTimepoint, configTimepoint));

        CPPUNIT_ASSERT_EQUAL(kLastValueSet, conf.get<int>("int32Property"));
        CPPUNIT_ASSERT_EQUAL(false, configAtTimepoint);
        karabo::util::Epochstamp configStamp(configTimepoint);
        CPPUNIT_ASSERT(configStamp > beforeAnything);
        // if migration failed or was incompatible we would not get a timestamp matching this condition
        // there wouldn't be any data in the DB before this.
        CPPUNIT_ASSERT(configStamp < m_fileMigratedDataEndsBefore);
        std::clog << "\n... "
                << "Timestamp of retrieved configuration: " << configTimepoint << "\n "
                << "Ok (retrieved configuration with last known value for 'int32Property' from file logger migrated data)."
                << std::endl;
    }


}


void DataLogging_Test::testCfgFromPastRestart() {
    std::clog << "Testing past configuration retrieval with stamp older than device..." << std::flush;

    // Start device and take care that the logger is ready for it
    // Use platform-dependent name for the device: concurrent tests in CI operate
    // on the same InfluxDB database ...
    const std::string deviceId(deviceIdPrefix + "deviceWithOldStamp");
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
    const Epochstamp oldStamp = threeDaysBack;
    for (unsigned int i = 0; i < numCycles; ++i) {
        // Increase "variable" value and store after increasing it
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(deviceId, "slotIncreaseValue", KRB_TEST_MAX_TIMEOUT));
        stampsAfter.push_back(Epochstamp());

        // Get configuration, check expected values, check (static) time stamp of "oldValue" and store stamp of "value"
        Hash cfg;
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(deviceId, cfg));
        CPPUNIT_ASSERT_EQUAL(static_cast<int> (i) + 1, cfg.get<int>("value"));

        CPPUNIT_ASSERT_EQUAL(99, cfg.get<int>("oldValue"));
        const Epochstamp stamp = Epochstamp::fromHashAttributes(cfg.getAttributes("oldValue"));
        CPPUNIT_ASSERT_MESSAGE("'oldValue' has wrong time stamp: " + stamp.toIso8601(), stamp == oldStamp);
        valueStamps.push_back(Epochstamp::fromHashAttributes(cfg.getAttributes("value")));

        // Stop logging our device and check that it is not logged anymore.
        // Sleep needed before flush to ensure that - for file logger - the output stream has actually seen the data
        boost::this_thread::sleep(boost::posix_time::milliseconds(250)); // locally 100 was always enough
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                                .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());
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
                            .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());

    boost::this_thread::sleep(boost::posix_time::milliseconds(15250));

    // Now check that for all stored stamps, the stamps gathered for the reader are correct
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
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
        while (nTries > 0 && conf.empty()) {
            try {
                nChecks++;
                m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast",
                                   deviceId, stampAfter.toIso8601())
                        .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                        .receive(conf, schema);
            } catch (const RemoteException& re) {
                ++nRemoteExceptions;
                // The data might not yet be available for the reader - despite the flush and the long sleep above!
                // File and influx logger will reply then with exceptions, but their text is different. Here we allow
                // for these expected exceptions (and go on with next try), but bail out for any other remote exception.
                const std::string fileLoggerMsg("Requested time point for device configuration is earlier than anything logged");
                const std::string influxLoggerMsg("Failed to query schema digest");
                CPPUNIT_ASSERT_MESSAGE("Unexpected RemoteException received: " + std::string(re.what()),
                                       (re.detailedMsg().find(fileLoggerMsg) != std::string::npos
                                        || re.detailedMsg().find(influxLoggerMsg) != std::string::npos));
            } catch (const TimeoutException &te) {
                // Also timeouts are allowed...
                karabo::util::Exception::clearTrace();
            }
            boost::this_thread::sleep(boost::posix_time::milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
            nTries--;
        }
        CPPUNIT_ASSERT_MESSAGE("Failed to retrieve a non-empty configuration for device '" + m_deviceId +
                               "' after " + toString(nChecks) + " attempts - " + toString(nRemoteExceptions) +
                               " remote exceptions among them",
                               conf.size() > 0);
        CPPUNIT_ASSERT_EQUAL(99, conf.get<int>("oldValue"));
        CPPUNIT_ASSERT_EQUAL(static_cast<int> (i + 1), conf.get<int>("value")); // +1: stamp is after update

        // Check received stamps: The one of "oldValue is always the same, for "value" be aware that we store with
        // microsec precision only: we might be 1 off since we cut off digits instead of rounding
        const Epochstamp stampOldFromPast = Epochstamp::fromHashAttributes(conf.getAttributes("oldValue"));
        std::string oldFromPastStr = stampOldFromPast.toIso8601();  // convert to microsecond precision
        std::string oldStr = oldStamp.toIso8601();                  // convert to microsecond precision
        CPPUNIT_ASSERT_EQUAL_MESSAGE("'oldValue' from past has wrong time stamp", oldStr, oldFromPastStr);
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

    testAllInstantiated(false);

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
        bool condition = (exc.detailedMsg().find("Could not connect to InfluxDb at") != std::string::npos)
            || (exc.detailedMsg().find("Reading from InfluxDB failed") != std::string::npos);
        CPPUNIT_ASSERT(condition);
        remoteExceptionCaught = true;
    }

    CPPUNIT_ASSERT(remoteExceptionCaught);

    std::clog << "... request failed with RemoteException as expected." << std::endl;

    // By simply starting the devices related to Influx logging, some logging writing activity takes place.
    // If this point of the test is reached with an invalid url configured for the Influx Server, it is safe
    // to conclude that the Influx Logger doesn't get stuck when no server is available.

    std::clog << "OK" << std::endl;
}


void DataLogging_Test::testInfluxDbNotAvailableTelegraf() {
    if (!::getenv("KARABO_TEST_TELEGRAF")) {
        std::clog << "==== Test only executed for Telegraf environment. Skipping test..." << std::endl;
        std::clog << "     (requires environment where db cannot be created on-the-fly)" << std::endl;
        std::clog << "====" << std::endl;
        return;
    }

    std::clog << "Testing handling of Influx Database not available scenarios ...." << std::endl;

    switchToTelegrafEnv();

    std::pair<bool, std::string> success = m_deviceClient->instantiate(m_server, "PropertyTest",
                                                                       Hash("deviceId", m_deviceId),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Starts the loggers with an invalid database name.
    // Note: it is required for the InfluxDb writing user to not have admin privileges on the Influx server.
    //       This requirement is fullfilled by both the CI and the Production environments. A local Influx server
    //       test environment must be configured properly.
    success = startLoggers("InfluxDataLogger", false, true);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated(false);

    int timeout = KRB_TEST_MAX_TIMEOUT * 1000; // milliseconds
    karabo::util::State loggerState = karabo::util::State::UNKNOWN;
    const std::string &dataLoggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    while (timeout > 0) {
        loggerState = m_deviceClient->get<karabo::util::State>(dataLoggerId, "state");
        if (loggerState == karabo::util::State::ERROR) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(50));
        timeout -= 50;
    }

    CPPUNIT_ASSERT_MESSAGE("Timeout while waiting for DataLogger '" + dataLoggerId + "' to reach ERROR state.",
                           loggerState == karabo::util::State::ERROR);

    std::clog << "... logger in ERROR state as expected." << std::endl;

    // Restores Influx environment for upcoming tests.
    switchFromTelegrafEnv();

    std::clog << "OK" << std::endl;
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

// Needed in case we add a testDouble():
//  template <>
//  void isEqualMessage(const std::string& message, const double& expected, const double& actual,
//                      const std::vector<karabo::util::Hash>& fullHistory) {
//      std::string msg(message);
//      if (expected != actual) {
//          (msg += ": ") += toString(fullHistory);
//      }
//      CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(message, expected, actual, expected * 1.e-6);
//  }

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

    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                            .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());

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
                                 " checks:\n\tdeviceId: " + m_deviceId +
                                 "\n\tkey: " + key +
                                 "\n\tparam.from: " + beforeWrites +
                                 "\n\tparam.to: " + afterWrites + "\n\tparam.maxNumData: " + toString(max_set * 2) +
                                 "\n\thistory.size(): " + toString(history.size()) +
                                 "\n\tNumber of Exceptions: " + toString(numExceptions) +
                                 "\n\tExceptions:\n" + boost::algorithm::join(exceptionsMsgs, "\n") +
                                 "\nhistory\t" + toString(history),
                                 static_cast<size_t> (max_set), history.size());

    CPPUNIT_ASSERT_EQUAL(numGetPropHist + numChecks, m_deviceClient->get<unsigned int>(dlreader0, "numGetPropertyHistory"));

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
                            .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());

    const unsigned int numGetCfgFromPast = m_deviceClient->get<unsigned int>(dlreader0, "numGetConfigurationFromPast");
    nTries = NUM_RETRY;
    numExceptions = 0;
    numChecks = 0;
    // place holder schema, could be checked in future tests
    Schema schema;
    Hash conf;
    while (nTries >= 0) { // '&& !conf.empty()' check not needed as in DataLogging_Test::testCfgFromPastRestart:
        //                   The 'history.size() != max_set' check in the loop above already ensures that all data available

        // TODO: use the deviceClient to retrieve the configuration from past
        // auto pair = m_deviceClient->getConfigurationFromPast(m_deviceId, before);
        // conf = pair.first;
        bool excepted = false;
        try {
            numChecks++;
            m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", m_deviceId, beforeWrites)
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(conf, schema);
        } catch (const karabo::util::TimeoutException &e) {
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
            excepted = true;
        } catch (const karabo::util::RemoteException &e) {
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
            excepted = true;
        }
        if (!excepted) break; // Any result should be trustworthy!
        boost::this_thread::sleep(boost::posix_time::milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
        nTries--;
    }

    CPPUNIT_ASSERT_MESSAGE("Configuration size from slotGetConfigurationFromPast still 0 after " + toString(numChecks) +
                           " checks.\n\tdeviceId: " + m_deviceId + "\n\tparam.before: " + beforeWrites +
                           "\n\tconf.size(): " + toString(conf.size()) +
                           "\n\tNumber of Exceptions: " + toString(numExceptions) +
                           "\n\tExceptions:\n" + boost::algorithm::join(exceptionsMsgs, "\n"),
                           conf.size() > 0);
    CPPUNIT_ASSERT_EQUAL(numGetCfgFromPast + numChecks, m_deviceClient->get<unsigned int>(dlreader0, "numGetConfigurationFromPast"));

    // One needs to check only the content here, therefore only the leaves are examined
    std::vector<std::string> leaves;
    getLeaves(beforeWritesCfg, schema, leaves, '.');
    std::vector<std::string> confLeaves;
    getLeaves(conf, schema, confLeaves, '.');
    std::string missingKeysFromPast;
    for (const std::string & leaf : leaves) {
        if (std::find(confLeaves.begin(), confLeaves.end(), leaf) != confLeaves.end()) {
            // Leaf is in the configuration retrieved from past - check its value against the
            // one in the configuration snapshot obtained directly from the device.
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong configuration from past (before writes) for key :" + leaf,
                                         beforeWritesCfg.getAs<std::string>(leaf),
                                         conf.getAs<std::string>(leaf));
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
                                 0ul,
                                 missingKeysFromPast.size());

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
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(conf, schema);
        } catch (const karabo::util::TimeoutException& e) {
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
            excepted = true;
        } catch (const karabo::util::RemoteException &e) {
            exceptionsMsgs.push_back("At check #" + toString(numChecks) + ": " + e.what());
            ++numExceptions;
            excepted = true;
        }
        if (!excepted) break; // Any result should be trustworthy!
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
    getLeaves(afterWritesCfg, schema, leaves, '.');
    confLeaves.clear();
    getLeaves(conf, schema, confLeaves, '.');
    missingKeysFromPast.clear();
    for (const std::string & leaf : leaves) {
        if (std::find(confLeaves.begin(), confLeaves.end(), leaf) != confLeaves.end()) {
            // Leaf is in the configuration retrieved from past - check its value against the
            // one in the configuration snapshot obtained directly from the device.
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong configuration from past (after writes) for key :" + leaf,
                                         afterWritesCfg.getAs<std::string>(leaf),
                                         conf.getAs<std::string>(leaf));
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


void DataLogging_Test::testInt(bool testPastConf) {
    auto lambda = [] (int i) -> int {
        return i;
    };
    testHistory<int>("int32Property", lambda, testPastConf);
}


void DataLogging_Test::testUInt64(bool testPastConf) {
    auto lambda = [] (int i) -> unsigned long long {
        return (unsigned long long) i - 1;
    };
    testHistory<unsigned long long>("uint64Property", lambda, testPastConf);
}


void DataLogging_Test::testFloat(bool testPastConf) {
    auto lambda = [] (int i) -> float {
        return 2.5e-8f * i;
    };
    testHistory<float>("floatProperty", lambda, testPastConf);
}


void DataLogging_Test::testString(bool testPastConf) {
    auto lambda = [] (int i) -> string {
        return ( (i % 2)? string() : "(1|2|" + karabo::util::toString(i)) +")";
    };
    testHistory<string>("stringProperty", lambda, testPastConf);

    // Also test a string with a new line character
    auto lambda2 = [] (int i) -> string {
        return "with\nnewline" + karabo::util::toString(i);
    };
    testHistory<string>("stringProperty", lambda2, testPastConf);

}


void DataLogging_Test::testVectorString(bool testPastConf) {
    auto lambdaMixed = [] (int i) -> vector<string> {
        vector<string> v = {"abc" + toString(i), "xy|z" + toString(i), string(), "A\nB" + toString(i)};
        // rotate the vector to check all positions for the empty string
        std::rotate(v.begin(),v.begin() + (i % v.size()), v.end());
        return (i % 5 == 0)? vector<string>() : v;
    };
    testHistory<vector < string >> ("vectors.stringProperty", lambdaMixed, false);

    auto lambda = [] (int i) -> vector<string> {
        // Also test pipe '|' (the separator in our text files) and new line '\n'
        vector<string> v = {"abc" + toString(i), "xy|z" + toString(i), "A\nB" + toString(i)};
        return v;
    };
    testHistory<vector < string >> ("vectors.stringProperty", lambda, testPastConf);
}


void DataLogging_Test::testVectorChar(bool testPastConf) {
    auto lambda = [] (int i) -> vector<char> {

        return {i & 0xFF, i & 0xFF, i & 0xFF, i & 0xFF, i & 0xFF, 0};
    };
    testHistory<vector<char> >("vectors.charProperty", lambda, testPastConf);
}


void DataLogging_Test::testVectorSignedChar(bool testPastConf) {
    auto lambda = [] (int i) -> vector<signed char> {
        std::vector<signed char> result;
        if ((i % 3) != 0) { // every third is empty
            result = {2, -4, 8, -16, 32};
            if ((i % 2) == 0) result.push_back(std::numeric_limits<signed char>::lowest());
            if ((i % 5) == 0) result.push_back(std::numeric_limits<signed char>::max());
        }
        return result;
    };
    testHistory < vector<signed char> >("vectors.int8Property", lambda, testPastConf);
}


void DataLogging_Test::testVectorUnsignedChar(bool testPastConf) {
    auto lambda = [] (int i) -> vector<unsigned char> {
        std::vector<unsigned char> result;
        if ((i % 3) != 0) { // every third is empty
            result = {2, 4, 8, 16, 32};
            if ((i % 2) == 0) result.push_back(0);
            if ((i % 5) == 0) result.push_back(255);
        }
        return result;
    };
    testHistory<vector<unsigned char> >("vectors.uint8Property", lambda, testPastConf);
}


void DataLogging_Test::testVectorBool(bool testPastConf) {
    auto lambda = [](int i) -> vector<bool> {
        if (i % 13 == 0) {
            return vector<bool>();
        } else if (i % 11) {
            return vector<bool>(1, (i % 2 == 0));
        } else {
            return vector<bool>{(i % 2 == 0), (i % 3 == 0), (i % 5 == 0), (i % 7 == 0)};
        }
    };
    testHistory < vector<bool>>("vectors.boolProperty", lambda, testPastConf);
}


void DataLogging_Test::testVectorShort(bool testPastConf) {
    auto lambda = [] (int i) -> vector<short> {
        std::vector<short> result;
        if ((i % 3) != 0) { // every third is empty
            result = {-2, 4, 0, 16, -5000};
            if ((i % 2) == 0) result.push_back(std::numeric_limits<short>::lowest());
            if ((i % 5) == 0) result.push_back(std::numeric_limits<short>::max());
        }
        return result;
    };
    testHistory<vector<short> >("vectors.int16Property", lambda, testPastConf);
}


void DataLogging_Test::testVectorUnsignedShort(bool testPastConf) {
    auto lambda = [] (int i) -> vector<unsigned short> {
        std::vector<unsigned short> result;
        if ((i % 3) != 0) { // every third is empty
            result = {4, 2 * i, 8, 16, 5000};
            if ((i % 2) == 0) result.push_back(std::numeric_limits<unsigned short>::lowest());
            if ((i % 5) == 0) result.push_back(std::numeric_limits<unsigned short>::max());
        }
        return result;
    };
    testHistory<vector<unsigned short> >("vectors.uint16Property", lambda, testPastConf);
}


void DataLogging_Test::testVectorInt(bool testPastConf) {
    auto lambda = [] (int i) -> vector<int> {
        std::vector<int> result;
        if ((i % 3) != 0) { // every third is empty
            result = {2, -4 * i, 8 * i, 16, -5000};
            if ((i % 2) == 0) result.push_back(std::numeric_limits<int>::lowest());
            if ((i % 5) == 0) result.push_back(std::numeric_limits<int>::max());
        }
        return result;
    };
    testHistory<vector<int> >("vectors.int32Property", lambda, testPastConf);
}


void DataLogging_Test::testVectorUnsignedInt(bool testPastConf) {
    auto lambda = [] (int i) -> vector<unsigned int> {
        std::vector<unsigned int> result;
        if ((i % 3) != 0) { // every third is empty
            result = {2, 4 * i, 8, 16, 5000};
            if ((i % 2) == 0) result.push_back(std::numeric_limits<unsigned int>::lowest());
            if ((i % 5) == 0) result.push_back(std::numeric_limits<unsigned int>::max());
        }
        return result;
    };
    testHistory<vector<unsigned int> >("vectors.uint32Property", lambda, testPastConf);
}


void DataLogging_Test::testVectorLongLong(bool testPastConf) {
    auto lambda = [] (int i) -> vector<long long> {
        std::vector<long long> result;
        if ((i % 3) != 0) { // every third is empty
            result = {2ll, -4ll * i, 8ll, 16ll * i, -500055ll};
            if ((i % 2) == 0) result.push_back(std::numeric_limits<long long>::lowest());
            if ((i % 5) == 0) result.push_back(std::numeric_limits<long long>::max());
        }
        return result;
    };
    testHistory<vector<long long> >("vectors.int64Property", lambda, testPastConf);
}


void DataLogging_Test::testVectorUnsignedLongLong(bool testPastConf) {
    auto lambda = [] (int i) -> vector<unsigned long long> {
        std::vector<unsigned long long> result;
        if ((i % 3) != 0) { // every third is empty
            result = {2ull, 4ull, 8ull, 16ull, 500055ull * i};
            if ((i % 2) == 0) result.push_back(std::numeric_limits<unsigned long long>::lowest());
            if ((i % 5) == 0) result.push_back(std::numeric_limits<unsigned long long>::max());
        }
        return result;
    };
    testHistory<vector<unsigned long long> >("vectors.uint64Property", lambda, testPastConf);
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


void DataLogging_Test::testChar(bool testPastConf) {
    auto lambda = [] (int i) -> char {
        return static_cast<char>(i & 0xff);
    };
    testHistory<char>("charProperty", lambda, testPastConf);
}


void DataLogging_Test::testNans() {
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
    const size_t max_set = 100ul;
    const size_t full_return_size = max_set + 1ul;
    std::clog << "Testing NaN and infinity are treated by Loggers " << std::flush;

    // define some bad floating points to test against
    const vector<float> bad_floats = {std::numeric_limits<float>::quiet_NaN(),
                                      std::numeric_limits<float>::signaling_NaN(),
                                      std::numeric_limits<float>::infinity(),
                                      -1.f * std::numeric_limits<float>::infinity()};
    const vector<double> bad_doubles = {std::numeric_limits<double>::quiet_NaN(),
                                        std::numeric_limits<double>::signaling_NaN(),
                                        std::numeric_limits<double>::infinity(),
                                        -1. * std::numeric_limits<double>::infinity()};

    // save this instant as a iso string
    Epochstamp es_beforeWrites;
    std::string beforeWrites = es_beforeWrites.toIso8601();

    // Collect stamps for when each bad floating point has been set (once) - to later test slotGetConfigurationFromPast.
    // Use std::min with max_set as protection (max_set _should_ always be larger...)
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

        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set(m_deviceId, new_conf));
        const Hash cfg = m_deviceClient->get(m_deviceId);
        updateStamps.push_back(Epochstamp::fromHashAttributes(cfg.getAttributes("doubleProperty")));
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
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
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set(m_deviceId, end_conf));
    updateStamps.push_back(Epochstamp::fromHashAttributes(m_deviceClient->get(m_deviceId).getAttributes("doubleProperty")));
    // The sleep interval below had to be increased because of the Telegraf environment - the time required to save is
    // higher. If es_afterWrites captured after the sleep instruction refers to a time point that comes before the time
    // Telegraf + Influx are done writing the data, the property history will not be of the expected size and the test
    // will fail.
    boost::this_thread::sleep(boost::posix_time::milliseconds(WAIT_WRITES));

    // save this instant as a iso string
    Epochstamp es_afterWrites;
    std::string afterWrites = es_afterWrites.toIso8601();

    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                            .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());

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
        // TODO: Remove beforeFistsCheck, afterLastCheck and the printout of the statistics for obtaining history a.s.a.
        //       load on exflserv10 gets normal.
        Epochstamp beforeFirstCheck;
        while (nTries >= 0 && history.size() != property_pair.second) {
            std::string device, property;
            try {
                numChecks++;
                // TODO: use the deviceClient to retrieve the property history
                //history = m_deviceClient->getPropertyHistory(m_deviceId, key, before, after, max_set * 2);
                m_sigSlot->request(dlreader0, "slotGetPropertyHistory", m_deviceId, property_pair.first, params)
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

        Epochstamp afterLastCheck;
        if (static_cast<size_t> (property_pair.second), history.size()) {
            std::clog << "\ntestNans: History size check for property '"
                    << property_pair.first << "' succeeded after " << numChecks << " attempt(s) ranging from "
                    << beforeFirstCheck.toIso8601() << " to " << afterLastCheck.toIso8601()
                    << " (" << afterLastCheck.elapsed(beforeFirstCheck).getTotalSeconds() << " secs)." << std::endl;
        }

        CPPUNIT_ASSERT_EQUAL_MESSAGE("History size different than expected after " + toString(numChecks) +
                                     " checks:\n\tdeviceId: " + m_deviceId +
                                     "\n\tproperty : " + property_pair.first +
                                     "\n\tparam.from: " + beforeWrites +
                                     "\n\tparam.to: " + afterWrites + "\n\tparam.maxNumData: " + toString(max_set * 2) +
                                     "\n\thistory.size(): " + toString(history.size()) +
                                     "\n\tNumber of Exceptions: " + toString(numExceptions) +
                                     "\n\tExceptions:\n" + boost::algorithm::join(exceptionsMsgs, "\n"),
                                     static_cast<size_t> (property_pair.second), history.size());

        // Test that the return values match, incl. timestamps
        for (size_t i = 0; i <= max_set; ++i) {
            // First check timestamp - to microsecond precision
            const Epochstamp historyStamp = Epochstamp::fromHashAttributes(history[i].getAttributes("v"));
            const TimeDuration diff = historyStamp.elapsed(updateStamps[i]);
            CPPUNIT_ASSERT_MESSAGE(toString(diff), diff < TimeDuration(0ull, 1000000000000ull)); // 1e12 attosec, i.e. 1 microsec

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
                CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(i), static_cast<int> (i), history[i].get<int>("v"));
            }
        }
    }

    // Now test slotGetConfigurationFromPast with infinite values
    for (size_t i = 0; i < vec_es_afterWrites.size(); ++i) {
        Hash conf;
        Schema schema;
        bool configAtTimepoint = false;
        std::string configTimepoint;
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast",
                                                   m_deviceId, vec_es_afterWrites[i].toIso8601())
                                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                .receive(conf, schema, configAtTimepoint, configTimepoint));

        CPPUNIT_ASSERT(configAtTimepoint);
        // This equality check relies on the fact that the string representation implicitly rounds to micro second
        // precision, i.e. the precision in the data base. So if the test fails here, do like above with the TimeDuration.
        CPPUNIT_ASSERT_EQUAL(vec_es_updateStamps[i].toIso8601Ext(),
                             configTimepoint);
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
    std::clog << "Ok" << std::endl;

}


void DataLogging_Test::testSchemaEvolution() {

    std::clog << "Testing property history retrieval when schema evolution happens ..." << std::endl;

    // Instantiates a DataLogTestDevice to use for the schema evolution test.
    // "deviceIdPrefix" allows concurrent Influx tests on the different platform CI runners.
    const std::string deviceId(deviceIdPrefix + "SchemaEvolutionDevice");
    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    auto success = m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId),
                                               KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Checks that the instantiated device is being logged.
    CPPUNIT_ASSERT_MESSAGE(toString(m_deviceClient->get<std::vector < std::string >> (loggerId, "devicesToBeLogged")),
                           waitForCondition([this, &loggerId, &deviceId]() {
                               auto loggedIds =
                                       m_deviceClient->get<std::vector < std::string >> (loggerId, "devicesToBeLogged");
                                            return (std::find(loggedIds.begin(), loggedIds.end(), deviceId) != loggedIds.end());
                           },
                                            KRB_TEST_MAX_TIMEOUT * 1000)
                           );

    // Captures the timepoint before any property modification.
    Epochstamp fromTimePoint;

    // "reconfigurableValue" as string
    Schema schemaStr;
    STRING_ELEMENT(schemaStr).key("reconfigurableValue")
            .assignmentOptional().defaultValue("")
            .reconfigurable()
            .commit();
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStr)
                            .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                            .receive());
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotReconfigure", Hash("reconfigurableValue", "Non empty str"))
                            .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                            .receive());

    // "reconfigurableValue" as vector of strings
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotUpdateSchema", Schema())
                            .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                            .receive());
    Schema schemaVecStr;
    VECTOR_STRING_ELEMENT(schemaVecStr).key("reconfigurableValue")
            .assignmentOptional().defaultValue(std::vector<std::string>({"a"}))
            .reconfigurable()
            .commit();
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotUpdateSchema", schemaVecStr)
                            .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                            .receive());
    Hash strVecValueCfg;
    std::vector<std::string> &strVector = strVecValueCfg.bindReference<std::vector < std::string >> ("reconfigurableValue");
    strVector = std::vector<std::string>{"a", "", "b", "c"};
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotReconfigure", strVecValueCfg)
                            .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                            .receive());


    // "reconfigurableValue" as int32
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotUpdateSchema", Schema())
                            .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                            .receive());
    Schema schemaInt32;
    INT32_ELEMENT(schemaInt32).key("reconfigurableValue")
            .assignmentOptional().defaultValue(0)
            .reconfigurable()
            .commit();
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotUpdateSchema", schemaInt32)
                            .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                            .receive());
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(deviceId, "slotReconfigure", Hash("reconfigurableValue", 10))
                            .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                            .receive());


    // Makes sure all the writes are done before retrieval.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(karabo::util::DATALOGGER_PREFIX + m_server, "flush")
                            .timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());

    // The sleep interval below had to be added because of the Telegraf environment - the time required to save is
    // higher. If toTimePoint captured after the sleep instruction refers to a time point that comes before the time
    // Telegraf + Influx are done writing the data, the property history will not be of the expected size and the test
    // will fail.
    boost::this_thread::sleep(boost::posix_time::milliseconds(WAIT_WRITES));

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
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive(replyDevice, replyProperty, history);
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
                                 " checks:\n\tdeviceId: " + deviceId +
                                 "\n\tproperty: \"value\"" +
                                 "\n\tparam.from: " + fromTimePoint.toIso8601() +
                                 "\n\tparam.to: " + toTimePoint.toIso8601() +
                                 "\n\tparam.maxNumData: 10" + toString(maxNumData) +
                                 "\n\thistory.size(): " + toString(history.size()) +
                                 "\n\tNumber of Exceptions: " + toString(numExceptions) +
                                 "\n\tExceptions:\n" + boost::algorithm::join(exceptionsMsgs, "\n"),
                                 6,
                                 static_cast<int> (history.size()));

    // Checks the first two expected values - of string type.
    // The first is the empty string that is the default value.
    // The second is an explicitily set non empty string value.
    CPPUNIT_ASSERT_EQUAL_MESSAGE("First string value different from expected for history entry",
                                 std::string(""),
                                 history[0].get<std::string>("v"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Second string value different from expected for history entry",
                                 std::string("Non empty str"),
                                 history[1].get<std::string>("v"));
    // Checks the next two expected values - of vector of strings type.
    // The first is the empty vector that is the default value.
    // The second is an explicitily set non empty vector.

    CPPUNIT_ASSERT_EQUAL_MESSAGE("First vector of strings in history is not of the expected size",
                                 1ul,
                                 history[2].get<std::vector < std::string >> ("v").size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Value of sole element of first vector of strings in history different from exepected",
                                 std::string("a"),
                                 history[2].get<std::vector < std::string >> ("v")[0]);

    const auto &strVectorValue = history[3].get<std::vector < std::string >> ("v");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Size of second vector of strings in history different from expected",
                                 strVector.size(),
                                 strVectorValue.size());
    for (size_t i = 0; i < strVectorValue.size(); i++) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Value at index " + toString(i) + " of second vector of strings in history different from expected",
                                     strVector[i],
                                     strVectorValue[i]);
    }
    // Checks the last expected values - of int type.
    // The first is the default value of 0.
    // The second is an explicitily set int value.
    CPPUNIT_ASSERT_EQUAL_MESSAGE("First int value different from expected for history entry",
                                 0,
                                 history[4].get<int>("v"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Second int value different from expected for history entry",
                                 10,
                                 history[5].get<int>("v"));


    std::clog << "Ok" << std::endl;
}

// TODO: implement testVectorFloat() (include vectors of different length and empty vector);
// TODO: implement test and FIX for a vector of strings with an empty string as its only element

// TODO: ideally, all properties of the PropertyTest device should be implemented,
//       to add them one should add a method per property and add the proper cppunit helpers at the beginning of this
//       file


