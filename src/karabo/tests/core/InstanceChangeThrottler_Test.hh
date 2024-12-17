/*
 * File:   InstanceChangeThrottler_Test.hh
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

#ifndef INSTANCECHANGETHROTTLER_TEST_HH
#define INSTANCECHANGETHROTTLER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <boost/chrono.hpp>
#include <functional>
#include <karabo/core/InstanceChangeThrottler.hh>
#include <karabo/util/Hash.hh>
#include <mutex>
#include <string>
#include <vector>


//<editor-fold desc="Helper types for InstanceChangeThrottler unit tests">

struct InstanceChange {
    boost::chrono::high_resolution_clock::time_point timePoint;
    karabo::core::InstanceChangeThrottler::InstChangeType changeType;
    std::string instanceId;
    karabo::util::Hash instanceInfo;
};


class InstanceChangeObserver {
   public:
    int numOfInstNewChanges() const;
    int numOfInstGoneChanges() const;
    int numOfInstUpdateChanges() const;
    int numOfInstChanges() const;
    int numOfThrottlerBursts() const;
    InstanceChange instNewChangeAt(size_t position) const;
    InstanceChange instGoneChangeAt(size_t position) const;
    InstanceChange instUpdateChangeAt(size_t position) const;
    InstanceChange newestInstChange() const;
    InstanceChange oldestInstChange() const;
    void clearInstChanges();
    void addInstChanges(const karabo::util::Hash& changeInfo);
    void addInstChangesOfType(const karabo::util::Hash& srcTypesHash,
                              const karabo::core::InstanceChangeThrottler::InstChangeType changeType,
                              std::vector<InstanceChange>& destChangeVector);

   private:
    mutable std::mutex m_instChangesMutex;
    std::vector<InstanceChange> m_instNewChanges;
    std::vector<InstanceChange> m_instGoneChanges;
    std::vector<InstanceChange> m_instUpdateChanges;
    InstanceChange m_newestInstChange;
    InstanceChange m_oldestInstChange;
    int m_numOfThrottlerBursts;
};

//</editor-fold>


//<editor-fold desc="Unit test fixture">

class InstanceChangeThrottler_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(InstanceChangeThrottler_Test);

    CPPUNIT_TEST(testThrottleInterval);
    CPPUNIT_TEST(testNewGoneOptimization);
    CPPUNIT_TEST(testUpdateOptimization);
    CPPUNIT_TEST(testNewUpdateOptimization);
    CPPUNIT_TEST(testUpdateNewOptimization);
    CPPUNIT_TEST(testNewGoneOptimization2Cycles);
    CPPUNIT_TEST(testUpdateOptimization2Cycles);
    CPPUNIT_TEST(testMaxChangesPerCycle);
    CPPUNIT_TEST(testBigUpdateSequence);
    CPPUNIT_TEST(testThrottlerLifecycle);
    CPPUNIT_TEST(testChangesWithFlushes);


    CPPUNIT_TEST_SUITE_END();

   public:
    InstanceChangeThrottler_Test();
    virtual ~InstanceChangeThrottler_Test();
    void setUp();
    void tearDown();

   private:
    std::string m_instIdServer;
    karabo::util::Hash m_instInfoServer;

    std::string m_instIdDevice;
    karabo::util::Hash m_instInfoDevice;

    InstanceChangeObserver m_instChangeObserver;

    void handleInstChange(const karabo::util::Hash& changeInfo);

    /**
     * Test that the "bursts" of event changes received from the throttler are
     * propertly spaced in time.
     */
    void testThrottleInterval();

    /**
     * Test the throttler optimization for New->[Update]->Gone changes for
     * the same instanceId - the throttler is expected to "consume" those sequences
     * if they're still waiting to be dispatched.
     */
    void testNewGoneOptimization();

    /**
     * Test the throttler optimization for New->Update changes for the
     * same instanceId - the throttler is expected to keep just the New
     * with its "payload" overwritten by the payload of the Update.
     */
    void testNewUpdateOptimization();

    /**
     * Test the throttler optimization for Update->New changes without
     * an (expected) intermediary Gone change. The expected outcome is
     * for the Update change to be removed and the New change to be
     * added.
     */
    void testUpdateNewOptimization();

    /**
     * Test the throttler optimization for multiple Update changes for the
     * same instanceId - the throttler is expected to keep just the one (the most
     * recent) update change for an instanceId in its dispatch queue.
     */
    void testUpdateOptimization();

    /**
     * Test that the throttler optimization for New->[Update]->Gone changes for
     * the same instanceId does not happen if the changes are dispatched in
     * different cycles of the throttler.
     */
    void testNewGoneOptimization2Cycles();

    /**
     * Test that the throttler optimization for multiple Update changes for the
     * same instanceId does not happen if the changes are dispatched in different
     * cycles of the throttler.
     */
    void testUpdateOptimization2Cycles();

    /**
     * Test that the limit of maximum changes per cycle is really preempting the
     * interval between cycles for the throttler.
     */
    void testMaxChangesPerCycle();

    /**
     * Tests a big update sequence that should extend for multiple throttler cycles.
     * This is a stress test aimed at exposing internal data race conditions inside
     * the throttler.
     */
    void testBigUpdateSequence();

    /**
     * Tests the stability of the throttler across its lifecycle. To pass
     * this test the throttler must be able to always flush the instance
     * changes it has stored internally before being destructed.
     */
    void testThrottlerLifecycle();


    /**
     * Tests the stability of the throttler across multiple flushes requests in
     * the middle of a large sequence of changes. To pass this test the
     * throttler must be able to dispatch all the submitted instance changes.
     * The alternation between flushes and dispatches from the normal throttler
     * cycle must not compromisse the dispatches.
     */
    void testChangesWithFlushes();
};

//</editor-fold>


#endif /* INSTANCECHANGETHROTTLER_TEST_HH */
