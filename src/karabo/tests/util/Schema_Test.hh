/* 
 * File:   Schema_Test.hh
 * Author: irinak
 *
 * Created on September 28, 2012, 1:14 PM
 */

#ifndef SCHEMA_TEST_HH
#define	SCHEMA_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include "ConfigurationTestClasses.hh"


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
    CPPUNIT_TEST(testPathElement);
    CPPUNIT_TEST(testImageElement);
    CPPUNIT_TEST(testArchivePolicy);
    CPPUNIT_TEST(testOverwriteElement);
    
    CPPUNIT_TEST_SUITE_END();

public:

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
    void testPathElement();
    void testImageElement();
    void testArchivePolicy();
    void testOverwriteElement();

};

#endif	/* SCHEMA_TEST_HH */