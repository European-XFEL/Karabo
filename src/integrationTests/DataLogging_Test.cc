/*
 * File:   DataLogging_Test.cc
 * Author: silenzi
 *
 * Created on Nov 12, 2018, 6:07:07 PM
 */

#include "DataLogging_Test.hh"

#include <boost/filesystem.hpp>
#include <cstdlib>
#include <karabo/net/EventLoop.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include <karabo/util/StringTools.hh>
#include <sstream>

#include "karabo/util/DataLogUtils.hh"

USING_KARABO_NAMESPACES;

CPPUNIT_TEST_SUITE_REGISTRATION(DataLogging_Test);

void DataLogging_Test::fileAllTestRunner() {
    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", m_deviceId), KRB_TEST_MAX_TIMEOUT);
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

    testUnchangedNoDefaultProperties();
    // This must be the last test case that relies on the device in m_deviceId (the logged
    // PropertyTest instance) being available at the start of the test case.
    // 'testLastKnownConfiguration' stops the device being logged to make sure that the
    // last known configuration can be successfully retrieved after the device is gone.
    testLastKnownConfiguration(m_fileMigratedDataEndsBefore, m_dataWasMigrated);

    // These deal with their own devices, so comment above about using the PropertyTest instance
    // in m_deviceId is not applicable.
    testCfgFromPastRestart(true); // old, past device incarnation stamps are lept in file based logging

    // TODO: Uncomment test below as soon as FileLogReader::slotGetPropertyHistoryImpl is fixed.
    //       Currently it is failing to retrieve all the logged entries (see comment on discussions of
    //       https://git.xfel.eu/gitlab/Karabo/Framework/merge_requests/4455).
    // testSchemaEvolution();
    testNans();

    // At the end we shutdown the logger manager and try to bring it back in a bad state
    // Needs that the working logger was there before.
    testFailingManager();
}

