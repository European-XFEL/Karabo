/* 
 * File:   DataLogUtils_Test.hh
 * Author: costar
 *
 * Created on February 4, 2019, 9:58 AM
 */

#ifndef DATALOGUTILS_TEST_HH
#define	DATALOGUTILS_TEST_HH

#include <boost/regex.hpp>
#include <cppunit/extensions/HelperMacros.h>

class DataLogUtils_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(DataLogUtils_Test);

    CPPUNIT_TEST(testValidIndexString);
    CPPUNIT_TEST(testValidIndexTailString);
    CPPUNIT_TEST(testInvalidIndexString);
    CPPUNIT_TEST(testInvalidIndexTailString);

    CPPUNIT_TEST_SUITE_END();

public:

    DataLogUtils_Test();
    virtual ~DataLogUtils_Test() = default;
    void setUp() override;
    void tearDown() override;

private:

    void testValidIndexString();
    void testValidIndexTailString();
    void testInvalidIndexString();
    void testInvalidIndexTailString();

    boost::regex m_indexRegex;
    boost::regex m_indexTailRegex;

};

#endif	/* DATALOGUTILS_TEST_HH */

