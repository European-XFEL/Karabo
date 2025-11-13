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

#include <gtest/gtest.h>

#include <karabo/net/EventLoop.hh>
#include <sstream>

#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/karabo.hpp"

class TestPropertyTest : public ::testing::Test {
   protected:
    TestPropertyTest();
    ~TestPropertyTest() override;
    void SetUp() override;
    void TearDown() override;

    void testPropertyTest();
    void testSimpleProperties();

    // Asserts that read-only properties cannot be
    // written.
    void testReadOnlyProperties();

    void testVectorProperties();
    void testTableProperties();

    /**
     * Verifies that read-only table properties can
     * only be read.
     */
    void testReadOnlyTableProperties();

    void testAttributeEditing();
    void testNodedSlots();

    karabo::core::DeviceServer::Pointer m_deviceServer;
    karabo::core::DeviceClient::Pointer m_deviceClient;
    std::jthread m_eventLoopThread;
};


USING_KARABO_NAMESPACES;
using std::string;
using std::vector;

#define KRB_TEST_MAX_TIMEOUT 10


TestPropertyTest::TestPropertyTest() {}


TestPropertyTest::~TestPropertyTest() {}


void TestPropertyTest::SetUp() {
    // Start central event-loop
    m_eventLoopThread = std::jthread([](std::stop_token stoken) { karabo::net::EventLoop::work(); });
    // Create and start server
    // FATAL log level since testAttributeEditing() triggers ERRORs on purpose which
    // might mislead someone checking the log output (e.g. when hunting some other problem).
    Hash config("serverId", "propertyTestServer_0", "log.level", "FATAL");
    m_deviceServer = DeviceServer::create("DeviceServer", config);
    m_deviceServer->finalizeInternalInitialization();
    // Create client
    m_deviceClient = std::make_shared<DeviceClient>(std::string(), false);
    m_deviceClient->initialize();
}


void TestPropertyTest::TearDown() {
    std::clog << "Start tearDown " << Epochstamp().toIso8601Ext() << std::endl;

    m_deviceClient.reset();
    m_deviceServer.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EventLoop::stop();
    if (m_eventLoopThread.joinable()) m_eventLoopThread.join();

    std::clog << "End tearDown " << Epochstamp().toIso8601Ext() << std::endl;
}


