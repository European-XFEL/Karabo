/* 
 * File:   HashBinarySerializer_Test.hh
 * Author: heisenb
 *
 * Created on February 25, 2013, 6:03 PM
 */

#ifndef HASHBINARYSERIALIZER_TEST_HH
#define	HASHBINARYSERIALIZER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class HashBinarySerializer_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(HashBinarySerializer_Test);
    CPPUNIT_TEST(testSerialization);
    CPPUNIT_TEST_SUITE_END();

public:
    HashBinarySerializer_Test();
    virtual ~HashBinarySerializer_Test();
    void setUp();
    void tearDown();

private:
    void testSerialization();
    
private:
    
    karabo::util::Hash m_rootedHash;
    karabo::util::Hash m_bigHash;
    karabo::util::Hash m_unrootedHash;
    std::vector<karabo::util::Hash> m_vectorOfHashes;
    karabo::util::Hash m_sharedPtrHash;

    std::vector<double> m_data;
};

#endif	/* HASHXMLSERIALIZER_TEST_HH */

