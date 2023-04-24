/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   HashFilter_Test.hh
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on April 12, 2013, 11:56 AM
 */

#ifndef HASHFILTER_TEST_HH
#define HASHFILTER_TEST_HH
#include <cppunit/extensions/HelperMacros.h>

#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>

//#define HASHFILTER_HDF5TEST

class HashFilter_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(HashFilter_Test);
    CPPUNIT_TEST(testFilterByTag);
    CPPUNIT_TEST(testFilterByAccessMode);
#ifdef HASHFILTER_HDF5TEST
    CPPUNIT_TEST(testHdf5Filter);
#endif
    CPPUNIT_TEST_SUITE_END();

   public:
    KARABO_CLASSINFO(HashFilter_Test, "HashFilter_Test", "1.0");

    HashFilter_Test();
    virtual ~HashFilter_Test();
    void setUp();
    void tearDown();

   private:
    void testFilterByTag();
    void testFilterByAccessMode();
#ifdef HASHFILTER_HDF5TEST
    void testHdf5Filter();
#endif
};

#endif /* FILTER_TEST_HH */
