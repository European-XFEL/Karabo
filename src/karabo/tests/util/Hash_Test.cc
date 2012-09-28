/*
 * File:   Hash_Test.cc
 * Author: heisenb
 *
 * Created on Sep 18, 2012, 6:47:59 PM
 */

#include <karabo/util/Hash.hh>

#include "Hash_Test.hh"

using namespace karabo::util;

CPPUNIT_TEST_SUITE_REGISTRATION(Hash_Test);

Hash_Test::Hash_Test() {
}

Hash_Test::~Hash_Test() {
}

void Hash_Test::setUp() {
}

void Hash_Test::tearDown() {
}

void Hash_Test::testMethod() {
    Hash h("This.is.a.test", 5);
    CPPUNIT_ASSERT(h.getFromPath<int>("This.is.a.test") == 5);
}

void Hash_Test::testFailedMethod() {
    CPPUNIT_ASSERT(false);
}

