/*
 *
 * File:   Epochstamp_Test.hh
 * Author: <luis.maia@xfel.eu>
 *
 * Created on April 19, 2013, 4:47:47 PM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef TIMESTAMP_TEST_HH
#define TIMESTAMP_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/util/Epochstamp.hh>
#include <karabo/util/TimeDuration.hh>

class Epochstamp_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Epochstamp_Test);

    CPPUNIT_TEST(testConstructors);
    CPPUNIT_TEST(testOperators);
    //
    CPPUNIT_TEST(testToIso8601String);
    CPPUNIT_TEST(testToFormattedString);

    CPPUNIT_TEST_SUITE_END();

   public:
    Epochstamp_Test();
    virtual ~Epochstamp_Test();
    void setUp();
    void tearDown();

   private:
    void testConstructors();
    void testOperators();
    //
    void testToIso8601String();
    void testToFormattedString();

    // Auxiliary Methods
    void validateStringConstructor(const std::string& pTime, const unsigned long long& expectedSeconds,
                                   const unsigned long long& expectedFractionalSecond, bool isCompactString,
                                   const std::string& expectedToIso8601);

    void validateToFormattedString(const std::string& pTime, const std::string& format,
                                   const std::string& pTimeDesiredTimeZone, const std::string& expectedStringOutput);

    void toIso8601Precision(const karabo::util::Epochstamp epo, const karabo::util::Epochstamp epo2,
                            const karabo::util::TIME_UNITS precision, const std::string& precisionDesc,
                            const bool isCompactString, const bool writeToClog, const std::string& expectedToIso8601);
};

#endif /* TIMESTAMP_TEST_HH */
