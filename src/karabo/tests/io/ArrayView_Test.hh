/* 
 * File:   ArrayView_Test.hh
 * Author: wrona
 *
 * Created on November 27, 2012, 1:37 PM
 */

#ifndef ARRAYVIEW_TEST_HH
#define	ARRAYVIEW_TEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include <karabo/io/ArrayView.hh>
#include <karabo/io/ioProfiler.hh>

class ArrayView_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(ArrayView_Test);
    CPPUNIT_TEST(testCArray);
    CPPUNIT_TEST(testIndexable);
    CPPUNIT_TEST_SUITE_END();

public:
    ArrayView_Test();
    virtual ~ArrayView_Test();
    void setUp();
    void tearDown();

private:
    void testCArray();
    void testIndexable();

};

#endif	/* ARRAYVIEW_TEST_HH */

