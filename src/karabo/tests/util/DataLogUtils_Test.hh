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
 * File:   DataLogUtils_Test.hh
 * Author: costar
 *
 * Created on February 4, 2019, 9:58 AM
 */

#ifndef DATALOGUTILS_TEST_HH
#define DATALOGUTILS_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <regex>

class DataLogUtils_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(DataLogUtils_Test);

    CPPUNIT_TEST(testValidIndexLines);
    CPPUNIT_TEST(testInvalidIndexLines);
    CPPUNIT_TEST(testValueFromJSON);
    CPPUNIT_TEST(testMultipleJSONObjects);

    CPPUNIT_TEST_SUITE_END();

   public:
    DataLogUtils_Test();
    virtual ~DataLogUtils_Test() = default;
    void setUp();
    void tearDown();

   private:
    /**
     * Tests valid data logger index lines. For a detailed description
     * of the index line format see the declaration of
     * karabo::util::DataLogUtils::DATALOG_INDEX_LINE_REGEX.
     */
    void testValidIndexLines();

    /**
     * Tests invalids data logger index lines - some with errors in the
     * first three fields and some with error on further fields (in the
     * tail portion). Detailed description of the tail can be found at
     * karabo::util::DataLogUtils::DATALOG_INDEX_TAIL_REGEX.
     */
    void testInvalidIndexLines();

    void testValueFromJSON();
    void testMultipleJSONObjects();

    std::regex m_indexRegex;
    std::regex m_indexTailRegex;
};

#endif /* DATALOGUTILS_TEST_HH */
