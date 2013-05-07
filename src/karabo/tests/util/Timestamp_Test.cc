/*
 *
 * File:   Timestamp_Test.cc
 * Author: <luis.maia@xfel.eu>
 *
 * Created on April 19, 2013, 4:47:47 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "Timestamp_Test.hh"
#include <karabo/util/Timestamp.hh>

CPPUNIT_TEST_SUITE_REGISTRATION(Timestamp_Test);

using namespace std;
using namespace karabo::util;


Timestamp_Test::Timestamp_Test() {
}


Timestamp_Test::~Timestamp_Test() {
}


void Timestamp_Test::setUp() {
}


void Timestamp_Test::tearDown() {
}


void Timestamp_Test::testConstructors() {
    std::string pTime = "20121225T132536.789333";
    boost::posix_time::ptime pt = boost::posix_time::from_iso_string(pTime);

    // Validate the NULL constructor
    karabo::util::Timestamp t01 = karabo::util::Timestamp();
    std::string timestampStr01 = t01.toString();
    CPPUNIT_ASSERT(timestampStr01 != pTime);

    t01.setTime(pt);
    timestampStr01 = t01.toString();
    CPPUNIT_ASSERT(timestampStr01 == pTime);


    // Validate constructor that receives a "boost::posix_time::ptime"
    karabo::util::Timestamp t02 = karabo::util::Timestamp(pt);
    std::string timestampStr02 = t02.toString();
    CPPUNIT_ASSERT(timestampStr02 == pTime);

    // Validate constructor that receives a Timestamp class
    karabo::util::Timestamp t03 = karabo::util::Timestamp(t01);
    std::string timestampStr03 = t03.toString();
    CPPUNIT_ASSERT(timestampStr02 == timestampStr03);

    // Validate constructor that receives a "UNIVERSAL" string
    std::string pTime02 = "20111225T132536.259333";
    karabo::util::Timestamp t04 = karabo::util::Timestamp(pTime02);
    std::string timestampStr04 = t04.toString();
    CPPUNIT_ASSERT(pTime02 == timestampStr04);
}


void Timestamp_Test::testToString() {
    std::string pTime = "20121225T132536.789333";
    boost::posix_time::ptime pt = boost::posix_time::from_iso_string(pTime);
    karabo::util::Timestamp t01 = karabo::util::Timestamp(pt);

    // Validate "UNIVERSAL" format
    std::string timestampStr = t01.toString();
    CPPUNIT_ASSERT(timestampStr == pTime);
}


void Timestamp_Test::testToIsoString() {
    std::string pTime = "20121225T132536.789333";
    boost::posix_time::ptime pt = boost::posix_time::from_iso_string(pTime);
    karabo::util::Timestamp t01 = karabo::util::Timestamp(pt);

    // Validate "ISO 8601" format
    std::string pFormattedTime = "2012-12-25T13:25:36.789333";
    std::string timestampFormattedStr = t01.toIsoString();
    CPPUNIT_ASSERT(pFormattedTime == timestampFormattedStr);
}


void Timestamp_Test::testToFormattedString() {
    std::string pTime = "20121225T132536.789333";
    boost::posix_time::ptime pt = boost::posix_time::from_iso_string(pTime);
    karabo::util::Timestamp t01 = karabo::util::Timestamp(pt);

    // Validate "Human Readable" format
    std::string pReadableTime = "2012-Dec-25 13:25:36";
    std::string timestampReadableStr = t01.toFormattedString();
    CPPUNIT_ASSERT(pReadableTime == timestampReadableStr);
}


void Timestamp_Test::testCalculateAndReturnMsSinceEpoch() {
    std::string pTime = "20121225T132536.789333";
    boost::posix_time::ptime pt = boost::posix_time::from_iso_string(pTime);
    karabo::util::Timestamp t01 = karabo::util::Timestamp(pt);

    // Validate msSinceEpoc stored in the Object is the same it is calculated
    unsigned long long msSinceEpochObject = t01.getMsSinceEpoch();
    unsigned long long msSinceEpochCalc = t01.calculateAndReturnMsSinceEpoch(pt);
    CPPUNIT_ASSERT(msSinceEpochObject == msSinceEpochCalc);

    unsigned long long expectedMsValue = 1356441936789; // for date: "20121225T132536.789333"
    CPPUNIT_ASSERT(msSinceEpochObject == expectedMsValue);
    CPPUNIT_ASSERT(msSinceEpochCalc == expectedMsValue);
}


void Timestamp_Test::testStrToTimestamp() {
    CPPUNIT_ASSERT(true);
    //    std::string pTime = "20121225T132536.789333";
    //    boost::posix_time::ptime pt = boost::posix_time::from_iso_string(pTime);
    //    karabo::util::Timestamp t01 = karabo::util::Timestamp(pt);
    //
    //    // Validate stored ptime == to real time generated (from a string) ptime
    //    boost::posix_time::ptime timeStored = t01.getTime();
    //    boost::posix_time::ptime timeCalc = t01.getUniversalString2PTime(pTime);
    //    CPPUNIT_ASSERT(timeStored == timeCalc);
    //
    //    // Validate stored ptime == to real time generated (from a special string) ptime
    //    std::string pTimeSpecial = "2012-12-25T13:25:36.789333";
    //    const boost::posix_time::time_input_facet* inputFormat = new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%S%f");
    //    boost::posix_time::ptime timeSpecialCalc = t01.getStringFormated2PTime(pTimeSpecial, inputFormat);
    //    CPPUNIT_ASSERT(timeStored == timeSpecialCalc);
    //
    //    // Validate function getStringFormated2PTime
    //    std::string pTimeSpecial2 = "2012-12-25 13:25:36.789333";
    //    boost::posix_time::ptime timeSpecial2Calc = t01.getStringFormated2PTime(pTimeSpecial2);
    //    CPPUNIT_ASSERT(timeStored == timeSpecial2Calc);
    //
    //    // Validate update date from a string
    //    std::string pNewTime = "20130425T132536.789333";
    //    boost::posix_time::ptime ptNewTime = boost::posix_time::from_iso_string(pNewTime);
    //    t01.setTime(pNewTime);
    //    boost::posix_time::ptime ptNewTimeStored = t01.getTime();
    //    CPPUNIT_ASSERT(ptNewTime == ptNewTimeStored);
}