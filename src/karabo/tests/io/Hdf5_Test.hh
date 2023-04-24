/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Hdf5_Test.hh
 * Author: wrona
 *
 * Created on March 1, 2013, 4:02 PM
 */

#ifndef HDF5_TEST_HH
#define HDF5_TEST_HH
#include <cppunit/extensions/HelperMacros.h>

#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>

class Hdf5_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Hdf5_Test);
    CPPUNIT_TEST(testPureHdf5);
    CPPUNIT_TEST(testKaraboHdf5);
    CPPUNIT_TEST(testManyDatasets);
    CPPUNIT_TEST(testManyDatasets1);
    CPPUNIT_TEST(testSerializer);
    CPPUNIT_TEST(testKaraboNDArray);
    CPPUNIT_TEST(testKaraboPtr);
    CPPUNIT_TEST(testKaraboImageData);
    CPPUNIT_TEST_SUITE_END();

   public:
    KARABO_CLASSINFO(Hdf5_Test, "Hdf5_Test", "1.0");

    Hdf5_Test();
    virtual ~Hdf5_Test();
    void setUp();
    void tearDown();

   private:
    size_t m_numImages;     // number of images to be written
    int m_extentMultiplier; // image size multiplier: 1 means 1Mpx, 2 - 4Mpx, 3 - 9 Mpx, etc

    void testPureHdf5();
    void testKaraboHdf5();
    void testManyDatasets();
    void testManyDatasets1();
    void testSerializer();
    void testKaraboNDArray();
    void testKaraboPtr();
    void testKaraboImageData();
};

#endif /* HDF5_TEST_HH */
