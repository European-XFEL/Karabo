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

#include "ClassInfo_Test.hh"

#include "karabo/util/ClassInfo.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(ClassInfo_Test);


void ClassInfo_Test::setUp() {}


void ClassInfo_Test::tearDown() {}

namespace {
    class TestClass {
       public:
        KARABO_CLASSINFO(TestClass, "TestClass_str", "version")

        TestClass() : m_int(0) {}
        TestClass(int i) : m_int(i) {}
        virtual ~TestClass() = default;

        int m_int;
    };
} // namespace

void ClassInfo_Test::testClassInfo() {
    // 1) Test the macro KARABO_CLASSINFO
    // MakeShared and Pointer
    TestClass::Pointer ptr0 = TestClass::MakeShared();
    CPPUNIT_ASSERT_EQUAL(0, ptr0->m_int);
    TestClass::Pointer ptr1 = TestClass::MakeShared(1);
    CPPUNIT_ASSERT_EQUAL(1, ptr1->m_int);

    // 2) Test ClassInfo class
    CPPUNIT_ASSERT_EQUAL(std::string("TestClass"), ptr1->getClassInfo().getClassName());
    CPPUNIT_ASSERT_EQUAL(std::string("TestClass_str"), ptr1->getClassInfo().getClassId());
    CPPUNIT_ASSERT_EQUAL(std::string("version"), ptr1->getClassInfo().getVersion());
}
