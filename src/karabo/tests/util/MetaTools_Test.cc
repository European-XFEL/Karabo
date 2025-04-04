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
 * File:   MetaTools_Test.cc
 * Author: heisenb
 *
 * Created on August 11, 2016, 11:18 AM
 */

#include "MetaTools_Test.hh"

#include <memory>

#include "karabo/util/PackParameters.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(MetaTools_Test);

using namespace karabo::data;
using namespace karabo::util;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;


MetaTools_Test::MetaTools_Test() {}


MetaTools_Test::~MetaTools_Test() {}


void MetaTools_Test::testMethod() {
    CPPUNIT_ASSERT(PointerTest::isSharedPointer<std::shared_ptr<int> >());
    CPPUNIT_ASSERT(!PointerTest::isSharedPointer<int>());

    CPPUNIT_ASSERT((std::is_base_of<Hash, MyPublicHash>::value));
    CPPUNIT_ASSERT((std::is_base_of<Hash, MyProtectedHash>::value));
    CPPUNIT_ASSERT((std::is_base_of<Hash, MyPrivateHash>::value));
    CPPUNIT_ASSERT((!std::is_base_of<Hash, int>::value));
}


void MetaTools_Test::testWeakBind() {
    std::vector<std::string> messages;
    std::unique_ptr<Test_DeviceServer> d(new Test_DeviceServer(&messages));
    karabo::net::EventLoop::addThread(4);
    karabo::net::EventLoop::run();

    CPPUNIT_ASSERT(messages.size() >= 4);
    CPPUNIT_ASSERT(messages[0] == "Test_Device created");
    CPPUNIT_ASSERT(messages[1] == "Tick 5");
    CPPUNIT_ASSERT(messages[2] == "Tick 6");
    CPPUNIT_ASSERT(messages[messages.size() - 1] == "Test_Device deleted");
}


struct Base : public std::enable_shared_from_this<Base> {
    virtual ~Base() {}
};


struct VirtualBase : public std::enable_shared_from_this<VirtualBase> {
    virtual void foo() {};


    virtual ~VirtualBase(){};
};


struct FinalInterim : public Base {
    virtual ~FinalInterim(){};
};


struct FinalInterimVirtual : public virtual VirtualBase {
    virtual ~FinalInterimVirtual(){};
};


struct Final : public std::enable_shared_from_this<Final> {};


struct FinalVirtual : public std::enable_shared_from_this<FinalVirtual> {
    virtual void foo() {};


    virtual ~FinalVirtual(){};
};


void MetaTools_Test::testCastResolvers() {
    // note that we verify compile-time functionality here. This will simply not compile if the cast resolvers
    // do not treat cases appropriately.
    std::shared_ptr<Final> f(new Final());
    { std::shared_ptr<Final> sf = karabo::util::cond_dyn_cast<std::true_type>::cast(f.get()); }


    std::shared_ptr<FinalVirtual> fv(new FinalVirtual());
    { std::shared_ptr<FinalVirtual> sfv = karabo::util::cond_dyn_cast<std::true_type>::cast(fv.get()); }


    std::shared_ptr<FinalInterim> fi(new FinalInterim());
    { std::shared_ptr<FinalInterim> sfi = karabo::util::cond_dyn_cast<std::false_type>::cast(fi.get()); }

    std::shared_ptr<FinalInterimVirtual> fiv(new FinalInterimVirtual());

    { std::shared_ptr<FinalInterimVirtual> sfiv = karabo::util::cond_dyn_cast<std::false_type>::cast(fiv.get()); }

    CPPUNIT_ASSERT(true);
}


struct Foo {
    Foo() {}


    Foo(const Foo&) {
        nCopies++;
    }


    static int nCopies;
};

int Foo::nCopies = 0;


struct Bar {
    Bar() : gotCalled(false) {}


    void bar(int, const std::string&, const Foo& f) {
        gotCalled = true;
    }

    bool gotCalled;
};


void MetaTools_Test::testCallFromTuple() {
    int i(42);
    std::string s("test");
    Foo f;
    Bar b;
    CPPUNIT_ASSERT_EQUAL(0, Foo::nCopies);

    Hash h;
    pack(h, i, s, f); // We will copy f once here!!
    CPPUNIT_ASSERT_EQUAL(1, Foo::nCopies);
    auto barFn = std::bind(&Bar::bar, &b, _1, _2, _3);
    call(barFn, unpack<int, std::string, Foo>(h)); // But not here!!

    CPPUNIT_ASSERT(b.gotCalled == true);
    CPPUNIT_ASSERT_EQUAL(1, Foo::nCopies);

    // But we copy if we go via function with arguments by value
    b.gotCalled = false;
    std::function<void(int, std::string, Foo)> funcWithArgsByValue = barFn;
    call(funcWithArgsByValue, unpack<int, std::string, Foo>(h));
    CPPUNIT_ASSERT(b.gotCalled == true);
    CPPUNIT_ASSERT_GREATER(1, Foo::nCopies); // At least one extra copy
}
