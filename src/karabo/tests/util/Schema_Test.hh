/*
 * File:   Schema_Test.hh
 * Author: irinak
 *
 * Created on September 28, 2012, 1:14 PM
 */

#ifndef SCHEMA_TEST_HH
#define	SCHEMA_TEST_HH
#include <karabo/util/Configurator.hh>
#include <karabo/log/Logger.hh>

#include <cppunit/extensions/HelperMacros.h>

#include "ConfigurationTestClasses.hh"
#include <karabo/util.hpp>
#include <karabo/util/DataLogUtils.hh>

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
    CPPUNIT_TEST(testGetAlarmLowAlarmHigh);
    CPPUNIT_TEST(testGetWarnLowWarnHigh);
    CPPUNIT_TEST(testHasAlarmWarn);
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
    CPPUNIT_TEST(testList);
    CPPUNIT_TEST(testInvalidNodes);
    CPPUNIT_TEST(testOverwriteRestrictions);
    CPPUNIT_TEST(testOverwriteTags);
    CPPUNIT_TEST(testTagsFromVector);
    CPPUNIT_TEST(testOverwriteRestrictionsForOptions);
    CPPUNIT_TEST(testRuntimeAttributes);
    CPPUNIT_TEST(testStateAndAlarmSets);
    CPPUNIT_TEST(testSubSchema);
    CPPUNIT_TEST(testDaqDataType);
    CPPUNIT_TEST(testDaqPolicy);
    CPPUNIT_TEST(testNodeDisplayType);
    CPPUNIT_TEST(testGetLeaves);
    CPPUNIT_TEST(testAllowedActions);
    CPPUNIT_TEST(testDefaultReadOnlyThrows);

    CPPUNIT_TEST_SUITE_END();

public:

    KARABO_CLASSINFO(Schema_Test, "Schema_Test", "1.0");

    Schema_Test();
    virtual ~Schema_Test();
    void setUp();


private: //members
    karabo::util::Schema m_schema;

private: //functions

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
    void testGetAlarmLowAlarmHigh();
    void testGetWarnLowWarnHigh();
    void testHasAlarmWarn();
    void testSlotElement();
    void testVectorElements();
    void testArrayElements();
    void testPathElement();
    void testImageElement();
    void testArchivePolicy();
    void testOverwriteElement();
    void testMerge();
    void testTable();
    void testTableReadOnly();
    void testList();
    void testInvalidNodes();
    void testRuntimeAttributes();
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
    void testAllowedActions();
    /**
     * Checks that a schema element that uses the definition sequence
     * ...assignmentOptional().defaultValue(value).readOnly() throws an exception.
     * Conceptually, defaultValue and readOnly are incompatible and readOnly 
     * elements should use initialValue() instead of defaultValue().
     * As the invalid usage does not trigger compilation errors, a
     * throw-at-runtime approach has been used.
     */
    void testDefaultReadOnlyThrows();

};

#endif	/* SCHEMA_TEST_HH */