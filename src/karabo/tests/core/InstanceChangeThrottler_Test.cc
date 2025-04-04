/*
 * File:   InstanceChangeThrottler_Test.cc
 * Author: <raul.costa@xfel.eu>
 *
 * Created on Feb 11, 2019, 8:52:36 AM
 *
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

#include "InstanceChangeThrottler_Test.hh"

#include <cmath>
#include <functional>
#include <memory>

#include "karabo/tests/WaitUtils.hh"

using namespace std::chrono;
using karabo::tests::waitForCondition;
using std::placeholders::_1;

CPPUNIT_TEST_SUITE_REGISTRATION(InstanceChangeThrottler_Test);

//<editor-fold desc="Tests life-cycle methods">

InstanceChangeThrottler_Test::InstanceChangeThrottler_Test()
    : m_instIdServer("someServer/id"),
      m_instInfoServer("type", "server", "version", "d9c9d93", "heartbeatInterval", 10, "visibility", 4),
      m_instIdDevice("ONE/NINE/DEVICE"),
      m_instInfoDevice("type", "device", "version", "d9c9d94", "heartbeatInterval", 20, "visibility", 4) {}


InstanceChangeThrottler_Test::~InstanceChangeThrottler_Test() {}


void InstanceChangeThrottler_Test::setUp() {
    m_instChangeObserver.clearInstChanges();
}


void InstanceChangeThrottler_Test::tearDown() {}

//</editor-fold>


//<editor-fold desc="Test cases">


void InstanceChangeThrottler_Test::testThrottleInterval() {
    auto instChangeHandler = std::bind(&InstanceChangeThrottler_Test::handleInstChange, this, _1);

    std::shared_ptr<karabo::core::InstanceChangeThrottler> throttler =
          karabo::core::InstanceChangeThrottler::createThrottler(instChangeHandler, 200u);

    // Sends an initial burst of events.
    throttler->submitInstanceNew(m_instIdServer, m_instInfoServer);
    throttler->submitInstanceNew(m_instIdDevice, m_instInfoDevice);

    // Waits long enough for first burst of data to arrive.
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for first burst of instance changes to arrive.",
                           waitForCondition([this]() { return m_instChangeObserver.numOfInstChanges() >= 2; }, 2500u));

    auto fistBurstEndTime = m_instChangeObserver.newestInstChange().timePoint;

    m_instChangeObserver.clearInstChanges();

    // Sends the second burst of events immediately.
    throttler->submitInstanceUpdate(m_instIdDevice, m_instInfoDevice);

    // Waits long enough for the second burst of data to arrive - there's no problem to wait much longer here.
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for second burst of instance changes to arrive.",
                           waitForCondition([this]() { return m_instChangeObserver.numOfInstChanges() >= 1; }, 2500u));

    auto secondBurstStartTime = m_instChangeObserver.oldestInstChange().timePoint;
    auto interval = secondBurstStartTime - fistBurstEndTime;
    auto intervalMilli = duration_cast<milliseconds>(interval);

    CPPUNIT_ASSERT_MESSAGE("Spacing between throttler cycles, " + karabo::data::toString(intervalMilli.count()) +
                                 ", much smaller than expected.",
                           intervalMilli.count() > 190);
}


void InstanceChangeThrottler_Test::testNewGoneOptimization() {
    auto instChangeHandler = std::bind(&InstanceChangeThrottler_Test::handleInstChange, this, _1);

    // Instantiates a throttler with an update that is long enough to guarantee that the
    // sequence to be optimized will be dispatched in the same cycle.
    std::shared_ptr<karabo::core::InstanceChangeThrottler> throttler =
          karabo::core::InstanceChangeThrottler::createThrottler(instChangeHandler, 200u);

    // Sends a burst composed of a sequence that should be optimized and a sequence that
    // should not be optimized.
    throttler->submitInstanceNew(m_instIdServer, m_instInfoServer);
    throttler->submitInstanceUpdate(m_instIdServer, m_instInfoServer);
    throttler->submitInstanceGone(m_instIdServer, m_instInfoServer);
    throttler->submitInstanceNew(m_instIdDevice, m_instInfoDevice);

    // Waits long enough for the burst to arrive.
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for burst of instance changes to arrive.",
                           waitForCondition([this]() { return m_instChangeObserver.numOfInstChanges() >= 1; }, 250u));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong number of total instance changes received.", 1,
                                 m_instChangeObserver.numOfInstChanges());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong number of new instance changes received.", 1,
                                 m_instChangeObserver.numOfInstNewChanges());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance Id at new instance change received.", m_instIdDevice,
                                 m_instChangeObserver.instNewChangeAt(0).instanceId);
}


void InstanceChangeThrottler_Test::testUpdateOptimization() {
    auto instChangeHandler = std::bind(&InstanceChangeThrottler_Test::handleInstChange, this, _1);

    // Instantiates a throttler with an update that is long enough to guarantee that the
    // sequence to be optimized will be dispatched in the same cycle.
    std::shared_ptr<karabo::core::InstanceChangeThrottler> throttler =
          karabo::core::InstanceChangeThrottler::createThrottler(instChangeHandler, 200u);

    // InstanceInfo that will be the payload for the second update.
    karabo::data::Hash updatedInstInfo = karabo::data::Hash(m_instInfoServer);
    updatedInstInfo.set<bool>("UpdatedInfo", true);

    // Sends a sequence that should be optimized.
    throttler->submitInstanceUpdate(m_instIdServer, m_instInfoServer);
    throttler->submitInstanceUpdate(m_instIdServer, updatedInstInfo);

    // Waits long enough for the burst to arrive.
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for burst of instance changes to arrive.",
                           waitForCondition([this]() { return m_instChangeObserver.numOfInstChanges() >= 1; }, 250u));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong number of instance changes received.", 1,
                                 m_instChangeObserver.numOfInstChanges());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance Id at observation 0.", m_instIdServer,
                                 m_instChangeObserver.instUpdateChangeAt(0).instanceId);

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance info payload at observation 1.", true,
                                 m_instChangeObserver.instUpdateChangeAt(0).instanceInfo.get<bool>("UpdatedInfo"));
}


void InstanceChangeThrottler_Test::testNewUpdateOptimization() {
    auto instChangeHandler = std::bind(&InstanceChangeThrottler_Test::handleInstChange, this, _1);

    // Instantiates a throttler with an update that is long enough to guarantee that the
    // sequence to be optimized will be dispatched in the same cycle.
    std::shared_ptr<karabo::core::InstanceChangeThrottler> throttler =
          karabo::core::InstanceChangeThrottler::createThrottler(instChangeHandler, 200u);

    // InstanceInfo that will be the payload for the second update.
    karabo::data::Hash updatedInstInfo = karabo::data::Hash(m_instInfoServer);
    updatedInstInfo.set<bool>("UpdatedInfo", true);

    // Sends a sequence that should be optimized.
    throttler->submitInstanceNew(m_instIdServer, m_instInfoServer);
    throttler->submitInstanceUpdate(m_instIdServer, updatedInstInfo);

    // Waits long enough for the burst to arrive.
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for burst of instance changes to arrive.",
                           waitForCondition([this]() { return m_instChangeObserver.numOfInstChanges() >= 1; }, 250u));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong number of instance changes received.", 1,
                                 m_instChangeObserver.numOfInstChanges());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance Id at observation 0.", m_instIdServer,
                                 m_instChangeObserver.instNewChangeAt(0).instanceId);

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance info payload at observation 1.", true,
                                 m_instChangeObserver.instNewChangeAt(0).instanceInfo.get<bool>("UpdatedInfo"));
}


void InstanceChangeThrottler_Test::testUpdateNewOptimization() {
    auto instChangeHandler = std::bind(&InstanceChangeThrottler_Test::handleInstChange, this, _1);

    // Instantiates a throttler with an update that is long enough to guarantee that the
    // sequence to be optimized will be dispatched in the same cycle.
    std::shared_ptr<karabo::core::InstanceChangeThrottler> throttler =
          karabo::core::InstanceChangeThrottler::createThrottler(instChangeHandler, 200u);

    // Sends a sequence that should be optimized.
    throttler->submitInstanceUpdate(m_instIdServer, m_instInfoServer);
    throttler->submitInstanceNew(m_instIdServer, m_instInfoServer);

    // Waits long enough for the burst to arrive.
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for burst of instance changes to arrive.",
                           waitForCondition([this]() { return m_instChangeObserver.numOfInstChanges() >= 1; }, 250u));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong total number of instance changes received.", 1,
                                 m_instChangeObserver.numOfInstChanges());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong type of instance change at observation 0.", 1,
                                 m_instChangeObserver.numOfInstNewChanges());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance Id at observation 0.", m_instIdServer,
                                 m_instChangeObserver.instNewChangeAt(0).instanceId);
}


void InstanceChangeThrottler_Test::testNewGoneOptimization2Cycles() {
    auto instChangeHandler = std::bind(&InstanceChangeThrottler_Test::handleInstChange, this, _1);

    // Instantiates a throttler with an update that is long enough to guarantee that the
    // sequence that should not be optimized will be splitted in two different cycles.
    std::shared_ptr<karabo::core::InstanceChangeThrottler> throttler =
          karabo::core::InstanceChangeThrottler::createThrottler(instChangeHandler, 200u);

    // Sends a first burst composed of a sequence that should not be optimized - but is
    // the "prefix" of a sequence to be optimized if in the same cycle.
    throttler->submitInstanceNew(m_instIdDevice, m_instInfoDevice);
    throttler->submitInstanceNew(m_instIdServer, m_instInfoServer);

    // Waits long enough for the first burst to arrive.
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for first burst of instance changes to arrive.",
                           waitForCondition([this]() { return m_instChangeObserver.numOfInstChanges() >= 2; }, 250u));

    auto fistBurstEndTime = m_instChangeObserver.newestInstChange().timePoint;

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong number of instance changes received.", 2,
                                 m_instChangeObserver.numOfInstChanges());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance Id at observation 0.", m_instIdDevice,
                                 m_instChangeObserver.instNewChangeAt(0).instanceId);

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance Id at observation 1.", m_instIdServer,
                                 m_instChangeObserver.instNewChangeAt(1).instanceId);

    m_instChangeObserver.clearInstChanges();

    // Sends a second burst with the Gone changes that would be optimized if dispatched
    // in the same cycle of the first burst.
    throttler->submitInstanceGone(m_instIdDevice, m_instInfoDevice);
    throttler->submitInstanceGone(m_instIdServer, m_instInfoServer);

    // Waits long enough for the second burst to arrive.
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for second burst with Gone changes to arrive.",
                           waitForCondition([this]() { return m_instChangeObserver.numOfInstChanges() >= 2; }, 250u));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong number of instance changes received.", 2,
                                 m_instChangeObserver.numOfInstChanges());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance Id at observation 0.", m_instIdDevice,
                                 m_instChangeObserver.instGoneChangeAt(0).instanceId);

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance Id at observation 1.", m_instIdServer,
                                 m_instChangeObserver.instGoneChangeAt(1).instanceId);


    auto secondBurstStartTime = m_instChangeObserver.oldestInstChange().timePoint;
    auto interval = secondBurstStartTime - fistBurstEndTime;
    auto intervalMilli = duration_cast<milliseconds>(interval);

    CPPUNIT_ASSERT_MESSAGE("Could not verify that the Gone changes came in a different throttler cycle.",
                           intervalMilli.count() > 190u);
}


void InstanceChangeThrottler_Test::testUpdateOptimization2Cycles() {
    auto instChangeHandler = std::bind(&InstanceChangeThrottler_Test::handleInstChange, this, _1);

    // Instantiates a throttler with an update that is long enough to guarantee that the
    // sequence that shoud not be optimized will be splitted in two different cycles.
    std::shared_ptr<karabo::core::InstanceChangeThrottler> throttler =
          karabo::core::InstanceChangeThrottler::createThrottler(instChangeHandler, 200u);

    // InstanceInfo that will be the payload for the second update.
    karabo::data::Hash updatedInstInfo = karabo::data::Hash(m_instInfoDevice);
    updatedInstInfo.set<bool>("UpdatedInfo", true);

    // Sends a first burst composed of a sequence that should not be optimized - but is
    // the "prefix" of a sequence to be optimized if in the same cycle.
    throttler->submitInstanceUpdate(m_instIdDevice, m_instInfoDevice);
    throttler->submitInstanceUpdate(m_instIdServer, m_instInfoServer);

    // Waits long enough for the first burst to arrive.
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for first burst of instance changes to arrive.",
                           waitForCondition([this]() { return m_instChangeObserver.numOfInstChanges() >= 2; }, 250u));

    auto fistBurstEndTime = m_instChangeObserver.newestInstChange().timePoint;

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong number of instance changes received.", 2,
                                 m_instChangeObserver.numOfInstChanges());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance Id at observation 0.", m_instIdDevice,
                                 m_instChangeObserver.instUpdateChangeAt(0).instanceId);

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance Id at observation 1.", m_instIdServer,
                                 m_instChangeObserver.instUpdateChangeAt(1).instanceId);

    m_instChangeObserver.clearInstChanges();

    // Sends a second burst with the Update change that would be optimized if dispatched
    // in the same cycle of the first burst.
    throttler->submitInstanceUpdate(m_instIdDevice, updatedInstInfo);
    throttler->submitInstanceUpdate(m_instIdServer, updatedInstInfo);

    // Waits long enough for the second burst to arrive.
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for second burst with update changes to arrive.",
                           waitForCondition([this]() { return m_instChangeObserver.numOfInstChanges() >= 2; }, 250u));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong number of instance changes received.", 2,
                                 m_instChangeObserver.numOfInstChanges());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance Id at observation 0.", m_instIdDevice,
                                 m_instChangeObserver.instUpdateChangeAt(0).instanceId);

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance info payload at observation 0.", true,
                                 m_instChangeObserver.instUpdateChangeAt(0).instanceInfo.get<bool>("UpdatedInfo"));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance Id at observation 1.", m_instIdServer,
                                 m_instChangeObserver.instUpdateChangeAt(1).instanceId);

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong instance info payload at observation 1.", true,
                                 m_instChangeObserver.instUpdateChangeAt(1).instanceInfo.get<bool>("UpdatedInfo"));


    auto secondBurstStartTime = m_instChangeObserver.newestInstChange().timePoint;
    auto interval = secondBurstStartTime - fistBurstEndTime;
    auto intervalMilli = duration_cast<milliseconds>(interval);

    CPPUNIT_ASSERT_MESSAGE(
          "Could not verify that the second batch of update changes came in a different throttler cycle.",
          intervalMilli.count() > 190u);
}


void InstanceChangeThrottler_Test::testMaxChangesPerCycle() {
    auto instChangeHandler = std::bind(&InstanceChangeThrottler_Test::handleInstChange, this, _1);

    std::shared_ptr<karabo::core::InstanceChangeThrottler> throttler =
          karabo::core::InstanceChangeThrottler::createThrottler(instChangeHandler, 200u, 2u);

    // InstanceInfo that will be the payload for the update.
    karabo::data::Hash updatedInstInfo = karabo::data::Hash(m_instInfoDevice);
    updatedInstInfo.set<bool>("UpdatedInfo", true);

    // Sends a sequence that reaches the maximum allowed changes per cycle (taking optimizations into account).
    throttler->submitInstanceNew(m_instIdDevice, m_instInfoDevice);
    throttler->submitInstanceNew(m_instIdServer, m_instInfoServer);
    throttler->submitInstanceUpdate(m_instIdDevice, updatedInstInfo);

    // Waits long enough for the first burst to arrive.
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for first burst of changes to arrive.",
                           waitForCondition([this]() { return m_instChangeObserver.numOfInstChanges() >= 2; }, 250u));

    auto fistBurstEndTime = m_instChangeObserver.newestInstChange().timePoint;

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong number of instance changes received.", 2,
                                 m_instChangeObserver.numOfInstChanges());

    m_instChangeObserver.clearInstChanges();

    // Assures that the third instance change will come in a different cycle - if it wasn't for the max limit its body
    // would overwrite the one for the new change for the instance updated.

    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for second burst of changes to arrive.",
                           waitForCondition([this]() { return m_instChangeObserver.numOfInstChanges() >= 1; }, 250u));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong number of instance changes received.", 1,
                                 m_instChangeObserver.numOfInstChanges());

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong payload contents of instance update change.", true,
                                 m_instChangeObserver.instUpdateChangeAt(0).instanceInfo.get<bool>("UpdatedInfo"));

    auto secondBurstStartTime = m_instChangeObserver.newestInstChange().timePoint;
    auto interval = secondBurstStartTime - fistBurstEndTime;
    auto intervalMilli = duration_cast<milliseconds>(interval);

    CPPUNIT_ASSERT_MESSAGE("Could not verify that the second batch of changes came in a different throttler cycle.",
                           intervalMilli.count() > 190u);
}


void InstanceChangeThrottler_Test::testBigUpdateSequence() {
    auto instChangeHandler = std::bind(&InstanceChangeThrottler_Test::handleInstChange, this, _1);
    const int kNumOfUpdateChanges = 5000;
    const unsigned int kThrottlerIntervalMs = 100u;
    const unsigned int kThrottlerMaxChanges = 4500u;

    std::shared_ptr<karabo::core::InstanceChangeThrottler> throttler =
          karabo::core::InstanceChangeThrottler::createThrottler(instChangeHandler, kThrottlerIntervalMs,
                                                                 kThrottlerMaxChanges);

    auto startTimePoint = high_resolution_clock::now();

    // InstanceInfo that will be the payload for the updates.
    karabo::data::Hash updatedInstInfo = karabo::data::Hash(m_instInfoDevice);
    updatedInstInfo.set<int>("hashCount", -1);

    // Sends a sequence with a large number of updates that will spread across more than one cycle - either because
    // the maximum number of changes per cycle has been reached or the cycle period has elapsed.
    for (int i = 0; i < kNumOfUpdateChanges; i++) {
        updatedInstInfo.set<int>("hashCount", i);
        throttler->submitInstanceUpdate(m_instIdDevice + std::string("_") + std::to_string(i), updatedInstInfo);
    }

    // Waits long enough for the updates to arrive.
    // The test is considered successfull if all the instances are received - check is made on the hashCount which works
    // regardless of update optimizations (updates for same device in the same throttler cycle).
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for instance updates to arrive.",
                           waitForCondition(
                                 [this]() {
                                     int nReceived =
                                           m_instChangeObserver.newestInstChange().instanceInfo.get<int>("hashCount");
                                     return nReceived == (kNumOfUpdateChanges - 1);
                                 },
                                 12000u));

    auto finishTimePoint = high_resolution_clock::now();
    std::clog << "\ntestBigUpdateSequence parameters:" << std::endl;
    std::clog << "\tThrottler interval between bursts: " << kThrottlerIntervalMs << " milliseconds" << std::endl;
    std::clog << "\tThrottler max. changes per burst: " << kThrottlerMaxChanges << std::endl;
    std::clog << "testBigUpdateSequence results:" << std::endl;
    std::clog << "\tInstance update hashes sent: " << kNumOfUpdateChanges << std::endl;
    // The number of received may be smaller than the number of sent if there were updates for the same device in the
    // same throttler cycle. This is not the case for this test as the device names never collide.
    std::clog << "\tInstance update hashes received: " << m_instChangeObserver.numOfInstUpdateChanges() << std::endl;
    // Each reception burst can correspond to the elapsing of a throttler cycle period or to a throttler flush due to
    // the throttler maximum allowed number of changes being reached.
    std::clog << "\tThrottler bursts: " << m_instChangeObserver.numOfThrottlerBursts() << std::endl;
    auto testDuration = finishTimePoint - startTimePoint;
    std::clog << "\tTime to receive all updates: " << duration_cast<milliseconds>(testDuration).count()
              << " milliseconds" << std::endl;
}


void InstanceChangeThrottler_Test::testThrottlerLifecycle() {
    auto instChangeHandler = std::bind(&InstanceChangeThrottler_Test::handleInstChange, this, _1);

    const unsigned int kChangesPerInstantiation = 500u;
    const unsigned int kThrottlerIntervalMs = 5000u;
    const unsigned int kThrottlerMaxChanges = 2000u;

    std::shared_ptr<karabo::core::InstanceChangeThrottler> throttler =
          karabo::core::InstanceChangeThrottler::createThrottler(instChangeHandler, kThrottlerIntervalMs,
                                                                 kThrottlerMaxChanges);

    // InstanceInfo that will be the payload for the updates.
    karabo::data::Hash updatedInstInfo = karabo::data::Hash(m_instInfoDevice);
    updatedInstInfo.set<int>("hashCount", -1);

    // Sends a sequence with a large number of updates.
    for (unsigned int j = 0; j < kChangesPerInstantiation; j++) {
        updatedInstInfo.set<int>("hashCount", j);
        throttler->submitInstanceUpdate(m_instIdDevice + std::string("_") + std::to_string(j), updatedInstInfo);
    }

    // Makes sure the throttler didn't have time to send all the changes.
    CPPUNIT_ASSERT_MESSAGE("Throttler should not have sent any messages yet.",
                           m_instChangeObserver.numOfInstChanges() < (int)kChangesPerInstantiation);

    throttler.reset(); // Destructor should be called as we are the only owner.

    // Waits long enough for the updates to arrive.
    // The test is considered successfull if all the instance updates are received.
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for instance updates to arrive.",
                           waitForCondition(
                                 [this]() {
                                     int nReceived =
                                           m_instChangeObserver.newestInstChange().instanceInfo.get<int>("hashCount");
                                     return nReceived == (kChangesPerInstantiation - 1);
                                 },
                                 15000u));

    m_instChangeObserver.clearInstChanges();
}


void InstanceChangeThrottler_Test::testChangesWithFlushes() {
    auto instChangeHandler = std::bind(&InstanceChangeThrottler_Test::handleInstChange, this, _1);

    const unsigned int kChangesToSubmit = 5000u;
    const unsigned int kIntervalBetweenFlushes = 500u;
    const unsigned int kThrottlerIntervalMs = 500u;
    const unsigned int kThrottlerMaxChanges = 6000u;

    std::shared_ptr<karabo::core::InstanceChangeThrottler> throttler =
          karabo::core::InstanceChangeThrottler::createThrottler(instChangeHandler, kThrottlerIntervalMs,
                                                                 kThrottlerMaxChanges);

    // InstanceInfo that will be the payload for the updates.
    karabo::data::Hash updatedInstInfo = karabo::data::Hash(m_instInfoDevice);
    updatedInstInfo.set<int>("hashCount", -1);

    // Sends a sequence with a large number of updates.
    for (unsigned int j = 0; j < kChangesToSubmit; j++) {
        updatedInstInfo.set<int>("hashCount", j);
        if (j % kIntervalBetweenFlushes == 0) {
            throttler->flush();
        }
        throttler->submitInstanceUpdate(m_instIdDevice + std::string("_") + std::to_string(j), updatedInstInfo);
    }

    // Waits long enough for the updates to arrive.
    // The test is considered successfull if all the instance updates are received.
    CPPUNIT_ASSERT_MESSAGE("Timeout waiting for instance updates to arrive.",
                           waitForCondition(
                                 [this]() {
                                     int nReceived =
                                           m_instChangeObserver.newestInstChange().instanceInfo.get<int>("hashCount");
                                     return nReceived == (kChangesToSubmit - 1);
                                 },
                                 15000u));

    // Each flush should cause an immediate burst of the throttler.
    CPPUNIT_ASSERT_MESSAGE(
          std::string("Number of throttler bursts inferior to the minimum expected of ") +
                karabo::data::toString(kChangesToSubmit / kIntervalBetweenFlushes) + std::string(" bursts."),
          m_instChangeObserver.numOfThrottlerBursts() >= static_cast<int>(kChangesToSubmit / kIntervalBetweenFlushes));

    m_instChangeObserver.clearInstChanges();
}

//</editor-fold>


//<editor-fold desc="Helper methods shared by test cases">


void InstanceChangeThrottler_Test::handleInstChange(const karabo::data::Hash& changeInfo) {
    m_instChangeObserver.addInstChanges(changeInfo);
}


//</editor-fold>


//<editor-fold desc="InstanceChangeObserver helper class">

int InstanceChangeObserver::numOfInstNewChanges() const {
    std::lock_guard<std::mutex> lock(m_instChangesMutex);
    return m_instNewChanges.size();
}

int InstanceChangeObserver::numOfInstGoneChanges() const {
    std::lock_guard<std::mutex> lock(m_instChangesMutex);
    return m_instGoneChanges.size();
}

int InstanceChangeObserver::numOfInstUpdateChanges() const {
    std::lock_guard<std::mutex> lock(m_instChangesMutex);
    return m_instUpdateChanges.size();
}

int InstanceChangeObserver::numOfInstChanges() const {
    std::lock_guard<std::mutex> lock(m_instChangesMutex);
    return m_instNewChanges.size() + m_instGoneChanges.size() + m_instUpdateChanges.size();
}

int InstanceChangeObserver::numOfThrottlerBursts() const {
    std::lock_guard<std::mutex> lock(m_instChangesMutex);
    return m_numOfThrottlerBursts;
}

InstanceChange InstanceChangeObserver::instNewChangeAt(size_t obsNum) const {
    std::lock_guard<std::mutex> lock(m_instChangesMutex);
    return m_instNewChanges.at(obsNum);
}

InstanceChange InstanceChangeObserver::instGoneChangeAt(size_t obsNum) const {
    std::lock_guard<std::mutex> lock(m_instChangesMutex);
    return m_instGoneChanges.at(obsNum);
}

InstanceChange InstanceChangeObserver::instUpdateChangeAt(size_t obsNum) const {
    std::lock_guard<std::mutex> lock(m_instChangesMutex);
    return m_instUpdateChanges.at(obsNum);
}

void InstanceChangeObserver::clearInstChanges() {
    std::lock_guard<std::mutex> lock(m_instChangesMutex);
    m_numOfThrottlerBursts = 0;
    m_instNewChanges.clear();
    m_instGoneChanges.clear();
    m_instUpdateChanges.clear();
}

InstanceChange InstanceChangeObserver::newestInstChange() const {
    std::lock_guard<std::mutex> lock(m_instChangesMutex);
    return m_newestInstChange;
}

InstanceChange InstanceChangeObserver::oldestInstChange() const {
    std::lock_guard<std::mutex> lock(m_instChangesMutex);
    return m_oldestInstChange;
}

void InstanceChangeObserver::addInstChangesOfType(
      const karabo::data::Hash& srcInstTypeHash, const karabo::core::InstanceChangeThrottler::InstChangeType changeType,
      std::vector<InstanceChange>& destChangeVector) {
    std::vector<std::string> instTypesKeys;
    srcInstTypeHash.getKeys(instTypesKeys);

    for (auto& instTypeKey : instTypesKeys) {
        karabo::data::Hash changesEntries = srcInstTypeHash.get<karabo::data::Hash>(instTypeKey);
        std::vector<std::string> entriesKeys;
        changesEntries.getKeys(entriesKeys);

        for (auto& instIdKey : entriesKeys) {
            InstanceChange instChange;
            instChange.changeType = changeType;
            instChange.instanceId = instIdKey;
            karabo::data::Hash leafHash = changesEntries.get<karabo::data::Hash>(instIdKey);

            // The instance info payload, when existing, is always transported as attributes; recomposing it.
            const karabo::data::Hash::Attributes& attrs = changesEntries.getAttributes(instIdKey);
            karabo::data::Hash instInfo;
            for (karabo::data::Hash::Attributes::const_iterator attr = attrs.begin(); attr != attrs.end(); ++attr) {
                instInfo.set<std::any>(attr->getKey(), attr->getValueAsAny());
            }
            instChange.instanceInfo = instInfo;

            instChange.timePoint = high_resolution_clock::now();
            if (instChange.timePoint > m_newestInstChange.timePoint) {
                m_newestInstChange = instChange;
            }
            if (instChange.timePoint < m_oldestInstChange.timePoint) {
                m_oldestInstChange = instChange;
            }
            destChangeVector.push_back(instChange);
        }
    }
}


void InstanceChangeObserver::addInstChanges(const karabo::data::Hash& changeInfo) {
    std::lock_guard<std::mutex> lock(m_instChangesMutex);

    auto yearAgoTimePoint = high_resolution_clock::now() - hours(24 * 365);
    auto yearAheadTimePoint = high_resolution_clock::now() + hours(24 * 365);
    m_oldestInstChange.timePoint = yearAheadTimePoint;
    m_newestInstChange.timePoint = yearAgoTimePoint;

    m_numOfThrottlerBursts++;

    addInstChangesOfType(changeInfo.get<karabo::data::Hash>("new"),
                         karabo::core::InstanceChangeThrottler::InstChangeType::NEW, m_instNewChanges);
    addInstChangesOfType(changeInfo.get<karabo::data::Hash>("gone"),
                         karabo::core::InstanceChangeThrottler::InstChangeType::GONE, m_instGoneChanges);
    addInstChangesOfType(changeInfo.get<karabo::data::Hash>("update"),
                         karabo::core::InstanceChangeThrottler::InstChangeType::UPDATE, m_instUpdateChanges);
}

//</editor-fold>
