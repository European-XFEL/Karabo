/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   MqttClient_Test.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on July 30, 2018, 3:37 PM
 */

#ifndef MQTTCLIENT_TEST_HH
#define MQTTCLIENT_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/net/MqttClient.hh>


class MqttClient_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(MqttClient_Test);
    CPPUNIT_TEST(testTopicHasWildcard);
    CPPUNIT_TEST(testTopicMatch);

    CPPUNIT_TEST(testConnectSync);
    CPPUNIT_TEST(testConnectAsync);

    CPPUNIT_TEST(testTryingToCallOperationsWithoutBeingConnected);

    CPPUNIT_TEST(testPublishSubscribeAtMostOnceSync);
    CPPUNIT_TEST(testPublishSubscribeAtLeastOnceSync);
    CPPUNIT_TEST(testPublishSubscribeExactlyOnceSync);

    CPPUNIT_TEST(testPublishSubscribeAtMostOnceAsync);
    CPPUNIT_TEST(testPublishSubscribeAtLeastOnceAsync);
    CPPUNIT_TEST(testPublishSubscribeExactlyOnceAsync);

    CPPUNIT_TEST(testMultipleSubscribersToTheSameTopic);
    CPPUNIT_TEST(testMultipleSubscriptionsToTopicsWithAndWithoutWildcards);
    CPPUNIT_TEST(testMultipleSubscriptionsToTopicWithWildcardsAndSubtopics);
    CPPUNIT_TEST(testTopicsSubscriptionsInArbitraryOrder);

    CPPUNIT_TEST_SUITE_END();

   public:
    MqttClient_Test();
    virtual ~MqttClient_Test();

   private:
    void testTopicMatch();
    void testTopicHasWildcard();
    void testConnectSync();
    void testConnectSync_(const std::string& classId);
    void testConnectAsync();
    void testConnectAsync_(const std::string& classId);
    void testTryingToCallOperationsWithoutBeingConnected();
    void testTryingToCallOperationsWithoutBeingConnected_(const std::string& classId);

    void testPublishSubscribeSync(const std::string& classId, const int qos);
    void testPublishManySubscribeSync(const std::string& classId, const int qos);
    void testPublishMultiSubscribeSync(const std::string& classId, const int qos);
    void testPublishSubscribeAtMostOnceSync();
    void testPublishSubscribeAtLeastOnceSync();
    void testPublishSubscribeExactlyOnceSync();

    void testPublishSubscribeAsync(const std::string& classId, const int qos);
    void testPublishManySubscribeAsync(const std::string& classId, const int qos);
    void testPublishSubscribeAtMostOnceAsync();
    void testPublishSubscribeAtLeastOnceAsync();
    void testPublishSubscribeExactlyOnceAsync();

    void testMultipleSubscribersToTheSameTopic();
    void testMultipleSubscribersToTheSameTopic_(const std::string& classId);
    void testMultipleSubscriptionsToTopicsWithAndWithoutWildcards();
    void testMultipleSubscriptionsToTopicsWithAndWithoutWildcards_(const std::string& classId);
    void testMultipleSubscriptionsToTopicWithWildcardsAndSubtopics();
    void testMultipleSubscriptionsToTopicWithWildcardsAndSubtopics_(const std::string& classId);
    void testTopicsSubscriptionsInArbitraryOrder();
    void testTopicsSubscriptionsInArbitraryOrder_(const std::string& classId);

    std::string qos2operationString(const int qos);

   private:
    std::vector<std::string> m_brokers;
    std::string m_domain;
};


#endif /* MQTTCLIENT_TEST_HH */
