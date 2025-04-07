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
 * File:   Schema_Test.hh
 * Author: irinak
 *
 * Created on September 28, 2012, 1:14 PM
 */

#ifndef SCHEMA_TEST_HH
#define SCHEMA_TEST_HH
#include <cppunit/extensions/HelperMacros.h>

#include <karabo/data/types/Schema.hh>
#include <karabo/log/Logger.hh>
#include <karabo/util/DataLogUtils.hh>

#include "ConfigurationTestClasses.hh"
#include "karabo/data/schema/Configurator.hh"

class Schema_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Schema_Test);

    CPPUNIT_TEST(testBuildUp);
    CPPUNIT_TEST(testGetRootName);
    CPPUNIT_TEST(testGetTags);
    CPPUNIT_TEST(testGetNodeType);
    CPPUNIT_TEST(testGetValueType);
    CPPUNIT_TEST(testKeyHasAlias);
    CPPUNIT_TEST(testAliasHasKey);
    CPPUNIT_TEST(testGetAliasFromKey);
    CPPUNIT_TEST(testGetKeyFromAlias);
    CPPUNIT_TEST(testGetAliasAsString);
    CPPUNIT_TEST(testGetAccessMode);
    CPPUNIT_TEST(testGetAssignment);
    CPPUNIT_TEST(testGetOptions);
    CPPUNIT_TEST(testGetDefaultValue);
    CPPUNIT_TEST(testInvalidDefaultsThrow);
    CPPUNIT_TEST(testGetAllowedStates);
    CPPUNIT_TEST(testGetUnit);
    CPPUNIT_TEST(testGetMetricPrefix);
    CPPUNIT_TEST(testGetMinIncMaxInc);
    CPPUNIT_TEST(testGetMinExcMaxExc);
    CPPUNIT_TEST(testPerKeyFunctionality);
    CPPUNIT_TEST(testHelpFunction);
    CPPUNIT_TEST(testPaths);
    CPPUNIT_TEST(testGetRequiredAccessLevel);
    CPPUNIT_TEST(testSetRequiredAccessLevel);
    CPPUNIT_TEST(testSlotElement);
    CPPUNIT_TEST(testVectorElements);
    CPPUNIT_TEST(testArrayElements);
    CPPUNIT_TEST(testPathElement);
    CPPUNIT_TEST(testImageElement);
    CPPUNIT_TEST(testArchivePolicy);
    CPPUNIT_TEST(testOverwriteElement);
    CPPUNIT_TEST(testMerge);
    CPPUNIT_TEST(testTable);
    CPPUNIT_TEST(testTableReadOnly);
    CPPUNIT_TEST(testInvalidNodes);
    CPPUNIT_TEST(testOverwriteRestrictions);
    CPPUNIT_TEST(testOverwriteTags);
    CPPUNIT_TEST(testTagsFromVector);
    CPPUNIT_TEST(testOverwriteRestrictionsForOptions);
    CPPUNIT_TEST(testStateAndAlarmSets);
    CPPUNIT_TEST(testSubSchema);
    CPPUNIT_TEST(testDaqDataType);
    CPPUNIT_TEST(testDaqPolicy);
    CPPUNIT_TEST(testNodeDisplayType);
    CPPUNIT_TEST(testGetLeaves);
    CPPUNIT_TEST(testAlarmStateElement);
    CPPUNIT_TEST(testAllowedActions);
    CPPUNIT_TEST(testInvalidReadOnlyThrows);
    CPPUNIT_TEST(testTableColNoDefaultValue);
    CPPUNIT_TEST(testTableColInitOnly);
    CPPUNIT_TEST(testTableColUnsupportedType);
    CPPUNIT_TEST(testTableColWrongAccessMode);

    CPPUNIT_TEST_SUITE_END();

   public:
    KARABO_CLASSINFO(Schema_Test, "Schema_Test", "1.0");

    Schema_Test();
    virtual ~Schema_Test();
    void setUp();


   private: // members
    karabo::data::Schema m_schema;

   private: // functions
    void testBuildUp();

    void testGetRootName();
    void testGetTags();
    void testGetNodeType();
    void testGetValueType();
    void testKeyHasAlias();
    void testAliasHasKey();
    void testGetAliasFromKey();
    void testGetKeyFromAlias();
    void testGetAliasAsString();
    void testGetAccessMode();
    void testGetAssignment();
    void testGetOptions();
    void testGetDefaultValue();
    void testInvalidDefaultsThrow();
    void testGetAllowedStates();
    void testGetUnit();
    void testGetMetricPrefix();
    void testGetMinIncMaxInc();
    void testGetMinExcMaxExc();
    void testPerKeyFunctionality();
    void testHelpFunction();
    void testPaths();
    void testGetRequiredAccessLevel();
    void testSetRequiredAccessLevel();
    void testSlotElement();
    void testVectorElements();
    void testArrayElements();
    void testPathElement();
    void testImageElement();
    void testArchivePolicy();
    void testOverwriteElement();
    void testOverwriteElementScalarDefault();
    void testOverwriteElementVectorDefault();
    void testOverwriteElementMinMax();
    void testOverwriteElementMinMaxVector();
    void testMerge();
    void testInvalidNodes();
    void testOverwriteRestrictions();
    void testOverwriteTags();
    void testTagsFromVector();
    void testOverwriteRestrictionsForOptions();
    void testStateAndAlarmSets();
    void testSubSchema();
    void testDaqDataType();
    void testDaqPolicy();
    void testNodeDisplayType();
    void testGetLeaves();
    void testAlarmStateElement();
    void testAllowedActions();
    /**
     * @brief Checks that a schema element that uses the definition sequence
     * "...assignmentOptional().defaultValue(value).readOnly()" throws an
     * exception.
     *
     * Conceptually, defaultValue and readOnly are incompatible and readOnly
     * elements should use initialValue() instead of defaultValue().
     * As the invalid usage does not trigger compilation errors, a
     * throw-at-runtime approach has been used.
     */
    void testInvalidReadOnlyThrows();

    void testTable();
    void testTableReadOnly();

    /**
     * @brief Checks that reconfigurable table columns of supported types for
     * which no default value has been specified are fixed.
     *
     * The fix consists of adding the missing default value attribute to the
     * row schema with the default initialization value for the column type.
     * If it is not possible to synthesize a default value - e.g. the default
     * initialization value for the column type is outside the allowed range
     * of values - an exception is thrown.
     */
    void testTableColNoDefaultValue();

    /**
     * @brief Checks that init only table columns (unsupported) are fixed,
     * being converted to read-only columns when the table is read-only and
     * to reconfigurable columns when the table is not read-only.
     */
    void testTableColInitOnly();

    /**
     * @brief Checks that the presence of unsupported table column types
     * throws an exception.
     */
    void testTableColUnsupportedType();

    /**
     * @brief Checks that table columns with access mode incompatible with
     * the table's access mode are fixed (reconfigurable columns on read-only
     * tables become read-only with their initial value set to the default
     * value of the previously reconfigurable column).
     */
    void testTableColWrongAccessMode();
};

#endif /* SCHEMA_TEST_HH */
