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

    CPPUNIT_TEST(allTestRunner);

    CPPUNIT_TEST_SUITE_END();

public:
    DataLogging_Test();
    virtual ~DataLogging_Test();
    void setUp();
    void tearDown();

private:
    void allTestRunner();
    void testAllInstantiated();
    void testInt();
    void testFloat();
    void testString();
    void testVectorString();
    void testTable();
    template <class T> void testHistory(const std::string& key, const std::function<T(int)>& f, const bool testConf);
    std::string m_server;
    std::string m_deviceId;
    karabo::core::DeviceServer::Pointer m_deviceServer;
    boost::thread m_eventLoopThread;
    karabo::xms::SignalSlotable::Pointer m_sigSlot;
    karabo::core::DeviceClient::Pointer m_deviceClient;

    static const unsigned int m_flushIntervalSec;
};

#endif	/* DATALOGGING_TEST_HH */

