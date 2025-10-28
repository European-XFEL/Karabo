/*
 * File:   DateTimeString_Test.hh
 * Author: <luis.maia@xfel.eu>
 *
 * Created on March 19, 2014, 11:01 AM
 *
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

#include <gtest/gtest.h>

#include "karabo/data/time/DateTimeString.hh"

using namespace std;
using namespace karabo::data;


static void validateConstructor(const std::string& pTime, const std::string& expectedDate,
                                const std::string& expectedTime, const std::string& expectedFractionalSecond,
                                const std::string& expectedTimeZone, const std::string& expectedDateTime,
                                const std::string& expectedSecondsSinceEpoch) {
    bool writeToClog = false;

    if (writeToClog) {
        std::clog << "Validate Constructor (pTime == " << pTime << ")" << std::endl;
        std::clog << "DT => " << expectedDate << " |T => " << expectedTime << " |FSec => " << expectedFractionalSecond
                  << " |TZ => " << expectedTimeZone << " |" << std::endl;
        std::clog << "----------------------------------------------------------" << std::endl;
    }

    // Constructor
    karabo::data::DateTimeString dts;
    if (pTime == "") {
        // Empty constructor
        dts = karabo::data::DateTimeString();
    } else {
        // String constructor
        dts = karabo::data::DateTimeString(pTime);
    }
    // Constructor complete
    karabo::data::DateTimeString dts2 =
          karabo::data::DateTimeString(expectedDate, expectedTime, expectedFractionalSecond, expectedTimeZone);


    // Validations
    if (writeToClog) std::clog << "DT => " << dts.getDate() << " == " << expectedDate << std::endl;
    EXPECT_TRUE(dts.getDate() == expectedDate);
    EXPECT_TRUE(dts2.getDate() == expectedDate);

    if (writeToClog) std::clog << "Time => " << dts.getTime() << " == " << expectedTime << std::endl;
    EXPECT_TRUE(dts.getTime() == expectedTime);
    EXPECT_TRUE(dts2.getTime() == expectedTime);

    if (writeToClog)
        std::clog << "FSec <std::string> => " << dts.getFractionalSeconds<std::string>()
                  << " == " << expectedFractionalSecond << std::endl;
    EXPECT_TRUE(dts.getFractionalSeconds<std::string>() == expectedFractionalSecond);
    EXPECT_TRUE(dts2.getFractionalSeconds<std::string>() == expectedFractionalSecond);
    if (writeToClog)
        std::clog << "FSec <unsigned long long> => " << dts.getFractionalSeconds<std::string>()
                  << " == " << expectedFractionalSecond << std::endl;
    EXPECT_TRUE(dts.getFractionalSeconds<unsigned long long>() ==
                boost::lexical_cast<unsigned long long>(expectedFractionalSecond));
    EXPECT_TRUE(dts2.getFractionalSeconds<unsigned long long>() ==
                boost::lexical_cast<unsigned long long>(expectedFractionalSecond));

    if (writeToClog) std::clog << "TZ => " << dts.getTimeZone() << " == " << expectedTimeZone << std::endl;
    EXPECT_TRUE(dts.getTimeZone() == expectedTimeZone);
    EXPECT_TRUE(dts2.getTimeZone() == expectedTimeZone);

    if (writeToClog) std::clog << "DateAndTime => " << dts.getDateTime() << " == " << expectedDateTime << std::endl;
    EXPECT_TRUE(dts.getDateTime() == expectedDateTime);
    EXPECT_TRUE(dts2.getDateTime() == expectedDateTime);

    std::string secondsSinceEpoch = boost::lexical_cast<std::string>(dts.getSecondsSinceEpoch());
    std::string secondsSinceEpoch2 = boost::lexical_cast<std::string>(dts2.getSecondsSinceEpoch());
    if (writeToClog)
        std::clog << "SecondsSinceEpoch => " << secondsSinceEpoch << " == " << expectedSecondsSinceEpoch << std::endl;
    EXPECT_TRUE(secondsSinceEpoch == expectedSecondsSinceEpoch);
    EXPECT_TRUE(secondsSinceEpoch2 == expectedSecondsSinceEpoch);

    return;
}


TEST(TestDateTimeString, testConstructors) {
    // Validate "empty" constructor
    validateConstructor("", "19700101", "000000", "000000000000000000", "+0000", "19700101T000000",
                        boost::lexical_cast<std::string>(0ULL));

    unsigned long long expectedSecondsSinceEpochULL = 475111250;
    unsigned long long expectedSecondsSinceEpochMinos7hULL = 475136450;   // 475111250 + (60*60*7)
    unsigned long long expectedSecondsSinceEpochPlus3h30mULL = 475098650; // 475111250 - (60*60*3.5)

    std::string expectedSecondsSinceEpoch = boost::lexical_cast<std::string>(expectedSecondsSinceEpochULL);
    std::string expectedSecondsSinceEpochMinos7h =
          boost::lexical_cast<std::string>(expectedSecondsSinceEpochMinos7hULL);
    std::string expectedSecondsSinceEpochPlus3h30m =
          boost::lexical_cast<std::string>(expectedSecondsSinceEpochPlus3h30mULL);

    // Validate the ISO8601 "string" constructor
    // Validate Extended strings
    std::string expectedDateExt = "1985-01-20";
    std::string expectedTimeExt = "23:20:50";
    std::string expectedDateAndTimeExt = "1985-01-20T23:20:50";
    // validateConstructor("1985-01-20T23:20:50.789333123456789123", expectedDateExt, expectedTimeExt,
    // "789333123456789123", "", expectedDateAndTimeExt, expectedSecondsSinceEpoch);
    validateConstructor("1985-01-20T23:20:50", expectedDateExt, expectedTimeExt, "000000000000000000", "",
                        expectedDateAndTimeExt, expectedSecondsSinceEpoch);
    validateConstructor("1985-01-20T23:20:50,123", expectedDateExt, expectedTimeExt, "123000000000000000", "",
                        expectedDateAndTimeExt, expectedSecondsSinceEpoch);
    validateConstructor("1985-01-20T23:20:50.123", expectedDateExt, expectedTimeExt, "123000000000000000", "",
                        expectedDateAndTimeExt, expectedSecondsSinceEpoch);
    validateConstructor("1985-01-20T23:20:50.123z", expectedDateExt, expectedTimeExt, "123000000000000000", "Z",
                        expectedDateAndTimeExt, expectedSecondsSinceEpoch);
    validateConstructor("1985-01-20T23:20:50z", expectedDateExt, expectedTimeExt, "000000000000000000", "Z",
                        expectedDateAndTimeExt, expectedSecondsSinceEpoch);
    validateConstructor("1985-01-20T23:20:50Z", expectedDateExt, expectedTimeExt, "000000000000000000", "Z",
                        expectedDateAndTimeExt, expectedSecondsSinceEpoch);
    validateConstructor("1985-01-20T23:20:50+00:00", expectedDateExt, expectedTimeExt, "000000000000000000", "+00:00",
                        expectedDateAndTimeExt, expectedSecondsSinceEpoch);
    //
    validateConstructor("1985-01-20T23:20:50-07:00", expectedDateExt, expectedTimeExt, "000000000000000000", "-07:00",
                        expectedDateAndTimeExt, expectedSecondsSinceEpochMinos7h);
    validateConstructor("1985-01-20T23:20:50+03:30", expectedDateExt, expectedTimeExt, "000000000000000000", "+03:30",
                        expectedDateAndTimeExt, expectedSecondsSinceEpochPlus3h30m);

    // Validate Compact strings
    std::string expectedDateCom = "19850120";
    std::string expectedTimeCom = "232050";
    std::string expectedDateAndTimeCom = "19850120T232050";
    validateConstructor("19850120T232050.789333123456789123", expectedDateCom, expectedTimeCom, "789333123456789123",
                        "", expectedDateAndTimeCom, expectedSecondsSinceEpoch);
    validateConstructor("19850120T232050", expectedDateCom, expectedTimeCom, "000000000000000000", "",
                        expectedDateAndTimeCom, expectedSecondsSinceEpoch);
    validateConstructor("19850120T232050,123", expectedDateCom, expectedTimeCom, "123000000000000000", "",
                        expectedDateAndTimeCom, expectedSecondsSinceEpoch);
    validateConstructor("19850120T232050.123", expectedDateCom, expectedTimeCom, "123000000000000000", "",
                        expectedDateAndTimeCom, expectedSecondsSinceEpoch);
    validateConstructor("19850120T232050.123z", expectedDateCom, expectedTimeCom, "123000000000000000", "Z",
                        expectedDateAndTimeCom, expectedSecondsSinceEpoch);
    validateConstructor("19850120T232050z", expectedDateCom, expectedTimeCom, "000000000000000000", "Z",
                        expectedDateAndTimeCom, expectedSecondsSinceEpoch);
    validateConstructor("19850120T232050Z", expectedDateCom, expectedTimeCom, "000000000000000000", "Z",
                        expectedDateAndTimeCom, expectedSecondsSinceEpoch);
    validateConstructor("19850120T232050+0000", expectedDateCom, expectedTimeCom, "000000000000000000", "+0000",
                        expectedDateAndTimeCom, expectedSecondsSinceEpoch);
    //
    validateConstructor("19850120T232050-0700", expectedDateCom, expectedTimeCom, "000000000000000000", "-0700",
                        expectedDateAndTimeCom, expectedSecondsSinceEpochMinos7h);
    validateConstructor("19850120T232050+0330", expectedDateCom, expectedTimeCom, "000000000000000000", "+0330",
                        expectedDateAndTimeCom, expectedSecondsSinceEpochPlus3h30m);
}


static bool isValidIso8601(const std::string& pTimeStr) {
    karabo::data::DateTimeString dts = karabo::data::DateTimeString();
    return dts.isStringValidIso8601(pTimeStr);
}


TEST(TestDateTimeString, isStringValidIso8601) {
    /*
     * Strings that should be valid
     */

    /*
     * Calendar date
     */
    // Complete representation


    EXPECT_TRUE(isValidIso8601("19850412") == true);   // Basic format: YYYYMMDD
    EXPECT_TRUE(isValidIso8601("1985-04-12") == true); // Extended format: YYYY-MM-DD

    // Representations with reduced precision
    // A specific month
    EXPECT_TRUE(isValidIso8601("1985-04") == true); // Basic format: YYYY-MM
    // A specific year
    EXPECT_TRUE(isValidIso8601("1985") == true); // Basic format: YYYY
    // A specific century
    EXPECT_TRUE(isValidIso8601("19") == true); // Basic format: YY

    // Truncated representations
    // A specific date in the implied century
    EXPECT_TRUE(isValidIso8601("850412") == true);   // Basic format: YYMMDD
    EXPECT_TRUE(isValidIso8601("85-04-12") == true); // Extended format: YY-MM-DD
    // A specific year and month in the implied century
    EXPECT_TRUE(isValidIso8601("-8504") == true);  // Basic format: -YYMM
    EXPECT_TRUE(isValidIso8601("-85-04") == true); // Extended format: -YY-MM
    // A specific year in the implied century
    EXPECT_TRUE(isValidIso8601("-85") == true); // Basic format: -YY
    // A specific day of a month in the implied year
    EXPECT_TRUE(isValidIso8601("--0412") == true);  // Basic format: --MMDD
    EXPECT_TRUE(isValidIso8601("--04-12") == true); // Extended format: --MM-DD
    // A specific month in the implied year
    EXPECT_TRUE(isValidIso8601("--04") == true); // Basic format: --MM
    // A specific day in the implied month
    EXPECT_TRUE(isValidIso8601("---12") == true); // Basic format: ---DD

    // Expanded representations
    //  Only if agreed it's possible to increase the number of years digits (i.e. 1)
    // A specific day
    EXPECT_TRUE(isValidIso8601("+019850412") == false);   // Basic format: ±YYYYYMMDD
    EXPECT_TRUE(isValidIso8601("+01985-04-12") == false); // Extended format: ±YYYYY-MM-DD
    // A specific month
    EXPECT_TRUE(isValidIso8601("+01985-04") == false); // Basic format: ±YYYYY-MM
    // A specific year
    EXPECT_TRUE(isValidIso8601("+01985") == false); // Basic format: ±YYYYY
    // A specific century
    EXPECT_TRUE(isValidIso8601("+019") == false); // Basic format: ±YYY


    /*
     * Ordinal date
     */
    // Complete representation
    EXPECT_TRUE(isValidIso8601("1985102") == true);  // Basic format: YYYYDDD
    EXPECT_TRUE(isValidIso8601("1985-102") == true); // Extended format: YYYY-DDD

    // Truncated representations
    // A specific year and day in the implied century
    EXPECT_TRUE(isValidIso8601("85102") == true);  // Basic format: YYDDD
    EXPECT_TRUE(isValidIso8601("85-102") == true); // Extended format: YY-DDD
    // Day only in the implied year
    EXPECT_TRUE(isValidIso8601("-102") == true); // Basic format: -DDD

    // Expanded representations
    //  Only if agreed it's possible to increase the number of years digits (i.e. 1)
    // A specific day
    EXPECT_TRUE(isValidIso8601("+01985102") == false);  // Basic format: ±YYYYYDDD
    EXPECT_TRUE(isValidIso8601("+01985-102") == false); // Extended format: ±YYYYY-DDD


    /*
     * Week date
     */
    // Complete representation
    EXPECT_TRUE(isValidIso8601("1985W155") == true);   // Basic format: YYYYWwwD
    EXPECT_TRUE(isValidIso8601("1985-W15-5") == true); // Extended format: YYYY-Www-D

    // Representation with reduced precision
    // A specific week
    EXPECT_TRUE(isValidIso8601("1985W15") == true);  // Basic format: YYYYWww
    EXPECT_TRUE(isValidIso8601("1985-W15") == true); // Extended format: YYYY-Www

    // Truncated representations
    // Year, week and day in the implied century
    EXPECT_TRUE(isValidIso8601("85W155") == true);   // Basic format: YYWwwD
    EXPECT_TRUE(isValidIso8601("85-W15-5") == true); // Extended format: YY-Www-D
    // Year and week only in the implied century
    EXPECT_TRUE(isValidIso8601("85W15") == true);  // Basic format: YYWww
    EXPECT_TRUE(isValidIso8601("85-W15") == true); // Extended format: YY-Www
    // Year of the implied decade, week and day only
    EXPECT_TRUE(isValidIso8601("-5W155") == true);   // Basic format: -YWwwD
    EXPECT_TRUE(isValidIso8601("-5-W15-5") == true); // Extended format: -Y-Www-D
    // Year of the implied decade and week only
    EXPECT_TRUE(isValidIso8601("-5W15") == true);  // Basic format: -YWww
    EXPECT_TRUE(isValidIso8601("-5-W15") == true); // Extended format: -Y-Www
    // Week and day only of the implied year
    EXPECT_TRUE(isValidIso8601("-W155") == true);  // Basic format: -WwwD
    EXPECT_TRUE(isValidIso8601("-W15-5") == true); // Extended format: -Www-D
    // Week only of the implied year
    EXPECT_TRUE(isValidIso8601("-W15") == true); // Basic format: -Www
    // Day only of the implied week
    EXPECT_TRUE(isValidIso8601("-W-5") == true); // Basic format: -W-D

    // Expanded representations
    //  Only if agreed it's possible to increase the number of years digits (i.e. 1)
    // A specific day
    EXPECT_TRUE(isValidIso8601("+01985W155") == false);   // Basic format: ±YYYYYWwwD
    EXPECT_TRUE(isValidIso8601("+01985-W15-5") == false); // Extended format: ±YYYYY-Www-D
    // A specific week
    EXPECT_TRUE(isValidIso8601("+01985W15") == false);  // Basic format: ±YYYYYWww
    EXPECT_TRUE(isValidIso8601("+01985-W15") == false); // Extended format: ±YYYYY-Www


    /*
     * Time of the day
     */
    // Local time of the day
    // Complete representation
    EXPECT_TRUE(isValidIso8601("232050") == true);              // Basic format: hhmmss
    EXPECT_TRUE(isValidIso8601("19850120T23") == true);         // Basic format: YYYYMMDDThh
    EXPECT_TRUE(isValidIso8601("19850120T2320") == true);       // Basic format: YYYYMMDDThhmm
    EXPECT_TRUE(isValidIso8601("19850120T232050") == true);     // Basic format: YYYYMMDDThhmmss
    EXPECT_TRUE(isValidIso8601("23:20:50") == true);            // Extended format: hh:mm:ss
    EXPECT_TRUE(isValidIso8601("1985-01-20T23") == true);       // Basic format: YYYY-MM-DDThh
    EXPECT_TRUE(isValidIso8601("1985-01-20T23:20") == true);    // Basic format: YYYY-MM-DDThh:mm
    EXPECT_TRUE(isValidIso8601("1985-01-20T23:20:50") == true); // Basic format: YYYY-MM-DDThh:mm:ss

    // Representations with reduced precision
    // A specific hour and minute
    EXPECT_TRUE(isValidIso8601("2320") == true);             // Basic format: hhmm
    EXPECT_TRUE(isValidIso8601("19850120T2320") == true);    // Basic format: YYYYMMDDThhmm
    EXPECT_TRUE(isValidIso8601("23:20") == true);            // Extended format: hh:mm
    EXPECT_TRUE(isValidIso8601("1985-01-20T23:20") == true); // Extended format: YYYY-MM-DDThh:mm
    // A specific hour
    EXPECT_TRUE(isValidIso8601("23") == true);          // Basic format: hh
    EXPECT_TRUE(isValidIso8601("19850120T23") == true); // Basic format: YYYYMMDDThh

    // Representation of decimal fractions
    // A specific hour, minute and second and a decimal fraction of the second
    EXPECT_TRUE(isValidIso8601("232050,5") == true);              // Basic format: hhmmss,ss
    EXPECT_TRUE(isValidIso8601("19850120T23,5") == true);         // Basic format: YYYYMMDDThh,ss
    EXPECT_TRUE(isValidIso8601("19850120T2320,5") == true);       // Basic format: YYYYMMDDThhmm,ss
    EXPECT_TRUE(isValidIso8601("19850120T232050,5") == true);     // Basic format: YYYYMMDDThhmmss,ss
    EXPECT_TRUE(isValidIso8601("23:20:50,5") == true);            // Extended format: hh:mm:ss,ss
    EXPECT_TRUE(isValidIso8601("1985-01-20T23,5") == true);       // Extended format: YYYY-MM-DDThh,ss
    EXPECT_TRUE(isValidIso8601("1985-01-20T23:20,5") == true);    // Extended format: YYYY-MM-DDThh:mm,ss
    EXPECT_TRUE(isValidIso8601("1985-01-20T23:20:50,5") == true); // Extended format: YYYY-MM-DDThh:mm:ss,ss
    // A specific hour and minute and a decimal fraction of the minute
    EXPECT_TRUE(isValidIso8601("2320,8") == true);             // Basic format: hhmm,mm
    EXPECT_TRUE(isValidIso8601("19850120T2320,8") == true);    // Basic format: YYYYMMDDThhmm,mm
    EXPECT_TRUE(isValidIso8601("23:20,8") == true);            // Extended format: hh:mm,mm
    EXPECT_TRUE(isValidIso8601("1985-01-20T23:20,8") == true); // Extended format: YYYY-MM-DDThh:mm,mm
    // A specific hour and a decimal fraction of the hour
    EXPECT_TRUE(isValidIso8601("23,3") == true);          // Basic format: hh,hh
    EXPECT_TRUE(isValidIso8601("19850120T23,3") == true); // Basic format: YYYYMMDDThh,hh

    // Truncated representations
    // A specific minute and second of the implied hour
    EXPECT_TRUE(isValidIso8601("-2050") == true);  // Basic format: -mmss
    EXPECT_TRUE(isValidIso8601("-20:50") == true); // Extended format: -mm:ss
    // A specific minute of the implied hour
    EXPECT_TRUE(isValidIso8601("-20") == true); // Basic format: -mm
    // A specific second of the implied minute
    EXPECT_TRUE(isValidIso8601("-50") == true); // Basic format: -ss
    // A specific minute and second of the implied hour and a decimal fraction of the second
    EXPECT_TRUE(isValidIso8601("-2050,5") == true);  // Basic format: -mmss,s
    EXPECT_TRUE(isValidIso8601("-20:50,5") == true); // Extended format: -mm:ss,s
    // A specific minute of the implied hour and a decimal fraction of the minute
    EXPECT_TRUE(isValidIso8601("-20,8") == true); // Basic format: -mm,m
    // A specific second of the implied minute and a decimal fraction of the second
    EXPECT_TRUE(isValidIso8601("--50,5") == true); // Basic format: --ss,s

    // Midnight
    EXPECT_TRUE(isValidIso8601("19850120T240000") == true);     // Basic format: YYYYMMDDThhmmss
    EXPECT_TRUE(isValidIso8601("19850120T000000") == true);     // Basic format: YYYYMMDDThhmmss
    EXPECT_TRUE(isValidIso8601("1985-01-20T24:00:00") == true); // Extended format: YYYY-MM-DDThh:mm:ss
    EXPECT_TRUE(isValidIso8601("1985-01-20T00:00:00") == true); // Extended format: YYYY-MM-DDThh:mm:ss

    // Coordinated Universal Time (UTC)
    EXPECT_TRUE(isValidIso8601("232030Z") == true);   // Basic format: hhmmssZ
    EXPECT_TRUE(isValidIso8601("23:20:30Z") == true); // Extended format: hh:mm:ssZ
    EXPECT_TRUE(isValidIso8601("2320Z") == true);     // Basic format: hhmmZ
    EXPECT_TRUE(isValidIso8601("23:20Z") == true);    // Extended format: hh:mmZ
    EXPECT_TRUE(isValidIso8601("23Z") == true);       // Basic format: hhZ

    // Local time and Coordinated Universal Time
    // Difference between local time and Coordinated Universal Time
    EXPECT_TRUE(isValidIso8601("+0100") == true);  // Basic format: ±hhmm
    EXPECT_TRUE(isValidIso8601("+01:00") == true); // Extended format: ±hh:mm
    EXPECT_TRUE(isValidIso8601("+01") == true);    // Basic format: ±hh
    // Local time and the difference with Coordinated Universal Time
    EXPECT_TRUE(isValidIso8601("152746+0100") == true);    // Basic format: hhmmss±hhmm
    EXPECT_TRUE(isValidIso8601("152746-0500") == true);    // Basic format: hhmmss±hhmm
    EXPECT_TRUE(isValidIso8601("152746+01") == true);      // Basic format: hhmmss±hh
    EXPECT_TRUE(isValidIso8601("152746-05") == true);      // Basic format: hhmmss±hh
    EXPECT_TRUE(isValidIso8601("15:27:46+01:00") == true); // Extended format: hh:mm:ss±hh:mm
    EXPECT_TRUE(isValidIso8601("15:27:46-05:00") == true); // Extended format: hh:mm:ss±hh:mm
    EXPECT_TRUE(isValidIso8601("15:27:46+01") == true);    // Extended format: hh:mm:ss±hh
    EXPECT_TRUE(isValidIso8601("15:27:46-05") == true);    // Extended format: hh:mm:ss±hh

    // Combinations of date and time of the day
    // Complete representation
    EXPECT_TRUE(isValidIso8601("19850412T101530") == true);           // Basic format: YYYYMMDDThhmmss
    EXPECT_TRUE(isValidIso8601("19850412T101530Z") == true);          // Basic format: YYYYMMDDThhmmssZ
    EXPECT_TRUE(isValidIso8601("19850412T101530+0400") == true);      // Basic format: YYYYMMDDThhmmss±hhmm
    EXPECT_TRUE(isValidIso8601("19850412T101530+04") == true);        // Basic format: YYYYMMDDThhmmss±hh
    EXPECT_TRUE(isValidIso8601("1985-04-12T10:15:30") == true);       // Extended format: YYYY-MM-DDThh:mm:ss
    EXPECT_TRUE(isValidIso8601("1985-04-12T10:15:30Z") == true);      // Extended format: YYYY-MM-DDThh:mm:ssZ
    EXPECT_TRUE(isValidIso8601("1985-04-12T10:15:30+04:00") == true); // Extended format: YYYY-MM-DDThh:mm:ss±hh:mm
    EXPECT_TRUE(isValidIso8601("1985-04-12T10:15:30+04") == true);    // Extended format: YYYY-MM-DDThh:mm:ss±hh

    // Representations other than complete
    // Calendar date and local time of the day
    EXPECT_TRUE(isValidIso8601("19850412T1015") == true);    // Basic format: YYYYMMDDThhmm
    EXPECT_TRUE(isValidIso8601("1985-04-12T10:15") == true); // Extended format: YYYY-MM-DDThh:mm
    // Ordinal date and coordinated universal time
    EXPECT_TRUE(isValidIso8601("1985102T1015Z") == true);   // Basic format: YYYYDDDThhmmZ
    EXPECT_TRUE(isValidIso8601("1985-102T10:15Z") == true); // Extended format: YYYY-DDDThh:mmZ
    // Week date and local time and the difference with UTC
    EXPECT_TRUE(isValidIso8601("1985W155T1015+0400") == true);  // Basic format: YYYYWwwDThhmm±hhmm
    EXPECT_TRUE(isValidIso8601("1985-W15-5T10:15+04") == true); // Extended format: YYYY-Www-DThh:mm±hh


    // Other tests
    EXPECT_TRUE(isValidIso8601("2009-12T12:34:22") == true);
    EXPECT_TRUE(isValidIso8601("2009") == true);
    EXPECT_TRUE(isValidIso8601("2009-05-19") == true);
    EXPECT_TRUE(isValidIso8601("20090519") == true);
    EXPECT_TRUE(isValidIso8601("2009123") == true);
    EXPECT_TRUE(isValidIso8601("2009-05") == true);
    EXPECT_TRUE(isValidIso8601("2009-123") == true);
    EXPECT_TRUE(isValidIso8601("2009-222") == true);
    EXPECT_TRUE(isValidIso8601("2009-001") == true);
    EXPECT_TRUE(isValidIso8601("2009-W01-1") == true);
    EXPECT_TRUE(isValidIso8601("2009-W51-1") == true);
    EXPECT_TRUE(isValidIso8601("2009-W511") == true);
    EXPECT_TRUE(isValidIso8601("2009-W33") == true);
    EXPECT_TRUE(isValidIso8601("2009W511") == true);
    EXPECT_TRUE(isValidIso8601("2009-05-19") == true);
    EXPECT_TRUE(isValidIso8601("2009-05-19T00:00:00") == true);
    EXPECT_TRUE(isValidIso8601("2009-05-19T14:31:00") == true);
    EXPECT_TRUE(isValidIso8601("2009-05-19T14:39:22") == true);
    EXPECT_TRUE(isValidIso8601("2009-05-19T14:39Z") == true);
    EXPECT_TRUE(isValidIso8601("2009-05-19T14:39:22Z") == true);
    EXPECT_TRUE(isValidIso8601("2009-W21-2") == true);
    EXPECT_TRUE(isValidIso8601("2009-W21-2T01:22") == true);
    EXPECT_TRUE(isValidIso8601("2009-W21-2T01:22:59") == true);
    EXPECT_TRUE(isValidIso8601("2009-139") == true);
    EXPECT_TRUE(isValidIso8601("2009-05-19T14:39:22-06:00") == true);
    EXPECT_TRUE(isValidIso8601("2009-05-19T14:39:22+06:00") == true);
    EXPECT_TRUE(isValidIso8601("20090519T143922+0600") == true);
    EXPECT_TRUE(isValidIso8601("2009-05-19T14:39:22+06:00") == true);
    EXPECT_TRUE(isValidIso8601("2009-05-19T14:39:22-01") == true);
    EXPECT_TRUE(isValidIso8601("20090621T0545Z") == true);
    EXPECT_TRUE(isValidIso8601("20090621T054501Z") == true);
    EXPECT_TRUE(isValidIso8601("2007-04-06T00:00") == true);
    EXPECT_TRUE(isValidIso8601("2007-04-06T00:00:00") == true);
    EXPECT_TRUE(isValidIso8601("2007-04-05T24:00") == true);
    EXPECT_TRUE(isValidIso8601("2007-04-05T23:00:00") == true);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16:23:48.5") == true);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16:23:48,444") == true);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16:23:48,3-06:00") == true);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16:23.4") == true);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16:23:33.4") == true);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16:23,25") == true);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16:23:44,25") == true);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16:23.33+06:00") == true);
    EXPECT_TRUE(isValidIso8601("20100218T162352.33+0600") == true);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16.23334444") == true);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16:17:18.23334444") == true);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16,2283") == true);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16:17:18,2283") == true);
    EXPECT_TRUE(isValidIso8601("20090519T143922.500") == true);
    EXPECT_TRUE(isValidIso8601("20090519T1439,55") == true);
    EXPECT_TRUE(isValidIso8601("2009-05-19T14:39:22.500") == true);
    EXPECT_TRUE(isValidIso8601("2009-05-19T14:39,55") == true);

    EXPECT_TRUE(isValidIso8601("1994-11-05T08:15:30-05:00") == true);
    EXPECT_TRUE(isValidIso8601("1994-11-05T13:15:30Z") == true);
    EXPECT_TRUE(isValidIso8601("155300+0500") == true);
    EXPECT_TRUE(isValidIso8601("155300Z") == true);
    EXPECT_TRUE(isValidIso8601("155300+0500") == true);
    EXPECT_TRUE(isValidIso8601("20080915T155300") == true);
    EXPECT_TRUE(isValidIso8601("20080915T155300+0500") == true);
    EXPECT_TRUE(isValidIso8601("20080915T155300Z") == true);
    EXPECT_TRUE(isValidIso8601("20080915") == true);
    EXPECT_TRUE(isValidIso8601("2008-09-15") == true);
    EXPECT_TRUE(isValidIso8601("15:53:00.322348") == true);
    EXPECT_TRUE(isValidIso8601("15:53:00+05:00") == true);
    EXPECT_TRUE(isValidIso8601("2008-09-15T15:53:00") == true);
    EXPECT_TRUE(isValidIso8601("2008-09-15T15:53:00+05:00") == true);
    EXPECT_TRUE(isValidIso8601("2008-09-15") == true);
    EXPECT_TRUE(isValidIso8601("20121225T132536.789333123456789123") == true);
    EXPECT_TRUE(isValidIso8601("200905") == true); // Year + Month
    EXPECT_TRUE(isValidIso8601("200913") == true); // Year + Day
    EXPECT_TRUE(isValidIso8601("2007-04-05T23:50") == true);
    EXPECT_TRUE(isValidIso8601("2009-0519") == true); // Year - Time zone
    EXPECT_TRUE(isValidIso8601("200912-01") == true); // Year + Month - Time zone

    // Strings that shouldn't be valid

    // This String should be false, but for convenience, it will be ignored.
    EXPECT_TRUE(isValidIso8601("T200159") == true);
    EXPECT_TRUE(isValidIso8601("T20:01:59") == true);

    //
    EXPECT_TRUE(isValidIso8601("Wed Feb 26 17:11:26 CET 2014") == false);
    EXPECT_TRUE(isValidIso8601("2009367") == false);
    EXPECT_TRUE(isValidIso8601("2009-") == false);
    EXPECT_TRUE(isValidIso8601("2007-04-05T24:50") == false);
    EXPECT_TRUE(isValidIso8601("2009-000") == false);
    EXPECT_TRUE(isValidIso8601("2009-M511") == false);
    EXPECT_TRUE(isValidIso8601("2009M511") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19T14a39r") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19T14:39:22+0600") == false);
    EXPECT_TRUE(isValidIso8601("20090621T0545:01Z") == false);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16:23.33+0600") == false);
    EXPECT_TRUE(isValidIso8601("20100218T162352.33+06:00") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19T143922.500") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19T1439,55") == false);
    EXPECT_TRUE(isValidIso8601("20090519T14:39:22.500") == false);
    EXPECT_TRUE(isValidIso8601("20090519T14:39,55") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19T14:3924") == false);
    EXPECT_TRUE(isValidIso8601("2009-2519T") == false);
    EXPECT_TRUE(isValidIso8601("2009-2519") == false); // Year - Time zone that doesn't exist
    EXPECT_TRUE(isValidIso8601("200912-30") == false); // Year + Month - Time zone that doesn't exist
    EXPECT_TRUE(isValidIso8601("2009-05-1914:39") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19 14:") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19r14:39") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19 14a39a22") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19 14:39:22+06a00") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19 144922.500") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19T146922.500") == false);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16.5:23.35:48") == false);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16:23.35:48") == false);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16:23.35:48.45") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19 14.5.44") == false);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16:23.33.600") == false);
    EXPECT_TRUE(isValidIso8601("2010-02-18T16,25:23:48,444") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19 00:00:00") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19 14:31:00") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19 14:39:22") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19 14:39:22-06:00") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19 14:39:22+0600") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19 14:39:22-01") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19 143922.500") == false);
    EXPECT_TRUE(isValidIso8601("2009-05-19 1439,55") == false);

    EXPECT_TRUE(isValidIso8601("155300322348") == false);
    EXPECT_TRUE(isValidIso8601("T") == false);
    EXPECT_TRUE(isValidIso8601("20120120T") == false);
    EXPECT_TRUE(isValidIso8601("2012-01-20T") == false);
    EXPECT_TRUE(isValidIso8601("20080915 ") == false);
    EXPECT_TRUE(isValidIso8601(" 20080915") == false);

    EXPECT_TRUE(isValidIso8601("z") == false);
    EXPECT_TRUE(isValidIso8601("-z") == false);
    EXPECT_TRUE(isValidIso8601("z-") == false);
    EXPECT_TRUE(isValidIso8601("-z-") == false);
    EXPECT_TRUE(isValidIso8601("Z") == false);
    EXPECT_TRUE(isValidIso8601("-Z") == false);
    EXPECT_TRUE(isValidIso8601("Z-") == false);
    EXPECT_TRUE(isValidIso8601("-Z-") == false);

    EXPECT_TRUE(isValidIso8601("w") == false);
    EXPECT_TRUE(isValidIso8601("-w") == false);
    EXPECT_TRUE(isValidIso8601("-w") == false);
    EXPECT_TRUE(isValidIso8601("-w-") == false);
    EXPECT_TRUE(isValidIso8601("W") == false);
    EXPECT_TRUE(isValidIso8601("-W") == false);
    EXPECT_TRUE(isValidIso8601("-W") == false);
    EXPECT_TRUE(isValidIso8601("-W-") == false);

    EXPECT_TRUE(isValidIso8601("t") == false);
    EXPECT_TRUE(isValidIso8601("-t") == false);
    EXPECT_TRUE(isValidIso8601("-t") == false);
    EXPECT_TRUE(isValidIso8601("-t-") == false);
    EXPECT_TRUE(isValidIso8601("T") == false);
    EXPECT_TRUE(isValidIso8601("-T") == false);
    EXPECT_TRUE(isValidIso8601("-T") == false);
    EXPECT_TRUE(isValidIso8601("-T-") == false);

    EXPECT_TRUE(isValidIso8601("--") == false);
    EXPECT_TRUE(isValidIso8601("---") == false);
    EXPECT_TRUE(isValidIso8601("+") == false);
    EXPECT_TRUE(isValidIso8601(":") == false);
    EXPECT_TRUE(isValidIso8601(" ") == false);
    EXPECT_TRUE(isValidIso8601("-") == false);
    EXPECT_TRUE(isValidIso8601("") == false);
}


