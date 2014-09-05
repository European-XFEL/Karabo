/*
 *
 * File:   Epochstamp_Test.cc
 * Author: <luis.maia@xfel.eu>
 *
 * Created on April 19, 2013, 4:47:47 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "Epochstamp_Test.hh"
#include "karabo/util/DateTimeString.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(Epochstamp_Test);

using namespace std;
using namespace karabo::util;

Epochstamp_Test::Epochstamp_Test() {
}

Epochstamp_Test::~Epochstamp_Test() {
}

void Epochstamp_Test::setUp() {
}

void Epochstamp_Test::tearDown() {
}

void Epochstamp_Test::validateStringConstructor(const std::string& pTime,
        const unsigned long long& expectedSeconds,
        const unsigned long long& expectedFractionalSecond,
        bool isCompactString,
        const std::string& expectedToIso8601) {
    bool writeToClog = false;

    if (writeToClog) {
        std::clog << "Validate Constructor (pTime == " << pTime << ")" << std::endl;
        std::clog << "Sec => " << expectedSeconds << " |FSec => " << expectedFractionalSecond << std::endl;
        std::clog << "----------------------------------------------------------" << std::endl;
    }

    // Constructor
    karabo::util::Epochstamp epo;
    if (pTime == "") {
        //Empty constructor
        epo = karabo::util::Epochstamp();
    } else {
        //String constructor
        epo = karabo::util::Epochstamp(pTime);
    }
    // Constructor complete
    karabo::util::Epochstamp epo2 = karabo::util::Epochstamp(expectedSeconds, expectedFractionalSecond);


    // Validations
    if (writeToClog) std::clog << "epo.getSeconds() => " << epo.getSeconds() << " == " << expectedSeconds << std::endl;
    CPPUNIT_ASSERT(epo.getSeconds() == expectedSeconds);
    CPPUNIT_ASSERT(epo2.getSeconds() == expectedSeconds);

    if (writeToClog) std::clog << "epo.getFractionalSeconds() => " << epo.getFractionalSeconds() << " == " << expectedFractionalSecond << std::endl;
    CPPUNIT_ASSERT(epo.getFractionalSeconds() == expectedFractionalSecond);
    CPPUNIT_ASSERT(epo2.getFractionalSeconds() == expectedFractionalSecond);


    std::string expectedToIso8601Ext = expectedToIso8601 + "Z";
    if (expectedToIso8601 != "") {
        if (isCompactString == true) {
            // Validate "UNIVERSAL" compact ISO8601 format
            if (writeToClog) std::clog << "[Compact] toIso8601(ATTOSEC) => " << expectedToIso8601 << " == " << epo.toIso8601(ATTOSEC) << " == " << epo2.toIso8601(ATTOSEC) << std::endl;
            CPPUNIT_ASSERT(expectedToIso8601 == epo.toIso8601(ATTOSEC));
            CPPUNIT_ASSERT(expectedToIso8601 == epo2.toIso8601(ATTOSEC));
            //
            if (writeToClog) std::clog << "[Compact] toIso8601Ext(ATTOSEC) => " << expectedToIso8601Ext << " == " << epo.toIso8601Ext(ATTOSEC) << " == " << epo2.toIso8601Ext(ATTOSEC) << std::endl;
            CPPUNIT_ASSERT(expectedToIso8601Ext == epo.toIso8601Ext(ATTOSEC));
            CPPUNIT_ASSERT(expectedToIso8601Ext == epo2.toIso8601Ext(ATTOSEC));
        } else {
            // Validate "UNIVERSAL" extended ISO8601 format
            if (writeToClog) std::clog << "[Extended] toIso8601(ATTOSEC) => " << expectedToIso8601 << " == " << epo.toIso8601(ATTOSEC) << " == " << epo2.toIso8601(ATTOSEC) << std::endl;
            CPPUNIT_ASSERT(expectedToIso8601 == epo.toIso8601(ATTOSEC, true));
            CPPUNIT_ASSERT(expectedToIso8601 == epo2.toIso8601(ATTOSEC, true));
            //
            if (writeToClog) std::clog << "[Extended] toIso8601Ext(ATTOSEC) => " << expectedToIso8601Ext << " == " << epo.toIso8601Ext(ATTOSEC) << " == " << epo2.toIso8601Ext(ATTOSEC) << std::endl;
            CPPUNIT_ASSERT(expectedToIso8601Ext == epo.toIso8601Ext(ATTOSEC, true));
            CPPUNIT_ASSERT(expectedToIso8601Ext == epo2.toIso8601Ext(ATTOSEC, true));
        }

        // (i.e. expectedToIso8601 => 20121225T132536.789333123456789123)
        //ATTOSEC
        toIso8601Precision(epo, epo2, ATTOSEC, "ATTOSEC", isCompactString, writeToClog, expectedToIso8601);

        //FEMTOSEC
        toIso8601Precision(epo, epo2, FEMTOSEC, "FEMTOSEC", isCompactString, writeToClog, expectedToIso8601.substr(0, expectedToIso8601.size() - 3));

        //PICOSEC
        toIso8601Precision(epo, epo2, PICOSEC, "PICOSEC", isCompactString, writeToClog, expectedToIso8601.substr(0, expectedToIso8601.size() - 6));

        //NANOSEC
        toIso8601Precision(epo, epo2, NANOSEC, "NANOSEC", isCompactString, writeToClog, expectedToIso8601.substr(0, expectedToIso8601.size() - 9));

        //MICROSEC
        toIso8601Precision(epo, epo2, MICROSEC, "MICROSEC", isCompactString, writeToClog, expectedToIso8601.substr(0, expectedToIso8601.size() - 12));

        //MILLISEC
        toIso8601Precision(epo, epo2, MILLISEC, "MILLISEC", isCompactString, writeToClog, expectedToIso8601.substr(0, expectedToIso8601.size() - 15));

        //ONESECOND
        toIso8601Precision(epo, epo2, ONESECOND, "ONESECOND", isCompactString, writeToClog, expectedToIso8601.substr(0, expectedToIso8601.size() - 18) + "0");

        //NOFRACTION
        toIso8601Precision(epo, epo2, NOFRACTION, "NOFRACTION", isCompactString, writeToClog, expectedToIso8601.substr(0, expectedToIso8601.size() - 19));
    }


    // toTimestamp Validation
    ostringstream oss;
    oss << epo.getSeconds()
            << karabo::util::DateTimeString::fractionalSecondToString(MICROSEC, epo.getFractionalSeconds());

    double expectedTimestamp = boost::lexical_cast<double>(oss.str());

    if (writeToClog) std::clog << "toTimezone (MICROSEC) => " << epo.toTimestamp() << " == " << epo2.toTimestamp() << " == " << expectedTimestamp << std::endl;
    CPPUNIT_ASSERT(epo.toTimestamp() == expectedTimestamp);
    CPPUNIT_ASSERT(epo2.toTimestamp() == expectedTimestamp);
    CPPUNIT_ASSERT(epo.toTimestamp() == epo2.toTimestamp());

    return;
}

void Epochstamp_Test::toIso8601Precision(const karabo::util::Epochstamp epo,
        const karabo::util::Epochstamp epo2,
        const karabo::util::TIME_UNITS precision,
        const std::string& precisionDesc,
        const bool isCompactString,
        const bool writeToClog,
        const std::string& expectedToIso8601) {

    //Concatenate function name plus precision description
    std::string functionDesc = "toIso8601Precision >> toIso8601(" + precisionDesc + ") => '";

    //ATTOSEC
    if (isCompactString == true) {
        if (writeToClog) std::clog << "[Compact] " << functionDesc << epo.toIso8601(precision) << "' == '" << epo2.toIso8601(precision) << "'" << std::endl;
        CPPUNIT_ASSERT(epo.toIso8601(precision) == epo2.toIso8601(precision));
        if (writeToClog) std::clog << "[Compact] " << functionDesc << epo.toIso8601(precision) << "' == '" << expectedToIso8601 << "'" << std::endl;
        CPPUNIT_ASSERT(epo.toIso8601(precision) == expectedToIso8601);
        if (writeToClog) std::clog << "[Compact] " << functionDesc << epo2.toIso8601(precision) << " == " << expectedToIso8601 << std::endl;
        CPPUNIT_ASSERT(epo2.toIso8601(precision) == expectedToIso8601);
    } else {
        if (writeToClog) std::clog << "[Extended] " << functionDesc << epo.toIso8601(precision, true) << " == " << epo2.toIso8601(precision, true) << std::endl;
        CPPUNIT_ASSERT(epo.toIso8601(precision, true) == epo2.toIso8601(precision, true));
        if (writeToClog) std::clog << "[Extended] " << functionDesc << epo.toIso8601(precision, true) << " == " << expectedToIso8601 << std::endl;
        CPPUNIT_ASSERT(epo.toIso8601(precision, true) == expectedToIso8601);
        if (writeToClog) std::clog << "[Extended] " << functionDesc << epo2.toIso8601(precision, true) << " == " << expectedToIso8601 << std::endl;
        CPPUNIT_ASSERT(epo2.toIso8601(precision, true) == expectedToIso8601);
    }

    return;
}

void Epochstamp_Test::testConstructors() {

    int sleepDelay = 1;

    // Validate the NULL constructor that use NOW
    karabo::util::Epochstamp t01 = karabo::util::Epochstamp();
    sleep(sleepDelay);
    karabo::util::Epochstamp t02 = karabo::util::Epochstamp();
    CPPUNIT_ASSERT(t01.getSeconds() + sleepDelay == t02.getSeconds());
    CPPUNIT_ASSERT(t01.getFractionalSeconds() != t02.getFractionalSeconds());
    CPPUNIT_ASSERT(t01.getTime() != t02.getTime());

    // Validate the seconds + fractionalSeconds constructor
    karabo::util::Epochstamp t03 = karabo::util::Epochstamp();
    const unsigned long long& secs = t03.getSeconds();
    const unsigned long long& fraqs = t03.getFractionalSeconds();
    karabo::util::Epochstamp t04 = karabo::util::Epochstamp(secs, fraqs);

    CPPUNIT_ASSERT(t03.getSeconds() == t04.getSeconds());
    CPPUNIT_ASSERT(t03.getFractionalSeconds() == t04.getFractionalSeconds());
    CPPUNIT_ASSERT(t03.getTime() == t04.getTime());

    // Validate the "time_t" constructor
    const time_t& tm03 = t03.getTime();
    karabo::util::Epochstamp t05 = karabo::util::Epochstamp(tm03);

    CPPUNIT_ASSERT(t03.getSeconds() == t05.getSeconds());
    CPPUNIT_ASSERT(t04.getSeconds() == t05.getSeconds());
    CPPUNIT_ASSERT(t05.getFractionalSeconds() == 0);
    CPPUNIT_ASSERT(t03.getFractionalSeconds() != t05.getFractionalSeconds());
    CPPUNIT_ASSERT(t03.getTime() == t05.getTime());
    CPPUNIT_ASSERT(t04.getTime() == t05.getTime());

    // Validate the "timespec" constructor
    timespec ts06;
    ts06.tv_sec = t03.getTime();
    // Necessary this division because tv_nsec has only Nanosecond (10^9) resolution but getFractionalSeconds() returns Attosecond (10^18) resolution
    ts06.tv_nsec = t03.getFractionalSeconds() / 1000000000;
    karabo::util::Epochstamp t06 = karabo::util::Epochstamp(ts06);

    CPPUNIT_ASSERT(t03.getSeconds() == t06.getSeconds());
    CPPUNIT_ASSERT(t04.getSeconds() == t06.getSeconds());
    CPPUNIT_ASSERT(t05.getSeconds() == t06.getSeconds());
    CPPUNIT_ASSERT(t03.getTime() == t06.getTime());
    CPPUNIT_ASSERT(t04.getTime() == t06.getTime());
    CPPUNIT_ASSERT(t05.getTime() == t06.getTime());

    CPPUNIT_ASSERT(t03.getFractionalSeconds() == t06.getFractionalSeconds());
    CPPUNIT_ASSERT(t04.getFractionalSeconds() == t06.getFractionalSeconds());
    CPPUNIT_ASSERT(t05.getFractionalSeconds() != t06.getFractionalSeconds());


    /*
     * Validate the ISO8601 "string" constructor
     * 
     * (Old tests)
     */
    validateStringConstructor("20121225T132536.789333123456789123", 1356441936ULL, 789333123456789123ULL, true, "");
    validateStringConstructor("2012-12-25T13:25:36.123456789123456789", 1356441936ULL, 123456789123456789ULL, false, "");
    validateStringConstructor("20121225T132536.123456789123456789Z", 1356441936ULL, 123456789123456789ULL, true, "");
    validateStringConstructor("2012-12-25T13:25:36,123456789123456789z", 1356441936ULL, 123456789123456789ULL, false, "");
    validateStringConstructor("2012-12-25T13:25:36,123456789123456789+00:00", 1356441936ULL, 123456789123456789ULL, false, "");
    validateStringConstructor("2012-12-25T13:25:36+00:00", 1356441936ULL, 0ULL, false, "");


    /*
     * Validate the ISO8601 "string" constructor
     * 
     * (New tests)
     */
    unsigned long long expectedSeconds = 475111250ULL;
    unsigned long long expectedSecondsSinceEpochMinos7hULL = 475136450; //475111250 + (60*60*7)
    unsigned long long expectedSecondsSinceEpochPlus3h30mULL = 475098650; //475111250 - (60*60*3.5)

    // Validate Extended strings
    validateStringConstructor("1985-01-20T23:20:50.789333123456789123", expectedSeconds, 789333123456789123ULL, false, "");
    validateStringConstructor("1985-01-20T23:20:50", expectedSeconds, 0ULL, false, "");
    validateStringConstructor("1985-01-20T23:20:50,123", expectedSeconds, 123000000000000000ULL, false, "");
    validateStringConstructor("1985-01-20T23:20:50.123", expectedSeconds, 123000000000000000ULL, false, "");
    validateStringConstructor("1985-01-20T23:20:50.123z", expectedSeconds, 123000000000000000ULL, false, "");
    validateStringConstructor("1985-01-20T23:20:50z", expectedSeconds, 0ULL, false, "");
    validateStringConstructor("1985-01-20T23:20:50Z", expectedSeconds, 0ULL, false, "");
    validateStringConstructor("1985-01-20T23:20:50+00:00", expectedSeconds, 0ULL, false, "");
    //
    validateStringConstructor("1985-01-20T23:20:50-07:00", expectedSecondsSinceEpochMinos7hULL, 0ULL, false, "");
    validateStringConstructor("1985-01-20T23:20:50+03:30", expectedSecondsSinceEpochPlus3h30mULL, 0ULL, false, "");

    // Validate Compact strings
    validateStringConstructor("19850120T232050.789333123456789123", expectedSeconds, 789333123456789123ULL, true, "");
    validateStringConstructor("19850120T232050", expectedSeconds, 0ULL, true, "");
    validateStringConstructor("19850120T232050,123", expectedSeconds, 123000000000000000ULL, true, "");
    validateStringConstructor("19850120T232050.123", expectedSeconds, 123000000000000000ULL, true, "");
    validateStringConstructor("19850120T232050.123z", expectedSeconds, 123000000000000000ULL, true, "");
    validateStringConstructor("19850120T232050z", expectedSeconds, 0ULL, true, "");
    validateStringConstructor("19850120T232050Z", expectedSeconds, 0ULL, true, "");
    validateStringConstructor("19850120T232050+0000", expectedSeconds, 0ULL, true, "");
    //
    validateStringConstructor("19850120T232050-0700", expectedSecondsSinceEpochMinos7hULL, 0ULL, true, "");
    validateStringConstructor("19850120T232050+0330", expectedSecondsSinceEpochPlus3h30mULL, 0ULL, true, "");

}

