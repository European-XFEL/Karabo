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
 * File:   testTiming.cc
 * Author: steffen.hauf@xfel.eu
 */

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <karabo/devices/PropertyTest.hh>
#include <karabo/net/EventLoop.hh>
#include <memory>
#include <thread>

#include "karabo/core/DeviceClient.hh"
#include "karabo/core/DeviceServer.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/time/TimeId.hh"
#include "karabo/data/types/StringTools.hh"
#include "karabo/karabo.hpp"


class TestTiming : public ::testing::Test {
   protected:
    TestTiming();
    ~TestTiming() override;
    void SetUp() override;
    void TearDown() override;

    karabo::core::DeviceServer::Pointer m_deviceServer;
    karabo::core::DeviceServer::Pointer m_deviceServer2;
    karabo::core::DeviceClient::Pointer m_deviceClient;
    std::jthread m_eventLoopThread;
};


using namespace std::chrono;
using namespace std::literals::chrono_literals;
using namespace std;

#define KRB_TEST_MAX_TIMEOUT 10

USING_KARABO_NAMESPACES


class PropertyTestWithOnTimeUpdate : public karabo::devices::PropertyTest {
   public:
    KARABO_CLASSINFO(PropertyTestWithOnTimeUpdate, "PropertyTestWithOnTimeUpdate", "2.8")
    PropertyTestWithOnTimeUpdate(const karabo::data::Hash& cfg) : karabo::devices::PropertyTest(cfg) {}

    static void expectedParameters(karabo::data::Schema& expected) {
        UINT64_ELEMENT(expected).key("lastIdOnTimeUpdate").readOnly().initialValue(0ull).commit();
    }

    void onTimeUpdate(unsigned long long id, unsigned long long sec, unsigned long long frac,
                      unsigned long long period) override {
        set("lastIdOnTimeUpdate", id);
    }
};

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::Device, karabo::devices::PropertyTest, PropertyTestWithOnTimeUpdate)


TestTiming::TestTiming() {}


TestTiming::~TestTiming() {}


void TestTiming::SetUp() {
    // uncomment this if ever testing against a local broker
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);

    // Start central event-loop
    m_eventLoopThread = std::jthread([](std::stop_token stoken) { karabo::net::EventLoop::work(); });
    // Create and start server
    {
        // No need to connect the server hosting the time server device to any time server...
        Hash config("serverId", "testServerTiming", "log.level", "FATAL");
        m_deviceServer = DeviceServer::create("DeviceServer", config);
        m_deviceServer->finalizeInternalInitialization();
    }
    {
        Hash config("serverId", "testServerTimingClient", "log.level", "FATAL", "timeServerId", "Karabo_TimeServer");
        m_deviceServer2 = DeviceServer::create("DeviceServer", config);
        m_deviceServer2->finalizeInternalInitialization();
    }
    // Create client
    m_deviceClient = std::make_shared<DeviceClient>(std::string(), false);
    m_deviceClient->initialize();
}


void TestTiming::TearDown() {
    m_deviceServer2.reset();
    m_deviceServer.reset();
    m_deviceClient.reset();
    std::this_thread::sleep_for(200ms);
    EventLoop::stop();
    if (m_eventLoopThread.joinable()) m_eventLoopThread.join();
}


