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
 * File:   Amqp_Test.hh
 *
 * Created on March 4th, 2024
 */

#ifndef KARABO_AMQP_TEST_HH
#define KARABO_AMQP_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <atomic>
#include <string>
#include <vector>

#include "karabo/net/AmqpConnection.hh"

class Amqp_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Amqp_Test);
    CPPUNIT_TEST(testConnection);
    CPPUNIT_TEST(testClient);
    CPPUNIT_TEST(testClientConcurrentSubscripts);
    CPPUNIT_TEST(testClientSameId);
    CPPUNIT_TEST(testClientUnsubscribeAll);
    CPPUNIT_TEST(testClientTooBigMessage);
    CPPUNIT_TEST(testHashClient);

    CPPUNIT_TEST_SUITE_END();

   public:
    Amqp_Test();
    virtual ~Amqp_Test();

   private:
    std::vector<std::string> m_defaultBrokers;
    const std::chrono::seconds m_timeout;
    const unsigned int m_timeoutMs;
    static std::atomic<bool> m_loggingIsSetup;

    void testConnection();
    void testClient();
    void testClientConcurrentSubscripts();
    void testClientSameId();
    void testClientUnsubscribeAll();
    void testClientTooBigMessage();
    void testHashClient();
};

#endif /* KARABO_AMQP_TEST_HH */