void Epochstamp_Test::testToIso8601String() {

    // Validate "UNIVERSAL" compact ISO8601 format 
    validateStringConstructor("20121225T132536.789333123456789123", 1356441936ULL, 789333123456789123ULL, true, "20121225T132536.789333123456789123");
    validateStringConstructor("2012-12-25T13:25:36.789333123456789123", 1356441936ULL, 789333123456789123ULL, true, "20121225T132536.789333123456789123");
    validateStringConstructor("2012-12-25T13:25:36.100", 1356441936ULL, 100000000000000000ULL, true, "20121225T132536.100000000000000000");
    validateStringConstructor("2012-12-25T13:25:36.001", 1356441936ULL, 1000000000000000ULL, true, "20121225T132536.001000000000000000");
    validateStringConstructor("2012-12-25T13:25:36.000000000000000001", 1356441936ULL, 1ULL, true, "20121225T132536.000000000000000001");
    validateStringConstructor("2012-12-25T13:25:36.000000000000000123", 1356441936ULL, 123ULL, true, "20121225T132536.000000000000000123");
    validateStringConstructor("2012-12-25T13:25:36.123456789", 1356441936ULL, 123456789000000000ULL, true, "20121225T132536.123456789000000000");
    validateStringConstructor("2012-12-25T13:25:36.000123456789", 1356441936ULL, 123456789000000ULL, true, "20121225T132536.000123456789000000");

    // Validate "UNIVERSAL" extended ISO8601 format 
    validateStringConstructor("2012-12-25T13:25:36.123456789123456789", 1356441936ULL, 123456789123456789ULL, false, "2012-12-25T13:25:36.123456789123456789");
    validateStringConstructor("20121225T132536.123456789123456789", 1356441936ULL, 123456789123456789ULL, false, "2012-12-25T13:25:36.123456789123456789");
    validateStringConstructor("20121225T132536.100", 1356441936ULL, 100000000000000000ULL, false, "2012-12-25T13:25:36.100000000000000000");
    validateStringConstructor("20121225T132536.001", 1356441936ULL, 1000000000000000ULL, false, "2012-12-25T13:25:36.001000000000000000");
    validateStringConstructor("20121225T132536.000000000000000001", 1356441936ULL, 1ULL, false, "2012-12-25T13:25:36.000000000000000001");
    validateStringConstructor("20121225T132536.000000000000000123", 1356441936ULL, 123ULL, false, "2012-12-25T13:25:36.000000000000000123");
    validateStringConstructor("20121225T132536.000123456789", 1356441936ULL, 123456789000000ULL, true, "20121225T132536.000123456789000000");
}