void DataLogging_Test::testMigrateFileLoggerData() {
    // launch the migration script onto the logged path

    const std::string influxUrlWrite =
          (getenv("KARABO_INFLUXDB_WRITE_URL") ? getenv("KARABO_INFLUXDB_WRITE_URL") : "http://localhost:8086");
    const std::string influxUrlRead =
          (getenv("KARABO_INFLUXDB_QUERY_URL") ? getenv("KARABO_INFLUXDB_QUERY_URL") : "http://localhost:8086");
    const std::string influxDbName =
          (getenv("KARABO_INFLUXDB_DBNAME")
                 ? getenv("KARABO_INFLUXDB_DBNAME")
                 : (getenv("KARABO_BROKER_TOPIC") ? getenv("KARABO_BROKER_TOPIC") : getenv("USER")));
    const std::string influxUserWrite =
          (getenv("KARABO_INFLUXDB_WRITE_USER") ? getenv("KARABO_INFLUXDB_WRITE_USER") : std::string("infadm"));
    const std::string influxPwdWrite =
          (getenv("KARABO_INFLUXDB_WRITE_PASSWORD") ? getenv("KARABO_INFLUXDB_WRITE_PASSWORD") : std::string("admpwd"));
    const std::string influxUserRead =
          (getenv("KARABO_INFLUXDB_QUERY_USER") ? getenv("KARABO_INFLUXDB_QUERY_USER") : influxUserWrite);
    const std::string influxPwdRead =
          (getenv("KARABO_INFLUXDB_QUERY_PASSWORD") ? getenv("KARABO_INFLUXDB_QUERY_PASSWORD") : influxPwdWrite);
    const std::string absLoggerPath = boost::filesystem::absolute("./" + m_fileLoggerDirectory).string();
    const std::string migrationResultsPath = absLoggerPath + std::string("/migrationresults");
    std::ostringstream cmd;
    cmd << "cd ../../../src/pythonKarabo; ../../karabo/extern/bin/python3 ";
    cmd << "karabo/influxdb/dl_migrator.py ";

    cmd << influxDbName << " " << absLoggerPath << "/karaboHistory/"
        << " " << migrationResultsPath << " ";
    cmd << "--write-url " << boost::algorithm::replace_first_copy(influxUrlWrite, "tcp://", "http://") << " ";
    cmd << "--write-user " << influxUserWrite << " ";
    cmd << "--write-pwd " << influxPwdWrite << " ";
    cmd << "--read-url " << boost::algorithm::replace_first_copy(influxUrlRead, "tcp://", "http://") << " ";
    cmd << "--read-user " << influxUserRead << " ";
    cmd << "--read-pwd " << influxPwdRead << " ";
    cmd << "--lines-per-write 200 --write-timeout 50 --concurrent-tasks 2";

    const int ret = system(cmd.str().c_str());
    CPPUNIT_ASSERT_EQUAL(0, ret);

    boost::filesystem::path p(migrationResultsPath + "/processed/" + m_deviceId + "/");
    if (boost::filesystem::is_directory(p)) {
        for (auto &entry : boost::make_iterator_range(boost::filesystem::directory_iterator(p), {})) {
            std::ostringstream msg;
            msg << "Check if " << entry << " was migrated OK: " << boost::filesystem::extension(entry);
            std::clog << msg.str() << std::endl;
            CPPUNIT_ASSERT_MESSAGE(msg.str(), boost::filesystem::extension(entry) == ".ok");
        }
    }

    unsigned int errorCount = 0;
    boost::filesystem::path perr(migrationResultsPath + "/part_processed/" + m_deviceId + "/");
    if (boost::filesystem::is_directory(perr)) {
        for (auto &entry : boost::make_iterator_range(boost::filesystem::directory_iterator(perr), {})) {
            // print out the error
            std::ostringstream cmd;
            cmd << "cat " << entry;
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

    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", m_deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    setPropertyTestSchema();

    // Starts the same set of tests with InfluxDb logging instead of text-file based logging
    std::clog << "\n==== Starting sequence of Influx Logging tests on \"" << m_deviceId << "\" ====" << std::endl;
    success = startDataLoggerManager("InfluxDataLogger");
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();

    testMaxNumDataRange();

    // NOTE: The testMigrateFileLoggerData test assumes it is running from a directory inside the
    //       Framework source tree, which is not true when the tests are run in a Conda
    //       environment with the Framework installed as a package. As a temporary workaround,
    //       we skip the test if we are not inside the source tree.
    // TODO: Provide a more robust and Conda/CI friendly solution and get rid of this coupling.
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
    testDropBadData();
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
    testLastKnownConfiguration(m_fileMigratedDataEndsBefore, m_dataWasMigrated);

    // These deal with their own devices, so comment above about using the PropertyTest instance
    // in m_deviceId is not applicable.
    testCfgFromPastRestart(
          false); // in influx logging, old, past device incarnation stamps are logged as start of device logging
    testSchemaEvolution();
    testNans();
}


void DataLogging_Test::testInfluxMaxSchemaLogRate() {
    std::clog << "Testing enforcing of max schema logging rate limit for Influx ..." << std::endl;

    const unsigned int rateWinSecs = 2u;

    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
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
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Starts the logger and readers with a lower max schema rate threshold - 11 kb/s - over a rateWinSecs seconds
    // rating window. The 11 kb/s comes from the verified size of the different device schemas used in the
    // test - 6.258 bytes.
    success = startDataLoggerManager("InfluxDataLogger", false, false, 32, rateWinSecs, 11, rateWinSecs);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();

    // Wait some time to isolate the schema update bursts.
    boost::this_thread::sleep(boost::posix_time::milliseconds(rateWinSecs * 1000 - 500));

    ///////  Checks that a schema update within the rating limit is accepted.
    Epochstamp beforeFirstBurst;
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStrA).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    // Makes sure that data has been written to Influx.
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", SLOT_REQUEST_TIMEOUT_MILLIS / 1000));
    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));
    Epochstamp afterFirstBurst;

    // Checks that the schema update has not been flagged as bad data.
    Hash badDataAllDevices;
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot
                ->request(dlreader0, "slotGetBadData", beforeFirstBurst.toIso8601Ext(), afterFirstBurst.toIso8601Ext())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(badDataAllDevices));
    CPPUNIT_ASSERT_EQUAL(0ul, badDataAllDevices.size());

    // Wait some time to isolate the schema update bursts.
    boost::this_thread::sleep(boost::posix_time::milliseconds(rateWinSecs * 1000 - 500));

    ////////  Checks that two schema updates in a fast succession would go above the
    ////////  threshold and one of the updates (the second) would be rejected.
    Epochstamp beforeSecondBurst;
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStrB).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStrC).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    // Makes sure that data has been written to Influx.
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", SLOT_REQUEST_TIMEOUT_MILLIS / 1000));
    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));
    Epochstamp afterSecondBurst;

    // Checks that one of the schema updates failed.
    badDataAllDevices.clear();
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot
                                  ->request(dlreader0, "slotGetBadData", beforeSecondBurst.toIso8601Ext(),
                                            afterSecondBurst.toIso8601Ext())
                                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                  .receive(badDataAllDevices));
    CPPUNIT_ASSERT_EQUAL(1ul, badDataAllDevices.size());
    CPPUNIT_ASSERT(badDataAllDevices.has(deviceId));
    const auto &badDataEntries = badDataAllDevices.get<std::vector<Hash>>(deviceId);
    CPPUNIT_ASSERT_EQUAL(1ul, badDataEntries.size());
    const std::string &badDataInfo = badDataEntries[0].get<std::string>("info");
    CPPUNIT_ASSERT_MESSAGE(
          "Expected pattern, '" + deviceId + "::schema', not found in bad data description:\n'" + badDataInfo + "'",
          badDataInfo.find(deviceId + "::schema") != std::string::npos);

    // Wait some time to isolate the schema update bursts.
    boost::this_thread::sleep(boost::posix_time::milliseconds(rateWinSecs * 1000 - 500));

    //////  Checks that after the updates have settled down for a while, schemas
    //////  can be logged again.
    Epochstamp beforeThirdBurst;
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStrD).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    // Makes sure that data has been written to Influx.
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", SLOT_REQUEST_TIMEOUT_MILLIS / 1000));
    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));
    Epochstamp afterThirdBurst;
    // Checks that the schema update succeeded.
    badDataAllDevices.clear();
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot
                ->request(dlreader0, "slotGetBadData", beforeThirdBurst.toIso8601Ext(), afterThirdBurst.toIso8601Ext())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(badDataAllDevices));
    CPPUNIT_ASSERT_EQUAL(0ul, badDataAllDevices.size());

    // Checks that the latest version of the schema, and by consequence, the past configuration are retrieved correctly.
    // This check fails for schemas saved by versions of the InfluxDataLogger prior to the fixes in
    // https://git.xfel.eu/Karabo/Framework/-/merge_requests/6470 and retrieved with versions of the InfluxLogReader
    // prior to the modifications in https://git.xfel.eu/Karabo/Framework/-/merge_requests/6478.
    Schema schema;
    Hash pastCfg;
    bool cfgAtTime;
    std::string cfgTime;
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", deviceId, afterThirdBurst.toIso8601())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(pastCfg, schema, cfgAtTime, cfgTime));
    CPPUNIT_ASSERT_MESSAGE("Schema lacks expected key, \"stringPropertyD\"", schema.has("stringPropertyD"));
    CPPUNIT_ASSERT_EQUAL(Types::STRING, schema.getValueType("stringPropertyD"));
    CPPUNIT_ASSERT_EQUAL(pastCfg.get<std::string>("stringPropertyD"), "D_" + defValueSuffix);

    std::clog << "OK" << std::endl;
}


