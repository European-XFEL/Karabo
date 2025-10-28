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

#include <gtest/gtest.h>

#include <karabo/util/PluginLoader.hh>

#include "karabo/data/schema/Factory.hh"
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


using namespace karabo::data;
using namespace karabo::util;
using namespace std;


KARABO_REGISTER_IN_FACTORY(Interface, A);

KARABO_REGISTER_IN_FACTORY_1(Interface, A, std::string);

KARABO_REGISTER_IN_FACTORY_1(Interface, A, int);


TEST(TestFactory, testObjectCreation) {
    // Factory::registerClass<A>("A");
    // Factory::registerClass<A, std::string>("A");
    std::shared_ptr<Interface> ptr1 = Factory<Interface>::create("AAA");
    EXPECT_TRUE(ptr1->foo() == "A:void");
    Interface::Pointer ptr2 = Factory<Interface>::create<std::string>("AAA", "Horrey!");
    EXPECT_TRUE(ptr2->foo() == "A:string");
    // Factory<A>::create<int>("A", 42);
}


TEST(TestFactory, testClassInfo) {
    ClassInfo ci = Interface::classInfo();
    //    clog << "i cid: " << ci.getClassId() << endl;
    //    clog << "i cna: " << ci.getClassName() << endl;
    //    clog << "i log: " << ci.getLogCategory() << endl;
    //    clog << "i ns : " << ci.getNamespace() << endl;
    //    clog << "i ver: " << ci.getVersion() << endl;

    EXPECT_TRUE(Interface::classInfo().getClassId() == "SimpleInterface");
    EXPECT_TRUE(Interface::classInfo().getClassName() == "Interface");
    EXPECT_TRUE(Interface::classInfo().getLogCategory() == "SimpleInterface");
    EXPECT_TRUE(Interface::classInfo().getNamespace() == "");
    EXPECT_TRUE(Interface::classInfo().getVersion() == "1.0");

    ci = karabo::util::PluginLoader::classInfo();
    //    clog << "pl cid: " << ci.getClassId() << endl;
    //    clog << "pl cna: " << ci.getClassName() << endl;
    //    clog << "pl log: " << ci.getLogCategory() << endl;
    //    clog << "pl ns : " << ci.getNamespace() << endl;
    //    clog << "pl ver: " << ci.getVersion() << endl;


    EXPECT_TRUE(karabo::util::PluginLoader::classInfo().getClassId() == "PluginLoader");
    EXPECT_TRUE(karabo::util::PluginLoader::classInfo().getClassName() == "PluginLoader");
    EXPECT_TRUE(karabo::util::PluginLoader::classInfo().getLogCategory() == "karabo.util.PluginLoader");
    EXPECT_TRUE(karabo::util::PluginLoader::classInfo().getNamespace() == "karabo::util");
    EXPECT_TRUE(karabo::util::PluginLoader::classInfo().getVersion() == "1.0");

    std::shared_ptr<Interface> ptr1 = Factory<Interface>::create("AAA");
    ci = ptr1->getClassInfo();
    EXPECT_TRUE(ci.getClassId() == "AAA");
    EXPECT_TRUE(ci.getClassName() == "A");
    EXPECT_TRUE(ci.getLogCategory() == "AAA");
    EXPECT_TRUE(ci.getNamespace() == "");
    EXPECT_TRUE(ci.getVersion() == "2.0");


    PluginLoader pl("/tmp");
    ci = pl.getClassInfo();
    EXPECT_TRUE(ci.getClassId() == "PluginLoader");
    EXPECT_TRUE(ci.getClassName() == "PluginLoader");
    EXPECT_TRUE(ci.getLogCategory() == "karabo.util.PluginLoader");
    EXPECT_TRUE(ci.getNamespace() == "karabo::util");
    EXPECT_TRUE(ci.getVersion() == "1.0");

    B<int> b;
    ci = b.getClassInfo();
    EXPECT_TRUE(ci.getClassId() == "BBB");
    EXPECT_TRUE(ci.getClassName() == "B");
    EXPECT_TRUE(ci.getLogCategory() == "BBB");
    EXPECT_TRUE(ci.getNamespace() == "");
    EXPECT_TRUE(ci.getVersion() == "2.1");


    internal::B<int> ib;
    ci = ib.getClassInfo();

    //    clog << "ib cid: " << ci.getClassId() << endl;
    //    clog << "ib cna: " << ci.getClassName() << endl;
    //    clog << "ib log: " << ci.getLogCategory() << endl;
    //    clog << "ib ns : " << ci.getNamespace() << endl;
    //    clog << "ib ver: " << ci.getVersion() << endl;

    EXPECT_TRUE(ci.getClassId() == "BBB");
    EXPECT_TRUE(ci.getClassName() == "B");
    EXPECT_TRUE(ci.getLogCategory() == "internal.BBB");
    EXPECT_TRUE(ci.getNamespace() == "internal");
    EXPECT_TRUE(ci.getVersion() == "2.1");


    internal::B<std::string> ibs;
    ci = ibs.getClassInfo();

    //    clog << "ibs cid: " << ci.getClassId() << endl;
    //    clog << "ibs cna: " << ci.getClassName() << endl;
    //    clog << "ibs log: " << ci.getLogCategory() << endl;
    //    clog << "ibs ns : " << ci.getNamespace() << endl;
    //    clog << "ibs ver: " << ci.getVersion() << endl;

    EXPECT_TRUE(ci.getClassId() == "BBB");
    EXPECT_TRUE(ci.getClassName() == "B");
    EXPECT_TRUE(ci.getLogCategory() == "internal.BBB");
    EXPECT_TRUE(ci.getNamespace() == "internal");
    EXPECT_TRUE(ci.getVersion() == "2.1");


    B<std::string> bs;
    ci = bs.getClassInfo();

    //    clog << "bs cid: " << ci.getClassId() << endl;
    //    clog << "bs cna: " << ci.getClassName() << endl;
    //    clog << "bs log: " << ci.getLogCategory() << endl;
    //    clog << "bs ns : " << ci.getNamespace() << endl;
    //    clog << "bs ver: " << ci.getVersion() << endl;

    EXPECT_TRUE(ci.getClassId() == "BBB");
    EXPECT_TRUE(ci.getClassName() == "B");
    EXPECT_TRUE(ci.getLogCategory() == "BBB");
    EXPECT_TRUE(ci.getNamespace() == "");
    EXPECT_TRUE(ci.getVersion() == "2.1");
}
