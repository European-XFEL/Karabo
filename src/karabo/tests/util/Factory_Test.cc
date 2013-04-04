/* 
 * File:   Factory_Test.cc
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on August 26, 2012, 7:20 PM
 * 
 * © Copyright 2012 Burkhard C. Heisen
 */

#include <karabo/util/Factory.hh>
#include <karabo/util/PluginLoader.hh>

#include "Factory_Test.hh"

using namespace karabo::util;

CPPUNIT_TEST_SUITE_REGISTRATION(Factory_Test);

KARABO_REGISTER_IN_FACTORY(Interface, A);

KARABO_REGISTER_IN_FACTORY_1(Interface, A, std::string);

KARABO_REGISTER_IN_FACTORY_1(Interface, A, int);


Factory_Test::Factory_Test() {
}


Factory_Test::~Factory_Test() {
}


void Factory_Test::setUp() {
}


void Factory_Test::tearDown() {
}


void Factory_Test::testObjectCreation() {

    //Factory::registerClass<A>("A");
    //Factory::registerClass<A, std::string>("A");
    boost::shared_ptr<Interface> ptr1 = Factory<Interface>::create("AAA");
    CPPUNIT_ASSERT(ptr1->foo() == "A:void");
    Interface::Pointer ptr2 = Factory<Interface>::create<std::string>("AAA", "Horrey!");
    CPPUNIT_ASSERT(ptr2->foo() == "A:string");
    //Factory<A>::create<int>("A", 42);


}


void Factory_Test::testClassInfo() {

    CPPUNIT_ASSERT(Interface::classInfo().getClassId() == "SimpleInterface");
    CPPUNIT_ASSERT(Interface::classInfo().getClassName() == "Interface");
    CPPUNIT_ASSERT(Interface::classInfo().getLogCategory() == "SimpleInterface");
    CPPUNIT_ASSERT(Interface::classInfo().getNamespace() == "");
    CPPUNIT_ASSERT(Interface::classInfo().getVersion() == "1.0");

    CPPUNIT_ASSERT(karabo::util::PluginLoader::classInfo().getClassId() == "PluginLoader");
    CPPUNIT_ASSERT(karabo::util::PluginLoader::classInfo().getClassName() == "PluginLoader");
    CPPUNIT_ASSERT(karabo::util::PluginLoader::classInfo().getLogCategory() == "karabo.util.PluginLoader");
    CPPUNIT_ASSERT(karabo::util::PluginLoader::classInfo().getNamespace() == "karabo::util");
    CPPUNIT_ASSERT(karabo::util::PluginLoader::classInfo().getVersion() == "1.0");

    boost::shared_ptr<Interface> ptr1 = Factory<Interface>::create("AAA");
    ClassInfo ci = ptr1->getClassInfo();
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
    CPPUNIT_ASSERT(ci.getClassName() == "B<T>");
    CPPUNIT_ASSERT(ci.getLogCategory() == "BBB");
    CPPUNIT_ASSERT(ci.getNamespace() == "");
    CPPUNIT_ASSERT(ci.getVersion() == "2.1");
    
}