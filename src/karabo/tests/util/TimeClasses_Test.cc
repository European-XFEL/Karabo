/* 
 * File:   TimeClasses_Test.cc
 * Author: boukhelef
 * 
 * Created on July 10, 2013, 2:35 PM
 */

#include <iostream>
#include <iomanip>
#include <cmath>

#include <karabo/util/Epochstamp.hh>
#include <karabo/util/TimePeriod.hh>
#include <karabo/util/TimeDuration.hh>
#include <karabo/util/TimeProfiler.hh>
#include <karabo/util/Hash.hh>

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

    //    clog << "Duration: " << t2 - t1 << endl;

    Epochstamp t3;
    t2 += TimeDuration(0ULL, 1000000000000000ULL);

    //    clog << "Duration: " << t2 - t1 << endl;

    Epochstamp t4 = t3 + d;

    //    clog << "Duration: " << t4 - t3 << endl;

    //    clog << "Duration: " << t4 - t1 << endl;
    //    clog << "Duration: " << t4.elapsed(t1) << endl;

    t4 -= TimeDuration(0ULL, 2000000000000000ULL);

    //    clog << "Duration: " << t4.elapsed(t1) << endl;

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

    //    clog << "Duration: " << p1.getDuration() << endl;
    //    clog << "Duration: " << p2.getDuration() << endl;

    CPPUNIT_ASSERT(p1.after(t0));
    CPPUNIT_ASSERT(p1.contain(t1));
    CPPUNIT_ASSERT(p1.contain(t2));
    CPPUNIT_ASSERT(p1.before(t3));

    CPPUNIT_ASSERT(true);
}


void TimeClasses_Test::testTimeDuration() {

    const TimeDuration durZero;
    CPPUNIT_ASSERT(durZero.getSeconds() == 0ull);
    CPPUNIT_ASSERT(durZero.getFractions() == 0ull);

    const TimeValue seconds = 3600ull; // one hour
    const TimeValue fractionsAtto = 4565460000000ull; // 456.546 micro seconds

    const TimeDuration dur1(seconds, fractionsAtto);
    CPPUNIT_ASSERT(dur1.getSeconds() == 0ull);
    CPPUNIT_ASSERT(dur1.getTotalSeconds() == seconds);
    CPPUNIT_ASSERT(dur1.getMinutes() == 0ull);
    CPPUNIT_ASSERT(dur1.getTotalMinutes() == 60ull);
    CPPUNIT_ASSERT(dur1.getHours() == 1ull);
    CPPUNIT_ASSERT(dur1.getTotalHours() == 1ull);
    CPPUNIT_ASSERT(dur1.getFractions(util::ATTOSEC)  == fractionsAtto);
    CPPUNIT_ASSERT(dur1.getFractions(util::FEMTOSEC) == fractionsAtto / 1000ull);
    CPPUNIT_ASSERT(dur1.getFractions(util::PICOSEC)  == fractionsAtto / 1000000ull);
    CPPUNIT_ASSERT(dur1.getFractions(util::NANOSEC)  == fractionsAtto / 1000000000ull);
    CPPUNIT_ASSERT(dur1.getFractions(util::MICROSEC) == fractionsAtto / 1000000000000ull);
    CPPUNIT_ASSERT(dur1.getFractions(util::MILLISEC) == fractionsAtto / 1000000000000000ull);

    const util::Hash hash("seconds", seconds, "fractions", fractionsAtto);
    const TimeDuration dur2(hash);
    CPPUNIT_ASSERT(dur1 - dur2 == durZero);

    // days, hours, minutes (all as int), seconds, fractions as TimeValue
    const TimeDuration dur3(1, 3, 4, 56ull, 123456789012345678ull); // 123.456789... ms
    CPPUNIT_ASSERT(dur3.getDays() == 1ull);
    CPPUNIT_ASSERT(dur3.getHours() == 3ull);
    CPPUNIT_ASSERT(dur3.getTotalHours() == 27ull);
    CPPUNIT_ASSERT(dur3.getMinutes() == 4ull);
    CPPUNIT_ASSERT(dur3.getTotalMinutes() == 1624ull);
    CPPUNIT_ASSERT(dur3.getSeconds() == 56ull);
    CPPUNIT_ASSERT(dur3.getTotalSeconds() == 97496ull);
    CPPUNIT_ASSERT(dur3.getFractions(util::MILLISEC) == 123ull);
    CPPUNIT_ASSERT(dur3.getFractions(util::NANOSEC) == 123456789ull);
    CPPUNIT_ASSERT(dur3.getFractions(util::ATTOSEC) == 123456789012345678ull);

    // Test equal comparisons
    const TimeDuration durA(123ull, 4567890000ull);
    const TimeDuration durB(123ull, 4567890000ull);
    CPPUNIT_ASSERT(durA == durB);
    CPPUNIT_ASSERT(durA <= durB);
    CPPUNIT_ASSERT(durA >= durB);
    CPPUNIT_ASSERT((durA != durB) == false);

    // Test larger/smaller comparisons with equal seconds
    const TimeDuration durC(123ull, 4567890000ull);
    const TimeDuration durD(123ull, 4567890001ull);
    CPPUNIT_ASSERT(durC != durD);
    CPPUNIT_ASSERT(durC <  durD);
    CPPUNIT_ASSERT(durC <= durD);
    CPPUNIT_ASSERT(durD >  durC);
    CPPUNIT_ASSERT(durD >= durC);
    CPPUNIT_ASSERT((durD <  durC) == false);
    CPPUNIT_ASSERT((durD <= durC) == false);
    CPPUNIT_ASSERT((durC >  durD) == false);
    CPPUNIT_ASSERT((durC >= durD) == false);

    // Test larger/smaller comparisons with equal fractions
    const TimeDuration durE(3ull, 4567890000ull);
    const TimeDuration durF(4ull, 4567890000ull);
    CPPUNIT_ASSERT(durE != durF);
    CPPUNIT_ASSERT(durE <  durF);
    CPPUNIT_ASSERT(durE <= durF);
    CPPUNIT_ASSERT(durF >  durE);
    CPPUNIT_ASSERT(durF >= durE);
    CPPUNIT_ASSERT((durF <  durE) == false);
    CPPUNIT_ASSERT((durF <= durE) == false);
    CPPUNIT_ASSERT((durE >  durF) == false);
    CPPUNIT_ASSERT((durE >= durF) == false);

    // Test larger/smaller comparisons with seconds smaller, fractions larger
    const TimeDuration durG(444ull, 4567890000ull);
    const TimeDuration durH(555ull, 1234560000ull);
    CPPUNIT_ASSERT(durG != durH);
    CPPUNIT_ASSERT(durG <  durH);
    CPPUNIT_ASSERT(durG <= durH);
    CPPUNIT_ASSERT(durH >  durG);
    CPPUNIT_ASSERT(durH >= durG);
    CPPUNIT_ASSERT((durH <  durG) == false);
    CPPUNIT_ASSERT((durH <= durG) == false);
    CPPUNIT_ASSERT((durG >  durH) == false);
    CPPUNIT_ASSERT((durG >= durH) == false);

    // Testing operator+/- (operator +=/-= implicitly tested since used inside operator +/-))
    // 1) without 'crossing' seconds border
    const TimeDuration durI(222ull, 4567890000ull);
    const TimeDuration durJ(111ull, 1234560000ull);
    CPPUNIT_ASSERT(durI + durJ == TimeDuration(333ull, 5802450000ull));
    CPPUNIT_ASSERT(durI - durJ == TimeDuration(111ull, 3333330000ull));
    // 2) with 'crossing' seconds border
    const unsigned long long oneSec = 1000000000000000000ULL; // with 18 zeros
    const TimeDuration durK(111ull, oneSec - 100ull);
    CPPUNIT_ASSERT(durI + durK == TimeDuration(334ull, 4567889900ull));
    CPPUNIT_ASSERT(durI - durK == TimeDuration(110ull, 4567890100ull));

    // Testing operator/
    const TimeDuration durL(222ull, 222222222222222ull);
    const TimeDuration durM(444ull, 444444444444444ull);
    CPPUNIT_ASSERT(std::abs(durM / durL) - 2.L < 1.e-18L);
}


