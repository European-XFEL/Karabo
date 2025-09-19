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
 * File:   DataLogging_Test.hh
 * Author: silenzi
 *
 * Created on Nov 12, 2018, 6:07:07 PM
 */

#ifndef DATALOGGING_TEST_HH
#define DATALOGGING_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include "karabo/karabo.hpp"

class DataLogging_Test : public CPPUNIT_NS::TestFixture {
   public:
    DataLogging_Test();
    void setUp() override;
    void tearDown() override;

   private:
    CPPUNIT_TEST_SUITE(DataLogging_Test);

    CPPUNIT_TEST(influxAllTestRunner);
    CPPUNIT_TEST(testNoInfluxServerHandling);
    CPPUNIT_TEST(testInfluxMaxPerDevicePropLogRate);
    CPPUNIT_TEST(testInfluxMaxSchemaLogRate);
    CPPUNIT_TEST(testInfluxPropHistoryAveraging);
    CPPUNIT_TEST(testInfluxMaxStringLength);
    CPPUNIT_TEST(testInfluxSafeSchemaRetentionPeriod);

    CPPUNIT_TEST_SUITE_END();

    static int KRB_TEST_MAX_TIMEOUT;
    static int SLOT_REQUEST_TIMEOUT_MILLIS;
    static int FLUSH_REQUEST_TIMEOUT_MILLIS;
    static int FLUSH_INTERVAL_SEC;
    static int NUM_RETRY;
    static int PAUSE_BEFORE_RETRY_MILLIS;
    static int WAIT_WRITES;
    static const char* DEFAULT_TEST_LOG_PRIORITY;

    void testLoggerMapProperty();
    void influxAllTestRunner();

    void testAllInstantiated(bool waitForLoggerReady = true);

    void testInt(bool testPastConf = true);
    void testUInt64(bool testPastConf = false);
    void testFloat(bool testPastConf = false);
    void testString(bool testPastConf = false);
    void testVectorString(bool testPastConf = false);
    void testVectorChar(bool testPastConf = false);
    void testVectorSignedChar(bool testPastConf = false);
    void testVectorUnsignedChar(bool testPastConf = false);
    void testVectorShort(bool testPastConf = false);
    void testVectorUnsignedShort(bool testPastConf = false);
    void testVectorInt(bool testPastConf = false);
    void testVectorUnsignedInt(bool testPastConf = false);
    void testVectorLongLong(bool testPastConf = false);
    void testVectorUnsignedLongLong(bool testPastConf = false);

    void testVectorBool(bool testPastConf = false);
    void testTable(bool testPastConf = false);
    void testChar(bool testPastConf = true);

    void testLastKnownConfiguration();
    void testCfgFromPastRestart(bool pastStampStaysPast);

    /**
     * Checks that the DataLoggers handle NaN floats and doubles.
     */
    void testNans();

    template <class T>
    void testHistory(const std::string& key, const std::function<T(int)>& f, const bool testConf);

    std::pair<bool, std::string> startDataLoggerManager(
          const std::string& loggerType, bool useInvalidInfluxUrl = false, bool useInvalidDbName = false,
          unsigned int maxPerDevicePropLogRate = 5 * 1024, unsigned int propLogRatePeriod = 5,
          unsigned int maxSchemaLogRate = 15 * 1024, unsigned int schemaLogRatePeriod = 5,
          unsigned int maxStringLength = karabo::util::MAX_INFLUX_VALUE_LENGTH, double safeSchemaRetentionPeriod = 2.0);

    /**
     * Checks that slotGetPropertyHistory logging works when a
     * Schema evolution changes the device schema at some timepoint
     * within the requested history interval.
     */
    void testSchemaEvolution();

    /**
     * Checks that the InfluxLogReader doesn't accept out of range
     * values for the 'maxNumData' parameter in calls to
     * 'slotGetPropertyHistory'.
     */
    void testMaxNumDataRange();

    /**
     * Checks that the InfluxLogReader is properly enforcing the
     * 'maxNumData' parameter in calls to 'slotGetPropertyHistory'.
     * Histories with up to 'maxNumData' entries should return
     * 'maxNumData' property values as they were written. Histories
     * with more than 'maxNumData' entries should return 'maxNumData'
     * property values samples.
     */
    void testMaxNumDataHistory();

