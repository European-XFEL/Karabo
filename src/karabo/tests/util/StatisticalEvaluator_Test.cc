/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   StatisticalEvaluator.cc
 * Author: haufs
 *
 * Created on Jun 16, 2016, 9:10:01 AM
 */

#include "StatisticalEvaluator_Test.hh"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <karabo/util/Schema.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/TimeDuration.hh>
#include <karabo/util/TimePeriod.hh>
#include <karabo/util/TimeProfiler.hh>
#include <karabo/util/Validator.hh>
#include <sstream>


CPPUNIT_TEST_SUITE_REGISTRATION(StatisticalEvaluator);


StatisticalEvaluator::StatisticalEvaluator() {}


StatisticalEvaluator::~StatisticalEvaluator() {}


void StatisticalEvaluator::setUp() {}


void StatisticalEvaluator::tearDown() {}


void StatisticalEvaluator::testMean() {
    double EPSILON = 0.0001;
    karabo::util::RollingWindowStatistics stat(5);
    stat.update(-5);

    CPPUNIT_ASSERT(fabs(stat.getRollingWindowMean() + 5) < EPSILON);
    stat.update(0);

    CPPUNIT_ASSERT(fabs(stat.getRollingWindowMean() + 2.5) < EPSILON);
    stat.update(5);

    CPPUNIT_ASSERT(fabs(stat.getRollingWindowMean() - 0) < EPSILON);
    stat.update(5);

    CPPUNIT_ASSERT(fabs(stat.getRollingWindowMean() - 1.25) < EPSILON);
    stat.update(-5);

    CPPUNIT_ASSERT(fabs(stat.getRollingWindowMean() - 0.) < EPSILON);
    stat.update(-5); // should remove the first -5

    CPPUNIT_ASSERT(fabs(stat.getRollingWindowMean() - 0.) < EPSILON);
}


void StatisticalEvaluator::testSmallNumbers() {
    double EPSILON_MEAN = 1e-10;
    double EPSILON_VAR = 1e-13;

    karabo::util::RollingWindowStatistics stat(10);
    stat.update(123e-9);
    stat.update(23e-9);
    stat.update(33e-9);
    stat.update(43e-9);
    stat.update(1e-9);
    stat.update(134e-9);
    stat.update(14e-9);
    stat.update(123e-9);
    stat.update(-123e-9);
    stat.update(4123e-9);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowMean() - 4.494e-7) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 1.67183e-12) < EPSILON_VAR);
    stat.update(123e-9);
    stat.update(23e-9);
    stat.update(33e-9);
    stat.update(43e-9);
    stat.update(1e-9);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowMean() - 4.494e-7) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 1.67183e-12) < EPSILON_VAR);

    karabo::util::RollingWindowStatistics stat100(100);
    for (size_t i = 0; i < 11; i++) {
        stat100.update(123e-9);
        stat100.update(23e-9);
        stat100.update(33e-9);
        stat100.update(43e-9);
        stat100.update(1e-9);
        stat100.update(134e-9);
        stat100.update(14e-9);
        stat100.update(123e-9);
        stat100.update(-123e-9);
        stat100.update(4123e-9);
    }

    CPPUNIT_ASSERT(fabs(stat100.getRollingWindowMean() - 4.494e-07) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat100.getRollingWindowVariance() - 1.50465324e-12) < EPSILON_VAR);

    karabo::util::RollingWindowStatistics stat1000(1000);
    for (size_t i = 0; i < 101; i++) {
        stat1000.update(123e-9);
        stat1000.update(23e-9);
        stat1000.update(33e-9);
        stat1000.update(43e-9);
        stat1000.update(1e-9);
        stat1000.update(134e-9);
        stat1000.update(14e-9);
        stat1000.update(123e-9);
        stat1000.update(-123e-9);
        stat1000.update(4123e-9);
    }

    CPPUNIT_ASSERT(fabs(stat1000.getRollingWindowMean() - 4.494e-07) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat1000.getRollingWindowVariance() - 1.50465324e-12) < EPSILON_VAR);
}


