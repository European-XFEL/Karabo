/*
 * File:   DateTimeString_Test.hh
 * Author: <luis.maia@xfel.eu>
 *
 * Created on March 19, 2014, 11:01 AM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef DATETIMESTRING_TEST_HH
#define DATETIMESTRING_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/util/TimeDuration.hh>

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
