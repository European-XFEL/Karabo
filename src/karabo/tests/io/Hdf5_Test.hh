/* 
 * File:   Hdf5_Test.hh
 * Author: wrona
 *
 * Created on March 1, 2013, 4:02 PM
 */

#ifndef HDF5_TEST_HH
#define	HDF5_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Hdf5_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Hdf5_Test);
    CPPUNIT_TEST(testPureHdf5);
    
    CPPUNIT_TEST_SUITE_END();

public:
    Hdf5_Test();
    virtual ~Hdf5_Test();
    void setUp();
    void tearDown();

private:
 
    void testPureHdf5();
 
};

#endif	/* HDF5_TEST_HH */

