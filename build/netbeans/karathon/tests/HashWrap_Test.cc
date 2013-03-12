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
    bp::object o_Hash = o_main.attr("Hash");
    try {
        {
            bp::object h = o_Hash();
            CPPUNIT_ASSERT(h.attr("__len__")() == 0);
            CPPUNIT_ASSERT(h.attr("empty")());
        }

        {
            bp::object h = o_Hash("a", 1);
            CPPUNIT_ASSERT(!h.attr("empty")());
            CPPUNIT_ASSERT(h.attr("__len__")() == 1);
            CPPUNIT_ASSERT(h.attr("get")("a") == 1);
        }

        {
            bp::object h = o_Hash("a", 1, "b", 2.0);
            CPPUNIT_ASSERT(!h.attr("empty")());                 // h.empty() -> False
            CPPUNIT_ASSERT(h.attr("__len__")() == 2);           // len(h) -> 2
            CPPUNIT_ASSERT(h.attr("get")("a") == 1);            // h.get("a") -> 1
            CPPUNIT_ASSERT(h.attr("__getitem__")("b") == 2.0);  // h["b"] -> 2.0
        }
        
        {
            bp::list lst;
            for (size_t i = 0; i < 5; i++) lst.append(5);
            bp::object h = o_Hash("a.b.c", 1, "b.c", 2.0, "c", 3.7, "d.e", "4",
                                  "e.f.g.h", lst,
                                  "F.f.f.f.f", o_Hash("x.y.z", 99));
            CPPUNIT_ASSERT(!h.attr("empty")());
            CPPUNIT_ASSERT(h.attr("__len__")() == 6);
            CPPUNIT_ASSERT(h.attr("get")("a.b.c") == 1);
            CPPUNIT_ASSERT(h.attr("get")("a.b.c") == 1);
            CPPUNIT_ASSERT(h.attr("get")("b.c") == 2.0);
            CPPUNIT_ASSERT(h.attr("get")("c") == 3.7);
            CPPUNIT_ASSERT(h.attr("get")("d.e") == "4");
            CPPUNIT_ASSERT(h.attr("get")("e.f.g.h")[0] == 5);
            CPPUNIT_ASSERT(h.attr("get")("e.f.g.h").attr("__len__")() == 5);
            CPPUNIT_ASSERT(h.attr("get")("F.f.f.f.f").attr("get")("x.y.z") == 99);
            CPPUNIT_ASSERT(h.attr("get")("F.f.f.f.f.x.y.z") == 99);

            // Check 'flatten'
            bp::object flat = o_Hash();
            o_Hash.attr("flatten")(h, flat);

            CPPUNIT_ASSERT(!flat.attr("empty")());
            CPPUNIT_ASSERT(flat.attr("__len__")() == 6);
            CPPUNIT_ASSERT(flat.attr("get")("a.b.c", " ") == 1);
            CPPUNIT_ASSERT(flat.attr("get")("b.c", " ") == 2.0);
            CPPUNIT_ASSERT(flat.attr("get")("c", " ") == 3.7);
            CPPUNIT_ASSERT(flat.attr("get")("d.e", " ") == "4");
            CPPUNIT_ASSERT(flat.attr("get")("e.f.g.h", " ")[0] == 5);
            CPPUNIT_ASSERT(flat.attr("get")("F.f.f.f.f.x.y.z", " ") == 99);

            bp::object tree = o_Hash();
            flat.attr("unflatten")(tree);

            CPPUNIT_ASSERT(!tree.attr("empty")());
            CPPUNIT_ASSERT(tree.attr("__len__")() == 6);
            CPPUNIT_ASSERT(tree.attr("get")("a.b.c") == 1);
            CPPUNIT_ASSERT(tree.attr("get")("b.c") == 2.0);
            CPPUNIT_ASSERT(tree.attr("get")("c") == 3.7);
            CPPUNIT_ASSERT(tree.attr("get")("d.e") == "4");
            CPPUNIT_ASSERT(tree.attr("get")("e.f.g.h")[0] == 5);
            CPPUNIT_ASSERT(tree.attr("get")("F.f.f.f.f").attr("get")("x.y.z") == 99);
            CPPUNIT_ASSERT(tree.attr("get")("F.f.f.f.f.x.y.z") == 99);

        }
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

