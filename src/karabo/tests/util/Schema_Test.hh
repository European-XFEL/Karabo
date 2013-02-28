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
    CPPUNIT_TEST(testGetAlias);
    CPPUNIT_TEST(testGetAccessMode);
    CPPUNIT_TEST(testGetAssignment);
    CPPUNIT_TEST(testGetOptions);
    CPPUNIT_TEST(testGetDefaultValue);
    CPPUNIT_TEST(testGetAllowedStates);
    CPPUNIT_TEST(testGetUnit);
    CPPUNIT_TEST(testGetMetricPrefix);
    CPPUNIT_TEST(testPerKeyFunctionality);

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
    void testGetAlias();
    void testGetAccessMode();
    void testGetAssignment();
    void testGetOptions();
    void testGetDefaultValue();
    void testGetAllowedStates();
    void testGetUnit();
    void testGetMetricPrefix();
    void testPerKeyFunctionality();

};

#endif	/* SCHEMA_TEST_HH */

