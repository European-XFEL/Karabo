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
 * File:   Validator_Test.hh
 * Author: flucke
 *
 * Created on September 9, 2016, 12:18 PM
 */

#ifndef VALIDATOR_TEST_HH
#define VALIDATOR_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Validator_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Validator_Test);
    CPPUNIT_TEST(testLeafAssignmentInternal);
    CPPUNIT_TEST(testTableOptionalColumn);
    CPPUNIT_TEST(testTableMandatoryColumn);
    CPPUNIT_TEST(testTableMinMaxRows);
    CPPUNIT_TEST(testColumnMinMaxAttrs);
    CPPUNIT_TEST(testVectorCharVectorByteSize);
    CPPUNIT_TEST(testState);
    CPPUNIT_TEST(testAlarms);
    CPPUNIT_TEST(testSlots);
    CPPUNIT_TEST(testOutputChannelSchemaRemoval);
    CPPUNIT_TEST(testPropertyTestValidation);
    CPPUNIT_TEST(testNDArray);
    CPPUNIT_TEST(testStrictAndReadOnly);
    CPPUNIT_TEST_SUITE_END();

   public:
    Validator_Test();
    virtual ~Validator_Test();
    void setUp();
    void tearDown();

   private:
    /**
     * @brief Checks that optional columns with default values in the
     * row schema are properly initialized to their default values,
     * even when the default row value for the table element is an
     * empty Hash.
     *
     * The initialization is performed as part of the validation process.
     */
    void testTableOptionalColumn();

    /**
     * @brief Checks that mandatory columns in the row schema are
     * indeed required.
     *
     * An attempt to set the default value for the table rows with
     * a mandatory column missing throws an exception (ParameterException).
     *
     * A table element (actually a vector of Hashes with a defined
     * RowSchema) fails validation if any mandatory column specified
     * in the row schema is missing.
     *
     * @note As mandatory columns will be dropped in the future, this
     * test case will only be kept until the dropping happens. In the
     * meantime, the checkings made in here guarantee the contract of
     * validation for mandatory columns.
     */
    void testTableMandatoryColumn();

    /**
     * @brief Checks that minSize and maxSize attributes for table elements
     * size are enforced.
     */
    void testTableMinMaxRows();

    /**
     * @brief Checks that minInc, maxInc, minExc, and maxExc attributes for
     * table cells (elements in the RowSchema) are enforced.
     */
    void testColumnMinMaxAttrs();

    /**
     * @brief Checks that minSize and maxSize attributes are enforced for
     * vector types VECTOR_CHAR_ELEMENT and VECTOR_UINT8_ELEMENT. Those two
     * types are Base64 encoded when converted from/to strings, hence
     * the special test case.
     */
    void testVectorCharVectorByteSize();

    /**
     * @brief Checks that the validator rejects non standard states
     * and sets the indicate attribute
     */
    void testState();

    /**
     * @brief Checks that the validator rejects non standard alarm strings
     * and sets the indicate attribute
     */
    void testAlarms();
    /**
     * @brief Checks that the Validator::validate does not create nodes for slots
     */
    void testSlots();

    /**
     * @brief Checks that the Validator::validate can verify for assignment internal leaf elements
     */
    void testLeafAssignmentInternal();

    /**
     * @brief An output channel should not expose its schema node should for its
     * configuration.
     */
    void testOutputChannelSchemaRemoval();

    /**
     * @brief Check validation for every property of PropertyTest class
     * with 'reconfiguration' validation rules.
     */
    void testPropertyTestValidation();

    void testNDArray();

    void testStrictAndReadOnly();
};

#endif /* VALIDATOR_TEST_HH */