static bool isValidKaraboIso8601(const std::string& pTimeStr) {
    karabo::data::DateTimeString dts = karabo::data::DateTimeString();
    return dts.isStringKaraboValidIso8601(pTimeStr);
}


TEST(TestDateTimeString, isStringKaraboValidIso8601) {
    /*
     * Strings that should be valid
     */

    /*
     * Calendar date
     */
    // Complete representation
    EXPECT_TRUE(isValidKaraboIso8601("19850412") == false);   // Basic format: YYYYMMDD
    EXPECT_TRUE(isValidKaraboIso8601("1985-04-12") == false); // Extended format: YYYY-MM-DD

    // Representations with reduced precision
    // A specific month
    EXPECT_TRUE(isValidKaraboIso8601("1985-04") == false); // Basic format: YYYY-MM
    // A specific year
    EXPECT_TRUE(isValidKaraboIso8601("1985") == false); // Basic format: YYYY
    // A specific century
    EXPECT_TRUE(isValidKaraboIso8601("19") == false); // Basic format: YY

    // Truncated representations
    // A specific date in the implied century
    EXPECT_TRUE(isValidKaraboIso8601("850412") == false);   // Basic format: YYMMDD
    EXPECT_TRUE(isValidKaraboIso8601("85-04-12") == false); // Extended format: YY-MM-DD
    // A specific year and month in the implied century
    EXPECT_TRUE(isValidKaraboIso8601("-8504") == false);  // Basic format: -YYMM
    EXPECT_TRUE(isValidKaraboIso8601("-85-04") == false); // Extended format: -YY-MM
    // A specific year in the implied century
    EXPECT_TRUE(isValidKaraboIso8601("-85") == false); // Basic format: -YY
    // A specific day of a month in the implied year
    EXPECT_TRUE(isValidKaraboIso8601("--0412") == false);  // Basic format: --MMDD
    EXPECT_TRUE(isValidKaraboIso8601("--04-12") == false); // Extended format: --MM-DD
    // A specific month in the implied year
    EXPECT_TRUE(isValidKaraboIso8601("--04") == false); // Basic format: --MM
    // A specific day in the implied month
    EXPECT_TRUE(isValidKaraboIso8601("---12") == false); // Basic format: ---DD

    // Expanded representations
    //  Only if agreed it's possible to increase the number of years digits (i.e. 1)
    // A specific day
    EXPECT_TRUE(isValidKaraboIso8601("+019850412") == false);   // Basic format: ±YYYYYMMDD
    EXPECT_TRUE(isValidKaraboIso8601("+01985-04-12") == false); // Extended format: ±YYYYY-MM-DD
    // A specific month
    EXPECT_TRUE(isValidKaraboIso8601("+01985-04") == false); // Basic format: ±YYYYY-MM
    // A specific year
    EXPECT_TRUE(isValidKaraboIso8601("+01985") == false); // Basic format: ±YYYYY
    // A specific century
    EXPECT_TRUE(isValidKaraboIso8601("+019") == false); // Basic format: ±YYY


    /*
     * Ordinal date
     */
    // Complete representation
    EXPECT_TRUE(isValidKaraboIso8601("1985102") == false);  // Basic format: YYYYDDD
    EXPECT_TRUE(isValidKaraboIso8601("1985-102") == false); // Extended format: YYYY-DDD

    // Truncated representations
    // A specific year and day in the implied century
    EXPECT_TRUE(isValidKaraboIso8601("85102") == false);  // Basic format: YYDDD
    EXPECT_TRUE(isValidKaraboIso8601("85-102") == false); // Extended format: YY-DDD
    // Day only in the implied year
    EXPECT_TRUE(isValidKaraboIso8601("-102") == false); // Basic format: -DDD

    // Expanded representations
    //  Only if agreed it's possible to increase the number of years digits (i.e. 1)
    // A specific day
    EXPECT_TRUE(isValidKaraboIso8601("+01985102") == false);  // Basic format: ±YYYYYDDD
    EXPECT_TRUE(isValidKaraboIso8601("+01985-102") == false); // Extended format: ±YYYYY-DDD


    /*
     * Week date
     */
    // Complete representation
    EXPECT_TRUE(isValidKaraboIso8601("1985W155") == false);   // Basic format: YYYYWwwD
    EXPECT_TRUE(isValidKaraboIso8601("1985-W15-5") == false); // Extended format: YYYY-Www-D

    // Representation with reduced precision
    // A specific week
    EXPECT_TRUE(isValidKaraboIso8601("1985W15") == false);  // Basic format: YYYYWww
    EXPECT_TRUE(isValidKaraboIso8601("1985-W15") == false); // Extended format: YYYY-Www

    // Truncated representations
    // Year, week and day in the implied century
    EXPECT_TRUE(isValidKaraboIso8601("85W155") == false);   // Basic format: YYWwwD
    EXPECT_TRUE(isValidKaraboIso8601("85-W15-5") == false); // Extended format: YY-Www-D
    // Year and week only in the implied century
    EXPECT_TRUE(isValidKaraboIso8601("85W15") == false);  // Basic format: YYWww
    EXPECT_TRUE(isValidKaraboIso8601("85-W15") == false); // Extended format: YY-Www
    // Year of the implied decade, week and day only
    EXPECT_TRUE(isValidKaraboIso8601("-5W155") == false);   // Basic format: -YWwwD
    EXPECT_TRUE(isValidKaraboIso8601("-5-W15-5") == false); // Extended format: -Y-Www-D
    // Year of the implied decade and week only
    EXPECT_TRUE(isValidKaraboIso8601("-5W15") == false);  // Basic format: -YWww
    EXPECT_TRUE(isValidKaraboIso8601("-5-W15") == false); // Extended format: -Y-Www
    // Week and day only of the implied year
    EXPECT_TRUE(isValidKaraboIso8601("-W155") == false);  // Basic format: -WwwD
    EXPECT_TRUE(isValidKaraboIso8601("-W15-5") == false); // Extended format: -Www-D
    // Week only of the implied year
    EXPECT_TRUE(isValidKaraboIso8601("-W15") == false); // Basic format: -Www
    // Day only of the implied week
    EXPECT_TRUE(isValidKaraboIso8601("-W-5") == false); // Basic format: -W-D

    // Expanded representations
    //  Only if agreed it's possible to increase the number of years digits (i.e. 1)
    // A specific day
    EXPECT_TRUE(isValidKaraboIso8601("+01985W155") == false);   // Basic format: ±YYYYYWwwD
    EXPECT_TRUE(isValidKaraboIso8601("+01985-W15-5") == false); // Extended format: ±YYYYY-Www-D
    // A specific week
    EXPECT_TRUE(isValidKaraboIso8601("+01985W15") == false);  // Basic format: ±YYYYYWww
    EXPECT_TRUE(isValidKaraboIso8601("+01985-W15") == false); // Extended format: ±YYYYY-Www


    /*
     * Time of the day
     */
    // Local time of the day
    // Complete representation
    EXPECT_TRUE(isValidKaraboIso8601("232050") == false);             // Basic format: hhmmss
    EXPECT_TRUE(isValidKaraboIso8601("19850120T23") == false);        // Basic format: YYYYMMDDThh
    EXPECT_TRUE(isValidKaraboIso8601("19850120T2320") == false);      // Basic format: YYYYMMDDThhmm
    EXPECT_TRUE(isValidKaraboIso8601("19850120T232050") == true);     // Basic format: YYYYMMDDThhmmss
    EXPECT_TRUE(isValidKaraboIso8601("23:20:50") == false);           // Extended format: hh:mm:ss
    EXPECT_TRUE(isValidKaraboIso8601("1985-01-20T23") == false);      // Basic format: YYYY-MM-DDThh
    EXPECT_TRUE(isValidKaraboIso8601("1985-01-20T23:20") == false);   // Basic format: YYYY-MM-DDThh:mm
    EXPECT_TRUE(isValidKaraboIso8601("1985-01-20T23:20:50") == true); // Basic format: YYYY-MM-DDThh:mm:ss

    // Representations with reduced precision
    // A specific hour and minute
    EXPECT_TRUE(isValidKaraboIso8601("2320") == false);             // Basic format: hhmm
    EXPECT_TRUE(isValidKaraboIso8601("19850120T2320") == false);    // Basic format: YYYYMMDDThhmm
    EXPECT_TRUE(isValidKaraboIso8601("23:20") == false);            // Extended format: hh:mm
    EXPECT_TRUE(isValidKaraboIso8601("1985-01-20T23:20") == false); // Extended format: YYYY-MM-DDThh:mm
    // A specific hour
    EXPECT_TRUE(isValidKaraboIso8601("23") == false);          // Basic format: hh
    EXPECT_TRUE(isValidKaraboIso8601("19850120T23") == false); // Basic format: YYYYMMDDThh

    // Representation of decimal fractions
    // A specific hour, minute and second and a decimal fraction of the second
    EXPECT_TRUE(isValidKaraboIso8601("232050,5") == false);             // Basic format: hhmmss,ss
    EXPECT_TRUE(isValidKaraboIso8601("19850120T23,5") == false);        // Basic format: YYYYMMDDThh,ss
    EXPECT_TRUE(isValidKaraboIso8601("19850120T2320,5") == false);      // Basic format: YYYYMMDDThhmm,ss
    EXPECT_TRUE(isValidKaraboIso8601("19850120T232050,5") == true);     // Basic format: YYYYMMDDThhmmss,ss
    EXPECT_TRUE(isValidKaraboIso8601("23:20:50,5") == false);           // Extended format: hh:mm:ss,ss
    EXPECT_TRUE(isValidKaraboIso8601("1985-01-20T23,5") == false);      // Extended format: YYYY-MM-DDThh,ss
    EXPECT_TRUE(isValidKaraboIso8601("1985-01-20T23:20,5") == false);   // Extended format: YYYY-MM-DDThh:mm,ss
    EXPECT_TRUE(isValidKaraboIso8601("1985-01-20T23:20:50,5") == true); // Extended format: YYYY-MM-DDThh:mm:ss,ss
    // A specific hour and minute and a decimal fraction of the minute
    EXPECT_TRUE(isValidKaraboIso8601("2320,8") == false);             // Basic format: hhmm,mm
    EXPECT_TRUE(isValidKaraboIso8601("19850120T2320,8") == false);    // Basic format: YYYYMMDDThhmm,mm
    EXPECT_TRUE(isValidKaraboIso8601("23:20,8") == false);            // Extended format: hh:mm,mm
    EXPECT_TRUE(isValidKaraboIso8601("1985-01-20T23:20,8") == false); // Extended format: YYYY-MM-DDThh:mm,mm
    // A specific hour and a decimal fraction of the hour
    EXPECT_TRUE(isValidKaraboIso8601("23,3") == false);          // Basic format: hh,hh
    EXPECT_TRUE(isValidKaraboIso8601("19850120T23,3") == false); // Basic format: YYYYMMDDThh,hh

    // Truncated representations
    // A specific minute and second of the implied hour
    EXPECT_TRUE(isValidKaraboIso8601("-2050") == false);  // Basic format: -mmss
    EXPECT_TRUE(isValidKaraboIso8601("-20:50") == false); // Extended format: -mm:ss
    // A specific minute of the implied hour
    EXPECT_TRUE(isValidKaraboIso8601("-20") == false); // Basic format: -mm
    // A specific second of the implied minute
    EXPECT_TRUE(isValidKaraboIso8601("-50") == false); // Basic format: -ss
    // A specific minute and second of the implied hour and a decimal fraction of the second
    EXPECT_TRUE(isValidKaraboIso8601("-2050,5") == false);  // Basic format: -mmss,s
    EXPECT_TRUE(isValidKaraboIso8601("-20:50,5") == false); // Extended format: -mm:ss,s
    // A specific minute of the implied hour and a decimal fraction of the minute
    EXPECT_TRUE(isValidKaraboIso8601("-20,8") == false); // Basic format: -mm,m
    // A specific second of the implied minute and a decimal fraction of the second
    EXPECT_TRUE(isValidKaraboIso8601("--50,5") == false); // Basic format: --ss,s

    // Midnight
    EXPECT_TRUE(isValidKaraboIso8601("19850120T240000") == false);     // Basic format: YYYYMMDDThhmmss
    EXPECT_TRUE(isValidKaraboIso8601("19850120T000000") == true);      // Basic format: YYYYMMDDThhmmss
    EXPECT_TRUE(isValidKaraboIso8601("1985-01-20T24:00:00") == false); // Extended format: YYYY-MM-DDThh:mm:ss
    EXPECT_TRUE(isValidKaraboIso8601("1985-01-20T00:00:00") == true);  // Extended format: YYYY-MM-DDThh:mm:ss

    // Coordinated Universal Time (UTC)
    EXPECT_TRUE(isValidKaraboIso8601("232030Z") == false);   // Basic format: hhmmssZ
    EXPECT_TRUE(isValidKaraboIso8601("23:20:30Z") == false); // Extended format: hh:mm:ssZ
    EXPECT_TRUE(isValidKaraboIso8601("2320Z") == false);     // Basic format: hhmmZ
    EXPECT_TRUE(isValidKaraboIso8601("23:20Z") == false);    // Extended format: hh:mmZ
    EXPECT_TRUE(isValidKaraboIso8601("23Z") == false);       // Basic format: hhZ

    // Local time and Coordinated Universal Time
    // Difference between local time and Coordinated Universal Time
    EXPECT_TRUE(isValidKaraboIso8601("+0100") == false);  // Basic format: ±hhmm
    EXPECT_TRUE(isValidKaraboIso8601("+01:00") == false); // Extended format: ±hh:mm
    EXPECT_TRUE(isValidKaraboIso8601("+01") == false);    // Basic format: ±hh
    // Local time and the difference with Coordinated Universal Time
    EXPECT_TRUE(isValidKaraboIso8601("152746+0100") == false);    // Basic format: hhmmss±hhmm
    EXPECT_TRUE(isValidKaraboIso8601("152746-0500") == false);    // Basic format: hhmmss±hhmm
    EXPECT_TRUE(isValidKaraboIso8601("152746+01") == false);      // Basic format: hhmmss±hh
    EXPECT_TRUE(isValidKaraboIso8601("152746-05") == false);      // Basic format: hhmmss±hh
    EXPECT_TRUE(isValidKaraboIso8601("15:27:46+01:00") == false); // Extended format: hh:mm:ss±hh:mm
    EXPECT_TRUE(isValidKaraboIso8601("15:27:46-05:00") == false); // Extended format: hh:mm:ss±hh:mm
    EXPECT_TRUE(isValidKaraboIso8601("15:27:46+01") == false);    // Extended format: hh:mm:ss±hh
    EXPECT_TRUE(isValidKaraboIso8601("15:27:46-05") == false);    // Extended format: hh:mm:ss±hh

    // Combinations of date and time of the day
    // Complete representation
    EXPECT_TRUE(isValidKaraboIso8601("19850412T101530") == true);      // Basic format: YYYYMMDDThhmmss
    EXPECT_TRUE(isValidKaraboIso8601("19850412T101530Z") == true);     // Basic format: YYYYMMDDThhmmssZ
    EXPECT_TRUE(isValidKaraboIso8601("19850412T101530+0400") == true); // Basic format: YYYYMMDDThhmmss±hhmm
    EXPECT_TRUE(isValidKaraboIso8601("19850412T101530+04") == false);  // Basic format: YYYYMMDDThhmmss±hh
    EXPECT_TRUE(isValidKaraboIso8601("1985-04-12T10:15:30") == true);  // Extended format: YYYY-MM-DDThh:mm:ss
    EXPECT_TRUE(isValidKaraboIso8601("1985-04-12T10:15:30Z") == true); // Extended format: YYYY-MM-DDThh:mm:ssZ
    EXPECT_TRUE(isValidKaraboIso8601("1985-04-12T10:15:30+04:00") ==
                true);                                                    // Extended format: YYYY-MM-DDThh:mm:ss±hh:mm
    EXPECT_TRUE(isValidKaraboIso8601("1985-04-12T10:15:30+04") == false); // Extended format: YYYY-MM-DDThh:mm:ss±hh
    EXPECT_TRUE(isValidKaraboIso8601("1985-04-12T10:15:30-04:00") ==
                true);                                                    // Extended format: YYYY-MM-DDThh:mm:ss±hh:mm
    EXPECT_TRUE(isValidKaraboIso8601("1985-04-12T10:15:30-04") == false); // Extended format: YYYY-MM-DDThh:mm:ss±hh

    // Representations other than complete
    // Calendar date and local time of the day
    EXPECT_TRUE(isValidKaraboIso8601("19850412T1015") == false);    // Basic format: YYYYMMDDThhmm
    EXPECT_TRUE(isValidKaraboIso8601("1985-04-12T10:15") == false); // Extended format: YYYY-MM-DDThh:mm
    // Ordinal date and coordinated universal time
    EXPECT_TRUE(isValidKaraboIso8601("1985102T1015Z") == false);   // Basic format: YYYYDDDThhmmZ
    EXPECT_TRUE(isValidKaraboIso8601("1985-102T10:15Z") == false); // Extended format: YYYY-DDDThh:mmZ
    // Week date and local time and the difference with UTC
    EXPECT_TRUE(isValidKaraboIso8601("1985W155T1015+0400") == false);  // Basic format: YYYYWwwDThhmm±hhmm
    EXPECT_TRUE(isValidKaraboIso8601("1985-W15-5T10:15+04") == false); // Extended format: YYYY-Www-DThh:mm±hh


    // Other tests
    EXPECT_TRUE(isValidKaraboIso8601("2009-12T12:34:22") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19") == false);
    EXPECT_TRUE(isValidKaraboIso8601("20090519") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009123") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-123") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-222") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-001") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-W01-1") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-W51-1") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-W511") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-W33") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009W511") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T00:00:00") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14:31:00") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14:39:22") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14:39Z") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14:39:22Z") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2009-W21-2") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-W21-2T01:22") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-W21-2T01:22:59") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-139") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14:39:22-06:00") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14:39:22+06:00") == true);
    EXPECT_TRUE(isValidKaraboIso8601("20090519T143922+0600") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14:39:22+0600") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14:3922+06:00") == false);
    EXPECT_TRUE(isValidKaraboIso8601("200905-19T14:39:22+06:00") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14:39:2206:00") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14:39:22-01") == false);
    EXPECT_TRUE(isValidKaraboIso8601("20090621T0545Z") == false);
    EXPECT_TRUE(isValidKaraboIso8601("20090621T0545:01Z") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2007-04-06T00:00") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2007-04-06T00:00:00") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2007-04-05T24:00") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2007-04-05T23:00:00") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16:23:48.5") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16:23:48,444") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16:23:48,3-06:00") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16:23.4") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16:23:33.4") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16:23,25") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16:23:44,25") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16:23.33+0600") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16:23:52.33+06:00") == true);
    EXPECT_TRUE(isValidKaraboIso8601("20100218T162352.33+0600") == true);
    EXPECT_TRUE(isValidKaraboIso8601("20100218T162352,33+0600") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16.23334444") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16:17:18.23334444") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16,2283") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16:17:18,2283") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14:39:22.500") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14:39:22,500") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T143922.500") == false);
    EXPECT_TRUE(isValidKaraboIso8601("20090519T14:39:22.500") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14.39.22.500") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T1439,55") == false);

    EXPECT_TRUE(isValidKaraboIso8601("1994-11-05T08:15:30-05:00") == true);
    EXPECT_TRUE(isValidKaraboIso8601("1994-11-05T13:15:30Z") == true);
    EXPECT_TRUE(isValidKaraboIso8601("155300+0500") == false);
    EXPECT_TRUE(isValidKaraboIso8601("155300Z") == false);
    EXPECT_TRUE(isValidKaraboIso8601("155300+0500") == false);
    EXPECT_TRUE(isValidKaraboIso8601("20080915T155300") == true);
    EXPECT_TRUE(isValidKaraboIso8601("20080915T155300+0500") == true);
    EXPECT_TRUE(isValidKaraboIso8601("20080915T155300Z") == true);
    EXPECT_TRUE(isValidKaraboIso8601("20080915") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2008-09-15") == false);
    EXPECT_TRUE(isValidKaraboIso8601("15:53:00.322348") == false);
    EXPECT_TRUE(isValidKaraboIso8601("15:53:00+05:00") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2008-09-15T15:53:00") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2008-09-15T15:53:00+05:00") == true);
    EXPECT_TRUE(isValidKaraboIso8601("2008-09-15") == false);
    EXPECT_TRUE(isValidKaraboIso8601("20121225T132536.789333123456789123") == true);

    // Strings that shouldn't be valid
    EXPECT_TRUE(isValidKaraboIso8601("Wed Feb 26 17:11:26 CET 2014") == false);
    EXPECT_TRUE(isValidKaraboIso8601("200913") == false);
    EXPECT_TRUE(isValidKaraboIso8601("200905") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009367") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2007-04-05T24:50") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2007-04-05T23:50") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-000") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-M511") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009M511") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14a39r") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T14:3924") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-0519") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-1914:39") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19 14:") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19r14:39") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19 14a39a22") == false);
    EXPECT_TRUE(isValidKaraboIso8601("200912-01") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19 14:39:22+06a00") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19 144922.500") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19T146922.500") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16.5:23.35:48") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16:23.35:48") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16:23.35:48.45") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19 14.5.44") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16:23.33.600") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2010-02-18T16,25:23:48,444") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19 00:00:00") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19 14:31:00") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19 14:39:22") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19 14:39:22-06:00") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19 14:39:22+0600") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19 14:39:22-01") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19 143922.500") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2009-05-19 1439,55") == false);

    EXPECT_TRUE(isValidKaraboIso8601("155300322348") == false);
    EXPECT_TRUE(isValidKaraboIso8601("T") == false);
    EXPECT_TRUE(isValidKaraboIso8601("20120120T") == false);
    EXPECT_TRUE(isValidKaraboIso8601("2012-01-20T") == false);
    EXPECT_TRUE(isValidKaraboIso8601("T200159") == false);
    EXPECT_TRUE(isValidKaraboIso8601("T20:01:59") == false);
    EXPECT_TRUE(isValidKaraboIso8601("20080915 ") == false);
    EXPECT_TRUE(isValidKaraboIso8601(" 20080915") == false);

    EXPECT_TRUE(isValidKaraboIso8601("z") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-z") == false);
    EXPECT_TRUE(isValidKaraboIso8601("z-") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-z-") == false);
    EXPECT_TRUE(isValidKaraboIso8601("Z") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-Z") == false);
    EXPECT_TRUE(isValidKaraboIso8601("Z-") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-Z-") == false);

    EXPECT_TRUE(isValidKaraboIso8601("w") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-w") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-w") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-w-") == false);
    EXPECT_TRUE(isValidKaraboIso8601("W") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-W") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-W") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-W-") == false);

    EXPECT_TRUE(isValidKaraboIso8601("t") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-t") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-t") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-t-") == false);
    EXPECT_TRUE(isValidKaraboIso8601("T") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-T") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-T") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-T-") == false);

    EXPECT_TRUE(isValidKaraboIso8601("--") == false);
    EXPECT_TRUE(isValidKaraboIso8601("---") == false);
    EXPECT_TRUE(isValidKaraboIso8601("+") == false);
    EXPECT_TRUE(isValidKaraboIso8601(":") == false);
    EXPECT_TRUE(isValidKaraboIso8601(" ") == false);
    EXPECT_TRUE(isValidKaraboIso8601("-") == false);
    EXPECT_TRUE(isValidKaraboIso8601("") == false);
}


static bool isValidKaraboIso8601TimeZone(const std::string& timeZone) {
    karabo::data::DateTimeString dts = karabo::data::DateTimeString();
    return dts.isStringValidIso8601TimeZone(timeZone);
}


TEST(TestDateTimeString, isStringKaraboValidIso8601TimeZone) {
    // Complete representation
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("Z") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("z") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("+00:00") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("+0000") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("+03:30") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("+0330") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("-07:00") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("-0700") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("-23:59") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("-2359") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("-11:11") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("-1111") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("+23:59") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("+2359") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("+11:11") == true);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("+1111") == true);

    // Invalid Time Zone's
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("x") == false);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("0000") == false);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("00:00") == false);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("-70:00") == false);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("-7000") == false);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("+2400") == false);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("+24:00") == false);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("-2400") == false);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("-24:00") == false);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("-0060") == false);
    EXPECT_TRUE(isValidKaraboIso8601TimeZone("-00:60") == false);
}


