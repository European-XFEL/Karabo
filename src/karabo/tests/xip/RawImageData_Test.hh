/* 
 * File:   RawImageData_Test.hh
 * Author: heisenb
 *
 * Created on May 22, 2014, 3:31 PM
 */

#ifndef RAWIMAGEDATA_TEST_HH
#define	RAWIMAGEDATA_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class RawImageData_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(RawImageData_Test);
    CPPUNIT_TEST(testConstructor);
    CPPUNIT_TEST(testPerformance);
    CPPUNIT_TEST_SUITE_END();

public:
    RawImageData_Test();
    virtual ~RawImageData_Test();

private:
    
    void testConstructor();


    void testPerformance();
};

#endif	/* RAWIMAGEDATA_TEST_HH */