TEST_F(TestTiming, testWrongPeriod) {
    // Bring up a (simulated) time server and a time testing device
    const unsigned long long tickPeriodInMicrosec = 50000ull; // 50 ms
    const long long tickCountdown = 20u;                      // i.e. every 20th id is published
    const float periodVarFrac = 0.1f; // i.e. sometimes the published period is off by 10% up or down
    std::pair<bool, std::string> success =
          m_deviceClient->instantiate("testServerTiming", "SimulatedTimeServerDevice",
                                      Hash("deviceId", "Karabo_TimeServer", "period", tickPeriodInMicrosec,
                                           "tickCountdown", tickCountdown, "periodVariationFraction", periodVarFrac),
                                      KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    const size_t nDevices = 20;

    std::set<std::string> devices;
    for (size_t i = 1; i <= nDevices; ++i) {
        const std::string& deviceId("timeTester_" + toString(i));
        devices.insert(deviceId);
        m_deviceClient->instantiateNoWait("testServerTimingClient", "TimingTestDevice", Hash("deviceId", deviceId));
    }
    int timeout = 20000;
    while (timeout > 0) {
        for (const std::string& onlineDeviceId : m_deviceClient->getDevices()) {
            devices.erase(onlineDeviceId);
        }
        if (devices.empty()) break;
        std::this_thread::sleep_for(100ms);
        timeout -= 100;
    }

    ASSERT_TRUE(devices.empty()) << "Some devices did not get online: " << toString(devices) << " "
                                 << toString(timeout);
    // Give some time to connect the timing slot.
    for (size_t i = 1; i <= nDevices; ++i) {
        int counter = 0;
        while (true) {
            if (m_deviceClient->get<bool>("timeTester_" + toString(i), "slot_connected")) break;
            ASSERT_TRUE(counter++ < 500) << "'timeTester_" << i << "' not yet connected";
            std::this_thread::sleep_for(5ms);
        }
    }

    for (size_t i = 1; i <= nDevices; ++i) {
        m_deviceClient->execute("timeTester_" + toString(i), "start");
    }

    // some time to test the timing
    const unsigned int testDurationInMicrosec = 5432109u;
    std::this_thread::sleep_for(microseconds(testDurationInMicrosec));

    for (size_t i = nDevices; i >= 1; --i) {
        m_deviceClient->execute("timeTester_" + toString(i), "stop");
    }

    for (size_t k = 1; k <= nDevices; ++k) {
        const std::string deviceId = "timeTester_" + toString(k);
        const auto ids(m_deviceClient->get<std::vector<unsigned long long> >(deviceId, "ids"));
        const auto seconds(m_deviceClient->get<std::vector<unsigned long long> >(deviceId, "seconds"));
        const auto fractions(m_deviceClient->get<std::vector<unsigned long long> >(deviceId, "fractions"));

        // Test integrity, i.e. same size of vectors of ids and times
        ASSERT_EQ(ids.size(), seconds.size());
        ASSERT_EQ(ids.size(), fractions.size());
        ASSERT_TRUE(ids.size() >= 2);

        // Test that ids are subsequent and time stamps are increasing (== is allowed!)
        unsigned long long lastId = ids[0];
        karabo::data::Epochstamp lastStamp(seconds[0], fractions[0]);
        for (size_t i = 1; i < ids.size(); ++i) {
            ASSERT_EQ(lastId + 1ull, ids[i]); // (expected, actual)

            const karabo::data::Epochstamp currentStamp(seconds[i], fractions[i]);
            ASSERT_TRUE(currentStamp >= lastStamp);

            const karabo::data::TimeDuration diff = (currentStamp - lastStamp);
            KARABO_LOG_FRAMEWORK_DEBUG_C("TestTiming") << "diff for id: " << ids[i] << " " << static_cast<double>(diff);

            lastId = ids[i];
            lastStamp = currentStamp;
        }

        // Now test that the real ticks received from the time server have the expected spacing and are increasing
        // (== not allowed!).
        const auto idsTick(m_deviceClient->get<std::vector<unsigned long long> >(deviceId, "idsTick"));
        const auto secondsTick(m_deviceClient->get<std::vector<unsigned long long> >(deviceId, "secondsTick"));
        const auto fractionsTick(m_deviceClient->get<std::vector<unsigned long long> >(deviceId, "fractionsTick"));

        ASSERT_TRUE(ids.size() > idsTick.size());
        ASSERT_EQ(idsTick.size(), secondsTick.size());
        ASSERT_EQ(idsTick.size(), fractionsTick.size());
        ASSERT_TRUE(idsTick.size() >= 2);

        unsigned long long lastIdTick = idsTick[0];
        karabo::data::Epochstamp lastStampTick(secondsTick[0], fractionsTick[0]);
        for (size_t i = 1; i < idsTick.size(); ++i) {
            ASSERT_EQ(lastIdTick + static_cast<unsigned long long>(tickCountdown), idsTick[i]);

            const karabo::data::Epochstamp currentStamp(secondsTick[i], fractionsTick[i]);
            ASSERT_TRUE(currentStamp > lastStampTick);

            lastIdTick = idsTick[i];
            lastStampTick = currentStamp;
        }

        if (k == nDevices) { // Started last and stopped first, i.e. testDurationInMicrosec is appropriate.
            // As last test check how many ticks we really got - might be off a bit since time server sometimes reports
            // period that is off by periodVarFrac.
            const int numExpectedTicks = testDurationInMicrosec / tickPeriodInMicrosec;
            // XXX: '+ 2' as extra margin - not understood why needed to get this test stable:
            const int maxOff = int(std::ceil(tickCountdown * periodVarFrac)) + 2;
            const int idsSize = ids.size();
            const std::string msg("Ids received: " + toString(idsSize) + ", expected: " + toString(numExpectedTicks) +
                                  ", maxOff: " + toString(maxOff));
            ASSERT_TRUE(idsSize <= numExpectedTicks + maxOff) << msg;
            ASSERT_TRUE(idsSize >= numExpectedTicks - maxOff) << msg;
        }
    }
}


TEST_F(TestTiming, testIdReset) {
    // Bring up a (simulated) time server and a time testing device
    const std::string timeServerId("Karabo_TimeServer");
    const unsigned long long initialId = 1000000000ull; // 10^9
    const unsigned long long tickPeriodInMs = 10ull;
    const long long tickCountdown = 10u; // i.e. every 10th id is published
    std::pair<bool, std::string> success =
          m_deviceClient->instantiate("testServerTiming", "SimulatedTimeServerDevice",
                                      Hash("deviceId", timeServerId, "initialId", initialId, "tickCountdown",
                                           tickCountdown, "period", tickPeriodInMs * 1000ull),
                                      KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    const std::string testDevice("propTest");
    success = m_deviceClient->instantiate("testServerTimingClient", "PropertyTestWithOnTimeUpdate",
                                          Hash("deviceId", testDevice), KRB_TEST_MAX_TIMEOUT);
    ASSERT_TRUE(success.first) << success.second;

    ASSERT_NO_THROW(m_deviceClient->set(testDevice, "int32Property", 1));

    Hash cfg;
    ASSERT_NO_THROW(m_deviceClient->get(testDevice, cfg));

    ASSERT_EQ(1, cfg.get<int>("int32Property"));
    ASSERT_EQ(1, cfg.get<int>("int32PropertyReadOnly"));
    const unsigned long long lastIdOnTimeUpdate = cfg.get<unsigned long long>("lastIdOnTimeUpdate");
    // ensure that onTimeUpdate has really been called
    ASSERT_LE(initialId, lastIdOnTimeUpdate);

    // Get stamp - the newly set values have more recent stamps than the one from device initialisation
    const TimeId stampDevId = TimeId::fromHashAttributes(cfg.getAttributes("deviceId"));
    const TimeId stampInt32 = TimeId::fromHashAttributes(cfg.getAttributes("int32Property"));
    const TimeId stampInt32ReadOnly = TimeId::fromHashAttributes(cfg.getAttributes("int32PropertyReadOnly"));

    ASSERT_TRUE(stampDevId.getTid() == 0ull // if time stamp assigned before connected to time server
                || initialId < stampDevId.getTid())
          << "devId train " << stampDevId.getTid() << ", initialId " << initialId; // else
    // ASSERT_GREATER asserts that 2nd is greater than 1st
    ASSERT_LT(stampDevId.getTid(), stampInt32.getTid());
    ASSERT_LT(stampDevId.getTid(), stampInt32ReadOnly.getTid());

    // Start ticking from 1 again
    ASSERT_NO_THROW(m_deviceClient->execute(timeServerId, "resetId"));

    // Wait for a tick actually sent so this reset gets seen by devices
    std::this_thread::sleep_for(milliseconds(tickPeriodInMs * tickCountdown));

    ASSERT_NO_THROW(m_deviceClient->set(testDevice, "int32Property", 100));
    ASSERT_NO_THROW(m_deviceClient->get(testDevice, cfg));

    ASSERT_EQ(100, cfg.get<int>("int32Property"));
    ASSERT_EQ(100, cfg.get<int>("int32PropertyReadOnly"));
    // Ticking has restarted, but we guarantee that onTimeUpdate is not called with smaller ids than it has
    // already been called - so lastIdOnTimeUpdate is still greater than initialId before reset:
    ASSERT_LT(initialId, cfg.get<unsigned long long>("lastIdOnTimeUpdate"));

    // Now get stamps again - the newer ones are now smaller than the old ones!
    const TimeId stampInt32_2 = TimeId::fromHashAttributes(cfg.getAttributes("int32Property"));
    const TimeId stampInt32ReadOnly_2 = TimeId::fromHashAttributes(cfg.getAttributes("int32PropertyReadOnly"));

    // ASSERT_GREATER asserts that 2nd is greater than 1st
    ASSERT_LT(stampInt32_2.getTid(), stampInt32.getTid());
    ASSERT_LT(stampInt32ReadOnly_2.getTid(), stampInt32ReadOnly.getTid());
}
