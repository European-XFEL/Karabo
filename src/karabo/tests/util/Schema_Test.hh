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
};

#endif	/* SCHEMA_TEST_HH */

