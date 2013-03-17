/*
 * File:   Logger_Test.hh
 * Author: bheisen
 *
 * Created on Mar 14, 2013, 12:24:04 PM
 */

#ifndef LOGGER_TEST_HH
#define	LOGGER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Logger_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(Logger_Test);

    CPPUNIT_TEST(testLogging);
    
    CPPUNIT_TEST_SUITE_END();

public:
    Logger_Test();
    virtual ~Logger_Test();
    void setUp();
    void tearDown();

private:
    void testLogging();
};

#endif	/* LOGGER_TEST_HH */

