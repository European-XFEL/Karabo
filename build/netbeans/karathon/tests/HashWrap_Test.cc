/*
 * File:   HashWrap_Test.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on Mar 8, 2013, 10:47:44 AM
 */

#include <boost/python.hpp>
#include "HashWrap_Test.hh"

namespace bp = boost::python;

CPPUNIT_TEST_SUITE_REGISTRATION(HashWrap_Test);

HashWrap_Test::HashWrap_Test() {
    Py_Initialize();
    o_main = bp::import("__main__");
    o_global = o_main.attr("__dict__");
}

HashWrap_Test::~HashWrap_Test() {
}

void HashWrap_Test::setUp() {
    bp::exec(
            "from libkarathon import *\n"
            , o_global, o_global);
    bp::object o_global = o_main.attr("__dict__");
}

void HashWrap_Test::tearDown() {
}

void HashWrap_Test::testMethod() {
    CPPUNIT_ASSERT(true);
}

void HashWrap_Test::testFailedMethod() {
    CPPUNIT_ASSERT(false);
}