void StatisticalEvaluator::testLargeNumbers() {
    double EPSILON_MEAN = 1e5;
    double EPSILON_VAR = 1e19;

    karabo::util::RollingWindowStatistics stat(10);
    stat.update(123e9);
    stat.update(23e9);
    stat.update(33e9);
    stat.update(43e9);
    stat.update(1e9);
    stat.update(134e9);
    stat.update(14e9);
    stat.update(123e9);
    stat.update(-123e9);
    stat.update(4123e9);

    CPPUNIT_ASSERT(fabs(stat.getRollingWindowMean() - 449400000000.0) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 1.6718369e+24) < EPSILON_VAR);
    stat.update(123e9);
    stat.update(23e9);
    stat.update(33e9);
    stat.update(43e9);
    stat.update(1e9);

    CPPUNIT_ASSERT(fabs(stat.getRollingWindowMean() - 449400000000.0) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 1.6718369e+24) < EPSILON_VAR);

    karabo::util::RollingWindowStatistics stat100(100);
    for (size_t i = 0; i < 11; i++) {
        stat100.update(123e9);
        stat100.update(23e9);
        stat100.update(33e9);
        stat100.update(43e9);
        stat100.update(1e9);
        stat100.update(134e9);
        stat100.update(14e9);
        stat100.update(123e9);
        stat100.update(-123e9);
        stat100.update(4123e9);
    }

    CPPUNIT_ASSERT(fabs(stat100.getRollingWindowMean() - 449400000000.0) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat100.getRollingWindowVariance() - 1.51985e+24) < EPSILON_VAR);

    karabo::util::RollingWindowStatistics stat1000(1000);
    for (size_t i = 0; i < 101; i++) {
        stat1000.update(123e9);
        stat1000.update(23e9);
        stat1000.update(33e9);
        stat1000.update(43e9);
        stat1000.update(1e9);
        stat1000.update(134e9);
        stat1000.update(14e9);
        stat1000.update(123e9);
        stat1000.update(-123e9);
        stat1000.update(4123e9);
    }

    CPPUNIT_ASSERT(fabs(stat1000.getRollingWindowMean() - 449400000000.0) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat1000.getRollingWindowVariance() - 1.50616e+24) < EPSILON_VAR);
}


void StatisticalEvaluator::testVariance() {
    double EPSILON = 0.0001;
    karabo::util::RollingWindowStatistics stat(5);
    stat.update(5);
    CPPUNIT_ASSERT(std::isnan(stat.getRollingWindowVariance()));
    stat.update(0);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 12.5) < EPSILON);
    stat.update(-5);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 25) < EPSILON);
    stat.update(2.5);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 18.2292) < EPSILON);
    stat.update(2.5);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 14.375) < EPSILON);
    stat.update(4);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 12.575) < EPSILON);
}


void StatisticalEvaluator::testUpdateMeanTriggering() {
    double EPSILON = 0.0001;
    TestRollingWindowStatisticsFriend stat(10);
    stat.update(100);
    stat.update(101);
    stat.update(100);
    stat.update(101);
    double currentMeanEstimate = stat.getMeanEstimate();
    CPPUNIT_ASSERT(std::abs(currentMeanEstimate - 100) < EPSILON);
    CPPUNIT_ASSERT(std::abs(stat.getRollingWindowMean() - 100.5) < EPSILON);
    CPPUNIT_ASSERT(std::abs(stat.getRollingWindowVariance() - 1. / 3) < EPSILON);
    stat.update(-100);
    stat.update(-101);
    stat.update(-100);
    stat.update(-101);
    currentMeanEstimate = stat.getMeanEstimate();
    CPPUNIT_ASSERT(std::abs(currentMeanEstimate - 60.399999) < EPSILON);
    CPPUNIT_ASSERT(std::abs(stat.getRollingWindowMean() - 0) < EPSILON);
    CPPUNIT_ASSERT(std::abs(stat.getRollingWindowVariance() - 80804. / 7) < EPSILON);
}


