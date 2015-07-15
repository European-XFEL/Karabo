/* 
 * File:   Data_Test.hh
 * Author: flucke
 *
 * Created on July 14, 2015, 9:42 AM
 */

#ifndef DATA_TEST_HH
#define	DATA_TEST_HH

#include <vector>

#include <cppunit/extensions/HelperMacros.h>

class Data_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Data_Test);
    CPPUNIT_TEST(testSetHash);
    CPPUNIT_TEST(testSetHashPtr);
    CPPUNIT_TEST(testHashCtr);
    CPPUNIT_TEST_SUITE_END();

public:
    Data_Test();
    virtual ~Data_Test();
    void setUp();
    void tearDown();

private:
    /// tests adding a Hash to Data
    void testSetHash();
    /// tests adding a Hash::Pointer to Data
    void testSetHashPtr();
    /// tests the constructor from Hash
    void testHashCtr();

    std::vector<int> m_vec;
};

#endif	/* DATA_TEST_HH */

