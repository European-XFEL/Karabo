/*
 * File:   DataLogging_Test.hh
 * Author: silenzi
 *
 * Created on Nov 12, 2018, 6:07:07 PM
 */

#ifndef DATALOGGING_TEST_HH
#define DATALOGGING_TEST_HH

#include "BaseLogging_Test.hh"

class DataLogging_Test : public BaseLogging_Test {
    CPPUNIT_TEST_SUITE(DataLogging_Test);

    CPPUNIT_TEST(fileAllTestRunner);
    CPPUNIT_TEST(influxAllTestRunnerWithDataMigration);
    CPPUNIT_TEST(testNoInfluxServerHandling);
    CPPUNIT_TEST(testInfluxMaxPerDevicePropLogRate);
    CPPUNIT_TEST(testInfluxMaxSchemaLogRate);
    CPPUNIT_TEST(testInfluxPropHistoryAveraging);

    CPPUNIT_TEST_SUITE_END();

   private:
    void fileAllTestRunner();
    void influxAllTestRunnerWithDataMigration(); // Supports data migration test.
    void testMigrateFileLoggerData();

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
     * Test that manager goes to ERROR if server list and loggermap.xml are inconsistent
     *
     * Requires that a loggermap.xml is present from previous manager
     */
    void testFailingManager();

    bool m_dataWasMigrated = false;
    karabo::util::Epochstamp m_fileMigratedDataEndsBefore;
};

#endif /* DATALOGGING_TEST_HH */
