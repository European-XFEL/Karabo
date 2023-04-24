/*
 * File:   Factory_Test.h
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on August 26, 2012, 7:20 PM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef KARABO_UTIL_FACTORY_TEST_HH
#define KARABO_UTIL_FACTORY_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/util/ClassInfo.hh>

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
