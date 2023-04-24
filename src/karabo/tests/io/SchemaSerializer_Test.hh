/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
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
