/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   TelegrafLogging_Test.hh
 *
 * Created on Oct 20, 2020, 9:23:56 AM
 */

#ifndef TELEGRAFLOGGING_TEST_HH
#define TELEGRAFLOGGING_TEST_HH

#include <string>
#include <utility>

#include "BaseLogging_Test.hh"

class TelegrafLogging_Test : public BaseLogging_Test {
    CPPUNIT_TEST_SUITE(TelegrafLogging_Test);

    CPPUNIT_TEST(influxAllTestRunnerWithTelegraf);
    CPPUNIT_TEST(testInfluxDbNotAvailableTelegraf);

    CPPUNIT_TEST_SUITE_END();

   public:
    TelegrafLogging_Test();
    void setUp();

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
     *
     * @return a pair with first as boolean to indicate if the setup completed
     *         successfuly and second as a string describing the error in case
     *         of failure.
     */
    std::pair<bool, std::string> setupTelegrafEnv();

    bool m_telegrafEnvOk;
};

#endif /* TELEGRAFLOGGING_TEST_HH */