void StatisticalEvaluator::testPerformance() {
    karabo::util::TimeProfiler profiler("TestProfiler");
    profiler.open();

    karabo::util::RollingWindowStatistics stat1000(1000);
    profiler.startPeriod("varianceSingle");

    for (size_t i = 0; i < 10000; i++) {
        stat1000.update(123e9);
        stat1000.getRollingWindowVariance();
        stat1000.update(23e9);
        stat1000.getRollingWindowVariance();
        stat1000.update(33e9);
        stat1000.getRollingWindowVariance();
        stat1000.update(43e9);
        stat1000.getRollingWindowVariance();
        stat1000.update(1e9);
        stat1000.getRollingWindowVariance();
        stat1000.update(134e9);
        stat1000.getRollingWindowVariance();
        stat1000.update(14e9);
        stat1000.getRollingWindowVariance();
        stat1000.update(123e9);
        stat1000.getRollingWindowVariance();
        stat1000.update(-123e9);
        stat1000.getRollingWindowVariance();
        stat1000.update(4123e9);
        stat1000.getRollingWindowVariance();
    }

    profiler.stopPeriod("varianceSingle");

    profiler.close();

    KARABO_LOG_FRAMEWORK_DEBUG << "Single var time (100000 updates and reads): "
                               << profiler.getPeriod("varianceSingle").getDuration() << " [s]";
}


void StatisticalEvaluator::testValidatorPerformance() {
    using namespace karabo::util;

    TimeProfiler profiler("TestProfiler");
    profiler.open();

    Validator val;
    Schema schema;

    for (int i = 0; i < 50; i++) {
        std::ostringstream key_s;
        key_s << i;
        INT8_ELEMENT(schema)
              .key("i8_" + key_s.str())
              .readOnly()
              .initialValue(0)
              .enableRollingStats()
              .warnVarianceLow(0)
              .needsAcknowledging(true)
              .warnVarianceHigh(255)
              .needsAcknowledging(true)
              .evaluationInterval(100)
              .commit();
        UINT16_ELEMENT(schema)
              .key("ui16_" + key_s.str())
              .readOnly()
              .initialValue(0)
              .enableRollingStats()
              .warnVarianceLow(0)
              .needsAcknowledging(true)
              .warnVarianceHigh(255)
              .info("Test")
              .needsAcknowledging(true)
              .evaluationInterval(1000)
              .commit();
        FLOAT_ELEMENT(schema)
              .key("f_" + key_s.str())
              .readOnly()
              .initialValue(0)
              .enableRollingStats()
              .warnVarianceLow(0)
              .needsAcknowledging(true)
              .warnVarianceHigh(255)
              .needsAcknowledging(true)
              .evaluationInterval(10)
              .commit();
        DOUBLE_ELEMENT(schema)
              .key("d_" + key_s.str())
              .readOnly()
              .initialValue(0)
              .enableRollingStats()
              .warnVarianceLow(0)
              .needsAcknowledging(true)
              .warnVarianceHigh(255)
              .needsAcknowledging(true)
              .evaluationInterval(1000)
              .commit();
        UINT64_ELEMENT(schema)
              .key("ui64_" + key_s.str())
              .readOnly()
              .initialValue(0)
              .enableRollingStats()
              .warnVarianceLow(0)
              .needsAcknowledging(true)
              .warnVarianceHigh(255)
              .needsAcknowledging(true)
              .evaluationInterval(100)
              .commit();
    }

    profiler.startPeriod("varianceValidator");

    Hash h_out;

    for (size_t t = 0; t < 10; t++) {
        for (int i = 0; i < 50; i++) {
            std::ostringstream key_s;
            key_s << i;
            Hash h;
            h.set("i8_" + key_s.str(), 1);
            h.set("ui16_" + key_s.str(), 1);
            h.set("f_" + key_s.str(), 1);
            h.set("d_" + key_s.str(), 1);
            h.set("ui64_" + key_s.str(), 1);
            std::pair<bool, std::string> r = val.validate(schema, h, h_out);
        }
    }
    profiler.stopPeriod("varianceValidator");

    profiler.close();

    KARABO_LOG_FRAMEWORK_DEBUG << "Validation time 250 properties: "
                               << profiler.getPeriod("varianceValidator").getDuration() / 10 << " [s/per validation]";
}
