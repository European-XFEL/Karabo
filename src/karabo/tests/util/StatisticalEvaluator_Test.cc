/*
 * File:   StatisticalEvaluator.cc
 * Author: haufs
 *
 * Created on Jun 16, 2016, 9:10:01 AM
 */

#include "StatisticalEvaluator_Test.hh"
#include <math.h>
#include <karabo/util/TimePeriod.hh>
#include <karabo/util/TimeDuration.hh>
#include <karabo/util/TimeProfiler.hh>
#include <karabo/util/Validator.hh>
#include <karabo/util/Schema.hh>
#include <karabo/util/SimpleElement.hh>


#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>




CPPUNIT_TEST_SUITE_REGISTRATION(StatisticalEvaluator);

StatisticalEvaluator::StatisticalEvaluator() {
}

StatisticalEvaluator::~StatisticalEvaluator() {
}

void StatisticalEvaluator::setUp() {
    
}

void StatisticalEvaluator::tearDown() {
}

void StatisticalEvaluator::testMean() {
    #define EPSILON 0.0001
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
    #define EPSILON_MEAN 1e-10
    #define EPSILON_VAR 1e-13
    #define UPDATE_W_MAG(prefix, stat) stat.update(prefix * 1e-9d );
    karabo::util::RollingWindowStatistics stat(10);
    UPDATE_W_MAG(123,  stat)
    UPDATE_W_MAG(23,  stat)
    UPDATE_W_MAG(33,  stat)
    UPDATE_W_MAG(43,  stat)
    UPDATE_W_MAG(1,  stat)
    UPDATE_W_MAG(134,  stat)
    UPDATE_W_MAG(14,  stat)
    UPDATE_W_MAG(123,  stat)
    UPDATE_W_MAG(-123,  stat)
    UPDATE_W_MAG(4123,  stat)
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowMean() - 4.494e-7d) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 1.67183e-12d) < EPSILON_VAR);
    UPDATE_W_MAG(123,  stat)
    UPDATE_W_MAG(23,  stat)
    UPDATE_W_MAG(33,  stat)
    UPDATE_W_MAG(43,  stat)
    UPDATE_W_MAG(1,  stat)
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowMean() - 4.494e-7d) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 1.67183e-12d) < EPSILON_VAR);
    
    karabo::util::RollingWindowStatistics stat100(100);
    for(size_t i = 0; i< 11; i++){
        UPDATE_W_MAG(123,  stat100)
        UPDATE_W_MAG(23,  stat100)
        UPDATE_W_MAG(33,  stat100)
        UPDATE_W_MAG(43,  stat100)
        UPDATE_W_MAG(1,  stat100)
        UPDATE_W_MAG(134,  stat100)
        UPDATE_W_MAG(14,  stat100)
        UPDATE_W_MAG(123,  stat100)
        UPDATE_W_MAG(-123,  stat100)
        UPDATE_W_MAG(4123,  stat100)
    }
    
    CPPUNIT_ASSERT(fabs(stat100.getRollingWindowMean() - 4.494e-07d) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat100.getRollingWindowVariance() - 1.50465324e-12d) < EPSILON_VAR);
    
    karabo::util::RollingWindowStatistics stat1000(1000);
    for(size_t i = 0; i< 101; i++){
        UPDATE_W_MAG(123,  stat1000)
        UPDATE_W_MAG(23,  stat1000)
        UPDATE_W_MAG(33,  stat1000)
        UPDATE_W_MAG(43,  stat1000)
        UPDATE_W_MAG(1,  stat1000)
        UPDATE_W_MAG(134,  stat1000)
        UPDATE_W_MAG(14,  stat1000)
        UPDATE_W_MAG(123,  stat1000)
        UPDATE_W_MAG(-123,  stat1000)
        UPDATE_W_MAG(4123,  stat1000)
    }
    
    CPPUNIT_ASSERT(fabs(stat1000.getRollingWindowMean() - 4.494e-07d) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat1000.getRollingWindowVariance() - 1.50465324e-12) < EPSILON_VAR);
    
    
}

