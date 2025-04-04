/*
 *
 * File:   Epochstamp_Test.hh
 * Author: <luis.maia@xfel.eu>
 *
 * Created on April 19, 2013, 4:47:47 PM
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

#ifndef TIMESTAMP_TEST_HH
#define TIMESTAMP_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include "karabo/data/time/Epochstamp.hh"
#include "karabo/data/time/TimeDuration.hh"

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

    void toIso8601Precision(const karabo::data::Epochstamp epo, const karabo::data::Epochstamp epo2,
                            const karabo::data::TIME_UNITS precision, const std::string& precisionDesc,
                            const bool isCompactString, const bool writeToClog, const std::string& expectedToIso8601);
};

#endif /* TIMESTAMP_TEST_HH */
