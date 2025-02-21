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
 * File:   TelegrafLogging_Test.cc
 *
 * Created on Oct 20, 2020, 9:23:57 AM
 */

#include "TelegrafLogging_Test.hh"

#include <chrono>
#include <sstream>

#include "karabo/net/EventLoop.hh"
#include "karabo/util/DataLogUtils.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/StringTools.hh"

USING_KARABO_NAMESPACES;
using namespace std::chrono;
using namespace std::literals::chrono_literals;

CPPUNIT_TEST_SUITE_REGISTRATION(TelegrafLogging_Test);


TelegrafLogging_Test::TelegrafLogging_Test() {
    // Adjust the test timing parameters to the Telegraf environment.
    PAUSE_BEFORE_RETRY_MILLIS = 1000;
    NUM_RETRY = 2200;
    FLUSH_INTERVAL_SEC = 20;
    WAIT_WRITES = 8000;
}


void TelegrafLogging_Test::setUp() {
    BaseLogging_Test::setUp();
    auto result = setupTelegrafEnv();
    m_telegrafEnvOk = result.first;
    if (!m_telegrafEnvOk) {
        std::clog << result.second << std::endl;
    }
}


void TelegrafLogging_Test::testInfluxDbNotAvailableTelegraf() {
    if (!::getenv("KARABO_TEST_TELEGRAF") || !m_telegrafEnvOk) {
        std::clog << "==== Skip Telegraf availability test ====" << std::endl;
        return;
    }

    std::clog << "Testing handling of Influx Database not available scenarios ...." << std::endl;

    std::pair<bool, std::string> success =
          m_deviceClient->instantiate(m_server, "PropertyTest", Hash("deviceId", m_deviceId), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    // Starts the loggers with an invalid database name.
    // Note: it is required for the InfluxDb writing user to not have admin privileges on the Influx server.
    //       This requirement is fullfilled by both the CI and the Production environments. A local Influx server
    //       test environment must be configured properly.
    success = startDataLoggerManager("InfluxDataLogger", false, true);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    testAllInstantiated(false);

    int timeout = KRB_TEST_MAX_TIMEOUT * 1000; // milliseconds
    karabo::util::State loggerState = karabo::util::State::UNKNOWN;
    std::string loggerStatus;
    const std::string& dataLoggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    while (timeout > 0) {
        loggerState = m_deviceClient->get<karabo::util::State>(dataLoggerId, "state");
        loggerStatus = m_deviceClient->get<std::string>(dataLoggerId, "status");
        if (loggerState == karabo::util::State::ERROR) {
            break;
        }
        std::this_thread::sleep_for(50ms);
        timeout -= 50;
    }

    CPPUNIT_ASSERT_MESSAGE("Timeout while waiting for DataLogger '" + dataLoggerId + "' to reach ERROR state.",
                           loggerState == karabo::util::State::ERROR);

    std::clog << "... logger in ERROR state with status: '\n" << loggerStatus << "'\n(as expected)." << std::endl;

    std::clog << "OK" << std::endl;
}


std::pair<bool, std::string> TelegrafLogging_Test::setupTelegrafEnv() {
    const std::vector<std::string> varEnds{"_DBNAME",     "_QUERY_USER",     "_QUERY_PASSWORD", "_QUERY_URL",
                                           "_WRITE_USER", "_WRITE_PASSWORD", "_WRITE_URL"};

    const std::string telegrafPrefix{"KARABO_TEST_TELEGRAF"};
    const std::string influxPrefix{"KARABO_INFLUXDB"};

    std::vector<std::string> missingVars;

    for (const std::string& varEnd : varEnds) {
        const std::string reqVar{telegrafPrefix + varEnd};
        if (!::getenv(reqVar.c_str())) {
            missingVars.push_back(reqVar);
        } else {
            const std::string influxVar{influxPrefix + varEnd};
            ::setenv(influxVar.c_str(), ::getenv(reqVar.c_str()), 1);
        }
    }

    if (missingVars.size() > 0) {
        std::ostringstream ostr;
        ostr << "\n=== Telegraf connection parameters not fully defined ===" << "\nMissing environment variables:\n";
        for (const std::string& missingVar : missingVars) {
            ostr << "\t" << missingVar << "\n";
        }
        return std::make_pair(false, ostr.str());
    }

    return std::make_pair(true, "");
}


bool TelegrafLogging_Test::isTelegrafEnvResponsive() {
    const int timeoutSecs = 45;

    std::clog << "Check if Telegraf environment is responsive (updates retrieved within "
              << karabo::util::toString(timeoutSecs) << " secs.) ... " << std::endl;

    bool envResponsive = false;

    // Instantiates a DataLogTestDevice for performing the check: a single property update
    // followed by a property history retrieval will be used as the "probing" operation.
    const std::string deviceId(getDeviceIdPrefix() + "TelegrafEnvProbe");
    const std::string loggerId = karabo::util::DATALOGGER_PREFIX + m_server;
    auto success =
          m_deviceClient->instantiate(m_server, "DataLogTestDevice", Hash("deviceId", deviceId), KRB_TEST_MAX_TIMEOUT);

    if (!success.first) {
        return false;
    }

    // Checks that the probing device is being logged.
    bool isLogged = waitForCondition(
          [this, &loggerId, &deviceId]() {
              auto loggedIds = m_deviceClient->get<std::vector<std::string>>(loggerId, "devicesToBeLogged");
              return (std::find(loggedIds.begin(), loggedIds.end(), deviceId) != loggedIds.end());
          },
          KRB_TEST_MAX_TIMEOUT * 1000);

    if (!isLogged) {
        return false;
    }

    // Probing sequence
    Epochstamp beforePropUpdate;

    // Updates a property
    try {
        m_deviceClient->execute(deviceId, "slotIncreaseValue", KRB_TEST_MAX_TIMEOUT);
    } catch (std::exception& e) {
        std::clog << "... (not responsive).\nError during property update: " << e.what() << std::endl;
        return false;
    }


    // Captures the timepoint after updating the property
    std::this_thread::sleep_for(100ms);
    Epochstamp afterPropUpdate;

    // Makes sure all the writes are done before retrieval.
    try {
        m_sigSlot->request(loggerId, "flush").timeout(FLUSH_REQUEST_TIMEOUT_MILLIS).receive();
    } catch (std::exception& e) {
        std::clog << "... (not responsive).\nError during flush " << e.what() << std::endl;
        return false;
    }

    // Tries to obtain the property update from the Influx node.
    Hash params;
    params.set<std::string>("from", beforePropUpdate.toIso8601());
    params.set<std::string>("to", afterPropUpdate.toIso8601());
    const int maxNumData = 10;
    params.set<int>("maxNumData", maxNumData);

    std::vector<Hash> history;
    std::string replyDevice;
    std::string replyProperty;
    const std::string dlreader0 = karabo::util::DATALOGREADER_PREFIX + ("0-" + m_server);

    // History retrieval may take more than one try; if the retrieval happens
    // within the timeout limit, the environment is considered responsive.
    int nTries = timeoutSecs; // number of attempts spaced by 1 sec.
    while (nTries >= 0 && history.size() != 1) {
        try {
            m_sigSlot->request(dlreader0, "slotGetPropertyHistory", deviceId, "value", params)
                  .timeout(SLOT_REQUEST_TIMEOUT_MILLIS)
                  .receive(replyDevice, replyProperty, history);
        } catch (const karabo::util::TimeoutException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
        } catch (const karabo::util::RemoteException& e) {
            // Just consume the exception as it is expected while data is not
            // ready.
        }
        std::this_thread::sleep_for(1000ms);
        nTries--;
    }

    if (history.size() == 1) {
        envResponsive = true;
    }

    std::clog << "... (" << (envResponsive ? "" : "not") << " responsive)." << std::endl;

    return envResponsive;
}


void TelegrafLogging_Test::influxAllTestRunnerWithTelegraf() {
    // delete logger directory after this test
    m_keepLoggerDirectory = false;

    if (!::getenv("KARABO_TEST_TELEGRAF") || !m_telegrafEnvOk) {
        std::clog << "==== Skip sequence of Telegraf Logging tests ====" << std::endl;
        return;
    }

    std::clog << "\n==== Influx tests for Telegraf environment ====" << std::endl;

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

    if (!isTelegrafEnvResponsive()) {
        // Skip test sequence as Telegraf instance is not responsive and tests will most likely fail due to
        // response timeouts and/or delays on written data availability for reading.
        std::clog << "\n==== Telegraf environment not responsive; tests SKIPPED. ====\n" << std::endl;
        return;
    }

    testMaxNumDataRange();
    testMaxNumDataHistory();

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

    std::clog << "==== Telegraf Influx Logging test finished ====" << std::endl;
}