void DataLogging_Test::testInfluxMaxStringLength() {
    std::clog << "Testing enforcing of max string value length for Influx ..." << std::endl;

    const unsigned int maxStringLength = 8'192u;
    const std::string belowLimitStr(maxStringLength / 2, 'B');
    const std::string atLimitStr(maxStringLength, '@');
    const std::string aboveLimitStr(maxStringLength * 2, 'A');

    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
    // A device exclusive for this test case is used to guarantee that its
    // schema will be partitioned into multiple chunks. The default schema for
    // the PropertyTest device has around 78 Kb and for this test the
    // value used for the Influx logger "maxStringValueLength" property is 8 kb.
    const std::string propTestDevice = m_deviceId + "__MAX__STRING";

    const unsigned int afterFlushWait = 500u;

    Epochstamp beforeServerInstantiation;

    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", propTestDevice), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    success = startDataLoggerManager("InfluxDataLogger", false, false, 5120u, 5u, 15'360u, 5u, maxStringLength);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();

    ///////  Checks that a string below the length limit is accepted
    Epochstamp beforeBelowLimit;
    m_deviceClient->set(propTestDevice, "stringProperty", belowLimitStr);
    // Makes sure the data has been written to Influx
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());
    boost::this_thread::sleep(boost::posix_time::milliseconds(afterFlushWait));
    Epochstamp afterBelowLimit;

    Hash badDataAllDevices;
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot
                ->request(dlreader0, "slotGetBadData", beforeBelowLimit.toIso8601Ext(), afterBelowLimit.toIso8601Ext())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(badDataAllDevices));
    CPPUNIT_ASSERT_EQUAL(0ul, badDataAllDevices.size());

    ///////  Checks that a string whose length is exactly at the limit is accepted.
    Epochstamp beforeAtLimit;
    m_deviceClient->set(propTestDevice, "stringProperty", atLimitStr);
    // Makes sure the data has been written to Influx.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());
    boost::this_thread::sleep(boost::posix_time::milliseconds(afterFlushWait));
    Epochstamp afterAtLimit;

    badDataAllDevices.clear();
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(dlreader0, "slotGetBadData", beforeAtLimit.toIso8601Ext(), afterAtLimit.toIso8601Ext())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(badDataAllDevices));
    CPPUNIT_ASSERT_EQUAL(0ul, badDataAllDevices.size());


    ///////  Checks that a string above the length limit is rejected with the proper code.
    Epochstamp beforeAboveLimit;
    m_deviceClient->set(propTestDevice, "stringProperty", aboveLimitStr);
    // Makes sure the data has been written to Influx.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());
    boost::this_thread::sleep(boost::posix_time::milliseconds(afterFlushWait));
    Epochstamp afterAboveLimit;

    badDataAllDevices.clear();
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot
                ->request(dlreader0, "slotGetBadData", beforeAboveLimit.toIso8601Ext(), afterAboveLimit.toIso8601Ext())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(badDataAllDevices));
    CPPUNIT_ASSERT_EQUAL(1ul, badDataAllDevices.size());
    const auto deviceBadData = badDataAllDevices.get<std::vector<Hash>>(propTestDevice);
    const std::string badDataInfo = deviceBadData[0].get<std::string>("info");

    // [1] is the code for string metric values longer than the Influx limit.
    CPPUNIT_ASSERT_MESSAGE(
          "Expected pattern, \">> [1] 'stringProperty'\", not found in bad data description:\n'" + badDataInfo + "'",
          badDataInfo.find(">> [1] 'stringProperty") != std::string::npos);

    ///////  Checks that the PropertyTest device with the appended prefix had its schema properly chunked by
    ///////  asserting that the past configuration with a string logged exactly at the limit can be retrieved.
    Schema schema;
    Hash pastCfg;
    bool cfgAtTime;
    std::string cfgTime;
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", propTestDevice, afterAtLimit.toIso8601())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(pastCfg, schema, cfgAtTime, cfgTime));
    CPPUNIT_ASSERT_MESSAGE("Schema lacks expected key, \"stringProperty\"", schema.has("stringProperty"));
    CPPUNIT_ASSERT_EQUAL(Types::STRING, schema.getValueType("stringProperty"));
    CPPUNIT_ASSERT_EQUAL(pastCfg.get<std::string>("stringProperty"), atLimitStr);

    std::clog << "OK" << std::endl;
}


