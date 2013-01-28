/* 
 * File:   Factory_Test.cc
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on August 26, 2012, 7:20 PM
 * 
 * © Copyright 2012 Burkhard C. Heisen
 */

#include <karabo/util/Factory.hh>

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
    boost::shared_ptr<Interface> ptr1 = Factory<Interface>::create("A");
    CPPUNIT_ASSERT(ptr1->foo() == "A:void");
    Interface::Pointer ptr2 = Factory<Interface>::create<std::string>("A", "Horrey!");
    CPPUNIT_ASSERT(ptr2->foo() == "A:string");
    //Factory<A>::create<int>("A", 42);
    
    
}