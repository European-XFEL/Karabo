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
    CPPUNIT_TEST(testSerialization);
    CPPUNIT_TEST_SUITE_END();

public:
    HashXmlSerializer_Test();
    virtual ~HashXmlSerializer_Test();
    void setUp();
    void tearDown();

private:
    void testSerialization();

private:

    karabo::util::Hash m_rootedHash;
    karabo::util::Hash m_bigHash;
    karabo::util::Hash m_unrootedHash;
    std::vector<karabo::util::Hash> m_vectorOfHashes;
};

#endif	/* HASHXMLSERIALIZER_TEST_HH */

