/* 
 * File:   TimeClasses_Test.cc
 * Author: boukhele
 * 
 * Created on July 10, 2013, 2:35 PM
 */

#include <iostream>
#include <iomanip>

#include <karabo/util/Epochstamp.hh>
#include <karabo/util/TimePeriod.hh>
#include <karabo/util/TimeDuration.hh>
#include <karabo/util/TimeProfiler.hh>

#include "TimeClasses_Test.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(TimeClasses_Test);

using namespace std;
using namespace karabo;
using namespace karabo::util;

TimeClasses_Test::TimeClasses_Test() {
}

TimeClasses_Test::~TimeClasses_Test() {
}

void TimeClasses_Test::setUp() {
}

void TimeClasses_Test::tearDown() {
}

void TimeClasses_Test::testEpochstamp() {
    Epochstamp t1;

    sleep(2);

    Epochstamp t2;
    TimeDuration::setDefaultFormat("%s.%U");
    TimeDuration d = t2 - t1;

    clog << "Duration: " << t2 - t1 << endl;

    Epochstamp t3;
    t2 += TimeDuration(0ULL, 1000000000000000ULL);

    clog << "Duration: " << t2 - t1 << endl;

    Epochstamp t4 = t3 + d;

    clog << "Duration: " << t4 - t3 << endl;

    clog << "Duration: " << t4 - t1 << endl;
    clog << "Duration: " << t4.elpased(t1) << endl;

    t4 -= TimeDuration(0ULL, 2000000000000000ULL);

    clog << "Duration: " << t4.elpased(t1) << endl;

    CPPUNIT_ASSERT(true);
}

void TimeClasses_Test::testTimePeriod() {
    Epochstamp t0;
    TimePeriod p1;
    p1.start();
    Epochstamp t1;
    sleep(2);

    Epochstamp t2;
    p1.stop();
    Epochstamp t3;

    TimePeriod p2(t1, t2);

    clog << "Duration: " << p1.getDuration() << endl;
    clog << "Duration: " << p2.getDuration() << endl;

    CPPUNIT_ASSERT(p1.after(t0));
    CPPUNIT_ASSERT(p1.contain(t1));
    CPPUNIT_ASSERT(p1.contain(t2));
    CPPUNIT_ASSERT(p1.before(t3));

    CPPUNIT_ASSERT(true);
}

void TimeClasses_Test::testTimeDuration() {


    CPPUNIT_ASSERT(true);
}

void TimeClasses_Test::testTimeProfiler() {
    TimeProfiler profiler("Test");
    profiler.open();

    profiler.startPeriod("write");
    usleep(100000);
    profiler.startPeriod("format");
    usleep(100000);
    profiler.startPeriod();
    usleep(100000);
    profiler.startPeriod("open");
    usleep(100000);
    profiler.startPeriod();
    usleep(100000);
    profiler.stopPeriod();
    usleep(100000);
    profiler.startPeriod("flush");
    usleep(100000);
    profiler.stopPeriod();
    usleep(100000);
    profiler.startPeriod();
    usleep(100000);
    profiler.stopPeriod();
    usleep(100000);
    profiler.stopPeriod();
    usleep(100000);
    profiler.startPeriod();
    usleep(100000);
    profiler.stopPeriod();
    usleep(100000);
    profiler.stopPeriod();
    usleep(100000);
    profiler.startPeriod("close");
    usleep(100000);
    profiler.stopPeriod();
    usleep(100000);
    profiler.startPeriod();
    usleep(100000);
    profiler.stopPeriod();
    usleep(100000);
    profiler.startPeriod();
    usleep(100000);
    profiler.stopPeriod();
    usleep(100000);
    profiler.stopPeriod();
    usleep(100000);
    profiler.stopPeriod();
    
    profiler.close();

    clog << "Profiler:\n" << profiler << endl;

    //clog << "Profiler:\n" << profiler.sql() << endl;
    
    clog << "Profiler: " << TimePeriod(profiler.getPeriod()).getDuration() << endl;

    clog << "Profiler write: " << TimePeriod(profiler.getPeriod("write")).getDuration() << endl;

    clog << "Profiler write.format: " << TimePeriod(profiler.getPeriod("write.format")).getDuration() << endl;

    clog << "Profiler write.format.open: " << TimePeriod(profiler.getPeriod("write.format.open")).getDuration() << endl;

    CPPUNIT_ASSERT(true);
}

