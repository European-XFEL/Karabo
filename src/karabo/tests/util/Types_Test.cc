/* 
 * File:   Types_Test.cc
 * Author: heisenb
 * 
 * Created on February 27, 2013, 3:09 PM
 */

#include "Types_Test.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(Types_Test);

Types_Test::Types_Test() {
}

Types_Test::~Types_Test() {
}

void Types_Test::setUp() {
    this->example = new int(1);
}

void Types_Test::tearDown() {
    delete this->example;
}

void Types_Test::testMethod() {
    CPPUNIT_ASSERT(*example == 1);
}

void Types_Test::testFailedMethod() {
    CPPUNIT_ASSERT(++*example == 1);
}