static void isValidateFractionalSecondToString(const unsigned long long fractionalSeconds,
                                               const std::string& expectedAttoFractionalSeconds,
                                               const bool writeToClog) {
    karabo::data::DateTimeString dts = karabo::data::DateTimeString();
    std::string expectedFractionalSeconds = "." + expectedAttoFractionalSeconds;

    // ATTOSEC
    if (writeToClog)
        std::clog << "fractionalSecondToString(karabo::data::TIME_UNITS::ATTOSEC, " << fractionalSeconds << ") => '"
                  << dts.fractionalSecondToString(karabo::data::TIME_UNITS::ATTOSEC, fractionalSeconds) << "' == '"
                  << expectedFractionalSeconds << "'" << std::endl;
    EXPECT_TRUE(dts.fractionalSecondToString(karabo::data::TIME_UNITS::ATTOSEC, fractionalSeconds) ==
                expectedFractionalSeconds);

    // FEMTOSEC
    expectedFractionalSeconds = expectedFractionalSeconds.substr(0, expectedFractionalSeconds.size() - 3);
    if (writeToClog)
        std::clog << "fractionalSecondToString(karabo::data::TIME_UNITS::FEMTOSEC, " << fractionalSeconds << ") => '"
                  << dts.fractionalSecondToString(karabo::data::TIME_UNITS::FEMTOSEC, fractionalSeconds) << "' == '"
                  << expectedFractionalSeconds << "'" << std::endl;
    EXPECT_TRUE(dts.fractionalSecondToString(karabo::data::TIME_UNITS::FEMTOSEC, fractionalSeconds) ==
                expectedFractionalSeconds);

    // PICOSEC
    expectedFractionalSeconds = expectedFractionalSeconds.substr(0, expectedFractionalSeconds.size() - 3);
    if (writeToClog)
        std::clog << "fractionalSecondToString(karabo::data::TIME_UNITS::PICOSEC, " << fractionalSeconds << ") => '"
                  << dts.fractionalSecondToString(karabo::data::TIME_UNITS::PICOSEC, fractionalSeconds) << "' == '"
                  << expectedFractionalSeconds << "'" << std::endl;
    EXPECT_TRUE(dts.fractionalSecondToString(karabo::data::TIME_UNITS::PICOSEC, fractionalSeconds) ==
                expectedFractionalSeconds);

    // NANOSEC
    expectedFractionalSeconds = expectedFractionalSeconds.substr(0, expectedFractionalSeconds.size() - 3);
    if (writeToClog)
        std::clog << "fractionalSecondToString(karabo::data::TIME_UNITS::NANOSEC, " << fractionalSeconds << ") => '"
                  << dts.fractionalSecondToString(karabo::data::TIME_UNITS::NANOSEC, fractionalSeconds) << "' == '"
                  << expectedFractionalSeconds << "'" << std::endl;
    EXPECT_TRUE(dts.fractionalSecondToString(karabo::data::TIME_UNITS::NANOSEC, fractionalSeconds) ==
                expectedFractionalSeconds);

    // MICROSEC
    expectedFractionalSeconds = expectedFractionalSeconds.substr(0, expectedFractionalSeconds.size() - 3);
    if (writeToClog)
        std::clog << "fractionalSecondToString(karabo::data::TIME_UNITS::MICROSEC, " << fractionalSeconds << ") => '"
                  << dts.fractionalSecondToString(karabo::data::TIME_UNITS::MICROSEC, fractionalSeconds) << "' == '"
                  << expectedFractionalSeconds << "'" << std::endl;
    EXPECT_TRUE(dts.fractionalSecondToString(karabo::data::TIME_UNITS::MICROSEC, fractionalSeconds) ==
                expectedFractionalSeconds);

    // MILLISEC
    expectedFractionalSeconds = expectedFractionalSeconds.substr(0, expectedFractionalSeconds.size() - 3);
    if (writeToClog)
        std::clog << "fractionalSecondToString(karabo::data::TIME_UNITS::MILLISEC, " << fractionalSeconds << ") => '"
                  << dts.fractionalSecondToString(karabo::data::TIME_UNITS::MILLISEC, fractionalSeconds) << "' == '"
                  << expectedFractionalSeconds << "'" << std::endl;
    EXPECT_TRUE(dts.fractionalSecondToString(karabo::data::TIME_UNITS::MILLISEC, fractionalSeconds) ==
                expectedFractionalSeconds);

    // ONESECOND
    expectedFractionalSeconds = expectedFractionalSeconds.substr(0, expectedFractionalSeconds.size() - 3) + "0";
    if (writeToClog)
        std::clog << "fractionalSecondToString(karabo::data::TIME_UNITS::ONESECOND, " << fractionalSeconds << ") => '"
                  << dts.fractionalSecondToString(karabo::data::TIME_UNITS::ONESECOND, fractionalSeconds) << "' == '"
                  << expectedFractionalSeconds << "'" << std::endl;
    EXPECT_TRUE(dts.fractionalSecondToString(karabo::data::TIME_UNITS::ONESECOND, fractionalSeconds) ==
                expectedFractionalSeconds);

    // NOFRACTION
    expectedFractionalSeconds = "";
    if (writeToClog)
        std::clog << "fractionalSecondToString(karabo::data::TIME_UNITS::NOFRACTION, " << fractionalSeconds << ") => '"
                  << dts.fractionalSecondToString(karabo::data::TIME_UNITS::NOFRACTION, fractionalSeconds) << "' == '"
                  << expectedFractionalSeconds << "'" << std::endl;
    EXPECT_TRUE(dts.fractionalSecondToString(karabo::data::TIME_UNITS::NOFRACTION, fractionalSeconds) ==
                expectedFractionalSeconds);

    return;
}


