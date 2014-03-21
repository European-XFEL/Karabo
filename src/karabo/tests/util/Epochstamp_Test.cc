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
#include <karabo/util/Epochstamp.hh>

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
    bool writeToClog = true;

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
    if (writeToClog) std::clog << "Sec => " << epo.getSeconds() << " == " << expectedSeconds << std::endl;
    CPPUNIT_ASSERT(epo.getSeconds() == expectedSeconds);
    CPPUNIT_ASSERT(epo2.getSeconds() == expectedSeconds);

    if (writeToClog) std::clog << "Sec => " << epo.getFractionalSeconds() << " == " << expectedFractionalSecond << std::endl;
    CPPUNIT_ASSERT(epo.getFractionalSeconds() == expectedFractionalSecond);
    CPPUNIT_ASSERT(epo2.getFractionalSeconds() == expectedFractionalSecond);


    if (expectedToIso8601 != "") {
        if (isCompactString == true) {
            // Validate "UNIVERSAL" compact ISO8601 format
            if (writeToClog) std::clog << "[Compact] toIso8601(ATTOSEC) => " << expectedToIso8601 << " == " << epo.toIso8601(ATTOSEC) << " == " << epo2.toIso8601(ATTOSEC) << std::endl;
            CPPUNIT_ASSERT(expectedToIso8601 == epo.toIso8601(ATTOSEC));
            CPPUNIT_ASSERT(expectedToIso8601 == epo2.toIso8601(ATTOSEC));
        } else {
            // Validate "UNIVERSAL" extended ISO8601 format
            if (writeToClog) std::clog << "[Extended] toIso8601(ATTOSEC) => " << expectedToIso8601 << " == " << epo.toIso8601(ATTOSEC) << " == " << epo2.toIso8601(ATTOSEC) << std::endl;
            CPPUNIT_ASSERT(expectedToIso8601 == epo.toIso8601(ATTOSEC, true));
            CPPUNIT_ASSERT(expectedToIso8601 == epo2.toIso8601(ATTOSEC, true));
        }
    }

    if (writeToClog) std::clog << "[Compact] toIso8601(ATTOSEC) => " << epo.toIso8601(ATTOSEC) << " == " << epo2.toIso8601(ATTOSEC) << std::endl;
    CPPUNIT_ASSERT(epo.toIso8601(ATTOSEC) == epo2.toIso8601(ATTOSEC));

    if (writeToClog) std::clog << "[Extended] toIso8601(ATTOSEC) => " << epo.toIso8601(ATTOSEC) << " == " << epo2.toIso8601(ATTOSEC) << std::endl;
    CPPUNIT_ASSERT(epo.toIso8601(ATTOSEC, true) == epo2.toIso8601(ATTOSEC, true));

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
    ts06.tv_nsec = t03.getFractionalSeconds();
    karabo::util::Epochstamp t06 = karabo::util::Epochstamp(ts06);

    CPPUNIT_ASSERT(t03.getSeconds() == t06.getSeconds());
    CPPUNIT_ASSERT(t04.getSeconds() == t06.getSeconds());
    CPPUNIT_ASSERT(t05.getSeconds() == t06.getSeconds());
    CPPUNIT_ASSERT(t03.getTime() == t06.getTime());
    CPPUNIT_ASSERT(t04.getTime() == t06.getTime());
    CPPUNIT_ASSERT(t05.getTime() == t06.getTime());

    //TODO - Should this test work?!
    //std::clog << endl << "lmaia: t03.getFractionalSeconds() = " << t03.getFractionalSeconds();
    //std::clog << endl << "lmaia: t06.getFractionalSeconds() = " << t06.getFractionalSeconds();
    //CPPUNIT_ASSERT(t03.getFractionalSeconds() == t06.getFractionalSeconds());


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
    validateStringConstructor("1985-01-20T23:20:50,123", expectedSeconds, 123ULL, false, "");
    validateStringConstructor("1985-01-20T23:20:50.123", expectedSeconds, 123ULL, false, "");
    validateStringConstructor("1985-01-20T23:20:50.123z", expectedSeconds, 123ULL, false, "");
    validateStringConstructor("1985-01-20T23:20:50z", expectedSeconds, 0ULL, false, "");
    validateStringConstructor("1985-01-20T23:20:50Z", expectedSeconds, 0ULL, false, "");
    validateStringConstructor("1985-01-20T23:20:50+00:00", expectedSeconds, 0ULL, false, "");
    //
    validateStringConstructor("1985-01-20T23:20:50-07:00", expectedSecondsSinceEpochMinos7hULL, 0ULL, false, "");
    validateStringConstructor("1985-01-20T23:20:50+03:30", expectedSecondsSinceEpochPlus3h30mULL, 0ULL, false, "");

    // Validate Compact strings
    validateStringConstructor("19850120T232050.789333123456789123", expectedSeconds, 789333123456789123ULL, true, "");
    validateStringConstructor("19850120T232050", expectedSeconds, 0ULL, true, "");
    validateStringConstructor("19850120T232050,123", expectedSeconds, 123ULL, true, "");
    validateStringConstructor("19850120T232050.123", expectedSeconds, 123ULL, true, "");
    validateStringConstructor("19850120T232050.123z", expectedSeconds, 123ULL, true, "");
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

    // Validate "UNIVERSAL" extended ISO8601 format 
    validateStringConstructor("2012-12-25T13:25:36.123456789123456789", 1356441936ULL, 123456789123456789ULL, false, "2012-12-25T13:25:36.123456789123456789");
    validateStringConstructor("20121225T132536.123456789123456789", 1356441936ULL, 123456789123456789ULL, false, "2012-12-25T13:25:36.123456789123456789");

}


