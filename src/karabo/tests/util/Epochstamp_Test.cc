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
    CPPUNIT_ASSERT(t03.getTime() == t05.getTime());
    CPPUNIT_ASSERT(t04.getTime() == t05.getTime());

    // Validate the "timespec" constructor
    timespec ts06;
    ts06.tv_sec = t03.getTime();
    ts06.tv_nsec = t03.getFractionalSeconds();
    karabo::util::Epochstamp t06 = karabo::util::Epochstamp(ts06);

    /*std::cout << endl << "lmaia: t03.getSeconds() = " << t03.getSeconds();
    std::cout << endl << "lmaia: t05.getSeconds() = " << t05.getSeconds();
    std::cout << endl << "lmaia: t06.getSeconds() = " << t06.getSeconds();
    std::cout << endl << "lmaia: t03.getFractionalSeconds() = " << t03.getFractionalSeconds();
    std::cout << endl << "lmaia: t05.getFractionalSeconds() = " << t05.getFractionalSeconds();
    std::cout << endl << "lmaia: t06.getFractionalSeconds() = " << t06.getFractionalSeconds();*/

    CPPUNIT_ASSERT(t03.getSeconds() == t06.getSeconds());
    CPPUNIT_ASSERT(t04.getSeconds() == t06.getSeconds());
    CPPUNIT_ASSERT(t05.getSeconds() == t06.getSeconds());
    CPPUNIT_ASSERT(t03.getTime() == t06.getTime());
    CPPUNIT_ASSERT(t04.getTime() == t06.getTime());
    CPPUNIT_ASSERT(t05.getTime() == t06.getTime());
    //CPPUNIT_ASSERT(t03.getFractionalSeconds() == t06.getFractionalSeconds());
    //CPPUNIT_ASSERT(t04.getFractionalSeconds() == t06.getFractionalSeconds());


    // Validate the ISO8601 "string" constructor
    std::string pTimeStr01 = "20121225T132536.789333123456789123";
    karabo::util::Epochstamp t07 = karabo::util::Epochstamp(pTimeStr01);

    CPPUNIT_ASSERT(t07.getSeconds() == 1356441936);
    CPPUNIT_ASSERT(t07.getFractionalSeconds() == 789333123456789123);


    // Validate other "string" constructor
    std::string pTimeStr02 = "2012-12-25T13:25:36.123456789123456789";
    karabo::util::Epochstamp t08 = karabo::util::Epochstamp(pTimeStr02);

    CPPUNIT_ASSERT(t08.getSeconds() == 1356441936);
    CPPUNIT_ASSERT(t08.getFractionalSeconds() == 123456789123456789);


    // Validate other "string" constructor
    std::string pTimeStr03 = "2012-12-25 13:25:36.123456789123456789";
    karabo::util::Epochstamp t09 = karabo::util::Epochstamp(pTimeStr03);

    CPPUNIT_ASSERT(t09.getSeconds() == 1356441936);
    CPPUNIT_ASSERT(t09.getFractionalSeconds() == 123456789123456789);

}


void Epochstamp_Test::testToIso8601String() {

    // ISO8601 compact version
    std::string pTimeStr01 = "20121225T132536.789333123456789123";
    karabo::util::Epochstamp t01 = karabo::util::Epochstamp(pTimeStr01);

    CPPUNIT_ASSERT(t01.getSeconds() == 1356441936);
    CPPUNIT_ASSERT(t01.getFractionalSeconds() == 789333123456789123);

    // Validate "UNIVERSAL" format
    std::string pTimeConvertedStr01 = t01.toIso8601(ATTOSEC);
    CPPUNIT_ASSERT(pTimeConvertedStr01 == pTimeStr01);


    /**/
    // ISO8601 extended version
    std::string pTimeStr02 = "2012-12-25T13:25:36.123456789123456789";
    karabo::util::Epochstamp t02 = karabo::util::Epochstamp(pTimeStr02);

    CPPUNIT_ASSERT(t02.getSeconds() == 1356441936);
    CPPUNIT_ASSERT(t02.getFractionalSeconds() == 123456789123456789);

    // Validate "UNIVERSAL" format
    std::string pTimeConvertedStr02 = t02.toIso8601(ATTOSEC, true);
    CPPUNIT_ASSERT(pTimeConvertedStr02 == pTimeStr02);

    //std::cout << endl << "lmaia: t02.getSeconds() = " << t02.getSeconds();
    //std::cout << endl << "lmaia: t02.getFractionalSeconds() = " << t02.getFractionalSeconds();
    //std::cout << endl << "lmaia: pTimeConvertedStr02 = " << pTimeConvertedStr02;

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

    // Validate "Human Readable" format "%c"
    std::string pReadableTime04 = "Tue 25 Dec 2012 01:25:36 PM ";
    std::string timestampReadableStr04 = t01.toFormattedString("%c");
    CPPUNIT_ASSERT(pReadableTime04 == timestampReadableStr04);
}


