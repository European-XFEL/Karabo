/* 
 * File:   Configurator_Test.cc
 * Author: heisenb
 * 
 * Created on January 28, 2013, 2:49 PM
 */

#include <karabo/util/Configurator.hh>

#include "Configurator_Test.hh"

using namespace karabo::util;

CPPUNIT_TEST_SUITE_REGISTRATION(Configurator_Test);


Configurator_Test::Configurator_Test() {
}


Configurator_Test::~Configurator_Test() {
}


void Configurator_Test::setUp() {
}


void Configurator_Test::tearDown() {
}

//KARABO_REGISTER_FOR_CONFIGURATION(Base_N, Base_N);
//KARABO_REGISTER_FOR_CONFIGURATION(Base_Y, Base_Y);
//
//void Configurator_Test::testBase() {
//
//
//    {
//        Base_N::Pointer p = Configurator<Base_N>::createAndConfigure("Base_N", Hash("a", 1));
//        Hash h = p->getConfig();
//        CPPUNIT_ASSERT(h.size() == 0);
//
//    }
//
//    {
//        Base_Y::Pointer p = Configurator1<Base_Y>::createAndConfigure("Base_Y", Hash("a", 1));
//        Hash h = p->getConfig();
//        CPPUNIT_ASSERT(h.has("base.a") == true);
//    }
//}

KARABO_REGISTER_FOR_CONFIGURATION_2(Base_N, Sub1_N<Base_N>);
KARABO_REGISTER_FOR_CONFIGURATION_2(Base_N, Sub1_Y<Base_N>);
KARABO_REGISTER_FOR_CONFIGURATION_2(Base_Y, Sub1_N<Base_Y>);
KARABO_REGISTER_FOR_CONFIGURATION_2(Base_Y, Sub1_Y<Base_Y>);


void Configurator_Test::testSub1() {

    {
        Base_N::Pointer p = Configurator<Base_N>::create("Base_N-Sub1_N", Hash("a", 1), false);
        Hash h = p->getConfig();
        CPPUNIT_ASSERT(h.has("base.a") == false);
        CPPUNIT_ASSERT(h.has("sub1.a") == false);

    }

    {
        Base_N::Pointer p = Configurator<Base_N>::create("Base_N-Sub1_Y", Hash("a", 1), false);
        Hash h = p->getConfig();
        CPPUNIT_ASSERT(h.has("base.a") == false);
        CPPUNIT_ASSERT(h.has("sub1.a") == true);
    }

    {
        Base_Y::Pointer p = Configurator<Base_Y>::create("Base_Y-Sub1_N", Hash("a", 1), false);
        Hash h = p->getConfig();
        CPPUNIT_ASSERT(h.has("base.a") == true);
        CPPUNIT_ASSERT(h.has("sub1.a") == false);
    }

    {
        Base_Y::Pointer p = Configurator<Base_Y>::create("Base_Y-Sub1_Y", Hash("a", 1), false);
        Hash h = p->getConfig();
        CPPUNIT_ASSERT(h.has("base.a") == true);
        CPPUNIT_ASSERT(h.has("sub1.a") == true);

    }

}


