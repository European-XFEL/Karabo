/*
 * File:   DataLogging_Test.hh
 * Author: silenzi
 *
 * Created on Nov 12, 2018, 6:07:07 PM
 */

#ifndef DATALOGGING_TEST_HH
#define	DATALOGGING_TEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include "karabo/karabo.hpp"

class DataLogging_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(DataLogging_Test);

    CPPUNIT_TEST(fileAllTestRunner);
    CPPUNIT_TEST(influxAllTestRunner);
    CPPUNIT_TEST(influxAllTestRunnerWithTelegraf);
    CPPUNIT_TEST(testNoInfluxServerHandling);

    CPPUNIT_TEST_SUITE_END();

public:
    DataLogging_Test();
    virtual ~DataLogging_Test();
    void setUp();
    void tearDown();

private:
    void fileAllTestRunner();
    void influxAllTestRunner();
    void influxAllTestRunnerWithTelegraf();
    void testAllInstantiated(bool waitForLoggerReady = true);

    /**
     * Checks that a call to slotGetPropertyHistory within a time
     * interval that doesn't contain any record of change to the
     * property returns the last known value of the property before
     * the interval.
     */
    void testHistoryAfterChanges();

    void testInt(bool testPastConf = true);
    void testFloat(bool testPastConf = false);
    void testString(bool testPastConf = false);
    void testVectorString(bool testPastConf = false);
    void testTable(bool testPastConf = false);

    void testLastKnownConfiguration();
    void testCfgFromPastRestart();

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

    template <class T> void testHistory(const std::string& key, const std::function<T(int)>& f, const bool testConf);

    std::pair<bool, std::string> startLoggers(const std::string& loggerType, bool useInvalidInfluxUrl = false);

    const std::string m_server;
    const std::string m_deviceId;
    const std::string m_fileLoggerDirectory;
    bool m_changedPath;
    std::string m_oldPath;

    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;
    karabo::xms::SignalSlotable::Pointer m_sigSlot;
    karabo::core::DeviceClient::Pointer m_deviceClient;

    static const unsigned int m_flushIntervalSec;
};

#endif	/* DATALOGGING_TEST_HH */

