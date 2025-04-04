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
    karabo::data::Hash m_rootedHash;
    karabo::data::Hash m_bigHash;
    karabo::data::Hash m_unrootedHash;
    std::vector<karabo::data::Hash> m_vectorOfHashes;
};

#endif /* HASHXMLSERIALIZER_TEST_HH */
