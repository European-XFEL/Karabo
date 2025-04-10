/*
 * File:   Hash_Test.hh
 * Author: heisenb
 *
 * Created on Sep 18, 2012, 6:47:59 PM
 */

#ifndef HASH_TEST_HH
#define	HASH_TEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include <karabo/util/SimpleElement.hh>

class Hash_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Hash_Test);

    CPPUNIT_TEST(testConstructors);
    CPPUNIT_TEST(testGetSet);
    CPPUNIT_TEST(testGetAs);
    CPPUNIT_TEST(testFind);
    CPPUNIT_TEST(testIteration);
    CPPUNIT_TEST(testAttributes);
    CPPUNIT_TEST(testGetPaths);
    CPPUNIT_TEST(testMerge);
    CPPUNIT_TEST(testSubtract);
    CPPUNIT_TEST(testErase);
    CPPUNIT_TEST(testHas);
    CPPUNIT_TEST(testIs);
    CPPUNIT_TEST(testHelper);
    CPPUNIT_TEST(testTableValidation);
    CPPUNIT_TEST_SUITE_END();

public:
    Hash_Test();
    virtual ~Hash_Test();
    void setUp();
    void tearDown();

private:
    
    void testConstructors();
    void testGetSet();
    void testGetAs();
    void testFind();
    void testAttributes();
    void testGetPaths();
    void testIteration();
    void testMerge();
    void testSubtract();
    void testHelper();
    void testErase();
    void testHas();
    void testIs();
    void testTableValidation();
};

#endif