void StatisticalEvaluator::testLargeNumbers() {
    #define EPSILON_MEAN 1e5
    #define EPSILON_VAR 1e20
    #define UPDATE_W_MAG(prefix, stat) stat.update(prefix * 1e9d );
    karabo::util::RollingWindowStatistics stat(10);
    UPDATE_W_MAG(123.0d,  stat)
    UPDATE_W_MAG(23.0d,  stat)
    UPDATE_W_MAG(33.0d,  stat)
    UPDATE_W_MAG(43.0d,  stat)
    UPDATE_W_MAG(1.0d,  stat)
    UPDATE_W_MAG(134.0d,  stat)
    UPDATE_W_MAG(14.0d,  stat)
    UPDATE_W_MAG(123.0d,  stat)
    UPDATE_W_MAG(-123.0d,  stat)
    UPDATE_W_MAG(4123.0d,  stat)
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowMean() - 449400000000.0) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 1.6718369e+24d) < EPSILON_VAR);
    UPDATE_W_MAG(123.0d,  stat)
    UPDATE_W_MAG(23.0d,  stat)
    UPDATE_W_MAG(33.0d,  stat)
    UPDATE_W_MAG(43.0d,  stat)
    UPDATE_W_MAG(1.0d,  stat)
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowMean() - 449400000000.0) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 1.6718369e+24d) < EPSILON_VAR);
    
    karabo::util::RollingWindowStatistics stat100(100);
    for(size_t i = 0; i< 11; i++){
        UPDATE_W_MAG(123.0d,  stat100)
        UPDATE_W_MAG(23.0d,  stat100)
        UPDATE_W_MAG(33.0d,  stat100)
        UPDATE_W_MAG(43.0d,  stat100)
        UPDATE_W_MAG(1.0d,  stat100)
        UPDATE_W_MAG(134.0d,  stat100)
        UPDATE_W_MAG(14.0d,  stat100)
        UPDATE_W_MAG(123.0d,  stat100)
        UPDATE_W_MAG(-123.0d,  stat100)
        UPDATE_W_MAG(4123.0d,  stat100)
    }
    
    CPPUNIT_ASSERT(fabs(stat100.getRollingWindowMean() - 449400000000.0) < EPSILON_MEAN);
  
    CPPUNIT_ASSERT(fabs(stat100.getRollingWindowVariance() - 1.51985e+24d) < EPSILON_VAR);
    
    karabo::util::RollingWindowStatistics stat1000(1000);
    for(size_t i = 0; i< 101; i++){
        UPDATE_W_MAG(123.0d,  stat1000)
        UPDATE_W_MAG(23.0d,  stat1000)
        UPDATE_W_MAG(33.0d,  stat1000)
        UPDATE_W_MAG(43.0d,  stat1000)
        UPDATE_W_MAG(1.0d,  stat1000)
        UPDATE_W_MAG(134.0d,  stat1000)
        UPDATE_W_MAG(14.0d,  stat1000)
        UPDATE_W_MAG(123.0d,  stat1000)
        UPDATE_W_MAG(-123.0d,  stat1000)
        UPDATE_W_MAG(4123.0d,  stat1000)
    }
    
    CPPUNIT_ASSERT(fabs(stat1000.getRollingWindowMean() - 449400000000.0) < EPSILON_MEAN);
    CPPUNIT_ASSERT(fabs(stat1000.getRollingWindowVariance() - 1.50616e+24) < EPSILON_VAR);
    
    
}

void StatisticalEvaluator::testVariance() {
    #define EPSILON 0.0001
    karabo::util::RollingWindowStatistics stat(5);
    stat.update(5);
    CPPUNIT_ASSERT(isnan(stat.getRollingWindowVariance()));
    stat.update(0);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 12.5) < EPSILON);
    stat.update(-5);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 25)  < EPSILON);
    stat.update(2.5);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 18.2292d) < EPSILON);
    stat.update(2.5);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 14.375d) < EPSILON);
    stat.update(4);
    CPPUNIT_ASSERT(fabs(stat.getRollingWindowVariance() - 12.575d) < EPSILON);
}

