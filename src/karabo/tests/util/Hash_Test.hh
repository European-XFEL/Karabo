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
 * File:   Hash_Test.hh
 * Author: heisenb
 *
 * Created on Sep 18, 2012, 6:47:59 PM
 */

#ifndef HASH_TEST_HH
#define HASH_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include "karabo/data/schema/SimpleElement.hh"

class Hash_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Hash_Test);

    CPPUNIT_TEST(testConstructors);
    CPPUNIT_TEST(testGetSet);
    CPPUNIT_TEST(testSetMoveSemantics);
    CPPUNIT_TEST(testSetAttributeMoveSemantics);
    CPPUNIT_TEST(testConstructorMoveSemantics);
    CPPUNIT_TEST(testGetAs);
    CPPUNIT_TEST(testFind);
    CPPUNIT_TEST(testIteration);
    CPPUNIT_TEST(testAttributes);
    CPPUNIT_TEST(testGetPaths);
    CPPUNIT_TEST(testMerge);
    CPPUNIT_TEST(testSubtract);
    CPPUNIT_TEST(testErase);
    CPPUNIT_TEST(testHas);
    CPPUNIT_TEST(testIs);
    CPPUNIT_TEST(testHelper);
    CPPUNIT_TEST(testPack);
    CPPUNIT_TEST(testCounter);
    CPPUNIT_TEST(testKeys);
    CPPUNIT_TEST(testSimilarIsNotFullyEqual);
    CPPUNIT_TEST(testFullyEqualUnordered);
    CPPUNIT_TEST(testNode);
    CPPUNIT_TEST_SUITE_END();

   public:
    Hash_Test();
    virtual ~Hash_Test();
    void setUp();
    void tearDown();

   private:
    void testConstructors();
    void testGetSet();
    void testGetAs();
    void testSetMoveSemantics();
    void testSetAttributeMoveSemantics();
    void testConstructorMoveSemantics();
    void testFind();
    void testAttributes();
    void testGetPaths();
    void testIteration();
    void testMerge();
    void testSubtract();
    void testHelper();
    void testErase();
    void testHas();
    void testIs();
    void testPack();
    void testCounter();
    void testKeys();
    void testSimilarIsNotFullyEqual();
    void testFullyEqualUnordered();
    void testNode();
};
#endif
