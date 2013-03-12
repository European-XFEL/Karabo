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
    bp::object o_Hash = o_main.attr("Hash");
    try {
        bp::object h = o_Hash();
        h.attr("set")("a.b.c1.d", 1);
        CPPUNIT_ASSERT(h.attr("get")("a").attr("has")("b"));
        CPPUNIT_ASSERT(h.attr("get")("a.b").attr("has")("c1"));
        CPPUNIT_ASSERT(h.attr("get")("a.b.c1").attr("has")("d"));
        CPPUNIT_ASSERT(h.attr("get")("a.b.c1.d") == 1);
        CPPUNIT_ASSERT(h.attr("has")("a.b.c1.d"));
        CPPUNIT_ASSERT(h.attr("get")("a").attr("has")("b.c1"));

        h.attr("set")("a.b.c2.d", "1");
        CPPUNIT_ASSERT(h.attr("get")("a").attr("has")("b"));
        CPPUNIT_ASSERT(h.attr("get")("a.b").attr("has")("c1"));
        CPPUNIT_ASSERT(h.attr("get")("a.b").attr("has")("c2"));
        CPPUNIT_ASSERT(h.attr("get")("a.b").attr("has")("c2.d"));
        //CPPUNIT_ASSERT(h.get<Hash > ("a.b").is<string > ("c2.d") == true);
        CPPUNIT_ASSERT(h.attr("get")("a.b.c2").attr("has")("d"));
        CPPUNIT_ASSERT(h.attr("get")("a.b.c2.d") == "1");

        h.attr("set")("a.b[0]", o_Hash("a", 1));
        CPPUNIT_ASSERT(h.attr("get")("a").attr("has")("b"));
        CPPUNIT_ASSERT(h.attr("get")("a").attr("__len__")() == 1);
        //CPPUNIT_ASSERT(h.is<std::vector<Hash> >("a.b") == true);
        CPPUNIT_ASSERT(h.attr("get")("a.b").attr("__len__")() == 1);
        CPPUNIT_ASSERT(h.attr("get")("a.b")[0].attr("__len__")() == 1);
        CPPUNIT_ASSERT(h.attr("get")("a.b")[0].attr("get")("a") == 1);
        CPPUNIT_ASSERT(h.attr("get")("a.b[0].a") == 1);

        h.attr("set")("a.b[2]", o_Hash("a", "1"));
        CPPUNIT_ASSERT(h.attr("get")("a").attr("has")("b"));
        CPPUNIT_ASSERT(h.attr("get")("a").attr("__len__")() == 1);
//        CPPUNIT_ASSERT(h.is<std::vector<Hash> >("a.b") == true);
        CPPUNIT_ASSERT(h.attr("has")("a.b"));
        CPPUNIT_ASSERT(h.attr("get")("a.b").attr("__len__")() == 3);
        CPPUNIT_ASSERT(h.attr("get")("a.b[0].a") == 1);
//        CPPUNIT_ASSERT(h.attr("get")("a.b[1]").attr("empty")());
        CPPUNIT_ASSERT(h.attr("get")("a.b[2].a") == "1");
        CPPUNIT_ASSERT(h.attr("get")("a.b")[0].attr("get")("a") == 1);
        CPPUNIT_ASSERT(h.attr("get")("a.b")[1].attr("empty")());
        CPPUNIT_ASSERT(h.attr("get")("a.b")[2].attr("get")("a") == "1");

//        CPPUNIT_ASSERT(!h.attr("get")("a.b[0]").attr("empty")());
//        CPPUNIT_ASSERT( h.attr("get")("a.b[1]").attr("empty")());
//        CPPUNIT_ASSERT(h.attr("get")("a.b[2]").attr("empty")());
        
    } catch(const bp::error_already_set&) {
        PyErr_Print();
        CPPUNIT_ASSERT(false);
    }
    
    try {
        bp::object h = o_Hash();
        h.attr("set")("a.b.c", 1);
        h.attr("set")("a.b.c", 2);
        CPPUNIT_ASSERT(h.attr("get")("a.b.c") == 2);
//        CPPUNIT_ASSERT(h.get<Hash > ("a").is<Hash > ("b") == true);
//        CPPUNIT_ASSERT(h.is<int>("a.b.c") == true);
        CPPUNIT_ASSERT(h.attr("has")("a.b"));
        CPPUNIT_ASSERT(!h.attr("has")("a.b.c.d"));
    } catch(const bp::error_already_set&) {
        PyErr_Print();
        CPPUNIT_ASSERT(false);
    }

    try {
        bp::object h = o_Hash("a[0]", o_Hash("a", 1), "a[1]", o_Hash("a", 1));
        CPPUNIT_ASSERT(h.attr("get")("a[0].a") == 1);
        CPPUNIT_ASSERT(h.attr("get")("a[1].a") == 1);
    } catch(const bp::error_already_set&) {
        PyErr_Print();
        CPPUNIT_ASSERT(false);
    }

    try {
        bp::object h = o_Hash();
        h.attr("set")("x[0].y[0]", o_Hash("a", 4.2, "b", "red", "c", true));
        h.attr("set")("x[1].y[0]", o_Hash("a", 4.0, "b", "green", "c", false));
        CPPUNIT_ASSERT( h.attr("get")("x[0].y[0].c"));
        CPPUNIT_ASSERT(!h.attr("get")("x[1].y[0].c"));
        CPPUNIT_ASSERT( h.attr("get")("x[0].y[0].b") == "red");
        CPPUNIT_ASSERT( h.attr("get")("x[1].y[0].b") == "green");
    } catch(const bp::error_already_set&) {
        PyErr_Print();
        CPPUNIT_ASSERT(false);
    }

    try {
        bp::object h1 = o_Hash("a[0].b[0]", o_Hash("a", 1));
        bp::object h2 = o_Hash("a[0].b[0]", o_Hash("a", 2));

        h1.attr("set")("a[0]", h2);
        CPPUNIT_ASSERT(h1.attr("get")("a[0].a[0].b[0].a") == 2);
        h1.attr("set")("a", h2);
        CPPUNIT_ASSERT(h1.attr("get")("a.a[0].b[0].a") == 2);
    } catch(const bp::error_already_set&) {
        PyErr_Print();
        CPPUNIT_ASSERT(false);
    }

    try {
        bp::str s;
        bp::object h = o_Hash("a", "1");
        CPPUNIT_ASSERT(h.attr("__getitem__")("a") == "1");
        h.attr("__setitem__")("a", "2");
        CPPUNIT_ASSERT(h.attr("__getitem__")("a") == "2");
    } catch(const bp::error_already_set&) {
        PyErr_Print();
        CPPUNIT_ASSERT(false);
    }

    try {
        bp::object h = o_Hash();
        bool a = true;
        h.attr("__setitem__")("a", a);
        CPPUNIT_ASSERT(h.attr("getType")("a") == "BOOL");
//        CPPUNIT_ASSERT(h.attr("is_type")("a", "BOOL"));
    } catch(const bp::error_already_set&) {
        PyErr_Print();
        CPPUNIT_ASSERT(false);
    }
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
    //    bp::exec("h = Hash('a.b', 12)\nprint ' '\nprint h\n"
    //             , o_global, o_global);
    CPPUNIT_ASSERT(true);
}

