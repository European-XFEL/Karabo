/*
 * File:   TelegrafLogging_Test.hh
 *
 * Created on Oct 20, 2020, 9:23:56 AM
 */

#ifndef TELEGRAFLOGGING_TEST_HH
#define	TELEGRAFLOGGING_TEST_HH

#include "BaseLogging_Test.hh"

class TelegrafLogging_Test : public BaseLogging_Test {

    CPPUNIT_TEST_SUITE(TelegrafLogging_Test);

    CPPUNIT_TEST(influxAllTestRunnerWithTelegraf);
    CPPUNIT_TEST(testInfluxDbNotAvailableTelegraf);

    CPPUNIT_TEST_SUITE_END();

public:
    TelegrafLogging_Test();

private:
    void influxAllTestRunnerWithTelegraf();

    /**
     * Checks that the InfluxDataLogger goes to ERROR state
     * when an attempt to use a non-existing Influx database
     * is made in an environment where the Influx user lacks
     * admin privileges (like the Telegraf based environments).
     */
    void testInfluxDbNotAvailableTelegraf();

    /**
     * Returns whether the environment composed by the Telegraf writing node and
     * the InfluxDb reading node is available and responsive.
     */
    bool isTelegrafEnvResponsive();

    /**
     * Sets up an InfluxDB cluster with telegraf front-end and 2 InfluxDB cpus
     * as a backend.
     */
    void setupTelegrafEnv();
};

#endif	/* TELEGRAFLOGGING_TEST_HH */
