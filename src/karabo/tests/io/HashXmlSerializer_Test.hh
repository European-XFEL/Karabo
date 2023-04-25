/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   HashXmlSerializer_Test.hh
 * Author: heisenb
 *
 * Created on February 25, 2013, 6:03 PM
 */

#ifndef HASHXMLSERIALIZER_TEST_HH
#define HASHXMLSERIALIZER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/log/Logger.hh>

class HashXmlSerializer_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(HashXmlSerializer_Test);
    CPPUNIT_TEST(testSerialization);
    CPPUNIT_TEST(testLegacyDeserialization);
    CPPUNIT_TEST_SUITE_END();

   public:
    KARABO_CLASSINFO(HashXmlSerializer_Test, "HashXmlSerializer_Test", "1.0");

    HashXmlSerializer_Test();
    virtual ~HashXmlSerializer_Test();
    void setUp();
    void tearDown();

   private:
    void testSerialization();

    /**
     * Tests that xml files written before the modifications in the text serialization of
     * Schema and vector<Hash> attributes introduced in Merge Request 3592 (for Karabo 2.6.0)
     * are still supported.
     */
    void testLegacyDeserialization();

   private:
    karabo::util::Hash m_rootedHash;
    karabo::util::Hash m_bigHash;
    karabo::util::Hash m_unrootedHash;
    std::vector<karabo::util::Hash> m_vectorOfHashes;
};

#endif /* HASHXMLSERIALIZER_TEST_HH */
