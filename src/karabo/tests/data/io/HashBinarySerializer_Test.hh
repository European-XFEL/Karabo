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
 * File:   HashBinarySerializer_Test.hh
 * Author: heisenb
 *
 * Created on February 25, 2013, 6:03 PM
 */

#ifndef HASHBINARYSERIALIZER_TEST_HH
#define HASHBINARYSERIALIZER_TEST_HH
#include <cppunit/extensions/HelperMacros.h>

#include <karabo/log/Logger.hh>

#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/types/Hash.hh"


class HashBinarySerializer_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(HashBinarySerializer_Test);
    CPPUNIT_TEST(testSerialization);
    CPPUNIT_TEST(testSpeedLargeArrays);
    CPPUNIT_TEST(testMaxHashKeyLength);
    CPPUNIT_TEST(testReadVectorHashPointer);
    CPPUNIT_TEST_SUITE_END();

   public:
    KARABO_CLASSINFO(HashBinarySerializer_Test, "HashBinarySerializer_Test", "1.0");

    HashBinarySerializer_Test();
    virtual ~HashBinarySerializer_Test();

   private:
    void testSerialization();
    void testSpeedLargeArrays();
    void testMaxHashKeyLength();
    void testReadVectorHashPointer();

    void hashContentTest(const karabo::data::Hash& toBeTested, const std::string& whichSerialisation);

   private:
    karabo::data::Hash m_hash;
};

#endif /* HASHXMLSERIALIZER_TEST_HH */
