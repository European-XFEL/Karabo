/* 
 * File:   SchemaXsdSerializer_Test.hh
 * Author: irinak
 *
 * Created on March 15, 2013, 10:30 AM
 */

#ifndef SCHEMAXSDSERIALIZER_TEST_HH
#define	SCHEMAXSDSERIALIZER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class SchemaXsdSerializer_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(SchemaXsdSerializer_Test);
    CPPUNIT_TEST(testXsdSerialization);
    CPPUNIT_TEST(testTextFileOutputSchema);
    CPPUNIT_TEST_SUITE_END();

public:
    SchemaXsdSerializer_Test();
    virtual ~SchemaXsdSerializer_Test();
    void setUp();
    void tearDown();

private:
    void testXsdSerialization();
    void testTextFileOutputSchema();

};

#endif	/* SCHEMAXSDSERIALIZER_TEST_HH */

