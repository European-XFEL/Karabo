/*
 * File:   Factory_Test.h
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on August 26, 2012, 7:20 PM
 *
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

#ifndef KARABO_UTIL_FACTORY_TEST_HH
#define KARABO_UTIL_FACTORY_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include "karabo/data/types/ClassInfo.hh"

struct Interface {
    KARABO_CLASSINFO(Interface, "SimpleInterface", "1.0");

    virtual std::string foo() = 0;
};

struct A : public Interface {
    KARABO_CLASSINFO(A, "AAA", "2.0");

    std::string _ctorType;

    A() : _ctorType("void") {}

    A(const std::string& a1) : _ctorType("string") {}

    A(int a1) : _ctorType("int") {}

    virtual ~A() {}

    std::string foo() {
        return "A:" + _ctorType;
    }
};

template <class T>
class B {
   public:
    KARABO_CLASSINFO(B, "BBB", "2.1");
};

template <>
class B<std::string> {
   public:
    KARABO_CLASSINFO(B, "BBB", "2.1");
};

namespace internal {

    template <class T>
    class B {
       public:
        KARABO_CLASSINFO(B, "BBB", "2.1");
    };

    template <>
    class B<std::string> {
       public:
        KARABO_CLASSINFO(B, "BBB", "2.1");
    };
} // namespace internal

class Factory_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Factory_Test);

    CPPUNIT_TEST(testObjectCreation);
    CPPUNIT_TEST(testClassInfo);

    CPPUNIT_TEST_SUITE_END();

   public:
    Factory_Test();
    virtual ~Factory_Test();
    void setUp();
    void tearDown();

   private:
    void testClassInfo();
    void testObjectCreation();
};

#endif