void StatisticalEvaluator::testPerformance() {
    karabo::util::TimeProfiler profiler("TestProfiler");
    profiler.open();

    karabo::util::RollingWindowStatistics stat1000(1000);
    profiler.startPeriod("varianceSingle");
    
    double var;
    #define UPDATE_W_MAG(prefix, stat) stat.update(prefix * 1e9d ); var = stat.getRollingWindowVariance();
    
    {
       
        for(size_t i = 0; i< 10000; i++){
            UPDATE_W_MAG(123.0d,  stat1000)
            UPDATE_W_MAG(23.0d,  stat1000)
            UPDATE_W_MAG(33.0d,  stat1000)
            UPDATE_W_MAG(43.0d,  stat1000)
            UPDATE_W_MAG(1.0d,  stat1000)
            UPDATE_W_MAG(134.0d,  stat1000)
            UPDATE_W_MAG(14.0d,  stat1000)
            UPDATE_W_MAG(123.0d,  stat1000)
            UPDATE_W_MAG(-123.0d,  stat1000)
            UPDATE_W_MAG(4123.0d,  stat1000)
        }
    }
    profiler.stopPeriod("varianceSingle");

    //clog << p << endl;

    profiler.close();

    std::clog << "Single var time (10000 updates and reads): " << profiler.getPeriod("varianceSingle").getDuration() << " [s]" << std::endl;

    
}


void StatisticalEvaluator::testValidatorPerformance() {
    
    using namespace karabo::util;

    TimeProfiler profiler("TestProfiler");
    profiler.open();

    
    Validator val;
    Schema schema;
    
    for(int i = 0; i < 50; i++){
        std::ostringstream  key_s;
        key_s<<i;
        INT8_ELEMENT(schema).key("i8_"+key_s.str())
                .readOnly().initialValue(0)
                .enableRollingStats().warnVarianceLow(0).warnVarianceHigh(255).evaluationInterval(100)
                .commit();
        UINT16_ELEMENT(schema).key("ui16_"+key_s.str())
                .readOnly().initialValue(0)
                .enableRollingStats().warnVarianceLow(0).warnVarianceHigh(255).evaluationInterval(1000)
                .commit();
        FLOAT_ELEMENT(schema).key("f_"+key_s.str())
                .readOnly().initialValue(0)
                .enableRollingStats().warnVarianceLow(0).warnVarianceHigh(255).evaluationInterval(10)
                .commit();
        DOUBLE_ELEMENT(schema).key("d_"+key_s.str())
                .readOnly().initialValue(0)
                .enableRollingStats().warnVarianceLow(0).warnVarianceHigh(255).evaluationInterval(1000)
                .commit();
        UINT64_ELEMENT(schema).key("ui64_"+key_s.str())
                .readOnly().initialValue(0)
                .enableRollingStats().warnVarianceLow(0).warnVarianceHigh(255).evaluationInterval(100)
                .commit();
    }
    
    
    
    profiler.startPeriod("varianceValidator");
    
    Hash h_out;
   
    
    
       
    for(size_t t = 0; t< 100; t++){
        for(int i = 0; i < 50; i++){
            std::ostringstream  key_s;
            key_s<<i;
            Hash h;
            h.set("i8_"+key_s.str(), 1);
            h.set("ui16_"+key_s.str(), 1);
            h.set("f_"+key_s.str(), 1);
            h.set("d_"+key_s.str(), 1);
            h.set("ui64_"+key_s.str(), 1);
            std::pair<bool, std::string> r = val.validate(schema, h, h_out);
            
        }
    }
    profiler.stopPeriod("varianceValidator");

    //clog << p << endl;

    profiler.close();

    std::clog << "Validation time 250 properties: " << profiler.getPeriod("varianceValidator").getDuration()/100 << " [s/per validation]" << std::endl;

    
}
