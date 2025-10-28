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
 * File:   TimeClasses_Test.hh
 * Author: boukhele
 *
 * Created on July 10, 2013, 2:35 PM
 */

#include <gtest/gtest.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <karabo/log/Logger.hh>
#include <karabo/util/TimeProfiler.hh>
#include <limits>

#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/time/TimeDuration.hh"
#include "karabo/data/time/TimeId.hh"
#include "karabo/data/time/TimePeriod.hh"
#include "karabo/data/time/Timestamp.hh"
#include "karabo/data/types/Hash.hh"

using namespace std;
using namespace karabo;
using namespace karabo::util;
using namespace karabo::data;

#if __GNUC__ < 13

#include <date/date.h>
using hours = std::chrono::duration<long int, std::ratio<3600>>;
using minutes = std::chrono::duration<long int, std::ratio<60>>;
using seconds = std::chrono::duration<long int>;
using nanoseconds = std::chrono::duration<long int, std::nano>;

#else

#include <chrono>

#endif

using namespace std::literals::chrono_literals;


TEST(TestTimeClasses, testEpochstamp) {
    Epochstamp t1;

    sleep(2);

    Epochstamp t2;
    TimeDuration::setDefaultFormat("%s.%U");
    TimeDuration d = t2 - t1;

    //    KARABO_LOG_FRAMEWORK_DEBUG << "Duration: " << t2 - t1;

    Epochstamp t3;
    t2 += TimeDuration(0ULL, 1'000'000'000'000'000ULL);

    //    KARABO_LOG_FRAMEWORK_DEBUG << "Duration: " << t2 - t1;

    Epochstamp t4 = t3 + d;

    //    KARABO_LOG_FRAMEWORK_DEBUG << "Duration: " << t4 - t3;

    //    KARABO_LOG_FRAMEWORK_DEBUG << "Duration: " << t4 - t1;
    //    KARABO_LOG_FRAMEWORK_DEBUG << "Duration: " << t4.elapsed(t1);

    t4 -= TimeDuration(0ULL, 2'000'000'000'000'000ULL);

    //    KARABO_LOG_FRAMEWORK_DEBUG << "Duration: " << t4.elapsed(t1);

    EXPECT_TRUE(true);
}


TEST(TestTimeClasses, testEpochstampConversion) {
#if __GNUC__ < 13
    using namespace date;
#else
    using namespace std::chrono;
#endif
    // to std::chrono::time_point...
    const Epochstamp stamp(3600ull * 24ull * (365ull + 30ull) // 31.1.1971 0.00 h
                                 + 3ull * 3600ull             // => 3.00 h
                                 + 125ull,                    // => 3.02:05 h
                           123456ull * 1000000000ull);        // 123456 nanosec
    const auto asPtime = stamp.getPtime();
    auto dp = floor<days>(asPtime);
    const year_month_day ymd{dp};

#if __GNUC__ < 13
    // 'date' library
    EXPECT_EQ(1971_y, ymd.year());
    EXPECT_EQ(month(1), ymd.month());
    EXPECT_EQ(31_d, ymd.day());

    const auto time = (asPtime - dp);
    auto hh = floor<hours>(time);
    EXPECT_EQ(3L, hh.count());
    auto mm = floor<minutes>(time - hh);
    EXPECT_EQ(2L, mm.count());
    auto ss = floor<seconds>(time - hh - mm);
    EXPECT_EQ(5L, ss.count());
    auto nanosec = floor<nanoseconds>(time - hh - mm - ss);
    EXPECT_EQ(123456L, nanosec.count());
#else
    // 'chrono' library
    EXPECT_EQ(1971y, ymd.year());
    EXPECT_EQ(month(1), ymd.month());
    EXPECT_EQ(31d, ymd.day());

    const auto time = (asPtime - dp);
    auto hh = floor<hours>(time);
    EXPECT_EQ(3h, hh);
    auto mm = floor<minutes>(time - hh);
    EXPECT_EQ(2min, mm);
    auto ss = floor<seconds>(time - hh - mm);
    EXPECT_EQ(5s, ss);
    auto nanosec = floor<nanoseconds>(time - hh - mm - ss);
    EXPECT_EQ(123456ns, nanosec);
#endif

    // output to ostream
    std::ostringstream oss;
    oss << Epochstamp(12345ull, 12345678901234567ull); // full precision
    EXPECT_EQ(std::string("12345.012345678901234567 s"), oss.str());
    std::ostringstream oss2;
    oss2 << Epochstamp(12345ull, 12345678900000000ull); // trailing zeros, removed in output
    EXPECT_EQ(std::string("12345.0123456789 s"), oss2.str());
    std::ostringstream oss3;
    oss3 << Epochstamp(0ull, 12345678900000000ull); // less than a second and trailing zeros
    EXPECT_EQ(std::string("0.0123456789 s"), oss3.str());
}

TEST(TestTimeClasses, testTimePeriod) {
    Epochstamp t0;
    TimePeriod p1;
    p1.start();
    Epochstamp t1;
    sleep(2);

    Epochstamp t2;
    p1.stop();
    Epochstamp t3;

    TimePeriod p2(t1, t2);

    //    KARABO_LOG_FRAMEWORK_DEBUG << "Duration: " << p1.getDuration();
    //    KARABO_LOG_FRAMEWORK_DEBUG << "Duration: " << p2.getDuration();

    EXPECT_TRUE(p1.after(t0));
    EXPECT_TRUE(p1.contain(t1));
    EXPECT_TRUE(p1.contain(t2));
    EXPECT_TRUE(p1.before(t3));

    EXPECT_TRUE(true);
}


TEST(TestTimeClasses, testTimeDuration) {
    const TimeDuration durZero;
    EXPECT_TRUE(durZero.getSeconds() == 0ull);
    EXPECT_TRUE(durZero.getFractions() == 0ull);

    const TimeValue seconds = 3600ull;                 // one hour
    const TimeValue fractionsAtto = 456546'000'000ull; // 456.546 micro seconds

    const TimeDuration dur1(seconds, fractionsAtto);
    EXPECT_TRUE(dur1.getSeconds() == 0ull);
    EXPECT_TRUE(dur1.getTotalSeconds() == seconds);
    EXPECT_TRUE(dur1.getMinutes() == 0ull);
    EXPECT_TRUE(dur1.getTotalMinutes() == 60ull);
    EXPECT_TRUE(dur1.getHours() == 1ull);
    EXPECT_TRUE(dur1.getTotalHours() == 1ull);
    EXPECT_TRUE(dur1.getFractions(data::TIME_UNITS::ATTOSEC) == fractionsAtto);
    EXPECT_TRUE(dur1.getFractions(data::TIME_UNITS::FEMTOSEC) == fractionsAtto / 1000ull);
    EXPECT_TRUE(dur1.getFractions(data::TIME_UNITS::PICOSEC) == fractionsAtto / 1000000ull);
    EXPECT_TRUE(dur1.getFractions(data::TIME_UNITS::NANOSEC) == fractionsAtto / 1000000000ull);
    EXPECT_TRUE(dur1.getFractions(data::TIME_UNITS::MICROSEC) == fractionsAtto / 1000000000000ull);
    EXPECT_TRUE(dur1.getFractions(data::TIME_UNITS::MILLISEC) == fractionsAtto / 1000000000000000ull);

    const data::Hash hash("seconds", seconds, "fractions", fractionsAtto);
    const TimeDuration dur2(hash);
    EXPECT_TRUE(dur1 - dur2 == durZero);

    // days, hours, minutes (all as int), seconds, fractions as TimeValue
    const TimeDuration dur3(1, 3, 4, 56ull, 123456789012345678ull); // 123.456789... ms
    EXPECT_TRUE(dur3.getDays() == 1ull);
    EXPECT_TRUE(dur3.getHours() == 3ull);
    EXPECT_TRUE(dur3.getTotalHours() == 27ull);
    EXPECT_TRUE(dur3.getMinutes() == 4ull);
    EXPECT_TRUE(dur3.getTotalMinutes() == 1624ull);
    EXPECT_TRUE(dur3.getSeconds() == 56ull);
    EXPECT_TRUE(dur3.getTotalSeconds() == 97496ull);
    EXPECT_TRUE(dur3.getFractions(data::TIME_UNITS::MILLISEC) == 123ull);
    EXPECT_TRUE(dur3.getFractions(data::TIME_UNITS::NANOSEC) == 123456789ull);
    EXPECT_TRUE(dur3.getFractions(data::TIME_UNITS::ATTOSEC) == 123456789012345678ull);

    // Test equal comparisons
    const TimeDuration durA(123ull, 4567890000ull);
    const TimeDuration durB(123ull, 4567890000ull);
    EXPECT_TRUE(durA == durB);
    EXPECT_TRUE(durA <= durB);
    EXPECT_TRUE(durA >= durB);
    EXPECT_TRUE((durA != durB) == false);

    // Test larger/smaller comparisons with equal seconds
    const TimeDuration durC(123ull, 4567890000ull);
    const TimeDuration durD(123ull, 4567890001ull);
    EXPECT_TRUE(durC != durD);
    EXPECT_TRUE(durC < durD);
    EXPECT_TRUE(durC <= durD);
    EXPECT_TRUE(durD > durC);
    EXPECT_TRUE(durD >= durC);
    EXPECT_TRUE((durD < durC) == false);
    EXPECT_TRUE((durD <= durC) == false);
    EXPECT_TRUE((durC > durD) == false);
    EXPECT_TRUE((durC >= durD) == false);

    // Test larger/smaller comparisons with equal fractions
    const TimeDuration durE(3ull, 4567890000ull);
    const TimeDuration durF(4ull, 4567890000ull);
    EXPECT_TRUE(durE != durF);
    EXPECT_TRUE(durE < durF);
    EXPECT_TRUE(durE <= durF);
    EXPECT_TRUE(durF > durE);
    EXPECT_TRUE(durF >= durE);
    EXPECT_TRUE((durF < durE) == false);
    EXPECT_TRUE((durF <= durE) == false);
    EXPECT_TRUE((durE > durF) == false);
    EXPECT_TRUE((durE >= durF) == false);

    // Test larger/smaller comparisons with seconds smaller, fractions larger
    const TimeDuration durG(444ull, 4567890000ull);
    const TimeDuration durH(555ull, 1234560000ull);
    EXPECT_TRUE(durG != durH);
    EXPECT_TRUE(durG < durH);
    EXPECT_TRUE(durG <= durH);
    EXPECT_TRUE(durH > durG);
    EXPECT_TRUE(durH >= durG);
    EXPECT_TRUE((durH < durG) == false);
    EXPECT_TRUE((durH <= durG) == false);
    EXPECT_TRUE((durG > durH) == false);
    EXPECT_TRUE((durG >= durH) == false);

    // Testing operator+/- (operator +=/-= implicitly tested since used inside operator +/-))
    // 1) without 'crossing' seconds border
    const TimeDuration durI(222ull, 4567890000ull);
    const TimeDuration durJ(111ull, 1234560000ull);
    EXPECT_TRUE(durI + durJ == TimeDuration(333ull, 5802450000ull));
    EXPECT_TRUE(durI - durJ == TimeDuration(111ull, 3333330000ull));
    // 2) with 'crossing' seconds border
    const unsigned long long oneSec = 1000000000000000000ULL; // with 18 zeros
    const TimeDuration durK(111ull, oneSec - 100ull);
    EXPECT_TRUE(durI + durK == TimeDuration(334ull, 4567889900ull));
    EXPECT_TRUE(durI - durK == TimeDuration(110ull, 4567890100ull));
    // 3) with hitting the border
    const TimeDuration durQ(111ull, oneSec - 100ull);
    const TimeDuration hundredAttoDur(0ull, 100ull);
    const TimeDuration oneSecMinusHundredAttoDur(0ull, oneSec - 100ull);
    EXPECT_TRUE(durQ + hundredAttoDur == TimeDuration(112ull, 0ull));
    EXPECT_TRUE(durQ - oneSecMinusHundredAttoDur == TimeDuration(111ull, 0ull));

    // Testing operator* (operator *= implicitly tested since used inside operator*)
    // 1) without 'crossing' seconds border
    const TimeDuration durO(1ull, 123ull);
    EXPECT_TRUE(durO * 3ull == TimeDuration(3ull, 369ull));
    // 2) with 'crossing' seconds border
    const TimeDuration durP(1234ull, 400000000000000000ull); // 17 zeros: 0.4 s
    EXPECT_TRUE(durP * 7ull == TimeDuration(8640ull, 800000000000000000ull));
    // 3) with multiplication where factor * fractions is above largest ull
    //    (i.e. > 18.446 seconds)
    const TimeDuration durR(1ull, 900000000000000001ull); // 17 zeros: 0.9 s
    EXPECT_TRUE(durR * 9ull == TimeDuration(17ull, 100000000000000009ull));
    EXPECT_TRUE(durR * 100ull == TimeDuration(190ull, 100ull));
    EXPECT_TRUE(durR * 1000000ull == TimeDuration(1900000ull, 1000000ull));
    EXPECT_TRUE(durR * 100000000000000ull == TimeDuration(190000000000000ull, 100000000000000ull));

    // Testing operator/
    const TimeDuration durL(222ull, 222222222222222ull);
    const TimeDuration durM(444ull, 444444444444444ull);
    EXPECT_TRUE(std::abs(durM / durL) - 2.L < 1.e-18L);

    // Testing operator double()
    {
        const TimeDuration dur10(1ull, 45'000'000'000'000ull); // 1 second and 45 micro seconds
        EXPECT_NEAR(1.000045, static_cast<double>(dur10), 1.e-18);

        const TimeDuration dur11(1ull, 456'546'000'000ull); // 1 second and 456.546 nano seconds
        EXPECT_NEAR(1.000000456546, static_cast<double>(dur11), 1.e-18);

        const TimeDuration dur12(60ull, 0ull);
        EXPECT_NEAR(60., static_cast<double>(dur12), 1.e-18);

        const TimeDuration dur13(1, 1, 1, 10ull, 1'000'000'000'000'000); // 1 day, 1 hour, 1 minute, 10 seconds, 1 ms
        EXPECT_NEAR(((25 * 60) + 1) * 60ull + 10ull + 1.e-3, static_cast<double>(dur13), 1.e-12);

        // Can keep attosec precision if enough digits available in double
        const TimeDuration dur14(0ull, 1ull);
        EXPECT_NEAR(1.e-18, static_cast<double>(dur14), 1.e-30);

        // Loss of precision for double that has about 16 digits only
        const TimeDuration dur15(1ull, 1ull);
        EXPECT_NEAR(1., static_cast<double>(dur15), 1.e-18);

        // 16 digits precision can be reached
        const TimeDuration dur16(12345678ull, 12'345'670'000'000'000ull);
        EXPECT_NEAR(1.234567801234567e7, static_cast<double>(dur16), 1.e-8);
    }
}


TEST(TestTimeClasses, testTimeProfiler) {
    TimeProfiler profiler("TestProfiler");
    profiler.open();

    profiler.startPeriod("write");
    {
        usleep(500000);
        profiler.startPeriod("read");
        {
            usleep(500000);
            profiler.startPeriod();
            { usleep(500000); }
            profiler.stopPeriod();

            profiler.startPeriod();
            { usleep(500000); }
            profiler.stopPeriod();
        }
        profiler.stopPeriod("read");
    }
    profiler.stopPeriod("write");

    profiler.close();

    KARABO_LOG_FRAMEWORK_DEBUG_C("TestTimeClasses")
          << "Write time: " << profiler.getPeriod("write").getDuration() << " [s]";
    KARABO_LOG_FRAMEWORK_DEBUG_C("TestTimeClasses")
          << "Read time : " << profiler.getPeriod("write.read").getDuration() << " [s]";

    // TimeProfiler profiler("Test");
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
                    { usleep(100000); }
                    profiler.stopPeriod();
                    usleep(100000);
                    profiler.startPeriod("flush");
                    {
                        usleep(100000);
                        profiler.startPeriod();
                        { usleep(100000); }
                        profiler.stopPeriod();
                        usleep(100000);
                    }
                    profiler.stopPeriod("flush");
                    usleep(100000);
                }
                profiler.stopPeriod();
                usleep(100000);
                profiler.startPeriod();
                { usleep(100000); }
                profiler.stopPeriod();
                usleep(100000);
            }
            profiler.stopPeriod();
            usleep(100000);
            profiler.startPeriod("close");
            { usleep(100000); }
            profiler.stopPeriod("close");
            usleep(100000);
            profiler.startPeriod();
            { usleep(100000); }
            profiler.stopPeriod();
            usleep(100000);
            profiler.startPeriod();
            { usleep(100000); }
            profiler.stopPeriod();
            usleep(100000);
        }
        profiler.stopPeriod("format");
        usleep(100000);
    }
    profiler.stopPeriod();

    profiler.close();

    KARABO_LOG_FRAMEWORK_DEBUG_C("TestTimeClasses") << "Profiler:\n" << profiler;

    //    KARABO_LOG_FRAMEWORK_DEBUG << "Profiler:\n" << profiler.sql();

    //    KARABO_LOG_FRAMEWORK_DEBUG << "Profiler: " << profiler.getPeriod().getDuration();
    //
    //    KARABO_LOG_FRAMEWORK_DEBUG << "Profiler write: " << profiler.getPeriod("write").getDuration();
    //
    //    KARABO_LOG_FRAMEWORK_DEBUG << "Profiler write.format: " << profiler.getPeriod("write.format").getDuration();
    //
    //    KARABO_LOG_FRAMEWORK_DEBUG << "Profiler write.format.open: " <<
    //    profiler.getPeriod("write.format.open").getDuration();

    EXPECT_TRUE(true);
}


TEST(TestTimeClasses, testTimeId) {
    // default ctr.
    const TimeId stamp;
    EXPECT_EQ(0ull, stamp.getTid());

    // specific ctr.
    unsigned long long trainId = 123454321;
    const TimeId stamp2(trainId);
    EXPECT_EQ(trainId, stamp2.getTid());

    // operator == and !=
    const TimeId stamp3(trainId);
    const TimeId stamp4(trainId + 1);
    EXPECT_TRUE(stamp2 == stamp3);
    EXPECT_TRUE(stamp2 != stamp4);

    // conversion to Hash::Attributes etc.
    Hash::Attributes attrs;
    stamp2.toHashAttributes(attrs);
    EXPECT_TRUE(attrs.has("tid"));
    EXPECT_TRUE(TimeId::hashAttributesContainTimeInformation(attrs));
    EXPECT_EQ(trainId, attrs.get<decltype(trainId)>("tid"));

    attrs.erase("tid");
    EXPECT_TRUE(!TimeId::hashAttributesContainTimeInformation(attrs));
    EXPECT_THROW(TimeId::fromHashAttributes(attrs), karabo::data::ParameterException);

    attrs.set("tid", trainId + 2);
    EXPECT_TRUE(TimeId::hashAttributesContainTimeInformation(attrs));
    const TimeId stamp5(TimeId::fromHashAttributes(attrs));
    EXPECT_EQ(trainId + 2, stamp5.getTid());

    // Test that we can read a Train Id into an int
    auto trainIdAsInt = attrs.getNode("tid").getValue<int, decltype(trainId)>();
    EXPECT_EQ(trainId + 2, static_cast<decltype(trainId)>(trainIdAsInt));

    // Test that we can read a large Train Id
    attrs.set("tid", trainId * trainId);
    EXPECT_TRUE(TimeId::hashAttributesContainTimeInformation(attrs));
    const TimeId stamp6(TimeId::fromHashAttributes(attrs));
    EXPECT_EQ(trainId * trainId, stamp6.getTid());

    // Check that we cannot convert from string attributes to Train Id
    attrs.set("tid", "123454321");
    EXPECT_TRUE(TimeId::hashAttributesContainTimeInformation(attrs));
    EXPECT_THROW(TimeId::fromHashAttributes(attrs), karabo::data::ParameterException);

    attrs.erase("tid");
    EXPECT_TRUE(!TimeId::hashAttributesContainTimeInformation(attrs));

    // Use a signed long long as Train Id
    auto tid = static_cast<long long>(trainId);

    auto castToUInt = [&attrs, &tid]() {
        return attrs.getNode("tid").template getValue<unsigned int, int, short, decltype(tid)>();
    };

    // Test the numeric cast from small signed long long to unsigned int
    attrs.set("tid", tid);
    EXPECT_EQ(static_cast<unsigned int>(tid), castToUInt());

    // Check that the default method get() for reading from the attributes
    // will fail due to a type mismatch
    EXPECT_THROW(attrs.get("tid", trainId), karabo::data::CastException);

    // Check that we cannot cast a negative Train Id into an unsigned integer
    tid = -1;
    attrs.set("tid", tid);
    EXPECT_THROW(castToUInt(), karabo::data::CastException);

    // Check that we cannot cast a large Train Id into an unsigned int
    tid = std::numeric_limits<decltype(trainId)>::max();
    attrs.set("tid", tid);
    EXPECT_THROW(castToUInt(), karabo::data::CastException);
}


TEST(TestTimeClasses, testTimestamp) {
    const Epochstamp eStamp(1234567123ull, 79837534348ull);
    const TimeId trStamp(987654321ull);

    // default ctr.
    Timestamp stamp1;
    // cannot really test seconds and fractional seconds from now()...
    EXPECT_EQ(0ull, stamp1.getTid());

    // default from epoch and train stamp
    const Timestamp stamp2(eStamp, trStamp);
    EXPECT_EQ(1234567123ull, stamp2.getSeconds());
    EXPECT_EQ(79837534348ull, stamp2.getFractionalSeconds());
    EXPECT_EQ(987654321ull, stamp2.getTid());

    // copy ctr.
    const Timestamp stamp3(stamp2);
    EXPECT_EQ(1234567123ull, stamp3.getSeconds());
    EXPECT_EQ(79837534348ull, stamp3.getFractionalSeconds());
    EXPECT_EQ(987654321ull, stamp3.getTid());

    // assignment operator
    stamp1 = stamp3;
    EXPECT_EQ(1234567123ull, stamp1.getSeconds());
    EXPECT_EQ(79837534348ull, stamp1.getFractionalSeconds());
    EXPECT_EQ(987654321ull, stamp1.getTid());

    // operator == and !=: non-equality for both
    // - epoch is same, but trainId differs
    // - epoch differs, but trainId is same
    const Timestamp stamp2a(eStamp, TimeId(trStamp.getTid() + 2));
    const Timestamp stamp2b(eStamp + TimeDuration(12345ull, 987654321ull), trStamp);
    EXPECT_TRUE(stamp1 == stamp3);
    EXPECT_TRUE(stamp1 != stamp2a);
    EXPECT_TRUE(stamp1 != stamp2b);

    // Test building timestamp from unsigned long long attributes
    Hash::Attributes attrs;
    stamp1.toHashAttributes(attrs);

    EXPECT_TRUE(Timestamp::hashAttributesContainTimeInformation(attrs));

    const Timestamp stamp4(Timestamp::fromHashAttributes(attrs));
    EXPECT_EQ(stamp1.getTid(), stamp4.getTid());
    EXPECT_EQ(stamp1.getSeconds(), stamp4.getSeconds());
    EXPECT_EQ(stamp1.getFractionalSeconds(), stamp4.getFractionalSeconds());

    attrs.erase("tid");
    attrs.erase("sec");
    attrs.erase("frac");
    EXPECT_TRUE(!Timestamp::hashAttributesContainTimeInformation(attrs));

    // Test building timestamp from positive integer attributes
    int tid = 1;
    const int seconds = 1;
    const int frac = 12;
    attrs.set("tid", tid);
    attrs.set("sec", seconds);
    attrs.set("frac", frac);
    const Timestamp stamp5(Timestamp::fromHashAttributes(attrs));
    EXPECT_EQ(static_cast<unsigned long long>(tid), stamp5.getTid());
    EXPECT_EQ(static_cast<unsigned long long>(seconds), stamp5.getSeconds());
    EXPECT_EQ(static_cast<unsigned long long>(frac), stamp5.getFractionalSeconds());

    // Check that building trainstamp from a negative integer attribute fails
    tid = -1;
    attrs.set("tid", tid);
    EXPECT_EQ(tid, attrs.getNode("tid").getValue<decltype(tid)>());
    EXPECT_TRUE(Timestamp::hashAttributesContainTimeInformation(attrs));

    EXPECT_THROW(TimeId::fromHashAttributes(attrs), karabo::data::ParameterException);
    EXPECT_THROW(Timestamp::fromHashAttributes(attrs), karabo::data::ParameterException);
}
