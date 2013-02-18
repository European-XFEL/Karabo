/*
 * File:   H5Format_Test.hh
 * Author: wrona
 *
 * Created on Feb 13, 2013, 11:21:53 AM
 */


#ifndef H5FORMAT_TEST_HH
#define	H5FORMAT_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class H5Format_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(H5Format_Test);

    CPPUNIT_TEST(testDiscoverFromHash);
    
    CPPUNIT_TEST_SUITE_END();

public:
    H5Format_Test();
    virtual ~H5Format_Test();
    void setUp();
    void tearDown();

private:
  
    void testDiscoverFromHash();
};

#endif	/* H5FORMAT_TEST_HH */

