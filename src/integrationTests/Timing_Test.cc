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
 * File:   Timing_Test.cc
 * Author: steffen.hauf@xfel.eu

 */

#include "Timing_Test.hh"

#include <chrono>
#include <cstdlib>
#include <karabo/devices/PropertyTest.hh>
#include <karabo/net/EventLoop.hh>
#include <thread>

#include "PropertyTest_Test.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/time/Trainstamp.hh"
#include "karabo/data/types/StringTools.hh"

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

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::BaseDevice, karabo::core::Device, karabo::devices::PropertyTest,
                                  PropertyTestWithOnTimeUpdate)

CPPUNIT_TEST_SUITE_REGISTRATION(Timing_Test);


Timing_Test::Timing_Test() {}


Timing_Test::~Timing_Test() {}


void Timing_Test::setUp() {
    // uncomment this if ever testing against a local broker
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);

    // Start central event-loop
    m_eventLoopThread = std::jthread([](std::stop_token stoken) { karabo::net::EventLoop::work(); });
    // Create and start server
    {
        // No need to connect the server hosting the time server device to any time server...
        Hash config("serverId", "testServerTiming", "scanPlugins", false, "Logger.priority", "FATAL");
        m_deviceServer = DeviceServer::create("DeviceServer", config);
        m_deviceServer->finalizeInternalInitialization();
    }
    {
        Hash config("serverId", "testServerTimingClient", "scanPlugins", false, "Logger.priority", "FATAL",
                    "timeServerId", "Karabo_TimeServer");
        m_deviceServer2 = DeviceServer::create("DeviceServer", config);
        m_deviceServer2->finalizeInternalInitialization();
    }
    // Create client
    m_deviceClient = std::make_shared<DeviceClient>(std::string(), false);
    m_deviceClient->initialize();
}


void Timing_Test::tearDown() {
    m_deviceServer2.reset();
    m_deviceServer.reset();
    m_deviceClient.reset();
    EventLoop::stop();
}


