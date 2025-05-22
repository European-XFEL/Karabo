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
 * File:   Broker_Test.hh
 * Author: Sergey Esenov serguei.essenov@xfel.eu
 *
 * Created on February 19, 2021, 3:09 PM
 */

#ifndef BROKER_TEST_HH
#define BROKER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/net/Broker.hh>
#include <thread>

#include "karabo/data/types/Hash.hh"

class Broker_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Broker_Test);
    CPPUNIT_TEST(testConnectDisconnect);
    CPPUNIT_TEST(testPublishSubscribe);
    CPPUNIT_TEST(testPublishSubscribeAsync);
    CPPUNIT_TEST(testReadingHeartbeats);
    CPPUNIT_TEST(testReadingGlobalCalls);
    CPPUNIT_TEST(testProducerRestartConsumerContinues);
    CPPUNIT_TEST(testProducerContinuesConsumerRestart);
    CPPUNIT_TEST_SUITE_END();

   public:
    Broker_Test();
    virtual ~Broker_Test();
    void setUp();
    void tearDown();

   private:
    void testConnectDisconnect();
    void _testConnectDisconnect();
    void testPublishSubscribe();
    void _testPublishSubscribe();
    void testPublishSubscribeAsync();
    void _testPublishSubscribeAsync();
    void testReadingHeartbeats();
    void _testReadingHeartbeats();
    void testReadingGlobalCalls();
    void _testReadingGlobalCalls(const std::vector<std::string>& brokerAddress);
    void testReverseOrderedPublishSubscribe();
    void testProducerRestartConsumerContinues();
    void testProducerContinuesConsumerRestart();
    void _testProducerRestartConsumerContinues();
    void _testProducerContinuesConsumerRestart();
    void _loopFunction(const std::string& functionName, const std::function<void()>& testFunction);

   private:
    std::string m_domain;
    std::shared_ptr<std::jthread> m_thread;
    karabo::data::Hash m_config;
    // using a Karabo Hash to match the insertion order.
    karabo::data::Hash m_brokersUnderTest;
    std::map<std::string, std::string> m_invalidBrokers;
    const std::chrono::seconds m_timeout;
};

#endif /* BROKER_TEST_HH */
