/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   H5Format_Test.hh
 * Author: wrona
 *
 * Created on Feb 13, 2013, 11:21:53 AM
 */


#ifndef H5FORMAT_TEST_HH
#define H5FORMAT_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/util.hpp>

class H5Format_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(H5Format_Test);

    CPPUNIT_TEST(testEmptyFormat);
    CPPUNIT_TEST(testManualFormat);
    CPPUNIT_TEST(testDiscoverFromHash);


    CPPUNIT_TEST_SUITE_END();

   public:
    KARABO_CLASSINFO(H5Format_Test, "H5Format_Test", "1.0")

    H5Format_Test();
    virtual ~H5Format_Test();
    void setUp();
    void tearDown();

   private:
    void testDiscoverFromHash();
    void testEmptyFormat();
    void testManualFormat();

    //
    // list of attribute types supported (by karabo::io::h5::Format)
    karabo::util::Hash::Attributes m_attributeList;
};

#endif /* H5FORMAT_TEST_HH */
