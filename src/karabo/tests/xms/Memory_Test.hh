/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Memory_Test.hh
 * Author: wigginsj
 *
 * Created on September 26, 2016, 9:28 AM
 */

#ifndef MEMORY_TEST_HH
#define MEMORY_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/xms.hpp>

class Memory_Test : public CPPUNIT_NS::TestFixture {
    unsigned int m_channelId;
    unsigned int m_chunkId;

    CPPUNIT_TEST_SUITE(Memory_Test);
    CPPUNIT_TEST(testSimpleReadAndWrite);
    CPPUNIT_TEST(testModifyAfterWrite);
    CPPUNIT_TEST_SUITE_END();

   public:
    Memory_Test();
    virtual ~Memory_Test();
    void setUp();
    void tearDown();

   private:
    void testSimpleReadAndWrite();
    void testModifyAfterWrite();
};

#endif /* MEMORY_TEST_HH */