void DataLogging_Test::testInfluxMaxPerDevicePropLogRate() {
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
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);

    const std::string str32Kb(32768ul, 'A');
    const std::string str8Kb(8192ul, 'B');

    const TimeValue millisecInAtto = 1'000'000'000'000'000ull; // Resolution of fractional seconds is AttoSec (10^-18).

    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          m_server, "DataLogTestDevice", Hash("deviceId", m_deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Starts the logger and readers with a lower max rate threshold - 32 kb/s - over a rateWinSecs seconds rating
    // window.
    success = startDataLoggerManager("InfluxDataLogger", false, false, 32, rateWinSecs);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();

    // Checks that writing 32Kb of data is within the log rate tolerance.
    Epochstamp before32KbWrite;
    for (unsigned int i = 0; i < 4 * rateWinSecs; i++) {
        Hash updateProp = Hash("stringProperty", str8Kb);
        Epochstamp updateEpoch(before32KbWrite + TimeDuration(0, (i + 1) * millisecInAtto));
        Timestamp updateTime(updateEpoch, Trainstamp());
        updateTime.toHashAttributes(updateProp.getAttributes("stringProperty"));
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(m_deviceId, "slotUpdateConfigGeneric", updateProp)
                                      .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                      .receive());
    }
    // after32KbWrite is set to be the timestamp of the last write performed in the
    // previous loop plus a safety margin.
    Epochstamp after32KbWrite(before32KbWrite + TimeDuration(0, 5 * rateWinSecs * millisecInAtto));
    // Make sure that data has been written to Influx.
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", SLOT_REQUEST_TIMEOUT_MILLIS / 1000));
    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));

    // Checks that the 32Kb strings have not been flagged as bad data.
    Hash badDataAllDevices;
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(dlreader0, "slotGetBadData", before32KbWrite.toIso8601Ext(), after32KbWrite.toIso8601Ext())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(badDataAllDevices));
    CPPUNIT_ASSERT_EQUAL(0ul, badDataAllDevices.size());
    // Checks that the 8Kb strings have been successfully logged.
    std::vector<Hash> history;
    Hash historyParams{
          "from", before32KbWrite.toIso8601Ext(), "to", after32KbWrite.toIso8601Ext(), "maxNumData", maxPropHistSize};
    std::string replyDevice;
    std::string replyProperty;
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(dlreader0, "slotGetPropertyHistory", m_deviceId, "stringProperty", historyParams)
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(replyDevice, replyProperty, history));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("stringProperty history size different from expected.", 4ul * rateWinSecs,
                                 history.size());
    for (unsigned int i = 0; i < 4 * rateWinSecs; i++) {
        std::string &historyStr8Kb = history[i].get<std::string>("v");
        CPPUNIT_ASSERT_EQUAL_MESSAGE("stringProperty value doesn't have expected size.", 8192ul, historyStr8Kb.size());
        CPPUNIT_ASSERT_EQUAL_MESSAGE("stringProperty value doesn't have expected characters.", str8Kb.at(0ul),
                                     historyStr8Kb.at(0ul));
    }

    // Checks that updating a string property constantly above the rate will cause data to be rejected.
    // Use ratesWinSecs seconds after the time of the most recent write plus a safety margin of 4 milliseconds
    // as the starting time to guarantee that we have a complete independent rating window for the upcoming
    // burst.
    Epochstamp before64KbWrite(after32KbWrite + TimeDuration(rateWinSecs, 4 * millisecInAtto));
    for (unsigned int i = 0; i < 8 * rateWinSecs; i++) {
        Hash updateProps = Hash("stringProperty", str8Kb, "int32Property", 10);
        Epochstamp updateEpoch(before64KbWrite + TimeDuration(0, (i + 1) * millisecInAtto));
        Timestamp updateTime(updateEpoch, Trainstamp());
        updateTime.toHashAttributes(updateProps.getAttributes("stringProperty"));
        updateTime.toHashAttributes(updateProps.getAttributes("int32Property"));
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(m_deviceId, "slotUpdateConfigGeneric", updateProps)
                                      .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                      .receive());
    }
    Epochstamp after64KbWrite(before64KbWrite + TimeDuration(0, 9 * rateWinSecs * millisecInAtto));
    // Make sure that data has been written to Influx.
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", SLOT_REQUEST_TIMEOUT_MILLIS / 1000));
    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));

    // Checks that the half of the stringProperty updates has exceeded the max log rate and has been rated as bad data.
    badDataAllDevices.clear();
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(dlreader0, "slotGetBadData", before64KbWrite.toIso8601Ext(), after64KbWrite.toIso8601Ext())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(badDataAllDevices));
    CPPUNIT_ASSERT_EQUAL(1ul,
                         badDataAllDevices.size()); // 1 is because the bad data is grouped under a single deviceId.
    CPPUNIT_ASSERT_EQUAL(4ul * rateWinSecs, badDataAllDevices.get<std::vector<karabo::util::Hash>>(m_deviceId).size());
    // Checks that half of the 8Kb strings written have been successfully set as property values.
    history.clear();
    historyParams.set<std::string>("from", before64KbWrite.toIso8601Ext());
    historyParams.set<std::string>("to", after64KbWrite.toIso8601Ext());
    historyParams.set<int>("maxNumData", maxPropHistSize);
    replyDevice.clear();
    replyProperty.clear();
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(dlreader0, "slotGetPropertyHistory", m_deviceId, "stringProperty", historyParams)
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(replyDevice, replyProperty, history));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("stringProperty history size different from expected.", 4ul * rateWinSecs,
                                 history.size());
    for (unsigned int i = 0; i < 4 * rateWinSecs; i++) {
        const std::string historyStr8Kb = history[i].get<std::string>("v");
        CPPUNIT_ASSERT_EQUAL_MESSAGE("stringProperty value doesn't have expected size.", 8192ul, historyStr8Kb.size());
        CPPUNIT_ASSERT_EQUAL_MESSAGE("stringProperty value doesn't have expected characters.", str8Kb.at(0ul),
                                     historyStr8Kb.at(0ul));
    }

    // Checks that the int32Property updates were successfully logged even though the stringProperty was blocked.
    history.clear();
    historyParams.set<std::string>("from", before64KbWrite.toIso8601Ext());
    historyParams.set<std::string>("to", after64KbWrite.toIso8601Ext());
    historyParams.set<int>("maxNumData", maxPropHistSize);
    replyDevice.clear();
    replyProperty.clear();
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(dlreader0, "slotGetPropertyHistory", m_deviceId, "int32Property", historyParams)
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(replyDevice, replyProperty, history));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("int32Property history size different from expected.", 8ul * rateWinSecs,
                                 history.size());
    for (unsigned int i = 0; i < 8 * rateWinSecs; i++) {
        CPPUNIT_ASSERT_EQUAL_MESSAGE("int32Property value differs from expected.", 10, history[i].get<int>("v"));
    }

    // Updating a string property with a 32 Kb string should be accepted again after enough
    // time has passed since the previous max rate threshold reached condition.
    Epochstamp beforeSingle32KbWrite(after64KbWrite + TimeDuration(1 * rateWinSecs, 4 * millisecInAtto));
    Hash updateStr32Kb("stringProperty", str32Kb);
    Epochstamp updateEpoch(beforeSingle32KbWrite + TimeDuration(0, 6 * millisecInAtto));
    Timestamp updateTime(updateEpoch, Trainstamp());
    updateTime.toHashAttributes(updateStr32Kb.getAttributes("stringProperty"));
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(m_deviceId, "slotUpdateConfigGeneric", updateStr32Kb)
                                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                  .receive());
    Epochstamp afterSingle32KbWrite(beforeSingle32KbWrite + TimeDuration(0, 8 * millisecInAtto));
    // Make sure that data has been written to Influx.
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", SLOT_REQUEST_TIMEOUT_MILLIS / 1000));
    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));

    // Checks that the 32 Kb string has been successfully set as property values.
    history.clear();
    historyParams.set<std::string>("from", beforeSingle32KbWrite.toIso8601Ext());
    historyParams.set<std::string>("to", afterSingle32KbWrite.toIso8601Ext());
    historyParams.set<int>("maxNumData", maxPropHistSize);
    replyDevice.clear();
    replyProperty.clear();
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(dlreader0, "slotGetPropertyHistory", m_deviceId, "stringProperty", historyParams)
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(replyDevice, replyProperty, history));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("stringProperty history size different from expected.", 1ul, history.size());
    const std::string historySingleStr32kb = history[0].get<std::string>("v");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("stringProperty value doesn't have expected size.", 32768ul,
                                 historySingleStr32kb.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("stringProperty value doesn't have expected characters.", str32Kb.at(0ul),
                                 historySingleStr32kb.at(0ul));

    std::clog << "OK" << std::endl;
}


