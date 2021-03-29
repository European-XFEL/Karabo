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

CPPUNIT_TEST_SUITE_REGISTRATION(DataLogging_Test);

void DataLogging_Test::fileAllTestRunner() {
    std::pair<bool, std::string> success = m_deviceClient->instantiate(m_server, "PropertyTest",
                                                                       Hash("deviceId", m_deviceId),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    setPropertyTestSchema();

    std::clog << "\n==== Starting sequence of File Logging tests ====" << std::endl;
    success = startDataLoggerManager("FileDataLogger");
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();
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
    testLastKnownConfiguration(m_fileMigratedDataEndsBefore, m_dataWasMigrated);

    // These deal with their own devices, so comment above about using the PropertyTest instance
    // in m_deviceId is not applicable.
    testCfgFromPastRestart();

    // TODO: Uncomment test below as soon as FileLogReader::slotGetPropertyHistoryImpl is fixed.
    //       Currently it is failing to retrieve all the logged entries (see comment on discussions of
    //       https://git.xfel.eu/gitlab/Karabo/Framework/merge_requests/4455).
    //testSchemaEvolution();
    testNans();
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
        for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(p), {})) {
            std::ostringstream msg;
            msg << "Check if " << entry << " was migrated OK: "<<boost::filesystem::extension(entry);
            std::clog<<msg.str()<<std::endl;
            CPPUNIT_ASSERT_MESSAGE(msg.str(), boost::filesystem::extension(entry) == ".ok");

        }
    }

    unsigned int errorCount = 0;
    boost::filesystem::path perr(migrationResultsPath + "/part_processed/"+m_deviceId+"/");
    if(boost::filesystem::is_directory(perr)) {
        for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(perr), {})) {
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


void DataLogging_Test::influxAllTestRunnerWithDataMigration() {
    m_keepLoggerDirectory = false;

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
    success = startDataLoggerManager("InfluxDataLogger");
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();

    testMaxNumDataRange();

    // NOTE: The testMigrateFileLoggerData test assumes it is running from a directory inside the
    //       Framework source tree, which is not true when the tests are run in a Conda
    //       environment with the Framework installed as a package. As a temporary workaround,
    //       we skip the test if we are not inside the source tree.
    // TODO: Provide a more robust and Conda/CI friendly solution and get rid of this klundge.
    //       One possible approach: separate the file logger from the influx logger tests. The file logger
    //       test can be run on a job that belongs to a pipeline stage prior to the one that contains
    //       the influx logger test. The generated file logging tests would them be temporary
    //       CI artifacts passed from one stage to the other.
    const boost::filesystem::path migratorPackageBase("../../../src/pythonKarabo");
    boost::system::error_code ec;
    if (boost::filesystem::exists(migratorPackageBase, ec)) {
        testMigrateFileLoggerData();
    } else {
        std::clog << "Migrator script not available - skipping migration test." << std::endl;
    }

    testMaxNumDataHistory();
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
    testLastKnownConfiguration(m_fileMigratedDataEndsBefore, m_dataWasMigrated);

    // These deal with their own devices, so comment above about using the PropertyTest instance
    // in m_deviceId is not applicable.
    testCfgFromPastRestart();
    testSchemaEvolution();
    testNans();
}


void DataLogging_Test::testNoInfluxServerHandling() {

    std::clog << "Testing handling of no Influx Server available scenarios ..." << std::endl;

    std::pair<bool, std::string> success = m_deviceClient->instantiate(m_server, "PropertyTest",
                                                                       Hash("deviceId", m_deviceId),
                                                                       KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Starts the logger and readers with invalid InfluxDB (or Telegraf) URLs.
    success = startDataLoggerManager("InfluxDataLogger", true);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated(false);

    // The DataLogger should be in ERROR state.
    int timeout = KRB_TEST_MAX_TIMEOUT * 1000; // milliseconds
    karabo::util::State loggerState = karabo::util::State::UNKNOWN;
    std::string loggerStatus;
    const std::string &dataLoggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    while (timeout > 0) {
        loggerState = m_deviceClient->get<karabo::util::State>(dataLoggerId, "state");
        loggerStatus = m_deviceClient->get<std::string>(dataLoggerId, "status");
        if (loggerState == karabo::util::State::ERROR) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(50));
        timeout -= 50;
    }
    CPPUNIT_ASSERT_MESSAGE("Timeout while waiting for DataLogger '" + dataLoggerId + "' to reach ERROR state.",
                           loggerState == karabo::util::State::ERROR);

    std::clog << "... Influx logger in ERROR state, as expected, with status '" << loggerStatus << "'" << std::endl;

    // Any attempt to recover a configuration from Influx should fail when the Influx Server is not
    // available.
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
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
                || (exc.detailedMsg().find("Reading from InfluxDB failed") != std::string::npos)
                || (exc.detailedMsg().find("Connection reset by peer"));
        CPPUNIT_ASSERT_MESSAGE(std::string("Unexpected RemoteException while handling no Influx server:\n'")
                               + exc.detailedMsg() + "'\n",
                               condition);
        remoteExceptionCaught = true;
    }

    CPPUNIT_ASSERT(remoteExceptionCaught);

    std::clog << "... request failed with RemoteException as expected." << std::endl;

    // By simply starting the devices related to Influx logging, some logging writing activity takes place.
    // If this point of the test is reached with invalid urls configured for both reading and writing to the
    // Influx (or Telegraf) server, it is safe to conclude that the Influx Logger doesn't get compromissed by a
    // server not available condition - the host of the Influx logger is the same process that runs this test.

    std::clog << "OK" << std::endl;
}