void Epochstamp_Test::validateToFormattedString(const std::string& pTime,
        const std::string& format,
        const std::string& pTimeDesiredTimeZone,
        const std::string& expectedStringOutput) {
    bool writeToClog = false;
    std::string utcTimeZone = "Z"; //"UTC" == "Z"
    std::string localeNameUS = "en_US.UTF-8";

    if (writeToClog) {
        std::clog << "Validate Constructor (pTime == " << pTime << ")" << std::endl;
        std::clog << "Format => " << format << " |Expected output => " << expectedStringOutput << std::endl;
        std::clog << "----------------------------------------------------------" << std::endl;
    }


    // Constructor
    karabo::util::Epochstamp epo;
    if (pTime == "") {
        //Empty constructor
        epo = karabo::util::Epochstamp();
    } else {
        //String constructor
        epo = karabo::util::Epochstamp(pTime);
    }

    /**
     * Function toFormattedStringLocale and toFormattedString only differ in the used locale, since internally both use
     * the same function: toFormattedStringInternal
     * 
     * toFormattedStringLocale - requires that the locale name
     * toFormattedString - uses the System locale
     * 
     * In this test only the toFormattedStringLocale, since this way it's possible to run successfully this test 
     * in Systems with a different locale
     */
    std::string returnFormatedString;
    if (format == "") {
        if (pTimeDesiredTimeZone == utcTimeZone) {
            returnFormatedString = epo.toFormattedStringLocale(localeNameUS);
        } else {
            returnFormatedString = epo.toFormattedStringLocale(localeNameUS, "%Y-%b-%d %H:%M:%S", pTimeDesiredTimeZone);
        }
    } else {
        if (pTimeDesiredTimeZone == utcTimeZone) {
            returnFormatedString = epo.toFormattedStringLocale(localeNameUS, format);
        } else {
            returnFormatedString = epo.toFormattedStringLocale(localeNameUS, format, pTimeDesiredTimeZone);
        }
    }

    if (writeToClog) std::clog << "epo.toFormattedString('" << format << "', '" << pTimeDesiredTimeZone << "') => " << returnFormatedString << " == " << expectedStringOutput << std::endl;
    CPPUNIT_ASSERT(returnFormatedString == expectedStringOutput);
}

