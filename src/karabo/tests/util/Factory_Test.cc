/* 
 * File:   Factory_Test.cc
 * Author: heisenb
 * 
 * Created on September 19, 2012, 10:34 AM
 */

#include "Factory_Test.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(Factory_Test);

Factory_Test::Factory_Test() {
}

Factory_Test::~Factory_Test() {
}

void Factory_Test::setUp() {
    this->example = new int(1);
}

void Factory_Test::tearDown() {
    delete this->example;
}

void Factory_Test::testMethod() {
    CPPUNIT_ASSERT(*example == 1);
}

void Factory_Test::testFailedMethod() {
    CPPUNIT_ASSERT(++*example == 1);
}
