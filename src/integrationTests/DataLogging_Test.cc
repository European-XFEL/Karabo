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
 * File:   DataLogging_Test.cc
 * Author: silenzi
 *
 * Created on Nov 12, 2018, 6:07:07 PM
 */

#include "DataLogging_Test.hh"

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

using namespace std::chrono;
using namespace std::literals::chrono_literals;
USING_KARABO_NAMESPACES;

CPPUNIT_TEST_SUITE_REGISTRATION(DataLogging_Test);

void DataLogging_Test::fileAllTestRunner() {
    // Run the tests for the text file based DataLogger with FATAL logger level
    // to prevent temporary errors for indexes not yet available from polluting
    // the device server log.
    CPPUNIT_ASSERT_NO_THROW_MESSAGE(
          "Failed to set logger level of device server '" + m_server + "' to 'FATAL'",
          m_deviceClient->execute(m_server, "slotLoggerLevel", KRB_TEST_MAX_TIMEOUT, "FATAL"));

    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", m_deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    setPropertyTestSchema();

    std::clog << "\n==== Starting sequence of File Logging tests ====" << std::endl;
    success = startDataLoggerManager("FileDataLogger");
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();
    testLoggerMapProperty();
    waitUntilLogged(m_deviceId, "fileAllTestRunner");

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

void DataLogging_Test::testLoggerMapProperty() {
    std::clog << "Testing table of data loggers... " << std::flush;

    const auto& devices = m_deviceClient->getDevices();

    // We make sure all the devices in the system have an entry in the loggerMap table, and
    // that they have the same data logger
    const auto mapEntries = m_deviceClient->get<std::vector<karabo::data::Hash>>("loggerManager", "loggerMap");
    CPPUNIT_ASSERT_GREATER(0ULL, static_cast<unsigned long long>(mapEntries.size()));

    const auto& data_logger = mapEntries[0].get<std::string>("dataLogger");

    for (const Hash& entry : mapEntries) {
        CPPUNIT_ASSERT_EQUAL(data_logger, entry.get<std::string>("dataLogger"));
    }

    for (const auto& device : devices) {
        auto found = std::find_if(mapEntries.begin(), mapEntries.end(),
                                  [device](const Hash& entry) { return device == entry.get<std::string>("device"); });

        CPPUNIT_ASSERT_MESSAGE((device + " not in loggers map"), found != mapEntries.end());
    }

    std::clog << "OK" << std::endl;
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
    const std::string absLoggerPath = std::filesystem::absolute("./" + m_fileLoggerDirectory).string();
    const std::string migrationResultsPath = absLoggerPath + std::string("/migrationresults");
    std::ostringstream cmd;
    cmd << "cd ../../../src/pythonKarabo; ../../karabo/extern/bin/python3 ";
    cmd << "karabo/influx_db/dl_migrator.py ";

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

    std::filesystem::path p(migrationResultsPath + "/processed/" + m_deviceId + "/");
    if (std::filesystem::is_directory(p)) {
        for (auto& entry : boost::make_iterator_range(std::filesystem::directory_iterator(p), {})) {
            std::ostringstream msg;
            msg << "Check if " << entry << " was migrated OK: " << std::filesystem::path(entry).extension();
            std::clog << msg.str() << std::endl;
            CPPUNIT_ASSERT_MESSAGE(msg.str(), std::filesystem::path(entry).extension() == ".ok");
        }
    }

    unsigned int errorCount = 0;
    std::filesystem::path perr(migrationResultsPath + "/part_processed/" + m_deviceId + "/");
    if (std::filesystem::is_directory(perr)) {
        for (auto& entry : boost::make_iterator_range(std::filesystem::directory_iterator(perr), {})) {
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
    std::filesystem::remove_all(migrationResultsPath);
}


void DataLogging_Test::influxAllTestRunnerWithDataMigration() {
    m_keepLoggerDirectory = false;

    // and epoch stamp certainly before the next round of influx logging
    m_fileMigratedDataEndsBefore = Epochstamp();

    std::this_thread::sleep_for(1000ms);

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

    const std::filesystem::path migratorPackageBase("../../../src/pythonKarabo");
    boost::system::error_code ec;
    if (std::filesystem::exists(migratorPackageBase, ec)) {
        testMigrateFileLoggerData();
    } else {
        std::clog << "Migrator script not available - skipping migration test." << std::endl;
    }

    testMaxNumDataHistory();
    testDropBadData();

    testLoggerMapProperty();

    // Following tests use device m_deviceId, so ensure it is logged
    waitUntilLogged(m_deviceId, "influxAllTestRunnerWithDataMigration");
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

    const unsigned int rateWinSecs = 1u;
    const unsigned int afterFlushWait = 1'000u;

    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + m_server;
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

    // Starts the logger and readers with a lower max schema rate threshold - 18 kb/s - over a rateWinSecs seconds
    // rating window. The base64 encoded schema of the DataLogTestDevice is 12,516 bytes (before schema update),
    // so with rateWinSecs == 1, a single schema can be logged in that period, but two cannot.
    success = startDataLoggerManager("InfluxDataLogger", false, false, 32, rateWinSecs, 18, rateWinSecs);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();
    waitUntilLogged(deviceId, "testInfluxMaxSchemaLogRate");

    // Wait some time to isolate the schema update bursts.
    std::this_thread::sleep_for(milliseconds(rateWinSecs * 1000 + 1));

    ///////  Checks that a schema update within the rating limit is accepted.
    Epochstamp beforeFirstBurst;
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStrA).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    // Makes sure that data has been received by logger and written to Influx.
    std::this_thread::sleep_for(500ms);
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", FLUSH_REQUEST_TIMEOUT_MILLIS / 1000));
    std::this_thread::sleep_for(milliseconds(afterFlushWait));
    Epochstamp afterFirstBurst;

    // Checks that the schema update has not been flagged as bad data.
    Hash badDataAllDevices;
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot
                ->request(dlreader0, "slotGetBadData", beforeFirstBurst.toIso8601Ext(), afterFirstBurst.toIso8601Ext())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(badDataAllDevices));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(badDataAllDevices), 0ul, badDataAllDevices.size());

    // Wait some time to isolate the schema update bursts.
    std::this_thread::sleep_for(milliseconds(rateWinSecs * 1000 + 1));

    ////////  Checks that two schema updates in a fast succession would go above the
    ////////  threshold and one of the updates (the second) would be rejected.
    Epochstamp beforeSecondBurst;
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStrB).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStrC).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    // Makes sure that data has been received by logger and written to Influx.
    std::this_thread::sleep_for(500ms);
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", FLUSH_REQUEST_TIMEOUT_MILLIS / 1000));
    std::this_thread::sleep_for(milliseconds(afterFlushWait));
    Epochstamp afterSecondBurst;

    // Checks that one of the schema updates failed.
    badDataAllDevices.clear();
    waitForCondition(
          [this, &badDataAllDevices, &dlreader0, &beforeSecondBurst, &afterSecondBurst]() {
              try {
                  m_sigSlot
                        ->request(dlreader0, "slotGetBadData", beforeSecondBurst.toIso8601Ext(),
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
    CPPUNIT_ASSERT_EQUAL(1ul, badDataAllDevices.size());
    CPPUNIT_ASSERT(badDataAllDevices.has(deviceId));
    const auto& badDataEntries = badDataAllDevices.get<std::vector<Hash>>(deviceId);
    CPPUNIT_ASSERT_EQUAL(1ul, badDataEntries.size());
    const std::string& badDataInfo = badDataEntries[0].get<std::string>("info");
    CPPUNIT_ASSERT_MESSAGE(
          "Expected pattern, '" + deviceId + "::schema', not found in bad data description:\n'" + badDataInfo + "'",
          badDataInfo.find(deviceId + "::schema") != std::string::npos);

    // Wait some time to isolate the schema update bursts.
    std::this_thread::sleep_for(milliseconds(rateWinSecs * 1000 + 1));

    //////  Checks that after the updates have settled down for a while, schemas
    //////  can be logged again.
    Epochstamp beforeThirdBurst;
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(deviceId, "slotUpdateSchema", schemaStrD).timeout(SLOT_REQUEST_TIMEOUT_MILLIS).receive());
    // Makes sure that data has been received by logger and written to Influx.
    std::this_thread::sleep_for(500ms);
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", FLUSH_REQUEST_TIMEOUT_MILLIS / 1000));
    std::this_thread::sleep_for(milliseconds(afterFlushWait));
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
    int nTries = NUM_RETRY;
    do {
        CPPUNIT_ASSERT_NO_THROW(
              m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", deviceId, afterThirdBurst.toIso8601())
                    .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                    .receive(pastCfg, schema, cfgAtTime, cfgTime));
        if (schema.has("stringPropertyD")) break;
        std::this_thread::sleep_for(milliseconds(PAUSE_BEFORE_RETRY_MILLIS));
    } while (nTries-- > 0);
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
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + m_server;
    // A device exclusive for this test case is used to guarantee that its
    // schema will be partitioned into multiple chunks. The default schema for
    // the PropertyTest device has around 78 Kb and for this test the
    // value used for the Influx logger "maxStringValueLength" property is 8 kb.
    const std::string propTestDevice = m_deviceId + "__MAX__STRING";

    const unsigned int afterFlushWait = 1'000u;

    Epochstamp beforeServerInstantiation;

    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", propTestDevice), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    success = startDataLoggerManager("InfluxDataLogger", false, false, 5120u, 5u, 15'360u, 5u, maxStringLength);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();
    waitUntilLogged(propTestDevice, "testInfluxMaxStringLength");

    ///////  Checks that a string below the length limit is accepted
    Epochstamp beforeBelowLimit;
    m_deviceClient->set(propTestDevice, "stringProperty", belowLimitStr);
    // Makes sure the data has been written to Influx
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());
    std::this_thread::sleep_for(milliseconds(afterFlushWait));
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
    std::this_thread::sleep_for(milliseconds(afterFlushWait));
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
    std::this_thread::sleep_for(milliseconds(afterFlushWait));
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
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + m_server;

    const std::string str32Kb(32768ul, 'A');
    const std::string str8Kb(8192ul, 'B');

    const TimeValue millisecInAtto = 1'000'000'000'000'000ull; // Resolution of fractional seconds is AttoSec (10^-18).
    const unsigned int afterFlushWait = 1'500u;

    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          m_server, "DataLogTestDevice", Hash("deviceId", m_deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Starts the logger and readers with a lower max rate threshold - 32 kb/s - over a rateWinSecs seconds rating
    // window.
    success = startDataLoggerManager("InfluxDataLogger", false, false, 32, rateWinSecs);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();
    waitUntilLogged(m_deviceId, "testInfluxMaxPerDevicePropLogRate");

    // Checks that writing 32Kb of data is within the log rate tolerance.
    Epochstamp before32KbWrite;
    for (unsigned int i = 0; i < 4 * rateWinSecs; i++) {
        Hash updateProp = Hash("stringProperty", str8Kb);
        Epochstamp updateEpoch(before32KbWrite + TimeDuration(0, (i + 1) * millisecInAtto));
        Timestamp updateTime(updateEpoch, TimeId());
        updateTime.toHashAttributes(updateProp.getAttributes("stringProperty"));
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(m_deviceId, "slotUpdateConfigGeneric", updateProp)
                                      .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                      .receive());
    }
    // after32KbWrite is set to be the timestamp of the last write performed in the
    // previous loop plus a safety margin.
    Epochstamp after32KbWrite(before32KbWrite + TimeDuration(0, 5 * rateWinSecs * millisecInAtto));
    // Make sure that data has been written to Influx.
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", FLUSH_REQUEST_TIMEOUT_MILLIS / 1000));
    std::this_thread::sleep_for(milliseconds(afterFlushWait));

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
        std::string& historyStr8Kb = history[i].get<std::string>("v");
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
        Timestamp updateTime(updateEpoch, TimeId());
        updateTime.toHashAttributes(updateProps.getAttributes("stringProperty"));
        updateTime.toHashAttributes(updateProps.getAttributes("int32Property"));
        CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(m_deviceId, "slotUpdateConfigGeneric", updateProps)
                                      .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                      .receive());
    }
    Epochstamp after64KbWrite(before64KbWrite + TimeDuration(0, 9 * rateWinSecs * millisecInAtto));
    // Make sure that data has been written to Influx.
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", FLUSH_REQUEST_TIMEOUT_MILLIS / 1000));
    std::this_thread::sleep_for(milliseconds(afterFlushWait));

    // Checks that the half of the stringProperty updates has exceeded the max log rate and has been rated as bad data.
    badDataAllDevices.clear();
    CPPUNIT_ASSERT_NO_THROW(
          m_sigSlot->request(dlreader0, "slotGetBadData", before64KbWrite.toIso8601Ext(), after64KbWrite.toIso8601Ext())
                .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                .receive(badDataAllDevices));
    CPPUNIT_ASSERT_EQUAL(1ul,
                         badDataAllDevices.size()); // 1 is because the bad data is grouped under a single deviceId.
    CPPUNIT_ASSERT_EQUAL(4ul * rateWinSecs, badDataAllDevices.get<std::vector<karabo::data::Hash>>(m_deviceId).size());
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
    Timestamp updateTime(updateEpoch, TimeId());
    updateTime.toHashAttributes(updateStr32Kb.getAttributes("stringProperty"));
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(m_deviceId, "slotUpdateConfigGeneric", updateStr32Kb)
                                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                                  .receive());
    Epochstamp afterSingle32KbWrite(beforeSingle32KbWrite + TimeDuration(0, 8 * millisecInAtto));
    // Make sure that data has been written to Influx.
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", FLUSH_REQUEST_TIMEOUT_MILLIS / 1000));
    std::this_thread::sleep_for(milliseconds(afterFlushWait));

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

