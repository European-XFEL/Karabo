/*
 * File:   DataLogging_Test.hh
 * Author: silenzi
 *
 * Created on Nov 12, 2018, 6:07:07 PM
 */

#ifndef DATALOGGING_TEST_HH
#define	DATALOGGING_TEST_HH

#include "BaseLogging_Test.hh"

class DataLogging_Test : public BaseLogging_Test {

    CPPUNIT_TEST_SUITE(DataLogging_Test);

    CPPUNIT_TEST(fileAllTestRunner);
    CPPUNIT_TEST(influxAllTestRunnerWithDataMigration);
    CPPUNIT_TEST(testNoInfluxServerHandling);

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
     * Test that manager goes to ERROR if server list and loggermap.xml are inconsistent
     *
     * Requires that a loggermap.xml is present from previous manager
     */
    void testFailingManager();

    bool m_dataWasMigrated = false;
    karabo::util::Epochstamp m_fileMigratedDataEndsBefore;

};

#endif	/* DATALOGGING_TEST_HH */
