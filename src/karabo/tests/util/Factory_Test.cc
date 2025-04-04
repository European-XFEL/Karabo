/*
 * File:   Factory_Test.cc
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

#include "Factory_Test.hh"

#include <karabo/util/PluginLoader.hh>

#include "karabo/data/schema/Factory.hh"

using namespace karabo::data;
using namespace karabo::util;

using namespace std;

CPPUNIT_TEST_SUITE_REGISTRATION(Factory_Test);

KARABO_REGISTER_IN_FACTORY(Interface, A);

KARABO_REGISTER_IN_FACTORY_1(Interface, A, std::string);

KARABO_REGISTER_IN_FACTORY_1(Interface, A, int);


Factory_Test::Factory_Test() {}


Factory_Test::~Factory_Test() {}


void Factory_Test::setUp() {}


void Factory_Test::tearDown() {}


void Factory_Test::testObjectCreation() {
    // Factory::registerClass<A>("A");
    // Factory::registerClass<A, std::string>("A");
    std::shared_ptr<Interface> ptr1 = Factory<Interface>::create("AAA");
    CPPUNIT_ASSERT(ptr1->foo() == "A:void");
    Interface::Pointer ptr2 = Factory<Interface>::create<std::string>("AAA", "Horrey!");
    CPPUNIT_ASSERT(ptr2->foo() == "A:string");
    // Factory<A>::create<int>("A", 42);
}


void Factory_Test::testClassInfo() {
    ClassInfo ci = Interface::classInfo();
    //    clog << "i cid: " << ci.getClassId() << endl;
    //    clog << "i cna: " << ci.getClassName() << endl;
    //    clog << "i log: " << ci.getLogCategory() << endl;
    //    clog << "i ns : " << ci.getNamespace() << endl;
    //    clog << "i ver: " << ci.getVersion() << endl;

    CPPUNIT_ASSERT(Interface::classInfo().getClassId() == "SimpleInterface");
    CPPUNIT_ASSERT(Interface::classInfo().getClassName() == "Interface");
    CPPUNIT_ASSERT(Interface::classInfo().getLogCategory() == "SimpleInterface");
    CPPUNIT_ASSERT(Interface::classInfo().getNamespace() == "");
    CPPUNIT_ASSERT(Interface::classInfo().getVersion() == "1.0");

    ci = karabo::util::PluginLoader::classInfo();
    //    clog << "pl cid: " << ci.getClassId() << endl;
    //    clog << "pl cna: " << ci.getClassName() << endl;
    //    clog << "pl log: " << ci.getLogCategory() << endl;
    //    clog << "pl ns : " << ci.getNamespace() << endl;
    //    clog << "pl ver: " << ci.getVersion() << endl;


    CPPUNIT_ASSERT(karabo::util::PluginLoader::classInfo().getClassId() == "PluginLoader");
    CPPUNIT_ASSERT(karabo::util::PluginLoader::classInfo().getClassName() == "PluginLoader");
    CPPUNIT_ASSERT(karabo::util::PluginLoader::classInfo().getLogCategory() == "karabo.util.PluginLoader");
    CPPUNIT_ASSERT(karabo::util::PluginLoader::classInfo().getNamespace() == "karabo::util");
    CPPUNIT_ASSERT(karabo::util::PluginLoader::classInfo().getVersion() == "1.0");

    std::shared_ptr<Interface> ptr1 = Factory<Interface>::create("AAA");
    ci = ptr1->getClassInfo();
    CPPUNIT_ASSERT(ci.getClassId() == "AAA");
    CPPUNIT_ASSERT(ci.getClassName() == "A");
    CPPUNIT_ASSERT(ci.getLogCategory() == "AAA");
    CPPUNIT_ASSERT(ci.getNamespace() == "");
    CPPUNIT_ASSERT(ci.getVersion() == "2.0");


    PluginLoader pl("/tmp");
    ci = pl.getClassInfo();
    CPPUNIT_ASSERT(ci.getClassId() == "PluginLoader");
    CPPUNIT_ASSERT(ci.getClassName() == "PluginLoader");
    CPPUNIT_ASSERT(ci.getLogCategory() == "karabo.util.PluginLoader");
    CPPUNIT_ASSERT(ci.getNamespace() == "karabo::util");
    CPPUNIT_ASSERT(ci.getVersion() == "1.0");

    B<int> b;
    ci = b.getClassInfo();
    CPPUNIT_ASSERT(ci.getClassId() == "BBB");
    CPPUNIT_ASSERT(ci.getClassName() == "B");
    CPPUNIT_ASSERT(ci.getLogCategory() == "BBB");
    CPPUNIT_ASSERT(ci.getNamespace() == "");
    CPPUNIT_ASSERT(ci.getVersion() == "2.1");


    internal::B<int> ib;
    ci = ib.getClassInfo();

    //    clog << "ib cid: " << ci.getClassId() << endl;
    //    clog << "ib cna: " << ci.getClassName() << endl;
    //    clog << "ib log: " << ci.getLogCategory() << endl;
    //    clog << "ib ns : " << ci.getNamespace() << endl;
    //    clog << "ib ver: " << ci.getVersion() << endl;

    CPPUNIT_ASSERT(ci.getClassId() == "BBB");
    CPPUNIT_ASSERT(ci.getClassName() == "B");
    CPPUNIT_ASSERT(ci.getLogCategory() == "internal.BBB");
    CPPUNIT_ASSERT(ci.getNamespace() == "internal");
    CPPUNIT_ASSERT(ci.getVersion() == "2.1");


    internal::B<std::string> ibs;
    ci = ibs.getClassInfo();

    //    clog << "ibs cid: " << ci.getClassId() << endl;
    //    clog << "ibs cna: " << ci.getClassName() << endl;
    //    clog << "ibs log: " << ci.getLogCategory() << endl;
    //    clog << "ibs ns : " << ci.getNamespace() << endl;
    //    clog << "ibs ver: " << ci.getVersion() << endl;

    CPPUNIT_ASSERT(ci.getClassId() == "BBB");
    CPPUNIT_ASSERT(ci.getClassName() == "B");
    CPPUNIT_ASSERT(ci.getLogCategory() == "internal.BBB");
    CPPUNIT_ASSERT(ci.getNamespace() == "internal");
    CPPUNIT_ASSERT(ci.getVersion() == "2.1");


    B<std::string> bs;
    ci = bs.getClassInfo();

    //    clog << "bs cid: " << ci.getClassId() << endl;
    //    clog << "bs cna: " << ci.getClassName() << endl;
    //    clog << "bs log: " << ci.getLogCategory() << endl;
    //    clog << "bs ns : " << ci.getNamespace() << endl;
    //    clog << "bs ver: " << ci.getVersion() << endl;

    CPPUNIT_ASSERT(ci.getClassId() == "BBB");
    CPPUNIT_ASSERT(ci.getClassName() == "B");
    CPPUNIT_ASSERT(ci.getLogCategory() == "BBB");
    CPPUNIT_ASSERT(ci.getNamespace() == "");
    CPPUNIT_ASSERT(ci.getVersion() == "2.1");
}