void DataLogging_Test::testInfluxSafeSchemaRetentionPeriod() {
    namespace nl = nlohmann;

    std::clog << "Testing that schemas older than safeSchemaRetentionPeriod are preserved ..." << std::endl;

    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + m_server;
    const std::string propTestDevice = m_deviceId + "__SCHEMA_RETENTION_PERIOD";

    const unsigned int afterFlushWait = 500u;
    const double halfSecInYears = 0.5 / (365 * 24 * 60 * 60);

    Epochstamp testStartEpoch;

    std::pair<bool, std::string> success = startDataLoggerManager(
          "InfluxDataLogger", /* useInvalidInfluxUrl */ false, /* useInvalidDbName */ false,
          /* maxPerDevicePropLogRate */ 5120u, 5u, /* maxSchemaLogRate */ 15'360u, /* schemaLogRatePeriod */ 5u,
          /* maxStringLength */ 921'600u, /* safeSchemaRetentionPeriod */ halfSecInYears);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated();

    success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", propTestDevice), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    waitUntilLogged(propTestDevice, "testInfluxSafeSchemaRetentionPeriod - 1");

    // Restart the PropertyTest device under test - this will trigger a new attempt to save the device schema with the
    // same digest, since no change happened to the schema between the two instantiations.
    success = m_deviceClient->killDevice(propTestDevice, KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    // Waits for an interval long enough to guarantee that any other schema saving attempt will happen after the
    // one saved for the previous PropertyTest device under test has gone outside the safe retention window.
    std::this_thread::sleep_for(1600ms);
    success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", propTestDevice), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);
    waitUntilLogged(propTestDevice, "testInfluxSafeSchemaRetentionPeriod - 2");

    // Makes sure all the data has been saved in Influx.
    CPPUNIT_ASSERT_NO_THROW(m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive());
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

    CPPUNIT_ASSERT_MESSAGE("Didn't find the first expected schema", !firstDigest.empty());
    CPPUNIT_ASSERT_MESSAGE("Didn't find the second expected schema", !secondDigest.empty());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Schemas in Influx response don't have the same digest.", firstDigest, secondDigest);

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


void DataLogging_Test::testNoInfluxServerHandling() {
    std::clog << "Testing handling of no Influx Server available scenarios ..." << std::endl;

    // Temporarily set the logging level to FATAL to avoid spamming the logs of the device server (and the
    // test output) with connection errors.
    CPPUNIT_ASSERT_NO_THROW_MESSAGE(
          "Error setting the logger level to 'FATAL'",
          m_deviceClient->execute(m_server, "slotLoggerLevel", KRB_TEST_MAX_TIMEOUT, "FATAL"));

    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", m_deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Starts the logger and readers with invalid InfluxDB (or Telegraf) URLs.
    success = startDataLoggerManager("InfluxDataLogger", true);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

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

    CPPUNIT_ASSERT_MESSAGE("Timeout while waiting for DataLogger '" + dataLoggerId + "' to reach ERROR state.",
                           loggerState == karabo::data::State::ERROR);

    std::clog << "... Influx logger in ERROR state, as expected, with status '" << loggerStatus << "'" << std::endl;

    // The LogReader should be still in ON state - it only goes to error after failing to connect to the Influx
    // instance.
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + m_server;
    karabo::data::State readerState = karabo::data::State::UNKNOWN;
    waitForCondition(
          [this, &readerState, &dlreader0]() {
              readerState = m_deviceClient->get<karabo::data::State>(dlreader0, "state");
              return (readerState == karabo::data::State::ON);
          },
          KRB_TEST_MAX_TIMEOUT * 1000);

    CPPUNIT_ASSERT_MESSAGE("Timeout while waiting for LogReader '" + dlreader0 + "' to reach ON state.",
                           readerState == karabo::data::State::ON);

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
        m_sigSlot->request(dlreader0, "slotGetConfigurationFromPast", m_deviceId, withNoServer.toIso8601())
              .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
              .receive(conf, schema, cfgAtTime, cfgTime);
    } catch (const karabo::data::RemoteException& exc) {
        bool condition = (exc.detailedMsg().find("Could not connect to InfluxDb at") != std::string::npos) ||
                         (exc.detailedMsg().find("Reading from InfluxDB failed") != std::string::npos) ||
                         (exc.detailedMsg().find("Connection reset by peer"));
        CPPUNIT_ASSERT_MESSAGE(
              std::string("Unexpected RemoteException while handling no Influx server:\n'") + exc.detailedMsg() + "'\n",
              condition);
        remoteExceptionCaught = true;
    }

    CPPUNIT_ASSERT(remoteExceptionCaught);

    // At this point the LogReader will have tried to access Influx and failed. It should now be in ERROR.
    readerState = karabo::data::State::UNKNOWN;
    waitForCondition(
          [this, &readerState, &dlreader0]() {
              readerState = m_deviceClient->get<karabo::data::State>(dlreader0, "state");
              return (readerState == karabo::data::State::ERROR);
          },
          KRB_TEST_MAX_TIMEOUT * 1000);

    CPPUNIT_ASSERT_MESSAGE("Timeout while waiting for LogReader '" + dlreader0 + "' to reach ERROR state.",
                           readerState == karabo::data::State::ERROR);

    std::clog << "... request to retrieve past configuration failed with RemoteException as expected." << std::endl;

    // By simply starting the devices related to Influx logging, some logging writing activity takes place.
    // If this point of the test is reached with invalid urls configured for both reading and writing to the
    // Influx (or Telegraf) server, it is safe to conclude that the Influx Logger doesn't get compromissed by a
    // server not available condition - the host of the Influx logger is the same process that runs this test.

    // Restore the logger level of the device server that hosts the logger to the WARN level
    CPPUNIT_ASSERT_NO_THROW_MESSAGE(
          "Error setting the logger level level back to 'WARN'",
          m_deviceClient->execute(m_server, "slotLoggerLevel", KRB_TEST_MAX_TIMEOUT, DEFAULT_TEST_LOG_PRIORITY));

    std::clog << "OK" << std::endl;
}


void DataLogging_Test::testInfluxPropHistoryAveraging() {
    std::clog << "Testing InfluxLogReader averaging when the requested Property History has too many points ..."
              << std::flush;

    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + m_server;
    const int maxPropHistorySize = 40;
    const int numWrites = maxPropHistorySize + 20;

    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", m_deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    success = startDataLoggerManager("InfluxDataLogger");
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

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
        CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set<double>(m_deviceId, "doubleProperty", propValue));
    }
    Epochstamp afterPropWrites;

    // Make sure that data has been written to Influx.
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(loggerId, "flush", FLUSH_REQUEST_TIMEOUT_MILLIS / 1000));
    std::this_thread::sleep_for(1500ms);

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

    karabo::data::State loggerState = karabo::data::State::UNKNOWN;
    waitForCondition(
          [this, &loggerState, &dataLogManagerId]() {
              loggerState = m_deviceClient->get<karabo::data::State>(dataLogManagerId, "state");
              return loggerState == karabo::data::State::ERROR;
          },
          KRB_TEST_MAX_TIMEOUT * 1000);

    const std::string status = m_deviceClient->get<std::string>(dataLogManagerId, "status");
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Missed ERROR state - status: " + status, karabo::data::State::ERROR, loggerState);
    CPPUNIT_ASSERT_MESSAGE(status,
                           status.find("Failure in initialize(), likely a restart is needed:") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE(
          status, status.find("Inconsistent 'loggermap.xml' and 'serverList' configuration:") != std::string::npos);
    CPPUNIT_ASSERT_MESSAGE(status,
                           status.find("'DataLoggingTestServer' is in map, but not in list.") != std::string::npos);
}
