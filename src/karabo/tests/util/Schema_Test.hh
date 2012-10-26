/* 
 * File:   Schema_Test.hh
 * Author: irinak
 *
 * Created on September 28, 2012, 1:14 PM
 */

#ifndef SCHEMA_TEST_HH
#define	SCHEMA_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/util/Schema.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/VectorElement.hh>
#include <karabo/util/ImageElement.hh>

class Schema_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Schema_Test);
    CPPUNIT_TEST(testHasKey);
    CPPUNIT_TEST(testKeyHasAlias);
    CPPUNIT_TEST(testHasAlias);
    CPPUNIT_TEST(testAliasIsOfType);
    CPPUNIT_TEST(testParameterIsOfType);
    CPPUNIT_TEST(testKey2Alias);
    CPPUNIT_TEST(testAlias2Key);
    CPPUNIT_TEST(testGetAccessMode);
    CPPUNIT_TEST(testHasParameters);
    CPPUNIT_TEST(testGetAllParameters);
    CPPUNIT_TEST(testHasRoot);
    CPPUNIT_TEST(testGetRoot);
    CPPUNIT_TEST(testIsAttribute);
    CPPUNIT_TEST(testPerKeyFunctionality);
        
    CPPUNIT_TEST_SUITE_END();

public:
    Schema_Test();
    virtual ~Schema_Test();
    void setUp();
    karabo::util::Schema sch;

private:
    void settingExpectedParameters(karabo::util::Schema&);
    void oneElementExpectParams(karabo::util::Schema&);
    void testHasKey();
    void testKeyHasAlias();
    void testHasAlias();
    void testAliasIsOfType();
    void testParameterIsOfType();
    void testKey2Alias();
    void testAlias2Key();
    void testGetAccessMode();
    void testHasParameters();
    void testGetAllParameters();
    void testHasRoot();
    void testGetRoot();
    void testIsAttribute();
    void testPerKeyFunctionality();
};

#endif	/* SCHEMA_TEST_HH */

