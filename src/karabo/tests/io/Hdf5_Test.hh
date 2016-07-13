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
    CPPUNIT_TEST(testKaraboHdf5);
    CPPUNIT_TEST(testManyDatasets);
    CPPUNIT_TEST(testManyDatasets1);
    CPPUNIT_TEST(testSerializer);
    CPPUNIT_TEST_SUITE_END();

public:
    Hdf5_Test();
    virtual ~Hdf5_Test();
    void setUp();
    void tearDown();

private:

    size_t m_numImages; // number of images to be written
    int m_extentMultiplier; //image size multiplier: 1 means 1Mpx, 2 - 4Mpx, 3 - 9 Mpx, etc
    bool m_report;

    void testPureHdf5();
    void testKaraboHdf5();
    void testManyDatasets();
    void testManyDatasets1();
    void testSerializer();
};

#endif	/* HDF5_TEST_HH */

