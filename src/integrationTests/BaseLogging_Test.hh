/*
 * File:   BaseLogging_Test.hh
 *
 * Created on October 20, 2020, 3:30 PM
 */

#ifndef BASELOGGING_TEST_HH
#define BASELOGGING_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include "karabo/karabo.hpp"

class BaseLogging_Test : public CPPUNIT_NS::TestFixture {
   public:
    BaseLogging_Test();
    virtual ~BaseLogging_Test();
    void setUp();
    void tearDown();

   protected:
    static int KRB_TEST_MAX_TIMEOUT;
    static int SLOT_REQUEST_TIMEOUT_MILLIS;
    static int FLUSH_REQUEST_TIMEOUT_MILLIS;
    static int FLUSH_INTERVAL_SEC;
    static int NUM_RETRY;
    static int PAUSE_BEFORE_RETRY_MILLIS;
    static int WAIT_WRITES;

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

    void testLastKnownConfiguration(karabo::util::Epochstamp fileMigratedDataEndsBefore = karabo::util::Epochstamp(),
                                    bool dataWasMigrated = false);
    void testCfgFromPastRestart(bool pastStampStaysPast);

    /**
     * Checks that the DataLoggers handle NaN floats and doubles.
     */
    void testNans();

    template <class T>
    void testHistory(const std::string& key, const std::function<T(int)>& f, const bool testConf);

    std::pair<bool, std::string> startDataLoggerManager(const std::string& loggerType, bool useInvalidInfluxUrl = false,
                                                        bool useInvalidDbName = false,
                                                        unsigned int maxPerDevicePropLogRate = 5 * 1024,
                                                        unsigned int propLogRatePeriod = 5);

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
     * means either the last instantion of the device before the requested
     * timepoint, if the device was not active at the timepoint, or the
     * instantiation of the device that was active at the timepoint.
     */
    void testUnchangedNoDefaultProperties();

    std::string getDeviceIdPrefix();

    bool waitForCondition(boost::function<bool()> checker, unsigned int timeoutMillis);

    const std::string m_server;
    const std::string m_deviceId;

    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;
    karabo::xms::SignalSlotable::Pointer m_sigSlot;
    karabo::core::DeviceClient::Pointer m_deviceClient;

    const std::string m_fileLoggerDirectory;
    bool m_changedPath;
    std::string m_oldPath;

    bool m_keepLoggerDirectory = true;
};

#endif /* BASELOGGING_TEST_HH */
