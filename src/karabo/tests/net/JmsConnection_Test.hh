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
 * File:   JmsConnection_Test.hh
 * Author: heisenb
 *
 * Created on July 15, 2016, 5:05 PM
 */

#ifndef JMSCONNECTION_TEST_HH
#define JMSCONNECTION_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <atomic>
#include <string>

#include "karabo/net/JmsConnection.hh"
#include "karabo/net/JmsConsumer.hh"
#include "karabo/net/JmsProducer.hh"

class JmsConnection_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(JmsConnection_Test);
    CPPUNIT_TEST(testConnect);
    CPPUNIT_TEST(testCommunication1);
    CPPUNIT_TEST(testCommunication2);
    CPPUNIT_TEST(testPermanentRead);

    CPPUNIT_TEST_SUITE_END();

    std::vector<std::string> m_defaultBrokers;
    std::string m_baseTopic;
    std::atomic<unsigned int> m_messageCount;
    std::vector<std::string> m_failures;
    boost::posix_time::ptime m_tick;

   public:
    JmsConnection_Test();
    virtual ~JmsConnection_Test();

   private:
    void testConnect();

    void testCommunication1();

    void testCommunication2();

    void testPermanentRead();

    void readHandler1(karabo::net::JmsConsumer::Pointer consumer, karabo::net::JmsProducer::Pointer producer,
                      karabo::util::Hash::Pointer header, karabo::util::Hash::Pointer body);

    void readHandler2(karabo::net::JmsConsumer::Pointer channel, karabo::util::Hash::Pointer header,
                      karabo::util::Hash::Pointer body);
};

#endif /* JMSCONNECTION_TEST_HH */