    /**
     * Checks that the InfluxLogger is properly dropping values
     * too far ahead in the future.
     */
    void testDropBadData();

    /**
     * Sets PropertyTestDevice Schema
     *
     * circumvent min/max limits and vector size specification
     */
    void setPropertyTestSchema();

    /**
     * @brief Checks that getConfigurationFromPast does not retrieve properties
     * with no default value that have not been set during the instantiation
     * of the device that is closest to the requested timepoint.
     *
     * "Instantiation of the device that is closest to the requested timepoint"
     * means either the last instantiation of the device before the requested
     * timepoint, if the device was not active at the timepoint, or the
     * instantiation of the device that was active at the timepoint.
     */
    void testUnchangedNoDefaultProperties();

    /**
     * Checks that the Influx logger and reader fail as soon as
     * possible when there's no Influx server available. Uses an
     * invalid url configuration for simulating the scenario of
     * the Influx server not being available.
     *
     * Note: During the test run in the CI machine, the docker
     * command is not available (the CI test is already executed
     * in a container), so the karabo-startinfluxdb and
     * karabo-stopinfluxdb commands cannot be run. That's the
     * reason behind the forced invalid configuration.
     */
    void testNoInfluxServerHandling();

    /**
     * @brief Checks that the maximum per device property logging rate for Influx is being properly enforced.
     *
     * This test instantiates its own Influx DataLoggerManager with a much lower threshold for the maximum logging rate
     * allowed. The smaller value is needed to make it easier to violate the threshold. It has to be run in isolation
     * because a smaller threshold could easily interfere with the other tests.
     */
    void testInfluxMaxPerDevicePropLogRate();

    /**
     * @brief Checks that the maximum per device schema logging rate for Influx is being properly enforced.
     *
     * This test instantiates its own Influx DataLoggerManager with a much lower threshold for the maximum schema
     * logging rate allowed. The smaller value is needed to make it easier to violate the threshold. It has to be
     * run in isolation because a smaller threshold could easily interfere with the other tests.
     */
    void testInfluxMaxSchemaLogRate();


    /**
     * @brief Checks that the InfluxLogReader does averaging properly during the execution of slotPropertyHistory when
     * the number of data points available is larger than the maximum number of data points to be retrieved.
     *
     * This test is motivated by the bug fixed in https://git.xfel.eu/Karabo/Framework/-/merge_requests/6805.
     */
    void testInfluxPropHistoryAveraging();

    /**
     * @brief Checks that the maximum length allowed for a string value to be saved on Influx is being enforced and that
     * all violations are being properly logged.
     */
    void testInfluxMaxStringLength();

    /**
     * @brief Checks that schemas older than the safeSchemaRetentionPeriod are copied.
     */
    void testInfluxSafeSchemaRetentionPeriod();

    /**
     * Test that manager goes to ERROR if server list and loggermap.xml are inconsistent
     *
     * Requires that a loggermap.xml is present from previous manager
     */
    void testFailingManager();


    std::string getDeviceIdPrefix();

    bool waitForCondition(std::function<bool()> checker, unsigned int timeoutMillis,
                          unsigned int sleepIntervalMillis = 5u);

    /**
     * Waits until logger has started to log device, assert otherwise.
     * Timeout is DataLogging_Test::KRB_TEST_MAX_TIMEOUT)
     *
     * @param deviceId device that should be logged
     * @param textForFailure string prepended to assertion failure message if not logged
     */
    void waitUntilLogged(const std::string& deviceId, const std::string& textForFailure);


    const std::string m_server;
    const std::string m_deviceId;

    karabo::core::DeviceServer::Pointer m_deviceServer;
    std::jthread m_eventLoopThread;
    karabo::xms::SignalSlotable::Pointer m_sigSlot;
    karabo::core::DeviceClient::Pointer m_deviceClient;

    bool m_changedPath;
    std::string m_oldPath;
};

#endif /* DATALOGGING_TEST_HH */