void DataLogging_Test::testNoInfluxServerHandling() {
    std::clog << "Testing handling of no Influx Server available scenarios ..." << std::endl;

    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", m_deviceId), KRB_TEST_MAX_TIMEOUT);
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
        m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", m_deviceId, withNoServer.toIso8601())
              .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
              .receive(conf, schema, cfgAtTime, cfgTime);
    } catch (const karabo::util::RemoteException &exc) {
        bool condition = (exc.detailedMsg().find("Could not connect to InfluxDb at") != std::string::npos) ||
                         (exc.detailedMsg().find("Reading from InfluxDB failed") != std::string::npos) ||
                         (exc.detailedMsg().find("Connection reset by peer"));
        CPPUNIT_ASSERT_MESSAGE(
              std::string("Unexpected RemoteException while handling no Influx server:\n'") + exc.detailedMsg() + "'\n",
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


void DataLogging_Test::testInfluxPropHistoryAveraging() {
    std::clog << "Testing InfluxLogReader averaging when the requested Property History has too many points ..."
              << std::flush;

    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);
    const int maxPropHistorySize = 40;
    const int numWrites = maxPropHistorySize + 20;

    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", m_deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    success = startDataLoggerManager("InfluxDataLogger");
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();

    Epochstamp beforePropWrites;
    for (size_t i = 0; i < numWrites; i++) {
        double propValue = i * 2.0;
        if (i % 9 == 0) {
            // Insert some NaNs values - that, along with the number of data points in the history being above the
            // maxNumData parameter, were the trigger for the bug fixed in
            // https://git.xfel.eu/Karabo/Framework/-/merge_requests/6805.
            propValue = std::numeric_limits<double>::signaling_NaN();
        }
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set<double>(m_deviceId, "doubleProperty", propValue));
    }
    Epochstamp afterPropWrites;

    // Make sure that data has been written to Influx.
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", SLOT_REQUEST_TIMEOUT_MILLIS / 1000));
    boost::this_thread::sleep(boost::posix_time::milliseconds(1500));

    // Checks that slotGetPropertyHistory gets the averages consistently - the same number of data points and the same
    // values - when invoked multiple times with the same parameters. This test systematically fails if the fix
    // submitted in https://git.xfel.eu/Karabo/Framework/-/merge_requests/6805 is not present.
    std::vector<Hash> hist1, hist2;
    std::string replyDevice, replyProperty;
    Hash historyParams("from", beforePropWrites.toIso8601Ext(), "to", afterPropWrites.toIso8601Ext(), "maxNumData",
                       maxPropHistorySize);

    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(dlreader0, "slotGetPropertyHistory", m_deviceId, "doubleProperty", historyParams)
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(replyDevice, replyProperty, hist1));

    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(dlreader0, "slotGetPropertyHistory", m_deviceId, "doubleProperty", historyParams)
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(replyDevice, replyProperty, hist2));

    CPPUNIT_ASSERT_EQUAL(hist1.size(), hist2.size());
    for (size_t i = 0; i < hist1.size(); i++) {
        CPPUNIT_ASSERT_MESSAGE("History items at position " + toString(i) + " differ.", hist1[i].fullyEquals(hist2[i]));
    }

    std::clog << "OK" << std::endl;
}