TEST(TestDateTimeString, validateFractionalSecondToString) {
    bool writeToClog = false;

    isValidateFractionalSecondToString(100000000000000000ULL, "100000000000000000", writeToClog);
    isValidateFractionalSecondToString(10000000000000000ULL, "010000000000000000", writeToClog);
    isValidateFractionalSecondToString(1000000000000000ULL, "001000000000000000", writeToClog);
    isValidateFractionalSecondToString(100000000000000ULL, "000100000000000000", writeToClog);
    isValidateFractionalSecondToString(10000000000000ULL, "000010000000000000", writeToClog);
    isValidateFractionalSecondToString(1000000000000ULL, "000001000000000000", writeToClog);
    isValidateFractionalSecondToString(100000000000ULL, "000000100000000000", writeToClog);
    isValidateFractionalSecondToString(10000000000ULL, "000000010000000000", writeToClog);
    isValidateFractionalSecondToString(1000000000ULL, "000000001000000000", writeToClog);
    isValidateFractionalSecondToString(100000000ULL, "000000000100000000", writeToClog);
    isValidateFractionalSecondToString(10000000ULL, "000000000010000000", writeToClog);
    isValidateFractionalSecondToString(1000000ULL, "000000000001000000", writeToClog);
    isValidateFractionalSecondToString(100000ULL, "000000000000100000", writeToClog);
    isValidateFractionalSecondToString(10000ULL, "000000000000010000", writeToClog);
    isValidateFractionalSecondToString(1000ULL, "000000000000001000", writeToClog);
    isValidateFractionalSecondToString(100ULL, "000000000000000100", writeToClog);
    isValidateFractionalSecondToString(10ULL, "000000000000000010", writeToClog);
    isValidateFractionalSecondToString(1ULL, "000000000000000001", writeToClog);

    //
    isValidateFractionalSecondToString(101000000000000000ULL, "101000000000000000", writeToClog);
    isValidateFractionalSecondToString(10100000000000000ULL, "010100000000000000", writeToClog);
    isValidateFractionalSecondToString(1010000000000000ULL, "001010000000000000", writeToClog);
    isValidateFractionalSecondToString(101000000000000ULL, "000101000000000000", writeToClog);
    isValidateFractionalSecondToString(10100000000000ULL, "000010100000000000", writeToClog);
    isValidateFractionalSecondToString(1010000000000ULL, "000001010000000000", writeToClog);
    isValidateFractionalSecondToString(101000000000ULL, "000000101000000000", writeToClog);
    isValidateFractionalSecondToString(10100000000ULL, "000000010100000000", writeToClog);
    isValidateFractionalSecondToString(1010000000ULL, "000000001010000000", writeToClog);
    isValidateFractionalSecondToString(101000000ULL, "000000000101000000", writeToClog);
    isValidateFractionalSecondToString(10100000ULL, "000000000010100000", writeToClog);
    isValidateFractionalSecondToString(1010000ULL, "000000000001010000", writeToClog);
    isValidateFractionalSecondToString(101000ULL, "000000000000101000", writeToClog);
    isValidateFractionalSecondToString(10100ULL, "000000000000010100", writeToClog);
    isValidateFractionalSecondToString(1010ULL, "000000000000001010", writeToClog);
    isValidateFractionalSecondToString(101ULL, "000000000000000101", writeToClog);
    isValidateFractionalSecondToString(10ULL, "000000000000000010", writeToClog);
    isValidateFractionalSecondToString(1ULL, "000000000000000001", writeToClog);
}
