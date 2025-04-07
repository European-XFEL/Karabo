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
 * File:   SchemaSerializer_Test.hh
 * Author: boukhele
 *
 * Created on July 10, 2013, 11:49 AM
 */

#ifndef SCHEMASERIALIZER_TEST_HH
#define SCHEMASERIALIZER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class SchemaSerializer_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(SchemaSerializer_Test);
    CPPUNIT_TEST(testBinarySerializer);
    CPPUNIT_TEST(testXmlSerializer);
    CPPUNIT_TEST(testLoadLastFromSequence);
    CPPUNIT_TEST_SUITE_END();

   public:
    SchemaSerializer_Test();
    virtual ~SchemaSerializer_Test();
    void setUp();
    void tearDown();

   private:
    void testBinarySerializer();
    void testLoadLastFromSequence();
    void testXmlSerializer();
};

#endif /* SCHEMASERIALIZER_TEST_HH */