void Timing_Test::testWrongPeriod() {
    // Bring up a (simulated) time server and a time testing device
    const unsigned long long tickPeriodInMicrosec = 50000ull; // 50 ms
    const long long tickCountdown = 20u;                      // i.e. every 20th id is published
    const float periodVarFrac = 0.1f; // i.e. sometimes the published period is off by 10% up or down
    std::pair<bool, std::string> success =
          m_deviceClient->instantiate("testServerTiming", "SimulatedTimeServerDevice",
                                      Hash("deviceId", "Karabo_TimeServer", "period", tickPeriodInMicrosec,
                                           "tickCountdown", tickCountdown, "periodVariationFraction", periodVarFrac),
                                      KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

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

    CPPUNIT_ASSERT_MESSAGE("Some devices did not get online: " + toString(devices) += " " + toString(timeout),
                           devices.empty());
    // Give some time to connect the timing slot.
    for (size_t i = 1; i <= nDevices; ++i) {
        int counter = 0;
        while (true) {
            if (m_deviceClient->get<bool>("timeTester_" + toString(i), "slot_connected")) break;
            CPPUNIT_ASSERT_MESSAGE("'timeTester_" + toString(i) += "' not yet connected", counter++ < 500);
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
        CPPUNIT_ASSERT_EQUAL(ids.size(), seconds.size());
        CPPUNIT_ASSERT_EQUAL(ids.size(), fractions.size());
        CPPUNIT_ASSERT(ids.size() >= 2);

        // Test that ids are subsequent and time stamps are increasing (== is allowed!)
        unsigned long long lastId = ids[0];
        karabo::data::Epochstamp lastStamp(seconds[0], fractions[0]);
        for (size_t i = 1; i < ids.size(); ++i) {
            CPPUNIT_ASSERT_EQUAL(lastId + 1ull, ids[i]); // (expected, actual)

            const karabo::data::Epochstamp currentStamp(seconds[i], fractions[i]);
            CPPUNIT_ASSERT(currentStamp >= lastStamp);

            const karabo::data::TimeDuration diff = (currentStamp - lastStamp);
            KARABO_LOG_FRAMEWORK_DEBUG_C("Timing_Test")
                  << "diff for id: " << ids[i] << " " << static_cast<double>(diff);

            lastId = ids[i];
            lastStamp = currentStamp;
        }

        // Now test that the real ticks received from the time server have the expected spacing and are increasing
        // (== not allowed!).
        const auto idsTick(m_deviceClient->get<std::vector<unsigned long long> >(deviceId, "idsTick"));
        const auto secondsTick(m_deviceClient->get<std::vector<unsigned long long> >(deviceId, "secondsTick"));
        const auto fractionsTick(m_deviceClient->get<std::vector<unsigned long long> >(deviceId, "fractionsTick"));

        CPPUNIT_ASSERT(ids.size() > idsTick.size());
        CPPUNIT_ASSERT_EQUAL(idsTick.size(), secondsTick.size());
        CPPUNIT_ASSERT_EQUAL(idsTick.size(), fractionsTick.size());
        CPPUNIT_ASSERT(idsTick.size() >= 2);

        unsigned long long lastIdTick = idsTick[0];
        karabo::data::Epochstamp lastStampTick(secondsTick[0], fractionsTick[0]);
        for (size_t i = 1; i < idsTick.size(); ++i) {
            CPPUNIT_ASSERT_EQUAL(lastIdTick + static_cast<unsigned long long>(tickCountdown), idsTick[i]);

            const karabo::data::Epochstamp currentStamp(secondsTick[i], fractionsTick[i]);
            CPPUNIT_ASSERT(currentStamp > lastStampTick);

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
            CPPUNIT_ASSERT_MESSAGE(msg, idsSize <= numExpectedTicks + maxOff);
            CPPUNIT_ASSERT_MESSAGE(msg, idsSize >= numExpectedTicks - maxOff);
        }
    }
}


void Timing_Test::testIdReset() {
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
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    const std::string testDevice("propTest");
    success = m_deviceClient->instantiate("testServerTimingClient", "PropertyTestWithOnTimeUpdate",
                                          Hash("deviceId", testDevice), KRB_TEST_MAX_TIMEOUT);
    CPPUNIT_ASSERT_MESSAGE(success.second, success.first);

    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set(testDevice, "int32Property", 1));

    Hash cfg;
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(testDevice, cfg));

    CPPUNIT_ASSERT_EQUAL(1, cfg.get<int>("int32Property"));
    CPPUNIT_ASSERT_EQUAL(1, cfg.get<int>("int32PropertyReadOnly"));
    const unsigned long long lastIdOnTimeUpdate = cfg.get<unsigned long long>("lastIdOnTimeUpdate");
    // ensure that onTimeUpdate has really been called
    CPPUNIT_ASSERT_GREATEREQUAL(initialId, lastIdOnTimeUpdate);

    // Get stamp - the newly set values have more recent stamps than the one from device initialisation
    const Trainstamp stampDevId = Trainstamp::fromHashAttributes(cfg.getAttributes("deviceId"));
    const Trainstamp stampInt32 = Trainstamp::fromHashAttributes(cfg.getAttributes("int32Property"));
    const Trainstamp stampInt32ReadOnly = Trainstamp::fromHashAttributes(cfg.getAttributes("int32PropertyReadOnly"));

    CPPUNIT_ASSERT_MESSAGE("devId train " + toString(stampDevId.getTrainId()) += ", initialId " + toString(initialId),
                           stampDevId.getTrainId() == 0ull // if time stamp assigned before connected to time server
                                 || initialId < stampDevId.getTrainId()); // else
    // ASSERT_GREATER asserts that 2nd is greater than 1st
    CPPUNIT_ASSERT_GREATER(stampDevId.getTrainId(), stampInt32.getTrainId());
    CPPUNIT_ASSERT_GREATER(stampDevId.getTrainId(), stampInt32ReadOnly.getTrainId());

    // Start ticking from 1 again
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->execute(timeServerId, "resetId"));

    // Wait for a tick actually sent so this reset gets seen by devices
    std::this_thread::sleep_for(milliseconds(tickPeriodInMs * tickCountdown));

    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->set(testDevice, "int32Property", 100));
    CPPUNIT_ASSERT_NO_THROW(m_deviceClient->get(testDevice, cfg));

    CPPUNIT_ASSERT_EQUAL(100, cfg.get<int>("int32Property"));
    CPPUNIT_ASSERT_EQUAL(100, cfg.get<int>("int32PropertyReadOnly"));
    // Ticking has restarted, but we guarantee that onTimeUpdate is not called with smaller ids than it has
    // already been called - so lastIdOnTimeUpdate is still greater than initialId before reset:
    CPPUNIT_ASSERT_GREATER(initialId, cfg.get<unsigned long long>("lastIdOnTimeUpdate"));

    // Now get stamps again - the newer ones are now smaller than the old ones!
    const Trainstamp stampInt32_2 = Trainstamp::fromHashAttributes(cfg.getAttributes("int32Property"));
    const Trainstamp stampInt32ReadOnly_2 = Trainstamp::fromHashAttributes(cfg.getAttributes("int32PropertyReadOnly"));

    // ASSERT_GREATER asserts that 2nd is greater than 1st
    CPPUNIT_ASSERT_GREATER(stampInt32_2.getTrainId(), stampInt32.getTrainId());
    CPPUNIT_ASSERT_GREATER(stampInt32ReadOnly_2.getTrainId(), stampInt32ReadOnly.getTrainId());
}
