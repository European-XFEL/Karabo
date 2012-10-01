/*
 * File:   Hash_Test.hh
 * Author: heisenb
 *
 * Created on Sep 18, 2012, 6:47:59 PM
 */

#ifndef HASH_TEST_HH
#define	HASH_TEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include <karabo/util/Exception.hh>

class Hash_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Hash_Test);

    CPPUNIT_TEST(testGet);
    CPPUNIT_TEST_EXCEPTION(testGetCastException1, karabo::util::CastException);
    CPPUNIT_TEST_EXCEPTION(testGetCastException2, karabo::util::CastException);
    CPPUNIT_TEST_EXCEPTION(testGetCastException3, karabo::util::CastException);
    CPPUNIT_TEST_EXCEPTION(testGetCastException4, karabo::util::CastException);
    
    CPPUNIT_TEST(testGetTypeAsStringOrId);
    CPPUNIT_TEST(testGetFromPath);
    CPPUNIT_TEST(testHasFromPath);
    CPPUNIT_TEST(testConvertFromString);
    CPPUNIT_TEST_EXCEPTION(testConvertFromStringBadLexicalCast1, boost::bad_lexical_cast);
    CPPUNIT_TEST_EXCEPTION(testConvertFromStringBadLexicalCast2, boost::bad_lexical_cast);
    CPPUNIT_TEST_EXCEPTION(testConvertFromStringCastException, karabo::util::CastException);
    CPPUNIT_TEST_SUITE_END();

public:
    Hash_Test();
    virtual ~Hash_Test();

private:
    void testGet();
    void testGetCastException1();
    void testGetCastException2();
    void testGetCastException3();
    void testGetCastException4();
    
    void testGetTypeAsStringOrId();
    void testGetFromPath();
    void testHasFromPath();
    void testConvertFromString();
    void testConvertFromStringBadLexicalCast1();
    void testConvertFromStringBadLexicalCast2();
    void testConvertFromStringCastException();
};

#endif	/* HASH_TEST_HH */

