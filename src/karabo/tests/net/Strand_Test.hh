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
 * File:   Strand_Test.hh
 * Author: flucke
 *
 * Created on November 15, 2017, 12:26 PM
 */

#ifndef STRAND_TEST_HH
#define STRAND_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <boost/thread.hpp>
#include <memory>

class Strand_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Strand_Test);
    CPPUNIT_TEST(testSequential);
    CPPUNIT_TEST(testThrowing);
    CPPUNIT_TEST(testStrandDies);
    CPPUNIT_TEST(testMaxInARow);
    CPPUNIT_TEST_SUITE_END();

   public:
    Strand_Test();
    virtual ~Strand_Test();
    void setUp();
    void tearDown();

   private:
    void testSequential();

    void testThrowing();

    void testStrandDies();

    void testMaxInARow();

    std::shared_ptr<boost::thread> m_thread;
    const unsigned int m_nThreadsInPool;
};

#endif /* STRAND_TEST_HH */