void DataLogging_Test::testFailingManager() {
    std::clog << "Testing logger manager goes to ERROR with inconsistent config ..." << std::flush;
    const std::string dataLogManagerId("loggerManager");
    std::pair<bool, std::string> success = m_deviceClient->killDevice(dataLogManagerId, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    const Hash conf("deviceId", dataLogManagerId,
                    // Place list that is inconsistent with existing loggermap.xml (i.e. server in loggerMap is
                    // missing), this will be noticed by the logger and bring it to ERROR.
                    "serverList", std::vector<std::string>{"garbageServer"});

    success = m_deviceClient->instantiate(m_server, "DataLoggerManager", conf, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    karabo::util::State loggerState = karabo::util::State::UNKNOWN;
    const std::string &dataLoggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    int timeout = KRB_TEST_MAX_TIMEOUT * 1000;
    while (timeout > 0) {
        loggerState = m_deviceClient->get<karabo::util::State>(dataLogManagerId, "state");
        if (loggerState == karabo::util::State::ERROR) {
            break;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(25));
        timeout -= 25;
    }

    const std::string status = m_deviceClient->get<std::string>(dataLogManagerId, "status");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Missed ERROR state - status: " + status, karabo::util::State::ERROR, loggerState);
    CPPUNIT_ASSERT_MESSAGE(status,
                           status.find("Failure in initialize(), likely a restart is needed:") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE(
          status, status.find("Inconsistent 'loggermap.xml' and 'serverList' configuration:") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE(status,
                           status.find("'DataLoggingTestServer' is in map, but not in list.") != std::string::npos);
}
