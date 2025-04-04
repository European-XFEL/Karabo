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

#ifndef DATETIMESTRING_TEST_HH
#define DATETIMESTRING_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include "karabo/data/time/TimeDuration.hh"

class DateTimeString_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(DateTimeString_Test);

    CPPUNIT_TEST(testConstructors);
    //
    CPPUNIT_TEST(isStringValidIso8601);
    CPPUNIT_TEST(isStringKaraboValidIso8601);
    CPPUNIT_TEST(isStringKaraboValidIso8601TimeZone);
    CPPUNIT_TEST(validateFractionalSecondToString);

    CPPUNIT_TEST_SUITE_END();

   public:
    DateTimeString_Test();
    virtual ~DateTimeString_Test();
    void setUp();
    void tearDown();

   private:
    // Tested Methods
    void testConstructors();
    bool isValidIso8601(const std::string& pTimeStr);
    bool isValidKaraboIso8601(const std::string& pTimeStr);
    bool isValidKaraboIso8601TimeZone(const std::string& timeZone);
    void validateFractionalSecondToString();

    // Auxiliary Methods
    void isStringValidIso8601();
    void isStringKaraboValidIso8601();
    void isStringKaraboValidIso8601TimeZone();

    void validateConstructor(const std::string& pTime, const std::string& expectedDate, const std::string& expectedTime,
                             const std::string& expectedFractionalSecond, const std::string& expectedTimeZone,
                             const std::string& expectedDateTime, const std::string& expectedSecondsSinceEpoch);

    void isValidateFractionalSecondToString(const unsigned long long fractionalSeconds,
                                            const std::string& expectedAttoFractionalSeconds, const bool writeToClog);
};

#endif /* DATETIMESTRING_TEST_HH */
