/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   StatisticalEvaluator.hh
 * Author: haufs
 *
 * Created on Jun 16, 2016, 9:10:00 AM
 */

#ifndef STATISTICALEVALUATOR_HH
#define STATISTICALEVALUATOR_HH
#include <cppunit/extensions/HelperMacros.h>

#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/RollingWindowStatistics.hh>

class TestRollingWindowStatisticsFriend : public karabo::util::RollingWindowStatistics {
   public:
    TestRollingWindowStatisticsFriend(unsigned int evalInterval) : RollingWindowStatistics(evalInterval) {}

    double getMeanEstimate() {
        return m_meanEstimate;
    }
};

class StatisticalEvaluator : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(StatisticalEvaluator);

    CPPUNIT_TEST(testMean);
    CPPUNIT_TEST(testSmallNumbers);
    CPPUNIT_TEST(testLargeNumbers);
    CPPUNIT_TEST(testVariance);
    CPPUNIT_TEST(testUpdateMeanTriggering);
    CPPUNIT_TEST(testPerformance);
    CPPUNIT_TEST(testValidatorPerformance);

    CPPUNIT_TEST_SUITE_END();

   public:
    KARABO_CLASSINFO(StatisticalEvaluator, "StatisticalEvaluator", "1.0");

    StatisticalEvaluator();
    virtual ~StatisticalEvaluator();
    void setUp();
    void tearDown();

   private:
    void testMean();
    void testSmallNumbers();
    void testLargeNumbers();
    void testVariance();
    void testUpdateMeanTriggering();
    void testPerformance();
    void testValidatorPerformance();
};

#endif /* STATISTICALEVALUATOR_HH */