void Epochstamp_Test::testToFormattedString() {

    std::string pTime = "20121225T132536.789333123456789123";
    std::string utcTimeZone = "Z"; //"UTC" == "Z"

    validateToFormattedString(pTime, "", utcTimeZone, "2012-Dec-25 13:25:36");
    validateToFormattedString(pTime, "%Y/%m/%d %H:%M:%S", utcTimeZone, "2012/12/25 13:25:36");
    validateToFormattedString(pTime, "%Y/%m/%d", utcTimeZone, "2012/12/25");
    validateToFormattedString(pTime, "%c", utcTimeZone, "Tue 25 Dec 2012 01:25:36 PM ");
    validateToFormattedString(pTime, "%H:%M:%S", utcTimeZone, "13:25:36");
    validateToFormattedString(pTime, "%H:%M:%S.%f", utcTimeZone, "13:25:36.789333");


    std::string pTime2 = "1985-01-20T23:20:50-07:00";
    validateToFormattedString(pTime2, "", utcTimeZone, "1985-Jan-21 06:20:50");
    validateToFormattedString(pTime2, "", "+03:30", "1985-Jan-21 09:50:50");
    validateToFormattedString(pTime2, "", "-07:00", "1985-Jan-20 23:20:50");
    validateToFormattedString(pTime2, "", "+01:00", "1985-Jan-21 07:20:50");


    std::string pTime3 = "1985-01-20T23:20:50+03:30";
    validateToFormattedString(pTime3, "", utcTimeZone, "1985-Jan-20 19:50:50");
    validateToFormattedString(pTime3, "", "+03:30", "1985-Jan-20 23:20:50");
    validateToFormattedString(pTime3, "", "-07:00", "1985-Jan-20 12:50:50");
    validateToFormattedString(pTime3, "", "+01:00", "1985-Jan-20 20:50:50");

}

