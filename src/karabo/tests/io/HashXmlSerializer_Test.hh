/* 
 * File:   HashXmlSerializer_Test.hh
 * Author: heisenb
 *
 * Created on February 25, 2013, 6:03 PM
 */

#ifndef HASHXMLSERIALIZER_TEST_HH
#define	HASHXMLSERIALIZER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class HashXmlSerializer_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(HashXmlSerializer_Test);
    CPPUNIT_TEST(testSave);
    CPPUNIT_TEST(testLoad);
    CPPUNIT_TEST_SUITE_END();

public:
    HashXmlSerializer_Test();
    virtual ~HashXmlSerializer_Test();
    void setUp();
    void tearDown();

private:
    void testSave();
    void testLoad();
};

#endif	/* HASHXMLSERIALIZER_TEST_HH */

