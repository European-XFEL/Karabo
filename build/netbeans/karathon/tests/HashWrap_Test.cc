/*
 * File:   HashWrap_Test.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on Mar 8, 2013, 10:47:44 AM
 */

#include <boost/python.hpp>
#include <iostream>
#include "HashWrap_Test.hh"

namespace bp = boost::python;
using namespace std;

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
             "import os\nimport sys\nsys.path.append(os.getcwd()+'/dist/Debug/GNU-Linux-x86')\nfrom libkarathon import *\n"
             , o_global, o_global);
}

void HashWrap_Test::tearDown() {
}

void HashWrap_Test::testConstructors() {
    try {
        bp::object o_Hash = o_main.attr("Hash");
        bp::object h = o_Hash();
        CPPUNIT_ASSERT(bp::extract<int>(h.attr("__len__")()) == 0);
        CPPUNIT_ASSERT(bp::extract<bool>(h.attr("empty")()) == true);
    } catch (const bp::error_already_set&) {
        PyErr_Print();
        CPPUNIT_ASSERT(false);
    }
}

void HashWrap_Test::testGetSet() {
    //    bp::exec("h = Hash('a.b', 12)\nprint ' '\nprint h\n"
    //             , o_global, o_global);
    CPPUNIT_ASSERT(true);
}

void HashWrap_Test::testGetAs() {
    CPPUNIT_ASSERT(true);
}

void HashWrap_Test::testFind() {
    CPPUNIT_ASSERT(true);
}

void HashWrap_Test::testAttributes() {
    CPPUNIT_ASSERT(true);
}

void HashWrap_Test::testIteration() {
    CPPUNIT_ASSERT(true);
}

void HashWrap_Test::testMerge() {
    CPPUNIT_ASSERT(true);
}