void Epochstamp_Test::testStrToEpochstamp() {

    CPPUNIT_ASSERT(true);

    // Validate the ISO8601 "string" constructor
    const std::string pTimeStr01 = "20121225T132536.789333123456789123";
    karabo::util::Epochstamp t01 = karabo::util::Epochstamp(pTimeStr01);
    karabo::util::Epochstamp t02 = karabo::util::Epochstamp::fromIso8601(pTimeStr01);

    CPPUNIT_ASSERT(t01.getSeconds() == t02.getSeconds());
    CPPUNIT_ASSERT(t01.getFractionalSeconds() == t02.getFractionalSeconds());

    // Validate other "string" constructor
    std::string pTimeStr02 = "2012-12-25T13:25:36.123456789123456789";
    karabo::util::Epochstamp t03 = karabo::util::Epochstamp(pTimeStr02);
    const karabo::util::Epochstamp t04 = karabo::util::Epochstamp::fromIso8601(pTimeStr02);

    CPPUNIT_ASSERT(t03.getSeconds() == t04.getSeconds());
    CPPUNIT_ASSERT(t03.getFractionalSeconds() == t04.getFractionalSeconds());


    // Validate other "string" constructor
    std::string pTimeStr03 = "2012-12-25 13:25:36.123456789123456789";
    karabo::util::Epochstamp t05 = karabo::util::Epochstamp(pTimeStr03);
    const karabo::util::Epochstamp t06 = karabo::util::Epochstamp::fromIso8601(pTimeStr03);

    CPPUNIT_ASSERT(t05.getSeconds() == t06.getSeconds());
    CPPUNIT_ASSERT(t05.getFractionalSeconds() == t06.getFractionalSeconds());
}


void Epochstamp_Test::testStrToEpochstampExternal() {

    CPPUNIT_ASSERT(true);

    // Validate the ISO8601 "string" constructor
    const std::string pTimeStr01 = "20121225T132536.789333123456789123";
    karabo::util::Epochstamp t01 = karabo::util::Epochstamp(pTimeStr01);
    karabo::util::Epochstamp t02 = karabo::util::Epochstamp::fromIso8601Ext(pTimeStr01 + "Z");

    CPPUNIT_ASSERT(t01.getSeconds() == t02.getSeconds());
    CPPUNIT_ASSERT(t01.getFractionalSeconds() == t02.getFractionalSeconds());

    // Validate other "string" constructor
    std::string pTimeStr02 = "2012-12-25T13:25:36.123456789123456789";
    karabo::util::Epochstamp t03 = karabo::util::Epochstamp(pTimeStr02);
    const karabo::util::Epochstamp t04 = karabo::util::Epochstamp::fromIso8601Ext(pTimeStr02 + "Z");

    CPPUNIT_ASSERT(t03.getSeconds() == t04.getSeconds());
    CPPUNIT_ASSERT(t03.getFractionalSeconds() == t04.getFractionalSeconds());


    // Validate other "string" constructor
    std::string pTimeStr03 = "2012-12-25 13:25:36.123456789123456789";
    karabo::util::Epochstamp t05 = karabo::util::Epochstamp(pTimeStr03);
    const karabo::util::Epochstamp t06 = karabo::util::Epochstamp::fromIso8601Ext(pTimeStr03 + "Z");

    CPPUNIT_ASSERT(t05.getSeconds() == t06.getSeconds());
    CPPUNIT_ASSERT(t05.getFractionalSeconds() == t06.getFractionalSeconds());
}

