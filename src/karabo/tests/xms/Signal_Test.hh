/*
 * File:   Signal_Test.hh
 *
 * Created on Feb 9, 2023
 */

#ifndef SIGNAL_TEST_HH
#define SIGNAL_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Signal_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Signal_Test);

    CPPUNIT_TEST(testRegisterSlots);

    CPPUNIT_TEST_SUITE_END();

   public:
    Signal_Test();
    virtual ~Signal_Test();
    void setUp();
    void tearDown();

   private:
    void testRegisterSlots();
};

#endif /* SIGNAL_TEST_HH */
