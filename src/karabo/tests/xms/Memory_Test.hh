/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
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