TEST_F(TestPropertyTest, allTestRunner) {
    std::pair<bool, std::string> success = m_deviceClient->instantiate(
          "propertyTestServer_0", "PropertyTest", Hash("deviceId", "testPropertyTest_0"), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    testSimpleProperties();
    testReadOnlyProperties();
    testVectorProperties();
    testReadOnlyTableProperties();
    testTableProperties();
    testAttributeEditing();
    testNodedSlots();
}


void TestPropertyTest::testSimpleProperties() {
    { // bool
        bool value = true;
        m_deviceClient->get("testPropertyTest_0", "boolProperty", value);
        ASSERT_TRUE(value == false);

        value = true;
        m_deviceClient->set("testPropertyTest_0", "boolProperty", value);
        value = false;
        m_deviceClient->get("testPropertyTest_0", "boolProperty", value);
        ASSERT_TRUE(value == true);

        value = false;
        m_deviceClient->set("testPropertyTest_0", "boolProperty", value);
        value = true;
        m_deviceClient->get("testPropertyTest_0", "boolProperty", value);
        ASSERT_TRUE(value == false);
    }

    { // char
        char value = 'Z';
        m_deviceClient->get("testPropertyTest_0", "charProperty", value);
        ASSERT_TRUE(value == 'A');

        value = 'B';
        m_deviceClient->set("testPropertyTest_0", "charProperty", value);
        value = 'Z';
        m_deviceClient->get("testPropertyTest_0", "charProperty", value);
        ASSERT_TRUE(value == 'B');

        value = 'C';
        m_deviceClient->set("testPropertyTest_0", "charProperty", value);
        value = 'Z';
        m_deviceClient->get("testPropertyTest_0", "charProperty", value);
        ASSERT_TRUE(value == 'C');
    }

    { // int8
        signed char value = 0;
        m_deviceClient->get("testPropertyTest_0", "int8Property", value);
        ASSERT_TRUE(value == 33);

        value = 42;
        m_deviceClient->set("testPropertyTest_0", "int8Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "int8Property", value);
        ASSERT_TRUE(value == 42);

        value = -99;
        m_deviceClient->set("testPropertyTest_0", "int8Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "int8Property", value);
        ASSERT_TRUE(value == -99);
    }

    { // uint8
        unsigned char value = 0;
        m_deviceClient->get("testPropertyTest_0", "uint8Property", value);
        ASSERT_TRUE(value == 177);

        value = 142;
        m_deviceClient->set("testPropertyTest_0", "uint8Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "uint8Property", value);
        ASSERT_TRUE(value == 142);

        value = 199;
        m_deviceClient->set("testPropertyTest_0", "uint8Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "uint8Property", value);
        ASSERT_TRUE(value == 199);
    }

    { // int16
        short value = 0;
        m_deviceClient->get("testPropertyTest_0", "int16Property", value);
        ASSERT_TRUE(value == 3200);

        value = -3200;
        m_deviceClient->set("testPropertyTest_0", "int16Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "int16Property", value);
        ASSERT_TRUE(value == -3200);

        value = -7000;
        m_deviceClient->set("testPropertyTest_0", "int16Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "int16Property", value);
        ASSERT_TRUE(value == -7000);
    }

    { // uint16
        unsigned short value = 0;
        m_deviceClient->get("testPropertyTest_0", "uint16Property", value);
        ASSERT_TRUE(value == 32000);

        value = 1234;
        m_deviceClient->set("testPropertyTest_0", "uint16Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "uint16Property", value);
        ASSERT_TRUE(value == 1234);

        value = 7000;
        m_deviceClient->set("testPropertyTest_0", "uint16Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "uint16Property", value);
        ASSERT_TRUE(value == 7000);
    }

    { // int32
        int value = 0;
        m_deviceClient->get("testPropertyTest_0", "int32Property", value);
        ASSERT_TRUE(value == 32000000);

        value = 1234;
        m_deviceClient->set("testPropertyTest_0", "int32Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "int32Property", value);
        ASSERT_TRUE(value == 1234);

        value = 799;
        m_deviceClient->set("testPropertyTest_0", "int32Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "int32Property", value);
        ASSERT_TRUE(value == 799);
    }

    { // uint32
        unsigned int value = 0;
        m_deviceClient->get("testPropertyTest_0", "uint32Property", value);
        ASSERT_TRUE(value == 32000000);

        value = 12345;
        m_deviceClient->set("testPropertyTest_0", "uint32Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "uint32Property", value);
        ASSERT_TRUE(value == 12345);

        value = 799999;
        m_deviceClient->set("testPropertyTest_0", "uint32Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "uint32Property", value);
        ASSERT_TRUE(value == 799999);
    }


    { // int64
        long long value = 0;
        m_deviceClient->get("testPropertyTest_0", "int64Property", value);
        ASSERT_TRUE(value == 3200000000LL);

        value = 1234LL;
        m_deviceClient->set("testPropertyTest_0", "int64Property", value);
        m_deviceClient->get("testPropertyTest_0", "int64Property", value);
        ASSERT_TRUE(value == 1234LL);

        value = 7999999LL;
        m_deviceClient->set("testPropertyTest_0", "int64Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "int64Property", value);
        ASSERT_TRUE(value == 7999999LL);
    }

    { // uint64
        unsigned long long value = 0;
        m_deviceClient->get("testPropertyTest_0", "uint64Property", value);
        ASSERT_TRUE(value == 3200000000ULL);

        value = 123456789ULL;
        m_deviceClient->set("testPropertyTest_0", "uint64Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "uint64Property", value);
        ASSERT_TRUE(value == 123456789ULL);

        value = 7ULL;
        m_deviceClient->set("testPropertyTest_0", "uint64Property", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "uint64Property", value);
        ASSERT_TRUE(value == 7ULL);
    }

    { // float
        float value = 0;
        m_deviceClient->get("testPropertyTest_0", "floatProperty", value);
        ASSERT_TRUE(value == 3.141596F);

        value = 123.456F;
        m_deviceClient->set("testPropertyTest_0", "floatProperty", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "floatProperty", value);
        ASSERT_TRUE(value == 123.456F);

        value = 76.54321F;
        m_deviceClient->set("testPropertyTest_0", "floatProperty", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "floatProperty", value);
        ASSERT_TRUE(value == 76.54321F);
    }

    { // double
        double value = 0;
        m_deviceClient->get("testPropertyTest_0", "doubleProperty", value);
        ASSERT_TRUE(value == 3.1415967773331);

        value = 123.456000123;
        m_deviceClient->set("testPropertyTest_0", "doubleProperty", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "doubleProperty", value);
        ASSERT_TRUE(value == 123.456000123);

        value = 76.543211787654;
        m_deviceClient->set("testPropertyTest_0", "doubleProperty", value);
        value = 0;
        m_deviceClient->get("testPropertyTest_0", "doubleProperty", value);
        ASSERT_TRUE(value == 76.543211787654);
    }

    std::clog << "Tested simple properties.. Ok" << std::endl;
}


void TestPropertyTest::testReadOnlyProperties() {
    // Read-only float
    float initialFloatReadOnly;
    m_deviceClient->get("testPropertyTest_0", "floatPropertyReadOnly", initialFloatReadOnly);
    ASSERT_THROW(m_deviceClient->set("testPropertyTest_0", "floatPropertyReadOnly", initialFloatReadOnly + 1.0F),
                 karabo::data::ParameterException);
    float finalFloatReadOnly; // Value after call to property set.
    m_deviceClient->get("testPropertyTest_0", "floatPropertyReadOnly", finalFloatReadOnly);
    ASSERT_EQ(initialFloatReadOnly, finalFloatReadOnly);

    // Read-only double
    double initialDoubleReadOnly;
    m_deviceClient->get("testPropertyTest_0", "doublePropertyReadOnly", initialDoubleReadOnly);
    ASSERT_THROW(m_deviceClient->set("testPropertyTest_0", "doublePropertyReadOnly", initialDoubleReadOnly + 1.0),
                 karabo::data::ParameterException);
    double finalDoubleReadOnly; // Value after call to property set.
    m_deviceClient->get("testPropertyTest_0", "doublePropertyReadOnly", finalDoubleReadOnly);
    ASSERT_EQ(initialDoubleReadOnly, finalDoubleReadOnly);

    // Read-only uint8
    unsigned char initialUint8ReadOnly;
    m_deviceClient->get("testPropertyTest_0", "uint8PropertyReadOnly", initialUint8ReadOnly);
    ASSERT_THROW(m_deviceClient->set("testPropertyTest_0", "uint8PropertyReadOnly", initialUint8ReadOnly + 1),
                 karabo::data::ParameterException);
    unsigned char finalUint8ReadOnly; // Value after call to property set.
    m_deviceClient->get("testPropertyTest_0", "uint8PropertyReadOnly", finalUint8ReadOnly);
    ASSERT_EQ(initialUint8ReadOnly, finalUint8ReadOnly);

    // Read-only int8
    signed char initialInt8ReadOnly;
    m_deviceClient->get("testPropertyTest_0", "int8PropertyReadOnly", initialInt8ReadOnly);
    ASSERT_THROW(m_deviceClient->set("testPropertyTest_0", "int8PropertyReadOnly", initialInt8ReadOnly + 1),
                 karabo::data::ParameterException);
    signed char finalInt8ReadOnly; // Value after call to property set.
    m_deviceClient->get("testPropertyTest_0", "int8PropertyReadOnly", finalInt8ReadOnly);
    ASSERT_EQ(initialInt8ReadOnly, finalInt8ReadOnly);

    // Read-only uint16
    unsigned short initialUint16ReadOnly;
    m_deviceClient->get("testPropertyTest_0", "uint16PropertyReadOnly", initialUint16ReadOnly);
    ASSERT_THROW(m_deviceClient->set("testPropertyTest_0", "uint16PropertyReadOnly", initialUint16ReadOnly + 1),
                 karabo::data::ParameterException);
    unsigned short finalUint16ReadOnly; // Value after call to property set.
    m_deviceClient->get("testPropertyTest_0", "uint16PropertyReadOnly", finalUint16ReadOnly);
    ASSERT_EQ(initialUint16ReadOnly, finalUint16ReadOnly);

    // Read-only int16
    short initialInt16ReadOnly;
    m_deviceClient->get("testPropertyTest_0", "int16PropertyReadOnly", initialInt16ReadOnly);
    ASSERT_THROW(m_deviceClient->set("testPropertyTest_0", "int16PropertyReadOnly", initialInt16ReadOnly + 1),
                 karabo::data::ParameterException);
    short finalInt16ReadOnly; // Value after call to property set.
    m_deviceClient->get("testPropertyTest_0", "int16PropertyReadOnly", finalInt16ReadOnly);
    ASSERT_EQ(initialInt16ReadOnly, finalInt16ReadOnly);

    // Read-only uint32
    unsigned int initialUint32ReadOnly;
    m_deviceClient->get("testPropertyTest_0", "uint32PropertyReadOnly", initialUint32ReadOnly);
    ASSERT_THROW(m_deviceClient->set("testPropertyTest_0", "uint32PropertyReadOnly", initialUint32ReadOnly + 2),
                 karabo::data::ParameterException);
    unsigned int finalUint32ReadOnly; // Value after call to property set.
    m_deviceClient->get("testPropertyTest_0", "uint32PropertyReadOnly", finalUint32ReadOnly);
    ASSERT_EQ(initialUint32ReadOnly, finalUint32ReadOnly);

    // Read-only int32
    int initialInt32ReadOnly;
    m_deviceClient->get("testPropertyTest_0", "int32PropertyReadOnly", initialInt32ReadOnly);
    ASSERT_THROW(m_deviceClient->set("testPropertyTest_0", "int32PropertyReadOnly", initialInt32ReadOnly + 2),
                 karabo::data::ParameterException);
    int finalInt32ReadOnly; // Value after call to property set.
    m_deviceClient->get("testPropertyTest_0", "int32PropertyReadOnly", finalInt32ReadOnly);
    ASSERT_EQ(initialInt32ReadOnly, finalInt32ReadOnly);

    // Read-only uint64
    unsigned long long initialUint64ReadOnly;
    m_deviceClient->get("testPropertyTest_0", "uint64PropertyReadOnly", initialUint64ReadOnly);
    ASSERT_THROW(m_deviceClient->set("testPropertyTest_0", "uint64PropertyReadOnly", initialUint64ReadOnly + 2),
                 karabo::data::ParameterException);
    unsigned long long finalUint64ReadOnly; // Value after call to property set.
    m_deviceClient->get("testPropertyTest_0", "uint64PropertyReadOnly", finalUint64ReadOnly);
    ASSERT_EQ(initialUint64ReadOnly, finalUint64ReadOnly);

    // Read-only int64
    long long initialInt64ReadOnly;
    m_deviceClient->get("testPropertyTest_0", "int64PropertyReadOnly", initialInt64ReadOnly);
    ASSERT_THROW(m_deviceClient->set("testPropertyTest_0", "int64PropertyReadOnly", initialInt64ReadOnly + 2),
                 karabo::data::ParameterException);
    long long finalInt64ReadOnly; // Value after call to property set.
    m_deviceClient->get("testPropertyTest_0", "int64PropertyReadOnly", finalInt64ReadOnly);
    ASSERT_EQ(initialInt64ReadOnly, finalInt64ReadOnly);

    std::clog << "Tested read-only properties.. Ok" << std::endl;
}


void TestPropertyTest::testVectorProperties() {
    { // bool
        vector<bool> value;
        m_deviceClient->get("testPropertyTest_0", "vectors.boolProperty", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) {
            if (i % 2 == 0) ASSERT_TRUE(value[i] == true);
            else ASSERT_TRUE(value[i] == false);
        }

        value.assign(5, true);
        m_deviceClient->set("testPropertyTest_0", "vectors.boolProperty", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.boolProperty", value);
        ASSERT_TRUE(value.size() == 5);
        for (size_t i = 0; i < value.size(); ++i) {
            ASSERT_TRUE(value[i] == true);
        }

        value.assign(9, false);
        m_deviceClient->set("testPropertyTest_0", "vectors.boolProperty", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.boolProperty", value);
        ASSERT_TRUE(value.size() == 9);
        for (size_t i = 0; i < value.size(); ++i) {
            ASSERT_TRUE(value[i] == false);
        }
    }

    { // char
        string sample("ABCDEF");
        vector<char> value;
        m_deviceClient->get("testPropertyTest_0", "vectors.charProperty", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) {
            ASSERT_TRUE(value[i] == sample[i]);
        }

        value.assign(6, 'B');
        m_deviceClient->set("testPropertyTest_0", "vectors.charProperty", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.charProperty", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) {
            ASSERT_TRUE(value[i] == 'B');
        }

        value.assign(6, 'C');
        m_deviceClient->set("testPropertyTest_0", "vectors.charProperty", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.charProperty", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) {
            ASSERT_TRUE(value[i] == 'C');
        }
    }

    { // int8
        vector<signed char> value;
        m_deviceClient->get("testPropertyTest_0", "vectors.int8Property", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) {
            ASSERT_EQ(static_cast<signed char>(41 + i), value[i]);
        }

        value.assign(3, 42);
        m_deviceClient->set("testPropertyTest_0", "vectors.int8Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.int8Property", value);
        ASSERT_TRUE(value.size() == 3);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 42);

        value.assign(8, -99);
        m_deviceClient->set("testPropertyTest_0", "vectors.int8Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.int8Property", value);
        ASSERT_TRUE(value.size() == 8);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == -99);
    }

    { // uint8
        vector<unsigned char> value;
        m_deviceClient->get("testPropertyTest_0", "vectors.uint8Property", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == (41 + i));

        value.assign(8, 142);
        m_deviceClient->set("testPropertyTest_0", "vectors.uint8Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.uint8Property", value);
        ASSERT_TRUE(value.size() == 8);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 142);

        value.assign(6, 199);
        m_deviceClient->set("testPropertyTest_0", "vectors.uint8Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.uint8Property", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 199);
    }

    { // int16
        vector<short> value;
        m_deviceClient->get("testPropertyTest_0", "vectors.int16Property", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_EQ(static_cast<short>(20041 + i), value[i]);

        value.assign(4, -3200);
        m_deviceClient->set("testPropertyTest_0", "vectors.int16Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.int16Property", value);
        ASSERT_TRUE(value.size() == 4);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == -3200);

        value.assign(7, -7000);
        m_deviceClient->set("testPropertyTest_0", "vectors.int16Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.int16Property", value);
        ASSERT_TRUE(value.size() == 7);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == -7000);
    }

    { // uint16
        vector<unsigned short> value;
        m_deviceClient->get("testPropertyTest_0", "vectors.uint16Property", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == (i + 10041));

        value.assign(6, 1234);
        m_deviceClient->set("testPropertyTest_0", "vectors.uint16Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.uint16Property", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 1234);

        value.assign(7, 7000);
        m_deviceClient->set("testPropertyTest_0", "vectors.uint16Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.uint16Property", value);
        ASSERT_TRUE(value.size() == 7);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 7000);
    }

    { // int32
        vector<int> value;
        m_deviceClient->get("testPropertyTest_0", "vectors.int32Property", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_EQ(static_cast<int>(20000041 + i), value[i]);

        value.assign(6, 1234);
        m_deviceClient->set("testPropertyTest_0", "vectors.int32Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.int32Property", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 1234);

        value.assign(5, 799);
        m_deviceClient->set("testPropertyTest_0", "vectors.int32Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.int32Property", value);
        ASSERT_TRUE(value.size() == 5);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 799);
    }

    { // uint32
        vector<unsigned int> value;
        m_deviceClient->get("testPropertyTest_0", "vectors.uint32Property", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == (90000041 + i));

        value.assign(1, 12345);
        m_deviceClient->set("testPropertyTest_0", "vectors.uint32Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.uint32Property", value);
        ASSERT_TRUE(value.size() == 1);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 12345);

        value.assign(10, 799999);
        m_deviceClient->set("testPropertyTest_0", "vectors.uint32Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.uint32Property", value);
        ASSERT_TRUE(value.size() == 10);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 799999);
    }


    { // int64
        vector<long long> value;
        m_deviceClient->get("testPropertyTest_0", "vectors.int64Property", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_EQ(static_cast<long long>(i) + 20000000041LL, value[i]);

        value.assign(10, 1234LL);
        m_deviceClient->set("testPropertyTest_0", "vectors.int64Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.int64Property", value);
        ASSERT_TRUE(value.size() == 10);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 1234LL);

        value.assign(1, 7999999LL);
        m_deviceClient->set("testPropertyTest_0", "vectors.int64Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.int64Property", value);
        ASSERT_TRUE(value.size() == 1);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 7999999LL);
    }

    { // uint64
        vector<unsigned long long> value;
        m_deviceClient->get("testPropertyTest_0", "vectors.uint64Property", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 90000000041ULL + i);

        value.assign(4, 123456789ULL);
        m_deviceClient->set("testPropertyTest_0", "vectors.uint64Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.uint64Property", value);
        ASSERT_TRUE(value.size() == 4);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 123456789ULL);

        value.assign(4, 7ULL);
        m_deviceClient->set("testPropertyTest_0", "vectors.uint64Property", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.uint64Property", value);
        ASSERT_TRUE(value.size() == 4);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 7ULL);
    }

    { // float
        vector<float> sample({1.23456, 2.34567, 3.45678, 4.56789, 5.67891, 6.78912});
        vector<float> value;
        m_deviceClient->get("testPropertyTest_0", "vectors.floatProperty", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == sample[i]);

        value.assign(9, 123.456F);
        m_deviceClient->set("testPropertyTest_0", "vectors.floatProperty", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.floatProperty", value);
        ASSERT_TRUE(value.size() == 9);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 123.456F);

        std::ostringstream str;

        value.assign(3, 76.54321F);
        m_deviceClient->set("testPropertyTest_0", "vectors.floatProperty", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.floatProperty", value);
        str << "Actual size: " << value.size();
        ASSERT_EQ(3ul, value.size());
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 76.54321F);
    }

    { // double
        vector<double> sample({1.234567891, 2.345678912, 3.456789123, 4.567891234, 5.678901234, 6.123456789});
        vector<double> value;
        m_deviceClient->get("testPropertyTest_0", "vectors.doubleProperty", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == sample[i]);

        value.assign(8, 123.456000123);
        m_deviceClient->set("testPropertyTest_0", "vectors.doubleProperty", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.doubleProperty", value);
        ASSERT_TRUE(value.size() == 8);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 123.456000123);

        value.assign(2, 76.543211787654);
        m_deviceClient->set("testPropertyTest_0", "vectors.doubleProperty", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.doubleProperty", value);
        ASSERT_TRUE(value.size() == 2);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == 76.543211787654);
    }

    {
        vector<string> sample({"1111111", "2222222", "3333333", "4444444", "5555555", "6666666"});
        vector<string> value;
        m_deviceClient->get("testPropertyTest_0", "vectors.stringProperty", value);
        ASSERT_TRUE(value.size() == 6);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == sample[i]);

        value.assign(8, "ABCD");
        m_deviceClient->set("testPropertyTest_0", "vectors.stringProperty", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.stringProperty", value);
        ASSERT_TRUE(value.size() == 8);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == "ABCD");

        value.assign(2, "HELLO");
        m_deviceClient->set("testPropertyTest_0", "vectors.stringProperty", value);
        value.clear();
        m_deviceClient->get("testPropertyTest_0", "vectors.stringProperty", value);
        ASSERT_TRUE(value.size() == 2);
        for (size_t i = 0; i < value.size(); ++i) ASSERT_TRUE(value[i] == "HELLO");
    }

    std::clog << "Tested vector properties.. Ok" << std::endl;
}


void TestPropertyTest::testTableProperties() {
    vector<Hash> value;
    m_deviceClient->get("testPropertyTest_0", "table", value);

    ASSERT_TRUE(value.size() == 2);

    ASSERT_TRUE(value[0].get<string>("e1") == "abc");
    ASSERT_TRUE(value[0].get<bool>("e2") == true);
    ASSERT_TRUE(value[0].get<int>("e3") == 12);
    ASSERT_TRUE(value[0].get<float>("e4") == 0.9837F);
    ASSERT_TRUE(value[0].get<double>("e5") == 1.2345);

    ASSERT_TRUE(value[1].get<string>("e1") == "xyz");
    ASSERT_TRUE(value[1].get<bool>("e2") == false);
    ASSERT_TRUE(value[1].get<int>("e3") == 42);
    ASSERT_TRUE(value[1].get<float>("e4") == 2.33333F);
    ASSERT_TRUE(value[1].get<double>("e5") == 7.77777);

    value.clear();
    value.push_back(Hash("e1", "abc", "e2", true, "e3", 12, "e4", 0.0011F, "e5", 9.87654321));
    value.push_back(Hash("e1", "xyz", "e2", false, "e3", 42, "e4", 2.2222F, "e5", 3.33333333));
    value.push_back(Hash("e1", "xyz", "e2", false, "e3", 42, "e4", 55.5555F, "e5", 9.99999999));

    m_deviceClient->set("testPropertyTest_0", "table", value);
    value.clear();
    m_deviceClient->get("testPropertyTest_0", "table", value);

    ASSERT_TRUE(value.size() == 3);

    ASSERT_TRUE(value[0].get<string>("e1") == "abc");
    ASSERT_TRUE(value[0].get<bool>("e2") == true);
    ASSERT_TRUE(value[0].get<int>("e3") == 12);
    ASSERT_TRUE(value[0].get<float>("e4") == 0.0011F);
    ASSERT_TRUE(value[0].get<double>("e5") == 9.87654321);

    ASSERT_TRUE(value[1].get<string>("e1") == "xyz");
    ASSERT_TRUE(value[1].get<bool>("e2") == false);
    ASSERT_TRUE(value[1].get<int>("e3") == 42);
    ASSERT_TRUE(value[1].get<float>("e4") == 2.2222F);
    ASSERT_TRUE(value[1].get<double>("e5") == 3.33333333);

    ASSERT_TRUE(value[2].get<string>("e1") == "xyz");
    ASSERT_TRUE(value[2].get<bool>("e2") == false);
    ASSERT_TRUE(value[2].get<int>("e3") == 42);
    ASSERT_TRUE(value[2].get<float>("e4") == 55.5555F);
    ASSERT_TRUE(value[2].get<double>("e5") == 9.99999999);

    std::clog << "Tested table element.. Ok" << std::endl;
}


void TestPropertyTest::testReadOnlyTableProperties() {
    vector<Hash> value;
    m_deviceClient->get("testPropertyTest_0", "tableReadOnly", value);

    ASSERT_TRUE(value.size() == 2);

    ASSERT_TRUE(value[0].get<string>("e1") == "abc");
    ASSERT_TRUE(value[0].get<bool>("e2") == true);
    ASSERT_TRUE(value[0].get<int>("e3") == 12);
    ASSERT_TRUE(value[0].get<float>("e4") == 0.9837F);
    ASSERT_TRUE(value[0].get<double>("e5") == 1.2345);

    ASSERT_TRUE(value[1].get<string>("e1") == "xyz");
    ASSERT_TRUE(value[1].get<bool>("e2") == false);
    ASSERT_TRUE(value[1].get<int>("e3") == 42);
    ASSERT_TRUE(value[1].get<float>("e4") == 2.33333F);
    ASSERT_TRUE(value[1].get<double>("e5") == 7.77777);

    value.clear();
    value.push_back(Hash("e1", "abc", "e2", true, "e3", 12, "e4", 0.0011F, "e5", 9.87654321));
    value.push_back(Hash("e1", "xyz", "e2", false, "e3", 42, "e4", 2.2222F, "e5", 3.33333333));
    value.push_back(Hash("e1", "xyz", "e2", false, "e3", 42, "e4", 55.5555F, "e5", 9.99999999));

    // An attempt to set a read-only property is expected to throw a ParameterException.
    ASSERT_THROW(m_deviceClient->set("testPropertyTest_0", "tableReadOnly", value), karabo::data::ParameterException);
    value.clear();

    std::clog << "Tested read-only table element.. Ok" << std::endl;
}


void TestPropertyTest::testAttributeEditing() {
    // Here we test attribute editing affecting reconfiguration requests.
    // The example attributes tested here are maxSize and minSize for vectors.
    // Attributes relevant for readOnly values are tested in the
    // RunTimeSchemaAttributes_Test that also tests the proper forwarding in the
    // GuiServerDevice from a (fake) Gui client.

    // Need a SignalSlotable instead of DeviceClient to circumvent the checks done in the
    // DeviceClient before sending requests!
    auto caller = std::make_shared<SignalSlotable>("caller");
    caller->start();

    Hash toSend;
    std::vector<int>& vec = toSend.bindReference<std::vector<int> >("vectors.int32Property");

    // Allowed size is 1 - 10 elements
    vec = std::vector<int>{1, 2, 3};
    ASSERT_NO_THROW(caller->request("testPropertyTest_0", "slotReconfigure", toSend)
                          .timeout(1000) // in ms
                          .receive());

    // Empty is too short
    vec = std::vector<int>();
    ASSERT_THROW(caller->request("testPropertyTest_0", "slotReconfigure", toSend)
                       .timeout(1000) // in ms
                       .receive(),
                 karabo::data::RemoteException);

    // 11 is too long
    vec = std::vector<int>(11, 1);
    ASSERT_THROW(caller->request("testPropertyTest_0", "slotReconfigure", toSend)
                       .timeout(1000) // in ms
                       .receive(),
                 karabo::data::RemoteException);

    std::clog << "Tested attribute editing.. Ok" << std::endl;
}


void TestPropertyTest::testNodedSlots() {
    std::clog << "Tested noded slots.. ";
    unsigned int counter = 42u;
    std::string remoteState;
    for (unsigned int i = 0; i < 10; i++) {
        ASSERT_NO_THROW(m_deviceClient->get("testPropertyTest_0", "node.counter", counter));
        ASSERT_EQ(i, counter);
        ASSERT_NO_THROW(remoteState = m_deviceClient->execute1<std::string>("testPropertyTest_0", "node.increment"));
        ASSERT_EQ(State::NORMAL.name(), remoteState);
        remoteState.clear();
    }
    ASSERT_NO_THROW(remoteState = m_deviceClient->execute1<std::string>("testPropertyTest_0", "node.reset"));
    ASSERT_EQ(State::NORMAL.name(), remoteState);
    ASSERT_NO_THROW(m_deviceClient->get("testPropertyTest_0", "node.counter", counter));
    ASSERT_EQ(0u, counter);
    std::clog << "Ok" << std::endl;
}