void Epochstamp_Test::testToIso8601StringExternal() {

    // ISO8601 compact version
    std::string pTimeStr01 = "20121225T132536.789333123456789123";
    karabo::util::Epochstamp t01 = karabo::util::Epochstamp(pTimeStr01);

    CPPUNIT_ASSERT(t01.getSeconds() == 1356441936);
    CPPUNIT_ASSERT(t01.getFractionalSeconds() == 789333123456789123);

    // Validate "UNIVERSAL" format
    std::string pTimeConvertedStr01 = t01.toIso8601Ext(ATTOSEC);
    CPPUNIT_ASSERT(pTimeConvertedStr01 == pTimeStr01 + "Z");


    /**/
    // ISO8601 extended version
    std::string pTimeStr02 = "2012-12-25T13:25:36.123456789123456789";
    karabo::util::Epochstamp t02 = karabo::util::Epochstamp(pTimeStr02);

    CPPUNIT_ASSERT(t02.getSeconds() == 1356441936);
    CPPUNIT_ASSERT(t02.getFractionalSeconds() == 123456789123456789);

    // Validate "UNIVERSAL" format
    std::string pTimeConvertedStr02 = t02.toIso8601Ext(ATTOSEC, true);
    CPPUNIT_ASSERT(pTimeConvertedStr02 == pTimeStr02 + "Z");

}


void Epochstamp_Test::testToFormattedString() {

    // ISO8601 compact version
    std::string pTimeStr01 = "20121225T132536.789333123456789123";
    karabo::util::Epochstamp t01 = karabo::util::Epochstamp(pTimeStr01);

    CPPUNIT_ASSERT(t01.getSeconds() == 1356441936);
    CPPUNIT_ASSERT(t01.getFractionalSeconds() == 789333123456789123);

    // Validate "Human Readable" format "%Y-%b-%d %H:%M:%S"
    std::string pReadableTime01 = "2012-Dec-25 13:25:36";
    std::string timestampReadableStr01 = t01.toFormattedString(); //Default format is: "%Y-%b-%d %H:%M:%S"
    CPPUNIT_ASSERT(pReadableTime01 == timestampReadableStr01);

    // Validate "Human Readable" format "%Y/%m/%d %H:%M:%S"
    std::string pReadableTime02 = "2012/12/25 13:25:36";
    std::string timestampReadableStr02 = t01.toFormattedString("%Y/%m/%d %H:%M:%S");
    CPPUNIT_ASSERT(pReadableTime02 == timestampReadableStr02);

    // Validate "Human Readable" format "%Y/%m/%d"
    std::string pReadableTime03 = "2012/12/25";
    std::string timestampReadableStr03 = t01.toFormattedString("%Y/%m/%d");
    CPPUNIT_ASSERT(pReadableTime03 == timestampReadableStr03);

    // Validate "Human Readable" format "%c" - commented out as it depends on current locale settings
    //    std::string pReadableTime04 = "Tue 25 Dec 2012 01:25:36 PM ";
    //    std::string timestampReadableStr04 = t01.toFormattedString("%c");
    //    CPPUNIT_ASSERT(pReadableTime04 == timestampReadableStr04);
}