void TimeClasses_Test::testTimeProfiler() {
    TimeProfiler profiler("TestProfiler");
    profiler.open();

    profiler.startPeriod("write");
    {
        usleep(500000);
        profiler.startPeriod("read");
        {
            usleep(500000);
            profiler.startPeriod();
            {
                usleep(500000);
            }
            profiler.stopPeriod();

            profiler.startPeriod();
            {
                usleep(500000);
            }
            profiler.stopPeriod();

        }
        profiler.stopPeriod("read");
    }
    profiler.stopPeriod("write");

    //clog << p << endl;

    profiler.close();

    clog << "Write time: " << profiler.getPeriod("write").getDuration() << " [s]" << endl;
    clog << "Read time : " << profiler.getPeriod("write.read").getDuration() << " [s]" << endl;

    //TimeProfiler profiler("Test");
    profiler.open();

    profiler.startPeriod("write");
    {
        usleep(100000);
        profiler.startPeriod("format");
        {
            usleep(100000);
            profiler.startPeriod();
            {
                usleep(100000);
                profiler.startPeriod("open");
                {
                    usleep(100000);
                    profiler.startPeriod();
                    {
                        usleep(100000);
                    }
                    profiler.stopPeriod();
                    usleep(100000);
                    profiler.startPeriod("flush");
                    {
                        usleep(100000);
                        profiler.startPeriod();
                        {
                            usleep(100000);
                        }
                        profiler.stopPeriod();
                        usleep(100000);
                    }
                    profiler.stopPeriod("flush");
                    usleep(100000);
                }
                profiler.stopPeriod();
                usleep(100000);
                profiler.startPeriod();
                {
                    usleep(100000);
                }
                profiler.stopPeriod();
                usleep(100000);
            }
            profiler.stopPeriod();
            usleep(100000);
            profiler.startPeriod("close");
            {
                usleep(100000);
            }
            profiler.stopPeriod("close");
            usleep(100000);
            profiler.startPeriod();
            {
                usleep(100000);
            }
            profiler.stopPeriod();
            usleep(100000);
            profiler.startPeriod();
            {
                usleep(100000);
            }
            profiler.stopPeriod();
            usleep(100000);
        }
        profiler.stopPeriod("format");
        usleep(100000);
    }
    profiler.stopPeriod();

    profiler.close();

    clog << "Profiler:\n" << profiler << endl;

    //clog << "Profiler:\n" << profiler.sql() << endl;

    //    clog << "Profiler: " << profiler.getPeriod().getDuration() << endl;
    //
    //    clog << "Profiler write: " << profiler.getPeriod("write").getDuration() << endl;
    //
    //    clog << "Profiler write.format: " << profiler.getPeriod("write.format").getDuration() << endl;
    //
    //    clog << "Profiler write.format.open: " << profiler.getPeriod("write.format.open").getDuration() << endl;

    CPPUNIT_ASSERT(true);
}

