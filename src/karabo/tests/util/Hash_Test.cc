/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   Hash_Test.cc
 * Author: heisenb
 *
 * Created on Sep 18, 2012, 6:47:59 PM
 */

#include "Hash_Test.hh"

#include <climits>
#include <karabo/util/Exception.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/NDArray.hh>
#include <karabo/util/PackParameters.hh>
#include <karabo/util/Schema.hh>
#include <karabo/util/SimpleElement.hh>
#include <stack>
#include <vector>

#include "karabo/util/ToLiteral.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(Hash_Test);

using namespace karabo::util;
using namespace std;


Hash_Test::Hash_Test() {}


Hash_Test::~Hash_Test() {}


void Hash_Test::setUp() {}


void Hash_Test::tearDown() {}


void Hash_Test::testConstructors() {
    {
        Hash h;
        h.set("h", Hash());
        Hash& i = h.get<Hash>("h");
        i.set("i", Hash("j", 5));
        CPPUNIT_ASSERT(h.get<int>("h.i.j") == 5);
    }

    {
        Hash h;
        CPPUNIT_ASSERT(h.empty() == true);
        CPPUNIT_ASSERT(h.size() == 0);
    }

    {
        Hash h("a", 1);
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 1);
        CPPUNIT_ASSERT(h.get<int>("a") == 1);
    }

    {
        Hash h("a", 1, "b", 2.0);
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 2);
        CPPUNIT_ASSERT(h.get<int>("a") == 1);
        CPPUNIT_ASSERT(h.get<double>("b") == 2.0);
    }

    {
        Hash h("a", 1, "b", 2.0, "c", 3.f);
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 3);
        CPPUNIT_ASSERT(h.get<int>("a") == 1);
        CPPUNIT_ASSERT(h.get<double>("b") == 2.0);
        CPPUNIT_ASSERT(h.get<float>("c") == 3.0);
    }

    {
        Hash h("a", 1, "b", 2.0, "c", 3.f, "d", "4");
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 4);
        CPPUNIT_ASSERT(h.get<int>("a") == 1);
        CPPUNIT_ASSERT(h.get<double>("b") == 2.0);
        CPPUNIT_ASSERT(h.get<float>("c") == 3.0);
        CPPUNIT_ASSERT(h.get<string>("d") == "4");
    }

    {
        const Dims shape(2, 5);
        std::vector<float> data(10, 4.2);
        NDArray arr(&data[0], data.size(), shape);

        Hash h("arr", arr);
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.get<NDArray>("arr").getShape().toVector() == shape.toVector());
    }

    {
        Hash h("a", 1, "b", 2.0, "c", 3.f, "d", "4", "e", std::vector<unsigned int>(5, 5));
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 5);
        CPPUNIT_ASSERT(h.get<int>("a") == 1);
        CPPUNIT_ASSERT(h.get<double>("b") == 2.0);
        CPPUNIT_ASSERT(h.get<float>("c") == 3.0);
        CPPUNIT_ASSERT(h.get<string>("d") == "4");
        CPPUNIT_ASSERT(h.get<std::vector<unsigned int>>("e")[0] == 5);
    }

    {
        Hash h("a", 1, "b", 2.0, "c", 3.f, "d", "4", "e", std::vector<unsigned int>(5, 5), "f", Hash("a", 6));
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 6);
        CPPUNIT_ASSERT(h.get<int>("a") == 1);
        CPPUNIT_ASSERT(h.get<double>("b") == 2.0);
        CPPUNIT_ASSERT(h.get<float>("c") == 3.0);
        CPPUNIT_ASSERT(h.get<string>("d") == "4");
        CPPUNIT_ASSERT(h.get<std::vector<unsigned int>>("e")[0] == 5);
        CPPUNIT_ASSERT(h.get<Hash>("f").get<int>("a") == 6);
        CPPUNIT_ASSERT(h.get<int>("f.a") == 6);
    }

    {
        Hash h("a", 1, "b", 2.0, "c", 3.f, "d", "4", "e", std::vector<unsigned int>(5, 5), "f",
               Hash::Pointer(new Hash("a", 6)));
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 6);
        CPPUNIT_ASSERT(h.get<int>("a") == 1);
        CPPUNIT_ASSERT(h.get<double>("b") == 2.0);
        CPPUNIT_ASSERT(h.get<float>("c") == 3.0);
        CPPUNIT_ASSERT(h.get<string>("d") == "4");
        CPPUNIT_ASSERT(h.get<std::vector<unsigned int>>("e")[0] == 5);
        CPPUNIT_ASSERT(h.get<Hash::Pointer>("f")->get<int>("a") == 6);
    }


    {
        Hash h("a", 1, "b", 2.0, "c", 3.f, "d", "4", "e", std::vector<unsigned int>(5, 5), "f",
               std::vector<Hash::Pointer>(5, Hash::Pointer(new Hash("a", 6))));
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 6);
        CPPUNIT_ASSERT(h.get<int>("a") == 1);
        CPPUNIT_ASSERT(h.get<double>("b") == 2.0);
        CPPUNIT_ASSERT(h.get<float>("c") == 3.0);
        CPPUNIT_ASSERT(h.get<string>("d") == "4");
        CPPUNIT_ASSERT(h.get<std::vector<unsigned int>>("e")[0] == 5);
        CPPUNIT_ASSERT(h.get<std::vector<Hash::Pointer>>("f")[3]->get<int>("a") == 6);
    }

    {
        Hash h("a.b.c", 1, "b.c", 2.0, "c", 3.f, "d.e", "4", "e.f.g.h", std::vector<unsigned long long>(5, 5),
               "F.f.f.f.f", Hash("x.y.z", 99));
        h.set("foo.array", NDArray(Dims(5, 5)));
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 7);
        CPPUNIT_ASSERT(h.get<int>("a.b.c") == 1);
        CPPUNIT_ASSERT(h.get<double>("b.c") == 2.0);
        CPPUNIT_ASSERT(h.get<float>("c") == 3.0);
        CPPUNIT_ASSERT(h.get<string>("d.e") == "4");
        CPPUNIT_ASSERT(h.get<std::vector<unsigned long long>>("e.f.g.h")[0] == 5);
        CPPUNIT_ASSERT(h.get<Hash>("F.f.f.f.f").get<int>("x.y.z") == 99);
        CPPUNIT_ASSERT(h.get<int>("F.f.f.f.f.x.y.z") == 99);
        // Internally, Hash-derived classes are stored as Hash
        CPPUNIT_ASSERT(h.getType("foo.array") == karabo::util::Types::HASH);

        // Check 'flatten'
        Hash flat;
        Hash::flatten(h, flat);

        CPPUNIT_ASSERT(flat.empty() == false);
        CPPUNIT_ASSERT(flat.size() == 7);
        CPPUNIT_ASSERT(flat.get<int>("a.b.c", 0) == 1);
        CPPUNIT_ASSERT(flat.get<double>("b.c", 0) == 2.0);
        CPPUNIT_ASSERT(flat.get<float>("c", 0) == 3.0);
        CPPUNIT_ASSERT(flat.get<string>("d.e", 0) == "4");
        CPPUNIT_ASSERT(flat.get<std::vector<unsigned long long>>("e.f.g.h", 0)[0] == 5);
        CPPUNIT_ASSERT(flat.get<int>("F.f.f.f.f.x.y.z", 0) == 99);
        // Internally, Hash-derived classes are stored as Hash
        CPPUNIT_ASSERT(flat.getType("foo.array", 0) == karabo::util::Types::HASH);

        Hash tree;
        flat.unflatten(tree);

        CPPUNIT_ASSERT(tree.empty() == false);
        CPPUNIT_ASSERT(tree.size() == 7);
        CPPUNIT_ASSERT(tree.get<int>("a.b.c") == 1);
        CPPUNIT_ASSERT(tree.get<double>("b.c") == 2.0);
        CPPUNIT_ASSERT(tree.get<float>("c") == 3.0);
        CPPUNIT_ASSERT(tree.get<string>("d.e") == "4");
        CPPUNIT_ASSERT(tree.get<std::vector<unsigned long long>>("e.f.g.h")[0] == 5);
        CPPUNIT_ASSERT(tree.get<Hash>("F.f.f.f.f").get<int>("x.y.z") == 99);
        CPPUNIT_ASSERT(tree.get<int>("F.f.f.f.f.x.y.z") == 99);
        // Internally, Hash-derived classes are stored as Hash
        CPPUNIT_ASSERT(flat.getType("foo.array", 0) == karabo::util::Types::HASH);
    }

    {
        // copy constructor
        Hash tmp("a", 1);
        Hash h(tmp);
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 1);
        CPPUNIT_ASSERT(h.get<int>("a") == 1);
        CPPUNIT_ASSERT(tmp.empty() == false);
    }

    {
        // lvalue assignment
        Hash tmp("a", 1);
        Hash h;
        h = tmp;
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 1);
        CPPUNIT_ASSERT(h.get<int>("a") == 1);
        CPPUNIT_ASSERT(tmp.empty() == false);
    }

    {
        // move constructor
        Hash tmp("a", 1);
        Hash h(std::move(tmp));
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 1);
        CPPUNIT_ASSERT(h.get<int>("a") == 1);
        CPPUNIT_ASSERT(tmp.empty() == true);
    }

    {
        // rvalue assignment
        Hash h;
        Hash tmp("a", 1);
        h = std::move(tmp);
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 1);
        CPPUNIT_ASSERT(h.get<int>("a") == 1);
        CPPUNIT_ASSERT(tmp.empty() == true);
    }
}


void Hash_Test::testGetSet() {
    {
        Hash h;
        h.set("a.b.c1.d", 1);
        CPPUNIT_ASSERT(h.get<Hash>("a").has("b") == true);
        CPPUNIT_ASSERT(h.get<Hash>("a.b").has("c1") == true);
        CPPUNIT_ASSERT(h.get<Hash>("a.b.c1").has("d") == true);
        CPPUNIT_ASSERT(h.get<int>("a.b.c1.d") == 1);
        CPPUNIT_ASSERT(h.has("a.b.c1.d") == true);
        CPPUNIT_ASSERT(h.get<Hash>("a").has("b.c1") == true);

        h.set("a.b.c2.d", "1");
        CPPUNIT_ASSERT(h.get<Hash>("a").has("b") == true);
        CPPUNIT_ASSERT(h.get<Hash>("a.b").has("c1") == true);
        CPPUNIT_ASSERT(h.get<Hash>("a.b").has("c2") == true);
        CPPUNIT_ASSERT(h.get<Hash>("a.b").has("c2.d") == true);
        CPPUNIT_ASSERT(h.get<Hash>("a.b").is<string>("c2.d") == true);
        CPPUNIT_ASSERT(h.get<Hash>("a.b.c2").has("d") == true);
        CPPUNIT_ASSERT(h.get<string>("a.b.c2.d") == "1");

        h.set("a.b[0]", Hash("a", 1));
        CPPUNIT_ASSERT(h.get<Hash>("a").has("b") == true);
        CPPUNIT_ASSERT(h.get<Hash>("a").size() == 1);
        CPPUNIT_ASSERT(h.is<std::vector<Hash>>("a.b") == true);
        CPPUNIT_ASSERT(h.get<std::vector<Hash>>("a.b").size() == 1);
        CPPUNIT_ASSERT(h.get<std::vector<Hash>>("a.b")[0].size() == 1);
        CPPUNIT_ASSERT(h.get<std::vector<Hash>>("a.b")[0].get<int>("a") == 1);
        CPPUNIT_ASSERT(h.get<int>("a.b[0].a") == 1);

        h.set("a.b[2]", Hash("a", "1"));
        CPPUNIT_ASSERT(h.get<Hash>("a").has("b") == true);
        CPPUNIT_ASSERT(h.get<Hash>("a").size() == 1);
        CPPUNIT_ASSERT(h.is<std::vector<Hash>>("a.b") == true);
        CPPUNIT_ASSERT(h.has("a.b") == true);
        CPPUNIT_ASSERT(h.get<std::vector<Hash>>("a.b").size() == 3);
        CPPUNIT_ASSERT(h.get<int>("a.b[0].a") == 1);
        CPPUNIT_ASSERT(h.get<Hash>("a.b[1]").empty() == true);
        CPPUNIT_ASSERT(h.get<string>("a.b[2].a") == "1");
        CPPUNIT_ASSERT(h.get<std::vector<Hash>>("a.b")[0].is<int>("a") == true);
        CPPUNIT_ASSERT(h.get<std::vector<Hash>>("a.b")[1].empty() == true);
        CPPUNIT_ASSERT(h.get<std::vector<Hash>>("a.b")[2].is<string>("a") == true);

        CPPUNIT_ASSERT(h.get<Hash>("a").is<Hash>("b[0]") == true);
        CPPUNIT_ASSERT(h.get<Hash>("a").is<Hash>("b[1]") == true);
        CPPUNIT_ASSERT(h.get<Hash>("a").is<Hash>("b[2]") == true);
        CPPUNIT_ASSERT(h.get<Hash>("a.b[0]").empty() == false);
        CPPUNIT_ASSERT(h.get<Hash>("a.b[1]").empty() == true);
        CPPUNIT_ASSERT(h.get<Hash>("a.b[2]").empty() == false);
    }

    {
        Hash h;
        h.set("a.b.c", 1);
        h.set("a.b.c", 2);
        CPPUNIT_ASSERT(h.get<int>("a.b.c") == 2);
        CPPUNIT_ASSERT(h.get<Hash>("a").is<Hash>("b") == true);
        CPPUNIT_ASSERT(h.is<int>("a.b.c") == true);
        CPPUNIT_ASSERT(h.has("a.b") == true);
        CPPUNIT_ASSERT(h.has("a.b.c.d") == false);
    }

    {
        Hash h("a[0]", Hash("a", 1), "a[1]", Hash("a", 2));
        CPPUNIT_ASSERT(h.get<int>("a[0].a") == 1);
        CPPUNIT_ASSERT(h.get<int>("a[1].a") == 2);
    }

    {
        Hash h;
        h.set("x[0].y[0]", Hash("a", 4.2, "b", "red", "c", true));
        h.set("x[1].y[0]", Hash("a", 4.0, "b", "green", "c", false));
        CPPUNIT_ASSERT(h.get<bool>("x[0].y[0].c") == true);
        CPPUNIT_ASSERT(h.get<bool>("x[1].y[0].c") == false);
        CPPUNIT_ASSERT(h.get<string>("x[0].y[0].b") == "red");
        CPPUNIT_ASSERT(h.get<string>("x[1].y[0].b") == "green");
    }

    {
        Hash h1("a[0].b[0]", Hash("a", 1));
        Hash h2("a[0].b[0]", Hash("a", 2));

        h1.set("a[0]", h2);
        CPPUNIT_ASSERT(h1.get<int>("a[0].a[0].b[0].a") == 2);
        h1.set("a", h2);
        CPPUNIT_ASSERT(h1.get<int>("a.a[0].b[0].a") == 2);
    }

    {
        std::string s;
        Hash h("a", "1");
        h.get("a", s);
        CPPUNIT_ASSERT(s == "1");
        h.get<string>("a") = "2";
        h.get("a", s);
        CPPUNIT_ASSERT(s == "2");
    }

    {
        Hash h;
        bool a = true;
        h.set<int>("a", a);
        CPPUNIT_ASSERT(h.getType("a") == Types::INT32);
        CPPUNIT_ASSERT(h.is<int>("a") == true);
    }

    {
        // test that correct exceptions  are thrown
        Hash h("a", 77, "b[1].c", 88);
        // no exceptions:
        CPPUNIT_ASSERT_NO_THROW(h.get<int>("a"));
        CPPUNIT_ASSERT_NO_THROW(h.get<Hash>("b[0]"));
        CPPUNIT_ASSERT_NO_THROW(h.get<Hash>("b[1]"));
        CPPUNIT_ASSERT_NO_THROW(h.get<int>("b[1].c"));

        // non-existing "normal" path
        CPPUNIT_ASSERT_THROW(h.get<int>("c"), karabo::util::ParameterException);

        // non-existing index of vector that is last item
        CPPUNIT_ASSERT(h.get<vector<Hash>>("b").size() == 2);
        bool caught2 = false;
        try {
            h.get<Hash>("b[2]");
        } catch (karabo::util::ParameterException const& e) {
            caught2 = true;
        }
        CPPUNIT_ASSERT(caught2 == true);

        // item under non-existing index of vector
        bool caught3 = false;
        try {
            h.get<int>("b[2].c");
        } catch (karabo::util::ParameterException const& e) {
            caught3 = true;
        }
        CPPUNIT_ASSERT(caught3 == true);
    }

    {
        // Checks implicit conversions between signed and unsigned integers.
        Hash h("uint32Prop", 30450u);
        CPPUNIT_ASSERT(h.getType("uint32Prop") == Types::UINT32);
        CPPUNIT_ASSERT(h.get<unsigned int>("uint32Prop") == 30450u);
        CPPUNIT_ASSERT_NO_THROW(h.set("uint32Prop", -1));
        // After the previous set, the node type becomes Types::INT32 and an
        // attempt to get it as Types::UINT32 will fail.
        CPPUNIT_ASSERT_THROW(h.get<unsigned int>("uint32Prop"), karabo::util::CastException);
        // Hash::getAs, on the other hand, will do the implicit conversion.
        CPPUNIT_ASSERT(h.getAs<unsigned int>("uint32Prop") == UINT32_MAX);
    }
}

/**
 * A helper class tracing move and copy construction to test Hash::set move-assignment.
 */
struct TraceCopies {
    explicit TraceCopies(int v = 0) : value(v) {}
    TraceCopies(const TraceCopies& other) : value(other.value) {
        ++countCopyConstr;
    }
    // Move constructor, leaving the source in a valid (but other) state,
    // noexcept needed to get used in std::vector<TraceCopies>::resize(n)
    TraceCopies(TraceCopies&& other) noexcept : value(other.value) {
        // ++countMoveConstr;
        other.value = -1;
    }

    int value;

    static int counts() {
        return (countCopyConstr + countMoveConstr);
    }
    static void reset() {
        countCopyConstr = countMoveConstr = 0;
    }

    static int countCopyConstr;
    static int countMoveConstr;
};
int TraceCopies::countCopyConstr = 0;
int TraceCopies::countMoveConstr = 0;


class TraceCopiesHash : protected Hash {
    // A Hash derived object tracing its copies.
    // Since inside the Hash it is stored like a Hash,
    // tracing has to be indirect via its TraceCopies member

   public:
    // Class info needed for these Hash-inheriting objects
    KARABO_CLASSINFO(TraceCopiesHash, "TraceCopiesHash", "2.11");

    TraceCopiesHash() {
        set("v", TraceCopies(0));
    }
    explicit TraceCopiesHash(const TraceCopies& v) {
        set("v", v);
    }
    TraceCopiesHash(const TraceCopiesHash& other) : Hash(other) {}
    TraceCopiesHash(TraceCopiesHash&& other) noexcept : Hash(other) {
        // Put back into a valid state - Hash(other) just cleared (i.e. removed key "v").
        // This will not be called inside Hash where this object is stored as Hash and not as TraceCopiesHash...
        other.setValue(TraceCopies(-1));
    }

    TraceCopiesHash& operator=(TraceCopiesHash&& rhs) noexcept {
        *static_cast<Hash*>(this) = static_cast<Hash&&>(rhs);
        return *this;
    }

    const TraceCopies& getValue() const {
        return get<TraceCopies>("v");
    }
    void setValue(TraceCopies&& v) {
        set("v", std::move(v));
    }
    void setValue(int v) {
        get<TraceCopies>("v").value = v;
    }
};

void Hash_Test::testSetMoveSemantics() {
    TraceCopies::reset(); // Ensure that nothing yet - e.g. when other test that ran before failed
    {
        // test Hash::set of normal non-const object
        TraceCopies ta(2);
        Hash h;
        // Normal set
        h.set("ta", ta);
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr); // copied into Hash
        CPPUNIT_ASSERT_EQUAL(2, h.get<TraceCopies>("ta").value);
        // Normal set to the now existing node
        ta.value = 4;
        h.set("ta", ta);
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr); // ta copied again into Hash
        CPPUNIT_ASSERT_EQUAL(4, h.get<TraceCopies>("ta").value);

        // 'moving' set
        h.set("tb", std::move(ta));
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr); // unchanged
        CPPUNIT_ASSERT_EQUAL(4, h.get<TraceCopies>("tb").value);
        // 'moving' set to the now existing node
        ta.value = 8; // get back into a defined state after object "behind" was moved away from 'ta'
        h.set("tb", std::move(ta));
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr); // again unchanged
        CPPUNIT_ASSERT_EQUAL(8, h.get<TraceCopies>("tb").value);

        // set of const
        const TraceCopies tc(3);
        h.set("tc", tc);
        CPPUNIT_ASSERT_EQUAL(3, TraceCopies::countCopyConstr); // copied...
        CPPUNIT_ASSERT_EQUAL(3, h.get<TraceCopies>("tc").value);
        // set of const to the now existing node
        h.get<TraceCopies>("tc").value = 42;
        CPPUNIT_ASSERT_EQUAL(42, h.get<TraceCopies>("tc").value);
        h.set("tc", tc);
        CPPUNIT_ASSERT_EQUAL(4, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(3, h.get<TraceCopies>("tc").value);

        TraceCopies::reset(); // Start next round from zero
    }

    {
        // test set of Hash
        TraceCopies ta(11);
        Hash h;
        Hash hInner;
        hInner.set("ta", ta);
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr);
        // We set a non-const Hash: It gets copied, so the contained TraceCopies does.
        h.set("h", hInner);
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(11, h.get<TraceCopies>("h.ta").value);
        // same again to now existing node
        h.get<TraceCopies>("h.ta").value = 22;
        CPPUNIT_ASSERT_EQUAL(22, h.get<TraceCopies>("h.ta").value);
        h.set("h", hInner);
        CPPUNIT_ASSERT_EQUAL(3, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(11, h.get<TraceCopies>("h.ta").value);

        // We move-set a Hash: It gets empty and - since content is just moved - no copy/assignment of TraceCopies
        h.set("h2", std::move(hInner));
        CPPUNIT_ASSERT(hInner.empty());
        CPPUNIT_ASSERT_EQUAL(11, h.get<TraceCopies>("h2.ta").value);
        // same again to now existing node
        hInner.set("ta", TraceCopies(17));
        h.set("h2", std::move(hInner));
        CPPUNIT_ASSERT(hInner.empty());
        CPPUNIT_ASSERT_EQUAL(17, h.get<TraceCopies>("h2.ta").value);

        // We set a const Hash: As for the non-const, it gets copied, so the contained TraceCopies does.
        const Hash hInner2("ta2", ta);
        h.set("h3", hInner2);
        CPPUNIT_ASSERT_EQUAL(5, TraceCopies::countCopyConstr);
        // same again to now existing node
        h.get<TraceCopies>("h3.ta2").value = 22;
        CPPUNIT_ASSERT_EQUAL(22, h.get<TraceCopies>("h3.ta2").value);
        h.set("h3", hInner2);
        CPPUNIT_ASSERT_EQUAL(6, TraceCopies::countCopyConstr); // another copy
        CPPUNIT_ASSERT_EQUAL(11, h.get<TraceCopies>("h3.ta2").value);

        TraceCopies::reset();
    }

    {
        // test set of Hash, but now to path with index
        // same test as above, extended to set also to non-existing index
        TraceCopies ta(11);
        Hash h;
        Hash hInner;
        hInner.set("ta", ta);
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr);
        // We set a non-const Hash: It gets copied, so the contained TraceCopies does.
        h.set("h[0]", hInner);
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(11, h.get<TraceCopies>("h[0].ta").value);
        // same again to now existing node
        h.get<TraceCopies>("h[0].ta").value = 22;
        CPPUNIT_ASSERT_EQUAL(22, h.get<TraceCopies>("h[0].ta").value);
        h.set("h[0]", hInner);
        CPPUNIT_ASSERT_EQUAL(3, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(11, h.get<TraceCopies>("h[0].ta").value);
        // and now to non-existing index
        h.set("h[1]", hInner);
        CPPUNIT_ASSERT_EQUAL(4, TraceCopies::countCopyConstr); // 5 without noexcept in Hash::Hash(Hash&&) etc.!
        CPPUNIT_ASSERT_EQUAL(11, h.get<TraceCopies>("h[1].ta").value);

        // We move-set a Hash: It gets empty and - since content is just moved - no copy/assignment of TraceCopies
        h.set("h2[0]", std::move(hInner));
        CPPUNIT_ASSERT(hInner.empty());
        CPPUNIT_ASSERT_EQUAL(11, h.get<TraceCopies>("h2[0].ta").value);
        // same again to now existing node
        hInner.set("ta", TraceCopies(18));
        h.set("h2[0]", std::move(hInner));
        CPPUNIT_ASSERT(hInner.empty());
        CPPUNIT_ASSERT_EQUAL(18, h.get<TraceCopies>("h2[0].ta").value);
        // now to not yet existing index
        hInner.set("ta", TraceCopies(19));
        h.set("h2[1]", std::move(hInner));
        CPPUNIT_ASSERT(hInner.empty());
        CPPUNIT_ASSERT_EQUAL(19, h.get<TraceCopies>("h2[1].ta").value);

        // We set a const Hash: As for the non-const, it gets copied, so the contained TraceCopies does.
        const Hash hInner2("ta2", ta);
        CPPUNIT_ASSERT_EQUAL(5, TraceCopies::countCopyConstr);
        h.set("h3[0]", hInner2);
        CPPUNIT_ASSERT_EQUAL(6, TraceCopies::countCopyConstr);
        // same again to now existing node
        h.get<TraceCopies>("h3[0].ta2").value = 22;
        CPPUNIT_ASSERT_EQUAL(22, h.get<TraceCopies>("h3[0].ta2").value);
        h.set("h3[0]", hInner2);
        CPPUNIT_ASSERT_EQUAL(7, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(11, h.get<TraceCopies>("h3[0].ta2").value);
        // same now to non-existing index
        h.set("h3[1]", hInner2);
        CPPUNIT_ASSERT_EQUAL(8, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(11, h.get<TraceCopies>("h3[1].ta2").value);

        TraceCopies::reset();
    }

    {
        // test Hash::set of Hash derived object like NDArray
        TraceCopiesHash ta(TraceCopies(2));
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr); // TraceCopiesHash ctr. takes it by rerefence, so copies
        Hash h;
        // Normal set
        h.set("ta", ta);
        CPPUNIT_ASSERT_EQUAL(2, h.get<TraceCopiesHash>("ta").getValue().value);
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr);
        // Normal set to the now existing node
        ta.setValue(4); // set inner value, no construction
        h.set("ta", ta);
        CPPUNIT_ASSERT_EQUAL(3, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(4, h.get<TraceCopiesHash>("ta").getValue().value);

        // 'moving' set - since the TraceCopiesHash object is moved (as a Hash!),
        //                this leaves no trace, so we cannot really test :-(
        h.set("tb", std::move(ta));
        // Moving ta indeed leaves no trace of TraceCopies construction,
        // but to leave 'ta' in a valid state (with existing key "v"), a default constructed TraceCopiesHash
        // is assigned to it and the default construction move-assigns the TraceCopies object inside.
        CPPUNIT_ASSERT_EQUAL(4, h.get<TraceCopiesHash>("tb").getValue().value);
        CPPUNIT_ASSERT_NO_THROW(ta.getValue()); // Ensure that 'ta' is in a valid state after moving from it.
        ta.setValue(42);
        // 'moving' set to the now existing node
        h.set("tb", std::move(ta));
        // Same counting as above: one move construction expected
        CPPUNIT_ASSERT_EQUAL(3, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(h), 42, h.get<TraceCopiesHash>("tb").getValue().value);

        // set of const
        const TraceCopiesHash tc(TraceCopies(3));
        CPPUNIT_ASSERT_EQUAL(4, TraceCopies::countCopyConstr);
        h.set("tc", tc);
        CPPUNIT_ASSERT_EQUAL(5, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(3, h.get<TraceCopiesHash>("tc").getValue().value);
        // set of const to the now existing node
        h.get<TraceCopiesHash>("tc").setValue(-42);
        CPPUNIT_ASSERT_EQUAL(-42, h.get<TraceCopiesHash>("tc").getValue().value);
        h.set("tc", tc);
        CPPUNIT_ASSERT_EQUAL(6, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(3, h.get<TraceCopiesHash>("tc").getValue().value);

        TraceCopies::reset();
    }

    {
        // Test Hash::set(path, std::any)
        Hash h;
        std::any a(TraceCopies(4));
        h.set("a", a);
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr); // a and thus its TraceCopies got copied
        CPPUNIT_ASSERT_EQUAL(4, h.get<TraceCopies>("a").value);

        const std::any& a2 = a;
        h.set("a2", a2);
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr); // a and thus its TraceCopies get copied
        CPPUNIT_ASSERT_EQUAL(4, h.get<TraceCopies>("a2").value);

        h.set("a3", std::move(a));
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr); // copied
        CPPUNIT_ASSERT_EQUAL(4, h.get<TraceCopies>("a3").value);

        TraceCopies::reset();
    }

    // The next tests are not fitting well into testSetMoveSemantics() since they have nothing
    // to do with move semantics - but with the special overloads of Element::setValue for 'char*' etc.,
    // so they are closely related.
    {
        // test Hash::set of various strings,
        Hash h;
        h.set("const_char_pointer", "a");
        CPPUNIT_ASSERT_EQUAL(std::string("a"), h.get<std::string>("const_char_pointer"));

        char cText[] = "a2and3";
        h.set("char_array", cText);
        CPPUNIT_ASSERT_EQUAL(std::string("a2and3"), h.get<std::string>("char_array"));

        char* cPtr = cText;
        h.set("char_ptr", cPtr);
        CPPUNIT_ASSERT_EQUAL(std::string("a2and3"), h.get<std::string>("char_ptr"));

        h.set("tmp_string", std::string("b"));
        CPPUNIT_ASSERT_EQUAL(std::string("b"), h.get<std::string>("tmp_string"));

        const std::string b1("b1");
        h.set("const_string", b1);
        CPPUNIT_ASSERT_EQUAL(std::string("b1"), h.get<std::string>("const_string"));

        std::string b2("b2");
        h.set("string", b2);
        CPPUNIT_ASSERT_EQUAL(std::string("b2"), h.get<std::string>("string"));
    }

    {
        // test wide characters
        // Shouldn't be used much in Karabo since there is no serialisation support,
        // but there is treament in Element::setValue to convert to wstring
        Hash h;
        h.set("const_wchart_pointer", L"a");
        // CPPUNIT_ASSERT_EQUAL does not support std::wstring...
        CPPUNIT_ASSERT(std::wstring(L"a") == h.get<std::wstring>("const_wchart_pointer"));

        wchar_t cText[] = L"a2and3";
        h.set("wchart_array", cText);
        CPPUNIT_ASSERT(std::wstring(L"a2and3") == h.get<std::wstring>("wchart_array"));

        wchar_t* cPtr = cText;
        h.set("wchart_ptr", cPtr);
        CPPUNIT_ASSERT(std::wstring(L"a2and3") == h.get<std::wstring>("wchart_ptr"));
    }

    // Some final checks
    {
        // Ensure that setting still works when type is not deduced, but explicitely specified
        // (as was allowed before introducing move semantics).
        Hash h;

        h.set<int>("int", 1);
        CPPUNIT_ASSERT_EQUAL(1, h.get<int>("int"));

        h.set<Hash>("hash", Hash("a", "b"));
        CPPUNIT_ASSERT_MESSAGE(toString(h), h.get<Hash>("hash").fullyEquals(Hash("a", "b")));

        h.set<NDArray>("ndarray", NDArray(Dims({20}), 5));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(h), 20ul, h.get<NDArray>("ndarray").size());

        h.set<TraceCopies>("trace", TraceCopies(77));
        CPPUNIT_ASSERT_EQUAL(77, h.get<TraceCopies>("trace").value);

        // Test also Element::setValue<typename>(..) directly
        h.getNode("int").setValue<int>(42);
        CPPUNIT_ASSERT_EQUAL(42, h.get<int>("int"));

        h.getNode("hash").setValue<Hash>(Hash("b", "c"));
        CPPUNIT_ASSERT_MESSAGE(toString(h), h.get<Hash>("hash").fullyEquals(Hash("b", "c")));

        h.getNode("ndarray").setValue<NDArray>(NDArray(Dims({10}), 6));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(h), 10ul, h.get<NDArray>("ndarray").size());

        h.getNode("trace").setValue<TraceCopies>(TraceCopies(88));
        CPPUNIT_ASSERT_EQUAL(88, h.get<TraceCopies>("trace").value);

        TraceCopies::reset();
    }
}

void Hash_Test::testSetAttributeMoveSemantics() {
    TraceCopies::reset(); // Ensure that nothing yet - e.g. when other test that ran before failed
    {
        // test Hash::setAttribute of normal non-const object
        TraceCopies ta(2);
        Hash h("a", 1);
        // Normal set
        h.setAttribute("a", "ta", ta);
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr); // copied into Hash
        CPPUNIT_ASSERT_EQUAL(2, h.getAttribute<TraceCopies>("a", "ta").value);
        // Normal set to the now existing node
        ta.value = 4;
        h.setAttribute("a", "ta", ta);
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr); // again copied into Hash
        CPPUNIT_ASSERT_EQUAL(4, h.getAttribute<TraceCopies>("a", "ta").value);

        // 'moving' set
        h.setAttribute("a", "tb", std::move(ta));
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr); // unchanged
        CPPUNIT_ASSERT_EQUAL(4, h.getAttribute<TraceCopies>("a", "tb").value);
        CPPUNIT_ASSERT_EQUAL(-1, ta.value); // the moved-from object gets -1 assigned to value in the move constructor
        // 'moving' set to the now existing node
        ta.value = 8; // in general, a moved-from object is in a valid state, but we do not necessarily know which...
        h.setAttribute("a", "tb", std::move(ta));
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr); // again unchanged
        CPPUNIT_ASSERT_EQUAL(8, h.getAttribute<TraceCopies>("a", "tb").value);
        CPPUNIT_ASSERT_EQUAL(
              -1, ta.value); // as before: the moved-from object gets -1 assigned to value in the move constructor
        ta.value = 9;
        h.setAttribute<TraceCopies>("a", "tb", std::move(ta));
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr); // again unchanged
        CPPUNIT_ASSERT_EQUAL(9, h.getAttribute<TraceCopies>("a", "tb").value);

        // set of const
        const TraceCopies tc(3);
        h.setAttribute("a", "tc", tc);
        CPPUNIT_ASSERT_EQUAL(3, TraceCopies::countCopyConstr); // copied...
        CPPUNIT_ASSERT_EQUAL(3, h.getAttribute<TraceCopies>("a", "tc").value);
        // set of const to the now existing node
        h.getAttribute<TraceCopies>("a", "tc").value = 42;
        CPPUNIT_ASSERT_EQUAL(42, h.getAttribute<TraceCopies>("a", "tc").value);
        h.setAttribute("a", "tc", tc);
        CPPUNIT_ASSERT_EQUAL(4, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(3, h.getAttribute<TraceCopies>("a", "tc").value);
        h.setAttribute<TraceCopies>("a", "tc", tc);
        CPPUNIT_ASSERT_EQUAL(5, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(3, h.getAttribute<TraceCopies>("a", "tc").value);

        TraceCopies::reset(); // Start next round from zero
    }

    {
        // Test Hash::setAttribute(path, attr, std::any)
        Hash h("a", 2);
        std::any a(TraceCopies(4));
        h.setAttribute("a", "attr", a);
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr); // a and thus its TraceCopies got copied
        CPPUNIT_ASSERT_EQUAL(4, h.getAttribute<TraceCopies>("a", "attr").value);

        const std::any& a2 = a;
        h.setAttribute("a", "attr2", a2);
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr); // a and thus its TraceCopies get copied
        CPPUNIT_ASSERT_EQUAL(4, h.getAttribute<TraceCopies>("a", "attr2").value);

        h.setAttribute("a", "attr3", std::move(a));
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(4, h.getAttribute<TraceCopies>("a", "attr3").value);

        TraceCopies::reset();
    }
    // test bulk setting of attributes
    {
        Hash::Attributes attrs;
        attrs.set("attr", TraceCopies(7));
        CPPUNIT_ASSERT_EQUAL(0, TraceCopies::countCopyConstr);
        Hash h("a", 1, "b", 2);

        // copy case
        h.setAttributes("a", attrs);
        CPPUNIT_ASSERT_EQUAL(7, h.getAttribute<TraceCopies>("a", "attr").value);
        CPPUNIT_ASSERT_EQUAL(1ul, h.getAttributes("a").size());
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr);

        // move case
        h.setAttributes("b", std::move(attrs));
        CPPUNIT_ASSERT_EQUAL(7, h.getAttribute<TraceCopies>("b", "attr").value);
        CPPUNIT_ASSERT_EQUAL(1ul, h.getAttributes("b").size());
        // Neither moved nor copied since entire 'attrs' now moved inside the Hash
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT(attrs.empty()); // since entirely 'moved away'

        TraceCopies::reset();
    }
    // test setting of various strings as also at the end of testSetMoveSemantics
    {
        // test Hash::set of various strings,
        Hash h("a", 1);
        h.setAttribute("a", "const_char_pointer", "a");
        CPPUNIT_ASSERT_EQUAL(std::string("a"), h.getAttribute<std::string>("a", "const_char_pointer"));

        char cText[] = "a2and3";
        h.setAttribute("a", "char_array", cText);
        CPPUNIT_ASSERT_EQUAL(std::string("a2and3"), h.getAttribute<std::string>("a", "char_array"));

        char* cPtr = cText;
        h.setAttribute("a", "char_ptr", cPtr);
        CPPUNIT_ASSERT_EQUAL(std::string("a2and3"), h.getAttribute<std::string>("a", "char_ptr"));

        h.setAttribute("a", "tmp_string", std::string("b"));
        CPPUNIT_ASSERT_EQUAL(std::string("b"), h.getAttribute<std::string>("a", "tmp_string"));

        const std::string b1("b1");
        h.setAttribute("a", "const_string", b1);
        CPPUNIT_ASSERT_EQUAL(std::string("b1"), h.getAttribute<std::string>("a", "const_string"));

        std::string b2("b2");
        h.setAttribute("a", "string", b2);
        CPPUNIT_ASSERT_EQUAL(std::string("b2"), h.getAttribute<std::string>("a", "string"));
    }
}

void Hash_Test::testConstructorMoveSemantics() {
    TraceCopies::reset(); // Clean start

    // First test setting single value as specially treated
    {
        // test ctr with normal non-const object
        TraceCopies ta(2);
        Hash h("ta", ta);
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr); // copied into Hash
        CPPUNIT_ASSERT_EQUAL(2, h.get<TraceCopies>("ta").value);
    }
    {
        // 'moving' set
        Hash h("tb", TraceCopies(4));
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr); // unchanged
        // CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countMoveConstr); // since now it is moved
        CPPUNIT_ASSERT_EQUAL(4, h.get<TraceCopies>("tb").value);
    }
    {
        // set of const
        const TraceCopies tc(3);
        Hash h("tc", tc);
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr); // copied...
        CPPUNIT_ASSERT_EQUAL(3, h.get<TraceCopies>("tc").value);
    }
    {
        // Now set of many of various const/ref types in one go, also Hash
        TraceCopies ta(1);
        const TraceCopies tb(2);
        Hash ha("a", ta, "b", tb);
        const Hash hb("a", ta, "b", tb);
        TraceCopies::reset(); // Only count for the following constructor(s)
        Hash h("int", 0,      // for first do not test TraceCopies since that is tested above
               "ta", ta, "tb", tb, "tc", TraceCopies(3), "ha", ha, "hb", hb, "hc", Hash("a", ta, "b", tb));

        CPPUNIT_ASSERT_EQUAL(8, TraceCopies::countCopyConstr); // ta, tb, 4x when ha and hb are copied into h and 2x
                                                               // when ta/tb are copied into hc

        CPPUNIT_ASSERT_EQUAL(1, h.get<TraceCopies>("ta").value);
        CPPUNIT_ASSERT_EQUAL(2, h.get<TraceCopies>("tb").value);
        CPPUNIT_ASSERT_EQUAL(3, h.get<TraceCopies>("tc").value);
        CPPUNIT_ASSERT_EQUAL(1, h.get<TraceCopies>("ha.a").value);
        CPPUNIT_ASSERT_EQUAL(2, h.get<TraceCopies>("ha.b").value);
        CPPUNIT_ASSERT_EQUAL(1, h.get<TraceCopies>("hb.a").value);
        CPPUNIT_ASSERT_EQUAL(2, h.get<TraceCopies>("hb.b").value);
        CPPUNIT_ASSERT_EQUAL(1, h.get<TraceCopies>("hc.a").value);
        CPPUNIT_ASSERT_EQUAL(2, h.get<TraceCopies>("hc.b").value);

        // Verify insertion order
        // (was wrong in first attempt to have move semantics in Hash constructor)
        Hash::const_iterator it = h.begin();
        CPPUNIT_ASSERT_EQUAL(std::string("int"), it->getKey());
        CPPUNIT_ASSERT_EQUAL(std::string("ta"), (++it)->getKey());
        CPPUNIT_ASSERT_EQUAL(std::string("tb"), (++it)->getKey());
        CPPUNIT_ASSERT_EQUAL(std::string("tc"), (++it)->getKey());
        CPPUNIT_ASSERT_EQUAL(std::string("ha"), (++it)->getKey());
        CPPUNIT_ASSERT_EQUAL(std::string("hb"), (++it)->getKey());
        CPPUNIT_ASSERT_EQUAL(std::string("hc"), (++it)->getKey());
        CPPUNIT_ASSERT(++it == h.end());
    }

    TraceCopies::reset(); // Start next round from zero
}

void Hash_Test::testGetAs() {
    {
        Hash h("a", true);
        CPPUNIT_ASSERT(h.getAs<string>("a") == "1");
        CPPUNIT_ASSERT(h.getAs<int>("a") == 1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, h.getAs<double>("a"), 0.00001);
        CPPUNIT_ASSERT(h.getAs<char>("a") == '1');
    }

    {
        Hash h("a", true);
        h.setAttribute("a", "a", true);
        CPPUNIT_ASSERT(h.getAttributeAs<string>("a", "a") == "1");
        CPPUNIT_ASSERT(h.getAttributeAs<int>("a", "a") == 1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, h.getAttributeAs<double>("a", "a"), 0.00001);
        CPPUNIT_ASSERT(h.getAttributeAs<char>("a", "a") == '1');
        std::any& any = h.getAttributeAsAny("a", "a");
        CPPUNIT_ASSERT(std::any_cast<bool>(any) == true);
        h.setAttribute("a", "b", 12);
        h.setAttribute("a", "c", 1.23);
        Hash::Attributes attrs = h.getAttributes("a");
        Hash g("Z.a.b.c", "Value");
        g.setAttributes("Z.a.b.c", attrs);
        CPPUNIT_ASSERT(g.getAttributeAs<string>("Z.a.b.c", "a") == "1");
        CPPUNIT_ASSERT(g.getAttributeAs<int>("Z.a.b.c", "a") == 1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, g.getAttributeAs<double>("Z.a.b.c", "a"), 0.00001);
    }

    {
        Hash h("a", std::vector<bool>(4, false));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "0,0,0,0");
        int tmp = h.getAs<int, std::vector>("a")[3];
        CPPUNIT_ASSERT(tmp == 0);
    }
    {
        Hash h("a", char('R'));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "R");
    }
    {
        // Assumes vector to contain binary data and does a base64 encode
        Hash h("a", std::vector<unsigned char>(3, '4'));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "NDQ0");
    }
    {
        // Assumes vector to contain binary data and does a base64 encode
        Hash h("a", std::vector<char>(3, '4'));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "NDQ0");
    }
    {
        // Assumes vector to contain printable (ASCII) characters
        Hash h("a", std::vector<signed char>(3, '4'));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "52,52,52");
    }
    {
        Hash h("a", static_cast<unsigned char>('R'));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "82");
    }
    {
        Hash h("a", static_cast<signed char>('R'));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "82");
    }
    {
        Hash h("a", std::vector<signed char>(4, '2'));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "50,50,50,50");
    }
    {
        Hash h("a", short(126));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "126");
    }
    {
        Hash h("a", std::vector<short>(4, 13));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "13,13,13,13");
    }
    {
        Hash h("a", int(-42));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "-42");
    }
    {
        Hash h("a", std::vector<int>(1, -42));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "-42");
    }
    {
        Hash h("a", static_cast<unsigned int>(42));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "42");
    }
    {
        Hash h("a", std::vector<unsigned int>());
        CPPUNIT_ASSERT(h.getAs<string>("a") == "");
    }
    {
        Hash h("a", static_cast<long long>(-2147483647));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "-2147483647");
    }
    {
        Hash h("a", static_cast<unsigned long long>(0));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "0");
    }
    {
        Hash h("a", static_cast<float>(0.1234567));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "0.1234567");
    }
    {
        Hash h("a", 0.123456789123456);
        CPPUNIT_ASSERT(h.getAs<string>("a") == "0.123456789123456");
    }
    {
        Hash h("a", std::complex<float>(1.2, 0.5));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "(1.2,0.5)");
    }
    {
        Hash h("a", std::complex<double>(1.2, 0.5));
        CPPUNIT_ASSERT(h.getAs<string>("a") == "(1.2,0.5)");
    }
    {
        // getAs as a container
        Hash h("a", std::vector<unsigned short>({2, 3, 5, 7, 11}));
        const auto result = h.getAs<std::string, std::vector>("a");
        CPPUNIT_ASSERT_MESSAGE("Result is " + karabo::util::toString(result),
                               result == std::vector<std::string>({"2", "3", "5", "7", "11"}));
    }
    {
        // There is some extra treatment of STRING as source in Element::getValueAs<T>
        Hash h("a", "5");
        CPPUNIT_ASSERT_EQUAL(5, h.getAs<int>("a"));
    }
    {
        // There is some extra treatment of STRING as source in Element::getValueAs<CONT<T>>
        Hash h("a", "5,6, 7 ");
        const auto result = h.getAs<int, std::vector>("a");
        CPPUNIT_ASSERT_MESSAGE("Result is: " + karabo::util::toString(result), std::vector<int>({5, 6, 7}) == result);
    }
    {
        // There is some extra treatment of empty string as source for containers
        Hash h("a", std::string());
        const auto result = h.getAs<std::string, std::vector>("a");
        // Empty string becomes empty vector of strings and not vector with a single empty string
        CPPUNIT_ASSERT_EQUAL(0ul, result.size());
    }
}


void Hash_Test::testFind() {
    // First test non-const version of Hash::find(..).
    {
        Hash h("a.b.c1.d", 1, "b[2].c.d", "some");
        // Check existing node and its value.
        boost::optional<Hash::Node&> node = h.find("a.b.c1.d");
        CPPUNIT_ASSERT(!node == false);
        CPPUNIT_ASSERT(node->getValue<int>() == 1);

        // Test that other separator fails
        node = h.find("a.b.c1.d", '/');
        CPPUNIT_ASSERT(!node == true);

        // Check existence of first level node.
        node = h.find("a");
        CPPUNIT_ASSERT(!node == false);

        // Check non-existence of first level node.
        node = h.find("nee");
        CPPUNIT_ASSERT(!node == true);

        // Check non-existence of last level node.
        node = h.find("a.b.c1.f");
        CPPUNIT_ASSERT(!node == true);

        // Check non-existence of middle level node.
        node = h.find("a.b.c2.d");
        CPPUNIT_ASSERT(!node == true);

        // Check existence with index as last but two.
        node = h.find("b[2].c.d");
        CPPUNIT_ASSERT(!node == false);

        // Check existence with index as last but one.
        node = h.find("b[2].c");
        CPPUNIT_ASSERT(!node == false);

        // Index at end is not allowed - would be Hash, not Node.
        node = h.find("b[2]");
        CPPUNIT_ASSERT(!node == true);

        // Same check, but with invalid index.
        node = h.find("b[3]");
        CPPUNIT_ASSERT(!node == true);

        // Check non-existence with invalid index as last but one.
        node = h.find("b[3].c");
        CPPUNIT_ASSERT(!node == true);

        // Check non-existence with invalid index as last but two.
        node = h.find("b[3].c.d");
        CPPUNIT_ASSERT(!node == true);
    }

    // Now test Hash::find(..) const.
    // (Same code as above except adding twice 'const'.)
    {
        const Hash h("a.b.c1.d", 1, "b[2].c.d", "some");
        // Check existing node and its value.
        boost::optional<const Hash::Node&> node = h.find("a.b.c1.d");
        CPPUNIT_ASSERT(!node == false);
        CPPUNIT_ASSERT(node->getValue<int>() == 1);

        // Test that other separator fails
        node = h.find("a.b.c1.d", '/');
        CPPUNIT_ASSERT(!node == true);

        // Check existence of first level node.
        node = h.find("a");
        CPPUNIT_ASSERT(!node == false);

        // Check non-existence of first level node.
        node = h.find("nee");
        CPPUNIT_ASSERT(!node == true);

        // Check non-existence of last level node.
        node = h.find("a.b.c1.f");
        CPPUNIT_ASSERT(!node == true);

        // Check non-existence of middle level node.
        node = h.find("a.b.c2.d");
        CPPUNIT_ASSERT(!node == true);

        // Check existence with index as last but two.
        node = h.find("b[2].c.d");
        CPPUNIT_ASSERT(!node == false);

        // Check existence with index as last but one.
        node = h.find("b[2].c");
        CPPUNIT_ASSERT(!node == false);

        // Index at end is not allowed - would be Hash, not Node.
        node = h.find("b[2]");
        CPPUNIT_ASSERT(!node == true);

        // Same check, but with invalid index.
        node = h.find("b[3]");
        CPPUNIT_ASSERT(!node == true);

        // Check non-existence with invalid index as last but one.
        node = h.find("b[3].c");
        CPPUNIT_ASSERT(!node == true);

        // Check non-existence with invalid index as last but two.
        node = h.find("b[3].c.d");
        CPPUNIT_ASSERT(!node == true);
    }
}


void Hash_Test::testAttributes() {
    {
        Hash h("a.b.a.b", 42);
        h.setAttribute("a", "attr1", "someValue");
        CPPUNIT_ASSERT(h.getAttribute<std::string>("a", "attr1") == "someValue");

        h.setAttribute("a", "attr2", 42);
        CPPUNIT_ASSERT(h.getAttribute<std::string>("a", "attr1") == "someValue");
        CPPUNIT_ASSERT(h.getAttribute<int>("a", "attr2") == 42);

        h.setAttribute("a", "attr2", 43);
        CPPUNIT_ASSERT(h.getAttribute<std::string>("a", "attr1") == "someValue");
        CPPUNIT_ASSERT(h.getAttribute<int>("a", "attr2") == 43);

        h.setAttribute("a.b.a.b", "attr1", true);
        CPPUNIT_ASSERT(h.getAttribute<bool>("a.b.a.b", "attr1") == true);

        const Hash::Attributes& attrs = h.getAttributes("a");
        CPPUNIT_ASSERT(attrs.size() == 2);
        CPPUNIT_ASSERT(attrs.get<std::string>("attr1") == "someValue");
        CPPUNIT_ASSERT(attrs.get<int>("attr2") == 43);

        Hash::Attributes::Node node = attrs.getNode("attr2");
        CPPUNIT_ASSERT(node.getType() == Types::INT32);
    }
    {
        Hash h("a", 1);
        bool b = true;
        h.getNode("a").setAttribute<int>("a", b);
        CPPUNIT_ASSERT(h.getNode("a").getType() == Types::INT32);
    }
}


void Hash_Test::testIteration() {
    Hash h("should", 1, "be", 2, "iterated", 3, "in", 4, "correct", 5, "order", 6);
    Hash::Attributes a("should", 1, "be", 2, "iterated", 3, "in", 4, "correct", 5, "order", 6);

    {
        std::vector<std::string> insertionOrder;
        for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
            insertionOrder.push_back(it->getKey());
        }
        CPPUNIT_ASSERT(insertionOrder[0] == "should");
        CPPUNIT_ASSERT(insertionOrder[1] == "be");
        CPPUNIT_ASSERT(insertionOrder[2] == "iterated");
        CPPUNIT_ASSERT(insertionOrder[3] == "in");
        CPPUNIT_ASSERT(insertionOrder[4] == "correct");
        CPPUNIT_ASSERT(insertionOrder[5] == "order");
    }

    {
        std::vector<std::string> alphaNumericOrder;
        for (Hash::const_map_iterator it = h.mbegin(); it != h.mend(); ++it) {
            alphaNumericOrder.push_back(it->second.getKey());
        }
        CPPUNIT_ASSERT(alphaNumericOrder[0] == "be");
        CPPUNIT_ASSERT(alphaNumericOrder[1] == "correct");
        CPPUNIT_ASSERT(alphaNumericOrder[2] == "in");
        CPPUNIT_ASSERT(alphaNumericOrder[3] == "iterated");
        CPPUNIT_ASSERT(alphaNumericOrder[4] == "order");
        CPPUNIT_ASSERT(alphaNumericOrder[5] == "should");
    }

    h.set("be", "2"); // Has no effect on order

    {
        std::vector<std::string> insertionOrder;
        for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
            insertionOrder.push_back(it->getKey());
        }
        CPPUNIT_ASSERT(insertionOrder[0] == "should");
        CPPUNIT_ASSERT(insertionOrder[1] == "be");
        CPPUNIT_ASSERT(insertionOrder[2] == "iterated");
        CPPUNIT_ASSERT(insertionOrder[3] == "in");
        CPPUNIT_ASSERT(insertionOrder[4] == "correct");
        CPPUNIT_ASSERT(insertionOrder[5] == "order");
    }

    {
        std::vector<std::string> alphaNumericOrder;
        for (Hash::const_map_iterator it = h.mbegin(); it != h.mend(); ++it) {
            alphaNumericOrder.push_back(it->second.getKey());
        }
        CPPUNIT_ASSERT(alphaNumericOrder[0] == "be");
        CPPUNIT_ASSERT(alphaNumericOrder[1] == "correct");
        CPPUNIT_ASSERT(alphaNumericOrder[2] == "in");
        CPPUNIT_ASSERT(alphaNumericOrder[3] == "iterated");
        CPPUNIT_ASSERT(alphaNumericOrder[4] == "order");
        CPPUNIT_ASSERT(alphaNumericOrder[5] == "should");
    }

    h.erase("be");    // Remove
    h.set("be", "2"); // Must be last element in sequence now

    {
        std::vector<std::string> insertionOrder;
        for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
            insertionOrder.push_back(it->getKey());
        }
        CPPUNIT_ASSERT(insertionOrder[0] == "should");
        CPPUNIT_ASSERT(insertionOrder[1] == "iterated");
        CPPUNIT_ASSERT(insertionOrder[2] == "in");
        CPPUNIT_ASSERT(insertionOrder[3] == "correct");
        CPPUNIT_ASSERT(insertionOrder[4] == "order");
        CPPUNIT_ASSERT(insertionOrder[5] == "be");
    }

    {
        std::vector<std::string> alphaNumericOrder;
        for (Hash::const_map_iterator it = h.mbegin(); it != h.mend(); ++it) {
            alphaNumericOrder.push_back(it->second.getKey());
        }
        CPPUNIT_ASSERT(alphaNumericOrder[0] == "be");
        CPPUNIT_ASSERT(alphaNumericOrder[1] == "correct");
        CPPUNIT_ASSERT(alphaNumericOrder[2] == "in");
        CPPUNIT_ASSERT(alphaNumericOrder[3] == "iterated");
        CPPUNIT_ASSERT(alphaNumericOrder[4] == "order");
        CPPUNIT_ASSERT(alphaNumericOrder[5] == "should");
    }

    //  getKeys(...) to ...
    //         "set"
    {
        std::set<std::string> tmp; // create empty set
        h.getKeys(tmp);            // fill set by keys
        std::set<std::string>::const_iterator it = tmp.begin();
        CPPUNIT_ASSERT(*it++ == "be");
        CPPUNIT_ASSERT(*it++ == "correct");
        CPPUNIT_ASSERT(*it++ == "in");
        CPPUNIT_ASSERT(*it++ == "iterated");
        CPPUNIT_ASSERT(*it++ == "order");
        CPPUNIT_ASSERT(*it++ == "should");
    }

    //         "vector"
    {
        std::vector<std::string> tmp; // create empty vector
        h.getKeys(tmp);               // fill vector by keys
        std::vector<std::string>::const_iterator it = tmp.begin();
        CPPUNIT_ASSERT(*it++ == "should");
        CPPUNIT_ASSERT(*it++ == "iterated");
        CPPUNIT_ASSERT(*it++ == "in");
        CPPUNIT_ASSERT(*it++ == "correct");
        CPPUNIT_ASSERT(*it++ == "order");
        CPPUNIT_ASSERT(*it++ == "be");
    }

    //         "list"
    {
        std::list<std::string> tmp; // create empty list
        h.getKeys(tmp);             // fill list by keys
        std::list<std::string>::const_iterator it = tmp.begin();
        CPPUNIT_ASSERT(*it++ == "should");
        CPPUNIT_ASSERT(*it++ == "iterated");
        CPPUNIT_ASSERT(*it++ == "in");
        CPPUNIT_ASSERT(*it++ == "correct");
        CPPUNIT_ASSERT(*it++ == "order");
        CPPUNIT_ASSERT(*it++ == "be");
    }

    //         "deque"
    {
        std::deque<std::string> tmp; // create empty queue
        h.getKeys(tmp);              // fill deque by keys
        std::deque<std::string>::const_iterator it = tmp.begin();
        CPPUNIT_ASSERT(*it++ == "should");
        CPPUNIT_ASSERT(*it++ == "iterated");
        CPPUNIT_ASSERT(*it++ == "in");
        CPPUNIT_ASSERT(*it++ == "correct");
        CPPUNIT_ASSERT(*it++ == "order");
        CPPUNIT_ASSERT(*it++ == "be");
    }
}


void Hash_Test::testGetPaths() {
    {
        Hash h;
        h.set("a", 1);
        h.set("b.c", "foo");
        h.set("array", NDArray(Dims(10, 10)));
        std::vector<Hash> vh;
        vh.push_back(Hash("a.b", 123));
        vh.push_back(Hash());
        h.set("vector.hash.one", vh);
        h.set("empty.vector.hash", std::vector<Hash>());
        h.set("empty.hash", Hash());

        std::vector<std::string> paths;
        h.getPaths(paths);
        CPPUNIT_ASSERT(paths.size() == 7);

        std::vector<std::string>::const_iterator it = paths.begin();
        CPPUNIT_ASSERT(*it++ == "a");
        CPPUNIT_ASSERT(*it++ == "b.c");
        CPPUNIT_ASSERT(*it++ == "array");
        CPPUNIT_ASSERT(*it++ == "vector.hash.one[0].a.b");
        CPPUNIT_ASSERT(*it++ == "vector.hash.one[1]");
        CPPUNIT_ASSERT(*it++ == "empty.vector.hash");
        CPPUNIT_ASSERT(*it++ == "empty.hash");
    }

    {
        Hash h;
        h.set("a", 1);
        h.set("b.c", "foo");
        h.set("b.array", NDArray(Dims(10, 10)));
        h.set("emptyhash", Hash());
        std::vector<std::string> paths;
        h.getDeepPaths(paths);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(paths) += "\n" + toString(h), 7ul, paths.size());
        std::vector<std::string>::const_iterator it = paths.begin();
        CPPUNIT_ASSERT_EQUAL(*it++, std::string("a"));
        CPPUNIT_ASSERT_EQUAL(*it++, std::string("b.c"));
        CPPUNIT_ASSERT_EQUAL(*it++, std::string("b.array.data"));
        CPPUNIT_ASSERT_EQUAL(*it++, std::string("b.array.type"));
        CPPUNIT_ASSERT_EQUAL(*it++, std::string("b.array.shape"));
        CPPUNIT_ASSERT_EQUAL(*it++, std::string("b.array.isBigEndian"));
        CPPUNIT_ASSERT_EQUAL(*it++, std::string("emptyhash"));
    }
}


void Hash_Test::testMerge() {
    Hash h1("a", 1, "b", 2, "c.b[0].g", 3, "c.c[0].d", 4, "c.c[1]", Hash("a.b.c", 6), "d.e", 7
            //,"f.g", 99 // can only set 6 keys in constructor...
    );
    h1.set("f.g", 99);
    h1.set("h", -1);
    h1.setAttribute("a", "attrKey", "Just a number");
    h1.setAttribute("c.b", "attrKey2", 3);
    h1.setAttribute("c.b[0].g", "attrKey3", 4.);
    h1.setAttribute("f", "attrKey6", std::string("buaah!"));
    h1.set("array2", NDArray(Dims(1, 1)));

    Hash h1b(h1);
    Hash h1c(h1);
    Hash h1d(h1);

    Hash h2("a", 21, "b.c", 22, "c.b[0]", Hash("key", "value"), "c.b[1].d", 24, "e", 27, "f", Hash());
    h2.set("g.h.i", -88);
    h2.set("g.h.j", -188);
    h2.set("h.i", -199);
    h2.set("h.j", 200);
    h2.set(".i[3]", Hash());
    h2.set(".i[1].j", 200);
    h2.set(".i[2]", Hash("k.l", 5.));
    h2.set("j", Hash("k", 5.));
    h2.set("array", NDArray(Dims(5, 5)));
    h2.set("array2", NDArray(Dims(5, 5)));
    h2.setAttribute("a", "attrKey", "Really just a number");
    h2.setAttribute("e", "attrKey4", -1);
    h2.setAttribute("e", "attrKey5", -11.f);
    h2.setAttribute("f", "attrKey7", 77u);
    h2.setAttribute(".i", "attrKey8", 123ll); // attribute on new vector<Hash> node
    h2.setAttribute("j", "attrKey9", 12.3);   // ... and new Hash node


    h1.merge(h2); // Hash::REPLACE_ATTRIBUTES is the default
    h1b.merge(h2, Hash::MERGE_ATTRIBUTES);
    h1d += h2; // same as h1d.merge(h2), only check similarity and once attribute replacement below

    CPPUNIT_ASSERT_MESSAGE("Replace or merge attributes influenced resulting paths", similar(h1, h1b));
    CPPUNIT_ASSERT_MESSAGE("merge and += don't do the same", similar(h1, h1d));

    CPPUNIT_ASSERT(h1.has("a"));
    CPPUNIT_ASSERT(h1.get<int>("a") == 21); // new value
    // Attribute kept, but value overwritten:
    CPPUNIT_ASSERT_MESSAGE("Attribute on node not kept", h1.hasAttribute("a", "attrKey"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Attribute not overwritten", std::string("Really just a number"),
                                 h1.getAttribute<std::string>("a", "attrKey"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Attribute added out of nothing", 1ul, h1.getAttributes("a").size());

    CPPUNIT_ASSERT_MESSAGE("Attribute on node not kept (MERGE)", h1b.hasAttribute("a", "attrKey"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Attribute not overwritten (MERGE)", std::string("Really just a number"),
                                 h1b.getAttribute<std::string>("a", "attrKey"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Attribute added out of nothing (MERGE)", 1ul, h1b.getAttributes("a").size());

    CPPUNIT_ASSERT(h1.has("b"));
    CPPUNIT_ASSERT(h1.is<Hash>("b")); // switch to new type...
    CPPUNIT_ASSERT(h1.has("b.c"));    // ...and as Hash can hold a child

    // Attributes overwritten by nothing or kept
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Attributes on node kept", 0ul, h1.getAttributes("c.b").size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Attributes on untouched leaf not kept", 1ul, h1.getAttributes("c.b[0].g").size());
    CPPUNIT_ASSERT_MESSAGE("Attribute on untouched leaf not kept", h1.hasAttribute("c.b[0].g", "attrKey3"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Attribute on untouched leaf changed", 4.,
                                 h1.getAttribute<double>("c.b[0].g", "attrKey3"));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Number of attributes on node changed (MERGE)", 1ul, h1b.getAttributes("c.b").size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Number of attributes on leaf changed (MERGE)", 1ul,
                                 h1b.getAttributes("c.b[0].g").size());
    CPPUNIT_ASSERT_MESSAGE("Attribute on node not kept (MERGE)", h1b.hasAttribute("c.b", "attrKey2"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Attribute on node changed (MERGE)", 3, h1b.getAttribute<int>("c.b", "attrKey2"));
    CPPUNIT_ASSERT_MESSAGE("Attribute on untouched leaf not kept (MERGE)", h1b.hasAttribute("c.b[0].g", "attrKey3"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Attribute on untouched leaf changed (MERGE)", 4.,
                                 h1b.getAttribute<double>("c.b[0].g", "attrKey3"));

    CPPUNIT_ASSERT(!h1.has("c.b.d"));
    CPPUNIT_ASSERT(h1.has("c.b[0]"));
    CPPUNIT_ASSERT(h1.has("c.b[1]"));
    CPPUNIT_ASSERT(!h1.has("c.b[2]"));
    CPPUNIT_ASSERT(h1.get<int>("c.b[1].d") == 24);
    CPPUNIT_ASSERT(h1.has("c.c[0].d"));
    CPPUNIT_ASSERT(h1.has("c.c[1].a.b.c"));
    CPPUNIT_ASSERT(h1.has("d.e"));
    CPPUNIT_ASSERT(h1.has("e"));
    CPPUNIT_ASSERT(h1.has("g.h.i"));
    CPPUNIT_ASSERT(h1.has("g.h.j"));
    CPPUNIT_ASSERT(h1.has("h.i"));
    CPPUNIT_ASSERT(h1.has("h.j"));
    CPPUNIT_ASSERT(h1.has(".i[1].j"));
    CPPUNIT_ASSERT(h1.has(".i[2].k.l"));
    CPPUNIT_ASSERT(h1.has(".i[3]"));
    CPPUNIT_ASSERT(h1.has("j.k"));
    CPPUNIT_ASSERT_MESSAGE(toString(h1), h1.has("array"));
    CPPUNIT_ASSERT(h1.has("array.data"));
    CPPUNIT_ASSERT(h1.has("array2"));
    CPPUNIT_ASSERT(h1.has("array2.data"));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Array size changed through merge", 25ull,
                                 h1.get<NDArray>("array2").getShape().size());

    // Just add attributes with leaf (identical for REPLACE or MERGE)
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Not all attributes on leaf added", 2ul, h1.getAttributes("e").size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Int attribute value incorrect", -1, h1.getAttribute<int>("e", "attrKey4"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Float attribute value incorrect", -11.f, h1.getAttribute<float>("e", "attrKey5"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Not all attributes on leaf added (MERGE)", 2ul, h1b.getAttributes("e").size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Int attribute value incorrect (MERGE)", -1, h1b.getAttribute<int>("e", "attrKey4"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Float attribute value incorrect (MERGE)", -11.f,
                                 h1b.getAttribute<float>("e", "attrKey5"));
    // Just add attributes for new Hash/vector<Hash> (identical for REPLACE or MERGE)
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Not all attributes on vector<Hash> added", 1ul, h1.getAttributes(".i").size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Int64 attributes on vector<Hash> wrong", 123ll,
                                 h1.getAttribute<long long>(".i", "attrKey8"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Not all attributes on Hash added", 1ul, h1.getAttributes("j").size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Double attributes on Hash wrong", 12.3, h1.getAttribute<double>("j", "attrKey9"));

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Not all attributes on vector<Hash> added (MERGE)", 1ul,
                                 h1b.getAttributes(".i").size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Int64 attributes on vector<Hash> wrong  (MERGE)", 123ll,
                                 h1b.getAttribute<long long>(".i", "attrKey8"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Not all attributes on Hash added (MERGE)", 1ul, h1b.getAttributes("j").size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Double attributes on Hash wrong (MERGE)", 12.3,
                                 h1b.getAttribute<double>("j", "attrKey9"));

    CPPUNIT_ASSERT_MESSAGE("Attribute on node not kept (MERGE)", h1b.hasAttribute("c.b", "attrKey2"));


    CPPUNIT_ASSERT(h1.has("f"));
    CPPUNIT_ASSERT(h1.has("f.g")); // merging does not overwrite h1["f"] with empty Hash

    CPPUNIT_ASSERT_EQUAL_MESSAGE("Attributes not replaced", 1ul, h1.getAttributes("f").size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("UInt attribute value incorrect", 77u, h1.getAttribute<unsigned int>("f", "attrKey7"));
    // += is merge with REPLACE_ATTRIBUTES
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Attributes not replaced (+=)", 1ul, h1d.getAttributes("f").size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("UInt attribute value incorrect (+=)", 77u,
                                 h1d.getAttribute<unsigned int>("f", "attrKey7"));
    // here is MERGE_ATTRIBUTES
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Attributes not merged", 2ul, h1b.getAttributes("f").size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("UInt attribute value incorrect (MERGE)", std::string("buaah!"),
                                 h1b.getAttribute<std::string>("f", "attrKey6"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("UInt attribute value incorrect (MERGE)", 77u,
                                 h1b.getAttribute<unsigned int>("f", "attrKey7"));

    // Now check the 'selectedPaths' feature (no extra test for attribute merging needed):
    std::set<std::string> selectedPaths;
    selectedPaths.insert("a");
    selectedPaths.insert("b.c");
    selectedPaths.insert("g.h.i");
    selectedPaths.insert("h.i");
    selectedPaths.insert(".i[2]");
    selectedPaths.insert(".i[5]"); // check that we tolerate to select path with invalid index
    h1c.merge(h2, Hash::MERGE_ATTRIBUTES, selectedPaths);

    // Keep everything it had before merging:
    CPPUNIT_ASSERT(h1c.has("a"));
    CPPUNIT_ASSERT(h1c.has("b"));
    CPPUNIT_ASSERT(h1c.has("c.b[0].g"));
    CPPUNIT_ASSERT(h1c.has("c.c[0].d"));
    CPPUNIT_ASSERT(h1c.has("c.c[1].a.b.c"));
    CPPUNIT_ASSERT(h1c.has("d.e"));
    CPPUNIT_ASSERT(h1c.has("f.g"));
    // The additionally selected ones from h2:
    CPPUNIT_ASSERT(h1c.has("b.c"));
    CPPUNIT_ASSERT(h1c.has("g.h.i"));
    CPPUNIT_ASSERT(h1c.has("h.i"));
    CPPUNIT_ASSERT(h1c.has(".i[2].k.l"));
    // But not the other ones from h2:
    CPPUNIT_ASSERT(!h1c.has("c.b[0].key")); // neither at old position of h2
    CPPUNIT_ASSERT(!h1c.has("c.b[2]"));     // nor an extended vector<Hash> at all
    CPPUNIT_ASSERT(!h1c.has("e"));
    // Take care that adding path "g.h.i" does not trigger that other children of "g.h" in h2 are taken as well:
    CPPUNIT_ASSERT(!h1c.has("g.h.j"));
    CPPUNIT_ASSERT(!h1c.has("h.j"));
    // Adding .i[2] should not trigger to add children of .i[1] nor .i[3]]
    CPPUNIT_ASSERT(!h1c.has(".i[1].j"));
    CPPUNIT_ASSERT(!h1c.has(".i[3]"));

    // Some further small tests for so far untested cases with selected paths...
    Hash hashTarget(".b", 1, ".c", Hash(), "c", "so so!");
    const Hash hashSource(".d", 8., "e..e[0]", Hash("f", 0), "e..e[1]", Hash("g", 1), "ha", 9);
    selectedPaths.clear();
    selectedPaths.insert(""); // trigger merging '.d'
    selectedPaths.insert("e..e[1]");
    hashTarget.merge(hashSource, Hash::MERGE_ATTRIBUTES, selectedPaths);
    CPPUNIT_ASSERT(hashTarget.has(".d"));
    CPPUNIT_ASSERT(hashTarget.has("e..e[0]"));
    CPPUNIT_ASSERT(!hashTarget.has("e..e[0].f")); // no children of e[0] since e[0] not selected (see test above)
    CPPUNIT_ASSERT(hashTarget.has("e..e[1]"));
    CPPUNIT_ASSERT(hashTarget.has("e..e[1].g"));

    Hash hashTargetB("a[1].b", 1, "c", "Does not matter");
    Hash hashTargetC(hashTargetB);
    Hash hashTargetD(hashTargetB);
    const Hash hashSourceBCD("a[2]", Hash("a", 33, "b", 4.4), "ha", 9, "c[1]", Hash("k", 5, "l", 6), "c[2]",
                             Hash("b", -3), "d[2].b", 66, "e[1]", Hash("1", 1, "2", 2, "3", 3));
    selectedPaths.clear();
    selectedPaths.insert("a"); // trigger merging full vector
    // trigger selecting first HashVec item overwriting what was not a hashVec before, but only keep selected items
    selectedPaths.insert("c[1].l");
    selectedPaths.insert("d");      // trigger adding full new vector
    selectedPaths.insert("e[1].2"); // trigger selective adding of hashVec where there was not node before
    selectedPaths.insert("e[1].3");
    hashTargetB.merge(hashSourceBCD, Hash::MERGE_ATTRIBUTES, selectedPaths);
    CPPUNIT_ASSERT(hashTargetB.has("a[1].b"));
    CPPUNIT_ASSERT(hashTargetB.has("a[2].a"));
    CPPUNIT_ASSERT(hashTargetB.has("a[2].b"));
    CPPUNIT_ASSERT(!hashTargetB.has("a[3]"));
    CPPUNIT_ASSERT(hashTargetB.has("c[0]"));
    CPPUNIT_ASSERT(!hashTargetB.has("c[0].k"));
    CPPUNIT_ASSERT(hashTargetB.has("c[1].l"));
    CPPUNIT_ASSERT(hashTargetB.has("d[2].b"));
    CPPUNIT_ASSERT(!hashTargetB.has("d[3]"));
    CPPUNIT_ASSERT(hashTargetB.has("e[0]"));
    CPPUNIT_ASSERT(hashTargetB.has("e[1].2"));
    CPPUNIT_ASSERT(hashTargetB.has("e[1].3"));

    selectedPaths.clear();
    selectedPaths.insert("a[0]");
    selectedPaths.insert("a[2].b"); // trigger selective vector items
    selectedPaths.insert("c");      // trigger overwriting with complete vector
    hashTargetC.merge(hashSourceBCD, Hash::MERGE_ATTRIBUTES, selectedPaths);
    CPPUNIT_ASSERT(hashTargetC.has("a[1].b"));
    CPPUNIT_ASSERT(!hashTargetC.has("a[3]"));
    CPPUNIT_ASSERT(hashTargetC.has("a[2].b"));
    CPPUNIT_ASSERT(hashTargetC.has("c[1].k"));
    CPPUNIT_ASSERT(hashTargetC.has("c[1].l"));
    CPPUNIT_ASSERT(hashTargetC.has("c[2].b"));
    CPPUNIT_ASSERT(!hashTargetC.has("c[3]"));

    // Now select only invalid indices - nothing should be added
    selectedPaths.clear();
    selectedPaths.insert("a[10]"); // to existing vector
    selectedPaths.insert("c[10]"); // where there was another node
    selectedPaths.insert("d[10]"); // where there was no node at all
    selectedPaths.insert("ha[0]"); // for leaves, all indices are invalid
    Hash copyD(hashTargetD);
    hashTargetD.merge(hashSourceBCD, Hash::MERGE_ATTRIBUTES, selectedPaths);
    CPPUNIT_ASSERT_MESSAGE("Selecting only invalid indices changed something", similar(copyD, hashTargetD));

    ////////////////////////////////////////////////////////////////////////////////////
    // Add test with merges of vector<Hash> marked as a table
    const Hash targetTemplate("table", std::vector<Hash>({Hash("a", 1, "b", "1"), Hash("a", 12, "b", "12")}));
    Hash source("table", std::vector<Hash>(
                               {Hash("a", 101, "b", "101"), Hash("a", 102, "b", "102"), Hash("a", 103, "b", "103")}));
    source.setAttribute("table", "rowSchema", true); // mark vector<Hash> as a table

    // Merging tables replaces complete vector<Hash>
    Hash target1(targetTemplate);
    target1.merge(source);
    CPPUNIT_ASSERT_MESSAGE(toString(target1), target1.fullyEquals(source));

    // But we can select to use some rows only
    Hash target2(targetTemplate);
    // Keep only first and last rows of source
    target2.merge(source, Hash::MERGE_ATTRIBUTES, {"table[0]", "table[2]"});
    const auto mergedTable = target2.get<std::vector<Hash>>("table");
    CPPUNIT_ASSERT_EQUAL(2ul, mergedTable.size());
    const Hash& row0 = mergedTable[0];
    CPPUNIT_ASSERT_MESSAGE(toString(row0), row0.fullyEquals(source.get<std::vector<Hash>>("table")[0]));
    const Hash& row1 = mergedTable[1];
    CPPUNIT_ASSERT_MESSAGE(toString(row1), row1.fullyEquals(source.get<std::vector<Hash>>("table")[2]));
}


void Hash_Test::testSubtract() {
    Hash h1("a", 1, "b", 2, "c.b[0].g", 3, "c.c[0].d", 4, "c.c[1]", Hash("a.b.c", 6), "d.e", 7);

    Hash h2("a", 21, "b.c", 22, "c.b[0]", Hash("key", "value"), "c.b[1].d", 24, "e", 27);
    h1 += h2;
    h1 -= h2;
    CPPUNIT_ASSERT(h1.has("a") == false);
    CPPUNIT_ASSERT(h1.get<Hash>("b").empty() == true);
    CPPUNIT_ASSERT(h1.get<int>("c.b[0].g") == 3);
    CPPUNIT_ASSERT(!h1.has("c.b[1]"));
    CPPUNIT_ASSERT(h1.get<int>("c.c[0].d") == 4);
    CPPUNIT_ASSERT(h1.get<int>("c.c[1].a.b.c") == 6);
    CPPUNIT_ASSERT(h1.get<int>("d.e") == 7);

    Hash h3("a.b.c", 1, "a.b.d", 2, "a.c.d", 22, "b.c.d", 33, "c.d.e", 44, "c.e.f", 55);
    Hash h4("a.b", Hash(), "c", Hash());
    h3 -= h4;
    CPPUNIT_ASSERT(h3.has("a.b") == true);
    CPPUNIT_ASSERT(h3.has("c") == true);
    CPPUNIT_ASSERT(h3.get<int>("a.c.d") == 22);
    CPPUNIT_ASSERT(h3.get<int>("b.c.d") == 33);
}


void Hash_Test::testErase() {
    // prepare two identical hashes
    Hash h1("a", 1, "b", 2, "c.d", 31, "e.f.g", 411, "e.f.h", 412, "e.i", 42);
    Hash h2(h1);

    // Start testing Hash::erase on h1
    CPPUNIT_ASSERT(h1.size() == 4);

    // erase existing key on first level => size decreases
    CPPUNIT_ASSERT(h1.erase("a") == true);
    CPPUNIT_ASSERT(h1.has("a") == false);
    CPPUNIT_ASSERT(h1.size() == 3);

    // non-existing key - return false and keep size:
    CPPUNIT_ASSERT(h1.erase("a") == false);
    CPPUNIT_ASSERT(h1.size() == 3);

    // "c.d": composite key without siblings
    CPPUNIT_ASSERT(h1.erase("c.d") == true);
    CPPUNIT_ASSERT(h1.has("c.d") == false);
    CPPUNIT_ASSERT(h1.has("c") == true);
    CPPUNIT_ASSERT(h1.size() == 3); // "c" still in!

    // "e.f": composite key with two children and a sibling
    CPPUNIT_ASSERT(h1.erase("e.f") == true);
    CPPUNIT_ASSERT(h1.has("e.f.g") == false);
    CPPUNIT_ASSERT(h1.has("e.f.h") == false);
    CPPUNIT_ASSERT(h1.has("e.f") == false);
    CPPUNIT_ASSERT(h1.has("e") == true); // stays
    CPPUNIT_ASSERT(h1.size() == 3);

    // now testing Hash::erasePath on h2
    CPPUNIT_ASSERT(h2.size() == 4);

    // erase existing key on first level => size decreases
    h2.erasePath("a");
    CPPUNIT_ASSERT(h2.has("a") == false);
    CPPUNIT_ASSERT(h2.size() == 3);

    // non-existing key: size just stays as it is
    h2.erasePath("a");
    CPPUNIT_ASSERT(h2.size() == 3);


    // "c.d": composite key without siblings
    h2.erasePath("c.d");
    CPPUNIT_ASSERT(h2.has("c.d") == false);
    CPPUNIT_ASSERT(h2.has("c") == false); // removed since nothing left
    CPPUNIT_ASSERT(h2.size() == 2);

    // "e.f": composite key with two children and a sibling
    h2.erasePath("e.f");
    CPPUNIT_ASSERT(h2.has("e.f.g") == false);
    CPPUNIT_ASSERT(h2.has("e.f.h") == false);
    CPPUNIT_ASSERT(h2.has("e.f") == false);
    CPPUNIT_ASSERT(h2.has("e") == true); // stays since there is "e.i"
    CPPUNIT_ASSERT(h2.size() == 2);

    // Now testing erasure of elements in a vector<Hash>.
    Hash hVector("a[2].b", 111);
    CPPUNIT_ASSERT(hVector.get<vector<Hash>>("a").size() == 3);
    CPPUNIT_ASSERT(hVector.erase("a[3]") == false);
    CPPUNIT_ASSERT(hVector.get<vector<Hash>>("a").size() == 3);
    CPPUNIT_ASSERT(hVector.erase("a[0]") == true);
    CPPUNIT_ASSERT(hVector.get<vector<Hash>>("a").size() == 2);
    CPPUNIT_ASSERT(hVector.get<int>("a[1].b") == 111);
    // index on non-existing key
    CPPUNIT_ASSERT(hVector.erase("c[2]") == false);
    CPPUNIT_ASSERT(hVector.erase("a.c[2]") == false);
    CPPUNIT_ASSERT(hVector.erase("a[0].c[1]") == false);

    // Now testing erasePath for paths containing indices.
    Hash hVector2("a[2].b", 111);
    CPPUNIT_ASSERT(hVector2.get<vector<Hash>>("a").size() == 3);
    Hash copy = hVector2;
    hVector2.erasePath("a[3]"); // nothing happens (not even an exception)
    CPPUNIT_ASSERT(hVector2 == copy);
    hVector2.erasePath("a[3].b"); // nothing happens (not even an exception)
    CPPUNIT_ASSERT(hVector2 == copy);
    hVector2.erasePath("a[0]"); // shrunk
    CPPUNIT_ASSERT(hVector2.get<vector<Hash>>("a").size() == 2);
    CPPUNIT_ASSERT(hVector2.get<int>("a[1].b") == 111);
    hVector2.erasePath("a[1].b"); // erase a[1] as well since b is only daughter
    CPPUNIT_ASSERT(hVector2.get<vector<Hash>>("a").size() == 1);
    // index for non-existing key must neither throw nor touch the content
    copy = hVector2;
    hVector2.erasePath("c[2]");
    CPPUNIT_ASSERT(hVector2 == copy);
    hVector2.erasePath("a.c[2]");
    CPPUNIT_ASSERT(hVector2 == copy);
    hVector2.erasePath("a[0].c[1]");
    CPPUNIT_ASSERT(hVector2 == copy);
    // single element vector<Hash>: vector is removed completely
    hVector2.erasePath("a[0]");
    CPPUNIT_ASSERT(hVector2.has("a") == false);

    // Test erase with empty keys at various places of the path
    Hash hEmptyKey("", 1, "a.", 2, "a1.", 3, "b..", 31, "c..d", 32, "e..f", 33);
    Hash hEmptyKey2(hEmptyKey); // for next test section
    // only empty key
    CPPUNIT_ASSERT_EQUAL(6ul, hEmptyKey.size());
    CPPUNIT_ASSERT(hEmptyKey.has(""));
    CPPUNIT_ASSERT(hEmptyKey.erase("")); // only empty key
    CPPUNIT_ASSERT_EQUAL(5ul, hEmptyKey.size());

    CPPUNIT_ASSERT(hEmptyKey.has("a"));
    CPPUNIT_ASSERT(hEmptyKey.has("a."));
    CPPUNIT_ASSERT(hEmptyKey.erase("a.")); // empty key at end
    CPPUNIT_ASSERT(!hEmptyKey.has("a."));
    CPPUNIT_ASSERT(hEmptyKey.has("a"));

    CPPUNIT_ASSERT(hEmptyKey.has("a1"));
    CPPUNIT_ASSERT(hEmptyKey.has("a1."));
    CPPUNIT_ASSERT(hEmptyKey.erase("a1"));
    CPPUNIT_ASSERT(!hEmptyKey.has("a1."));
    CPPUNIT_ASSERT(!hEmptyKey.has("a1"));

    CPPUNIT_ASSERT(hEmptyKey.has("b"));
    CPPUNIT_ASSERT(hEmptyKey.has("b."));
    CPPUNIT_ASSERT(hEmptyKey.has("b.."));
    Hash& b = hEmptyKey.get<Hash>("b");
    CPPUNIT_ASSERT(b.has("."));
    CPPUNIT_ASSERT(b.erase(".")); // empty key at begin and end
    CPPUNIT_ASSERT(!hEmptyKey.has("b.."));
    CPPUNIT_ASSERT(hEmptyKey.has("b."));

    CPPUNIT_ASSERT(hEmptyKey.has("c"));
    CPPUNIT_ASSERT(hEmptyKey.has("c."));
    CPPUNIT_ASSERT(hEmptyKey.has("c..d"));
    Hash& c = hEmptyKey.get<Hash>("c");
    CPPUNIT_ASSERT(c.erase(".d")); // empty key at begin
    CPPUNIT_ASSERT(!hEmptyKey.has("c..d"));
    CPPUNIT_ASSERT(hEmptyKey.has("c."));

    CPPUNIT_ASSERT(hEmptyKey.has("e"));
    CPPUNIT_ASSERT(hEmptyKey.has("e."));
    CPPUNIT_ASSERT(hEmptyKey.has("e..f"));
    CPPUNIT_ASSERT(hEmptyKey.erase("e..f")); // empty key in middle
    CPPUNIT_ASSERT(!hEmptyKey.has("e..f"));
    CPPUNIT_ASSERT(hEmptyKey.has("e."));

    // Test erasePath with empty keys at various places of the path.
    // Same test cases as for erase, but sometimes other result!

    // only empty key
    CPPUNIT_ASSERT_EQUAL(6ul, hEmptyKey2.size());
    CPPUNIT_ASSERT(hEmptyKey2.has(""));
    hEmptyKey2.erasePath("");
    CPPUNIT_ASSERT_EQUAL(5ul, hEmptyKey2.size());

    CPPUNIT_ASSERT(hEmptyKey2.has("a"));
    CPPUNIT_ASSERT(hEmptyKey2.has("a."));
    hEmptyKey2.erasePath("a."); // empty key an end
    CPPUNIT_ASSERT(!hEmptyKey2.has("a."));
    CPPUNIT_ASSERT(!hEmptyKey2.has("a"));

    CPPUNIT_ASSERT(hEmptyKey2.has("a1"));
    CPPUNIT_ASSERT(hEmptyKey2.has("a1."));
    hEmptyKey2.erasePath("a1");
    CPPUNIT_ASSERT(!hEmptyKey2.has("a1."));
    CPPUNIT_ASSERT(!hEmptyKey2.has("a1"));

    CPPUNIT_ASSERT(hEmptyKey2.has("b"));
    CPPUNIT_ASSERT(hEmptyKey2.has("b."));
    CPPUNIT_ASSERT(hEmptyKey2.has("b.."));
    Hash& b2 = hEmptyKey2.get<Hash>("b");
    CPPUNIT_ASSERT(b2.has("."));
    b2.erasePath("."); // empty key at begin and end
    CPPUNIT_ASSERT(!hEmptyKey2.has("b.."));
    CPPUNIT_ASSERT(!hEmptyKey2.has("b."));
    CPPUNIT_ASSERT(hEmptyKey2.has("b"));

    CPPUNIT_ASSERT(hEmptyKey2.has("c"));
    CPPUNIT_ASSERT(hEmptyKey2.has("c."));
    CPPUNIT_ASSERT(hEmptyKey2.has("c..d"));
    Hash& c2 = hEmptyKey2.get<Hash>("c");
    c2.erasePath(".d"); // empty key at begin
    CPPUNIT_ASSERT(!hEmptyKey2.has("c..d"));
    CPPUNIT_ASSERT(!hEmptyKey2.has("c."));
    CPPUNIT_ASSERT(hEmptyKey2.has("c"));

    CPPUNIT_ASSERT(hEmptyKey2.has("e"));
    CPPUNIT_ASSERT(hEmptyKey2.has("e."));
    CPPUNIT_ASSERT(hEmptyKey2.has("e..f"));
    hEmptyKey2.erasePath("e..f"); // empty key in middle
    CPPUNIT_ASSERT(!hEmptyKey2.has("e..f"));
    CPPUNIT_ASSERT(!hEmptyKey2.has("e."));
    CPPUNIT_ASSERT(!hEmptyKey2.has("e"));

    // Check vector treatment, i.e. erasePath("a.v[0]") where v was - either size 1 or longer
    // Test erasePath where it acts differently than erase
    Hash hEmptyKey3("a.vec[1]", Hash(), ".vecb[1]", Hash());
    hEmptyKey3.erasePath("a.vec[1]");
    CPPUNIT_ASSERT(hEmptyKey3.has("a.vec[0]"));
    hEmptyKey3.erasePath("a.vec[0]");
    CPPUNIT_ASSERT(!hEmptyKey3.has("a.vec"));
    CPPUNIT_ASSERT(!hEmptyKey3.has("a"));
    // Now empty key instead of "a":
    hEmptyKey3.erasePath(".vecb[1]");
    CPPUNIT_ASSERT(hEmptyKey3.has(".vecb[0]"));
    hEmptyKey3.erasePath(".vecb[0]");
    CPPUNIT_ASSERT(!hEmptyKey3.has(".vecb"));
    CPPUNIT_ASSERT(!hEmptyKey3.has(""));
    CPPUNIT_ASSERT(hEmptyKey3.empty());
}


void Hash_Test::testHas() {
    // Hash::has(path) is already used a lot in other tests, but some corner cases
    // are missing, e.g. non-existing array indices at different positions in path.
    Hash h1("a.b[2]", Hash(), "b[1]", Hash());
    CPPUNIT_ASSERT(h1.has("a") == true);
    CPPUNIT_ASSERT(h1.has("a.b") == true);
    CPPUNIT_ASSERT(h1.has("a.b[0]") == true);
    CPPUNIT_ASSERT(h1.has("a.b[1]") == true);
    CPPUNIT_ASSERT(h1.has("a.b[2]") == true);
    CPPUNIT_ASSERT(h1.has("a.b[2].some") == false);
    CPPUNIT_ASSERT(h1.has("a.b[2].some.other") == false);
    CPPUNIT_ASSERT(h1.has("a.b[3]") == false);
    CPPUNIT_ASSERT(h1.has("a.b[3].some") == false);
    CPPUNIT_ASSERT(h1.has("a.b[3].some.other") == false);
    // Test also vector<Hash> on first level:
    CPPUNIT_ASSERT(h1.has("b") == true);
    CPPUNIT_ASSERT(h1.has("b[2]") == false);
    // And now some index on a non-existing vector<Hash>:
    CPPUNIT_ASSERT(h1.has("c[0]") == false);
}


void Hash_Test::testIs() {
    // Test different cases: paths without indices, with index at end or in the middle.
    Hash h("a", 77, "b[1].d", 88.8, "b[2].c", 88);
    CPPUNIT_ASSERT(h.is<int>("a") == true);
    CPPUNIT_ASSERT(h.is<vector<Hash>>("b") == true);
    CPPUNIT_ASSERT(h.is<Hash>("b[0]") == true);
    CPPUNIT_ASSERT(h.is<double>("b[1].d") == true);
    CPPUNIT_ASSERT(h.is<Hash>("b[2]") == true);
    CPPUNIT_ASSERT(h.is<int>("b[2].c") == true);

    // Check for false on wrong type - cannot test all wrong types...
    CPPUNIT_ASSERT(h.is<float>("a") == false);
    CPPUNIT_ASSERT(h.is<Hash>("b") == false);
    CPPUNIT_ASSERT(h.is<int>("b[0]") == false);
    CPPUNIT_ASSERT(h.is<float>("b[1].d") == false);
    CPPUNIT_ASSERT(h.is<vector<Hash>>("b[2]") == false);
    CPPUNIT_ASSERT(h.is<double>("b[2].c") == false);

    // Check exceptions on bad paths:
    // 1) non-existing "normal" path
    bool caught1 = false;
    try {
        h.is<int>("c");
    } catch (karabo::util::ParameterException const& e) {
        caught1 = true;
    }
    CPPUNIT_ASSERT(caught1 == true);

    // 2) non-existing index of vector that is last item
    bool caught2 = false;
    try {
        h.is<Hash>("b[3]");
    } catch (karabo::util::ParameterException const& e) {
        caught2 = true;
    }
    CPPUNIT_ASSERT(caught2 == true);

    // 3) item under non-existing index of vector
    bool caught3 = false;
    try {
        h.is<int>("b[3].d");
    } catch (karabo::util::ParameterException const& e) {
        caught3 = true;
    }
    CPPUNIT_ASSERT(caught3 == true);

    // 4) non-existing item under existing index of vector
    bool caught4 = false;
    try {
        h.is<int>("b[0].a");
    } catch (karabo::util::ParameterException const& e) {
        caught4 = true;
    }
    CPPUNIT_ASSERT(caught4 == true);
}


namespace helper {


    class Helper {
       public:
        Helper(){};


        virtual ~Helper(){};


        bool operator()(const karabo::util::Hash::Node& node) {
            return eval(node);
        }

        virtual bool eval(const karabo::util::Hash::Node& node) = 0;
    };

    bool dfs(const karabo::util::Hash& hash, Helper& helper);
    bool dfs(const std::vector<karabo::util::Hash>& hash, Helper& helper);
    bool dfs(const karabo::util::Hash::Node& node, Helper& helper);


    bool dfs(const karabo::util::Hash& hash, Helper& helper) {
        if (hash.empty()) return false;

        for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
            if (!dfs(*it, helper)) return false;
        }
        return true;
    }


    bool dfs(const std::vector<karabo::util::Hash>& hash, Helper& helper) {
        if (hash.empty()) return false;

        for (size_t i = 0; i < hash.size(); ++i) {
            if (!dfs(hash[i], helper)) return false;
        }
        return true;
    }


    bool dfs(const karabo::util::Hash::Node& node, Helper& helper) {
        helper(node);

        if (node.getType() == Types::HASH) {
            return dfs(node.getValue<Hash>(), helper);
        }
        if (node.getType() == Types::VECTOR_HASH) {
            return dfs(node.getValue<std::vector<Hash>>(), helper);
        }
        return true;
    }


    template <class V, class E>
    class Visitor {
       public:
        Visitor(){};


        ~Visitor(){};


        E operator()(V& visitable) {
            E evaluator;
            visitable.visit(evaluator);
            return evaluator;
        }


        bool operator()(V& visitable, E& evaluator) {
            return visitable.visit(evaluator);
        }


        template <class Helper>
        static bool visit__(const karabo::util::Hash& hash, Helper& helper) {
            if (hash.empty()) return false;

            for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                if (!visit__(*it, helper)) return false;
            }
            return true;
        }


        template <class Helper>
        static bool visit__(const std::vector<karabo::util::Hash>& hash, Helper& helper) {
            if (hash.empty()) return false;

            for (size_t i = 0; i < hash.size(); ++i) {
                if (!visit__(hash[i], helper)) return false;
            }
            return true;
        }


        template <class Helper>
        static bool visit__(const karabo::util::Hash::Node& node, Helper& helper) {
            helper(node);

            if (node.getType() == Types::HASH) {
                return visit__(node.getValue<Hash>(), helper);
            }
            if (node.getType() == Types::VECTOR_HASH) {
                return visit__(node.getValue<std::vector<Hash>>(), helper);
            }
            return true;
        }
    };


} // namespace helper


class Counter : public helper::Helper {
   public:
    Counter() : m_counter(0) {}


    bool eval(const karabo::util::Hash::Node& node) {
        if (node.getType() == Types::VECTOR_HASH) {
            m_counter += node.getValue<std::vector<Hash>>().size();
        } else {
            ++m_counter;
        }
        return true;
    }


    size_t getResult() {
        return m_counter;
    }

   private:
    size_t m_counter;
};


class Concat : public helper::Helper {
   public:
    Concat() : m_concat("") {}


    bool eval(const karabo::util::Hash::Node& node) {
        m_concat += node.getKey();
        return true;
    }


    std::string getResult() {
        return m_concat;
    }

   private:
    std::string m_concat;
};


class Serializer : public helper::Helper {
   public:
    Serializer() : indent(0) {
        memset(fill, ' ', 256);
        fill[indent] = 0;
        indices.push(-1);
    }


    void pre(const karabo::util::Hash::Node& node) {
        int& top = indices.top();
        if (top >= 0) {
            fill[indent - 2] = 0;
            m_stream << fill << '[' << top++ << ']' << '\n';
            fill[indent - 2] = ' ';
        }
        m_stream << fill << node.getKey();

        const Hash::Attributes& attrs = node.getAttributes();
        if (attrs.size() > 0) {
            for (Hash::Attributes::const_iterator ait = attrs.begin(); ait != attrs.end(); ++ait) {
                m_stream << " " << ait->getKey() << "=\""
                         << ait->getValueAs<string>() /*<< " " << Types::to<ToLiteral>(ait->getType())*/ << "\"";
            }
        }

        switch (node.getType()) {
            case Types::HASH:
            case Types::VECTOR_HASH:
                fill[indent] = ' ';
                indent += 2;
                fill[indent] = 0;
                break;
            default:
                break;
        };
    }


    bool eval(const karabo::util::Hash::Node& node) {
        Types::ReferenceType type = node.getType();
        switch (type) {
            case Types::HASH:
                m_stream << " +";
                indices.push(-1);
                break;
            case Types::VECTOR_HASH:
                m_stream << " @";
                indices.push(0);
                break;
            case Types::SCHEMA:
                m_stream << " => " << node.getValue<karabo::util::Schema>();
                break;
            default:
                if (Types::isPointer(type)) { // TODO Add pointer types
                    m_stream << " => xxx " << Types::to<ToLiteral>(type);
                } else {
                    m_stream << " => " << node.getValueAs<string>() << " " << Types::to<ToLiteral>(type);
                }
        }
        m_stream << '\n';
        return true;
    }


    void post(const karabo::util::Hash::Node& node) {
        switch (node.getType()) {
            case Types::HASH:
            case Types::VECTOR_HASH:
                fill[indent] = ' ';
                indent -= 2;
                fill[indent] = 0;
                indices.pop();
                break;
            default:
                break;
        };
    }


    const std::ostringstream& getResult() {
        return m_stream;
    }

   private:
    std::ostringstream m_stream;
    char fill[256];
    int indent;
    std::stack<int> indices;
};


class Flatten : public helper::Helper {
   public:
    Flatten(const char sep = '/') : separator(sep) {
        prefix.push("");
        indices.push(-1);
    };


    ~Flatten(){};


    void pre(const karabo::util::Hash::Node& node) {
        ostringstream oss;
        if (prefix.top().empty()) oss << node.getKey();
        else {
            oss << prefix.top();
            int& top = indices.top();
            if (top >= 0) {
                oss << '[' << top++ << ']';
            }
            oss << separator << node.getKey();
        }

        switch (node.getType()) {
            case Types::HASH:
            case Types::VECTOR_HASH:
                prefix.push(oss.str());
                break;
            default:
                flat.set(oss.str(), node.getValueAsAny(), 0);
                flat.setAttributes(oss.str(), node.getAttributes(), 0);
        };
    }


    bool eval(const karabo::util::Hash::Node& node) {
        Types::ReferenceType type = node.getType();
        switch (type) {
            case Types::HASH:
                indices.push(-1);
                break;
            case Types::VECTOR_HASH:
                indices.push(0);
                break;
            default:
                break;
        }
        return true;
    }


    void post(const karabo::util::Hash::Node& node) {
        switch (node.getType()) {
            case Types::HASH:
            case Types::VECTOR_HASH:
                prefix.pop();
                indices.pop();
                break;
            default:
                break;
        };
    }


    const Hash& getResult() {
        return flat;
    }

   private:
    Hash flat;
    char separator;
    std::stack<string> prefix;
    std::stack<int> indices;
};


class Paths : public helper::Helper {
   public:
    Paths(const char sep = '/') : separator(sep) {
        prefix.push("");
        indices.push(-1);
    };


    ~Paths(){};


    void pre(const karabo::util::Hash::Node& node) {
        ostringstream oss;
        if (prefix.top().empty()) oss << node.getKey();
        else {
            oss << prefix.top();
            int& top = indices.top();
            if (top >= 0) {
                oss << '[' << top++ << ']';
            }
            oss << separator << node.getKey();
        }

        switch (node.getType()) {
            case Types::HASH:
            case Types::VECTOR_HASH:
                prefix.push(oss.str());
                break;
            default:
                paths.push_back(oss.str());
        };
    }


    bool eval(const karabo::util::Hash::Node& node) {
        Types::ReferenceType type = node.getType();
        switch (type) {
            case Types::HASH:
                indices.push(-1);
                break;
            case Types::VECTOR_HASH:
                indices.push(0);
                break;
            default:
                break;
        }
        return true;
    }


    void post(const karabo::util::Hash::Node& node) {
        switch (node.getType()) {
            case Types::HASH:
            case Types::VECTOR_HASH:
                prefix.pop();
                indices.pop();
                break;
            default:
                break;
        };
    }


    const std::vector<std::string>& getResult() {
        return paths;
    }

   private:
    std::vector<std::string> paths;
    char separator;
    std::stack<string> prefix;
    std::stack<int> indices;
};


void Hash_Test::testHelper() {
    {
        Hash h3("a", 21, "b.c", 22, "c.b[0]", Hash("key", "value"), "c.b[1].d", 24, "e", 23);
        h3.setAttribute("a", "at0", "value0");

        Hash h2("a", 21, "b.c", 22, "c.b[0]", Hash("key", "value"), "c.b[1].d", h3, "e", 27);
        h2.setAttribute("a", "at1", "value1");

        Hash h1("a", 1, "b", 2, "c.b[0].g", h2, "c.c[0].d", h2, "c.c[1]", Hash("a.b.c", h2), "d.e", 7);

        h1.setAttribute("a", "at2", "value2");

        Counter counter;
        helper::dfs(h1, counter);

        Concat concat;
        helper::dfs(h1, concat);

        Serializer serializer;
        helper::dfs(h1, serializer);

        // std::clog << "Count    1: " << counter.getResult() << std::endl;
        // std::clog << "Concate  1: " << concat.getResult() << std::endl;
        // std::clog << "Serialize1: " << serializer.getResult().str() << std::endl;

        Counter counter2;
        Concat concat2;
        Serializer serializer2;
        Flatten flatten;
        Paths paths;

        h1.visit(counter2);
        h1.visit(concat2);
        h1.visit2(serializer2);
        h1.visit2(flatten);
        h1.visit2(paths);

        // std::clog << "Count    2: " << counter2.getResult() << std::endl;
        // std::clog << "Concate  2: " << concat2.getResult() << std::endl;
        // std::clog << "SerializeH: \n" << h1 << std::endl;
        // std::clog << "Serialize2: \n" << serializer2.getResult().str() << std::endl;

        //        std::clog << "Hash : " << h1 << std::endl;
        //        std::clog << "FlatV: " << flatten.getResult() << std::endl;

        Hash flat;
        Hash::flatten(h1, flat, "", '/');
        //        std::clog << "FlatH: " << flat << std::endl;

        //        std::clog << "Paths : " << std::endl;
        //        for (int i = 0; i < paths.getResult().size(); ++i) {
        //            std::clog << "\t" << paths.getResult()[i] << std::endl;
        //        }

        // helper::Visitor<Hash, Counter> count;
        // std::clog << "Count 2 : " << count(h1).getResult() << std::endl;
        // std::clog << "Count 3 : " << helper::Visitor<Hash, Counter>()(h1).getResult() << std::endl;
        // std::clog << "Count H : " << karabo::util::counter(h1) << std::endl;
        // std::clog << "Count V : " << c2.getResult() << std::endl;
    }
}


void Hash_Test::testPack() {
    Hash h;
    pack(h);
    CPPUNIT_ASSERT(h.size() == 0);
    pack(h, 3);
    CPPUNIT_ASSERT(h.size() == 1);
    CPPUNIT_ASSERT(h.get<int>("a1") == 3);
    pack(h, 3, 2);
    pack(h, string("bla"), 2.5);
    CPPUNIT_ASSERT(h.size() == 2);
    CPPUNIT_ASSERT(h.get<string>("a1") == "bla");
    CPPUNIT_ASSERT(h.get<double>("a2") == 2.5);

    string s;
    double x;

    unpack(h, s, x);
    CPPUNIT_ASSERT(s == "bla");
    CPPUNIT_ASSERT(x == 2.5);
}

void Hash_Test::testCounter() {
    Hash h("a", true, "b", int(0), "c", NDArray(Dims(5, 5)), "d", std::vector<int>(3, 0));
    h.set("e", std::vector<NDArray>(3, NDArray(Dims(5, 5))));
    // if counter were not to skip over Hash derived classes the ND-Array internal reference type of type
    // INT32 would be counted leading to a count of 8
    CPPUNIT_ASSERT(karabo::util::counter(h, karabo::util::Types::INT32) == 4);
    // if counter were not to skip over Hash derived classes the ND-Array internal is big endian of type
    // BOOL would be counted leading to a count of 5
    CPPUNIT_ASSERT(karabo::util::counter(h, karabo::util::Types::BOOL) == 1);
    CPPUNIT_ASSERT(karabo::util::counter(h, karabo::util::Types::HASH) == 1);
}


void Hash_Test::testKeys() {
    // Test various funny keys/paths
    Hash h(" ", true, "", false, ".", 0, ".b", 1, "a.", 2, "c..b", 3);

    CPPUNIT_ASSERT(h.has(" "));
    CPPUNIT_ASSERT(h.has(""));
    CPPUNIT_ASSERT(h.has("a"));
    CPPUNIT_ASSERT(h.has("c"));
    CPPUNIT_ASSERT_EQUAL(4ul, h.size()); // no other 1st level keys!

    const Hash& g = h.get<Hash>("");
    CPPUNIT_ASSERT(g.has(""));
    CPPUNIT_ASSERT(g.has("b"));
    CPPUNIT_ASSERT_EQUAL(2ul, g.size()); // dito

    const Hash& a = h.get<Hash>("a");
    CPPUNIT_ASSERT(a.has(""));
    CPPUNIT_ASSERT_EQUAL(1ul, a.size()); // dito

    const Hash& c = h.get<Hash>("c");
    CPPUNIT_ASSERT(a.has(""));
    CPPUNIT_ASSERT_EQUAL(1ul, c.size()); // dito

    const Hash& c1 = c.get<Hash>("");
    CPPUNIT_ASSERT(c1.has("b"));
    CPPUNIT_ASSERT_EQUAL(1ul, c1.size()); // dito
}


void testSimilarIsNotFullyEqualByOrder(bool orderMatters) {
    std::clog << "testSimilarIsNotFullyEqualByOrder starts with orderMatters = " << orderMatters << std::endl;

    Hash h1("a.b", "value", "a.c", true);
    Hash h2("a1.b", "value", "a1.c", true);

    // Checks that hashes with elements with different keys of the same type and in the same order
    // are still similar.
    CPPUNIT_ASSERT_EQUAL(h1, h2); // 'Hash::operator==' actually checks for similarity.
    // But are not fullyEqual
    CPPUNIT_ASSERT_MESSAGE("h1 and h2 shouldn't be fullyEquals - they differ in key names.",
                           !h1.fullyEquals(h2, orderMatters));

    Hash h3("a1", 1, "a1.b", "value", "a1.c", false);
    // Checks that hashes with elements with different values of the same type and in the same
    // order are still similar.
    CPPUNIT_ASSERT_EQUAL(h2, h3); // 'Hash::operator==' actually checks for similarity.
    // But are not fullyEqual
    CPPUNIT_ASSERT_MESSAGE("h2 and h3 shouldn't be fullyEquals - they differ in key values.",
                           !h2.fullyEquals(h3, orderMatters));

    Hash h4("a1", 1, "a1.b", "value", "a1.c", true);
    h4.setAttribute("a1", "attr", true);
    h2.setAttribute("a1", "attr", 4);
    // Checks that hashes with elements with different attributes, with the same value, of the
    // same type and in the same order are still similar.
    CPPUNIT_ASSERT_EQUAL(h2, h4); // 'Hash::operator==' actually checks for similarity.
    // But are not fullyEqual
    CPPUNIT_ASSERT_MESSAGE("h4 and h2 shouldn't be fullyEquals - they differ in element attributes.",
                           !h2.fullyEquals(h4, orderMatters));

    Hash h5("a", 13.14159, "b[0]", Hash("hKey_0", "hValue_0"), "b[1]", Hash("hKey_1", "hValue_1"), "c",
            "1, 1, 2, 3, 5, 8, 11, 19, 30");
    Hash h6("a", 13.14159, "b[0]", Hash("hKey_0", "hValue_0"), "b[1]", Hash("hKey_1", "hValue_1"), "c",
            "1, 1, 2, 3, 5, 8, 11, 19, 30, 49, 79");
    // Repeats the test for hashes differing in node value, but this time with one
    // complex node, of type vector of hashes, that matches. The hashes are similar ...
    CPPUNIT_ASSERT_EQUAL(h5, h6); // 'Hash::operator==' actually checks for similarity.
    // But are not fullyEqual
    CPPUNIT_ASSERT_MESSAGE("h5 and h6 shouldn't be fullyEquals - they differ in element values.",
                           !h5.fullyEquals(h6, orderMatters));

    vector<Hash> vhAttr{Hash("key_0", "val_0"), Hash("key_1", "val_1")};
    h5.setAttribute("a", "attr", vhAttr);
    h6.setAttribute("a", "attr", 2);
    h6.set<std::string>("c", "1, 1, 2, 3, 5, 8, 11, 19, 30");
    CPPUNIT_ASSERT_MESSAGE("h5 and h6 shouldn't be fullyEquals - they differ in vector of hash attribute",
                           !h5.fullyEquals(h6, orderMatters));

    // A case where two hashes with complex attributes and nodes are fullyEquals.
    h6.setAttribute("a", "attr", vhAttr);
    CPPUNIT_ASSERT_MESSAGE("h5 and h6 should be fullyEquals!", h5.fullyEquals(h6, orderMatters));

    Hash h7("a", 1, "b", 2, "c", 3);
    Hash h8("b", 1, "a", 2, "c", 3);
    // Checks that hashes with keys in different order are still similar.
    CPPUNIT_ASSERT_EQUAL(h7, h8);
    // But are not fullyEqual.
    CPPUNIT_ASSERT_MESSAGE("h7 and h8 shouldn't be fullyEquals - they differ in the order of their elements.",
                           !h7.fullyEquals(h8, orderMatters));

    Hash h9("a", 1, "b", 2, "c", "3");
    // Checks that hashes with different value types for values that have the same string representation form are
    // neither similar nor fullyEquals.
    CPPUNIT_ASSERT_MESSAGE("h7 and h9 should not be similar, as their 'c' elements differ in type.", h7 != h9);
    CPPUNIT_ASSERT_MESSAGE("h7 and h9 should not be fullyEquals, as their 'c' elements differ in type.",
                           !h7.fullyEquals(h9, orderMatters));

    // Check VECTOR_STRING treatment
    Hash h11("vecStr", std::vector<std::string>({"with,comma", "with space", "onlyChar"}));
    Hash h12("vecStr", std::vector<std::string>({"with,comma", "with space"}));
    CPPUNIT_ASSERT_MESSAGE("Differ in number of elements in vector", !h11.fullyEquals(h12, orderMatters));
    h12.get<std::vector<std::string>>("vecStr").push_back("onlychar");
    CPPUNIT_ASSERT_MESSAGE("Differ in one character of last element in vector", !h11.fullyEquals(h12, orderMatters));
    h12.get<std::vector<std::string>>("vecStr").back() = "onlyChar"; // now make fully equal
    CPPUNIT_ASSERT(h11.fullyEquals(h12, orderMatters));
    // Now VECTOR_STRING as attribute
    h11.setAttribute("vecStr", "vecStrOpt", std::vector<std::string>({"With,comma", "With space", "OnlyChar"}));
    h12.setAttribute("vecStr", "vecStrOpt", std::vector<std::string>({"With,comma", "With space"}));
    CPPUNIT_ASSERT_MESSAGE("Differ in number of elements in vector attribute", !h11.fullyEquals(h12, orderMatters));
    h12.getAttribute<std::vector<std::string>>("vecStr", "vecStrOpt").push_back("Onlychar");
    CPPUNIT_ASSERT_MESSAGE("Differ in one character of last element in vector attribute",
                           !h11.fullyEquals(h12, orderMatters));
    h12.getAttribute<std::vector<std::string>>("vecStr", "vecStrOpt").back() = "OnlyChar";
    CPPUNIT_ASSERT(h11.fullyEquals(h12, orderMatters));

    Schema sch("hashSchema");
    INT32_ELEMENT(sch).key("a").tags("prop").assignmentOptional().defaultValue(10).commit();
    Hash h10("b", 2, "a", 1, "c", 3);
    h10.setAttribute("c", "schema", sch);
    h8.setAttribute("c", "schema", Schema("test"));
    // Checks that hashes with different attributes of type schema are similar
    CPPUNIT_ASSERT_EQUAL(h8, h10);
    // But are not fullyEquals
    CPPUNIT_ASSERT_MESSAGE(
          "h8 and h10 should not be fullyEquals, as they have different values for attributes of type Schema ",
          !h8.fullyEquals(h10, orderMatters));
}


void Hash_Test::testSimilarIsNotFullyEqual() {
    testSimilarIsNotFullyEqualByOrder(true);
    testSimilarIsNotFullyEqualByOrder(false);
}


void Hash_Test::testFullyEqualUnordered() {
    // Just two keys are swapped: hashes differ if order matters, otherwise not
    Hash h1("a.b", "value", "a.c", true, "1", 1);
    Hash h2("a.c", true, "a.b", "value", "1", 1);

    CPPUNIT_ASSERT(!h1.fullyEquals(h2, true));
    CPPUNIT_ASSERT(h1.fullyEquals(h2, false));

    // Just order of attributes is swapped: hashes differ if order matters, otherwise not
    Hash h3(h1);
    h3.setAttribute("1", "A", 1);
    h3.setAttribute("1", "B", 2);
    h1.setAttribute("1", "B", 2);
    h1.setAttribute("1", "A", 1);

    CPPUNIT_ASSERT_MESSAGE(toString(h1) + " vs " + toString(h3), !h1.fullyEquals(h3, true));
    CPPUNIT_ASSERT_MESSAGE(toString(h1) + " vs " + toString(h3), h1.fullyEquals(h3, false));
}


void Hash_Test::testNode() {
    // Hash::Node::setValue
    {
        Hash h1, h2;
        Hash::Node& node1 = h1.set("a", 1);
        Hash::Node& node2 = h2.set("a", 1);

        // setValue: Template specialization for Hash and the overload for Hash must have the same effect
        //           concerning __classId attribute:
        node1.setValue(Hash("1", 2));
        node2.setValue<Hash>(Hash("1", 2));
        CPPUNIT_ASSERT(!node1.hasAttribute("__classId"));
        CPPUNIT_ASSERT(!node2.hasAttribute("__classId"));

        CPPUNIT_ASSERT_EQUAL(0ul, node1.getAttributes().size());
        CPPUNIT_ASSERT_EQUAL(0ul, node2.getAttributes().size());
    }
    {
        // Test Hash::Node::setValue and the possible type change introduced by that.
        Hash h("a.b.c", "1");
        CPPUNIT_ASSERT(h.get<std::string>("a.b.c") == "1");
        CPPUNIT_ASSERT(h.getAs<int>("a.b.c") == 1);
        boost::optional<Hash::Node&> node = h.find("a.b.c");
        if (node) node->setValue(2);
        CPPUNIT_ASSERT(h.get<int>("a.b.c") == 2);
        CPPUNIT_ASSERT(h.getAs<std::string>("a.b.c") == "2");
    }
    {
        // Setting a Hash::Node is setting its value (due to Element::setValue(..) template specification).
        // Question arising: Why? h.set(node.getValueAsAny()) already does that...
        // But we keep backward compatibility here, i.e. this test succeeds in 2.11.4, but fails in 2.12.0,
        // see also https://git.xfel.eu/Karabo/Framework/-/merge_requests/5940.
        Hash::Node node("a", 1);
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(Types::ReferenceType::INT32), static_cast<int>(node.getType()));
        const Hash::Node constNode(node);

        // Test setting for all cases: normal object, rvalue reference and const object
        Hash h;
        h.set("normal", node);
        h.set("moved", std::move(node));
        h.set("const", constNode);

        CPPUNIT_ASSERT_EQUAL(static_cast<int>(Types::ReferenceType::INT32), static_cast<int>(h.getType("moved")));
        CPPUNIT_ASSERT_EQUAL(1, h.get<int>("moved"));

        CPPUNIT_ASSERT_EQUAL(static_cast<int>(Types::ReferenceType::INT32), static_cast<int>(h.getType("const")));
        CPPUNIT_ASSERT_EQUAL(1, h.get<int>("const"));

        CPPUNIT_ASSERT_EQUAL(static_cast<int>(Types::ReferenceType::INT32), static_cast<int>(h.getType("normal")));
        CPPUNIT_ASSERT_EQUAL(1, h.get<int>("normal"));
    }
    {
        // Similar as before, but now testing also move semantics (i.e. would not succeed in 2.11.4).
        Hash::Node node("a", TraceCopies(2));
        const Hash::Node constNode("a", TraceCopies(3));
        TraceCopies::reset();

        // Test setting for all cases: normal object, rvalue reference and const object
        Hash h;
        h.set("normal", node);
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(2, h.get<TraceCopies>("normal").value);
        CPPUNIT_ASSERT_EQUAL(
              2, node.getValue<TraceCopies>().value); // i.e. not -1 as for a 'moved away' TraceCopies instance

        // Moving the node means move-assignment of its m_value member (which is a std::any) to the new node inside h.
        // That leaves node's m_value in a valid, but undefined state, so better do not use it (e.g. by node.getValue)
        // anymore.
        h.set("moved", std::move(node));
        CPPUNIT_ASSERT_EQUAL(2, h.get<TraceCopies>("moved").value);
        // Next line would throw (see comment above) since node::m_value has an unknown type (void?) that cannot be cast
        // to TraceCopies CPPUNIT_ASSERT_EQUAL(-1, node.getValue<TraceCopies>().value);
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr); // There was no copy!
        // Next line succeeds, i.e. no move assignment either. Looks like since the move-assignment of std::any just
        // swaps between source and target instead of moving. But that is an implementation detail we should not tests.
        // CPPUNIT_ASSERT_EQUAL(0, TraceCopies::countMoveConstr);

        h.set("const", constNode);
        CPPUNIT_ASSERT_EQUAL(2, TraceCopies::countCopyConstr); // another copy now
        CPPUNIT_ASSERT_EQUAL(3, h.get<TraceCopies>("const").value);
        CPPUNIT_ASSERT_EQUAL(
              3, constNode.getValue<TraceCopies>().value); // i.e. not -1 as for a 'moved away' TraceCopies instance

        TraceCopies::reset();
    }
    {
        // Tests of Hash::Node constructors with move semantics from ValueType
        const TraceCopies a(1);
        Hash::Node nodeA("a", a);
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(1, nodeA.getValue<TraceCopies>().value);
        CPPUNIT_ASSERT_EQUAL(1, a.value); // not -1 as for a moved away object

        TraceCopies b(2);
        TraceCopies&& bRval = std::move(b); // std::move is in fact just a cast
        Hash::Node nodeB("b", std::move(bRval));
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(2, nodeB.getValue<TraceCopies>().value);
        CPPUNIT_ASSERT_EQUAL(-1, b.value); // -1 as for a moved away object

        TraceCopies::reset();
    }
    {
        // Tests of Hash::Node constructors with move semantics from std::any
        const std::any a(TraceCopies(1));
        TraceCopies::reset(); // Whatever the line before did does not matter...
        Hash::Node nodeA("a", a);
        CPPUNIT_ASSERT_EQUAL(1, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(1, nodeA.getValue<TraceCopies>().value);
        CPPUNIT_ASSERT_EQUAL(1, std::any_cast<TraceCopies>(a).value); // not -1 as for a moved away object

        std::any b(TraceCopies(2));
        TraceCopies::reset();            // Whatever the line before did does not matter...
        std::any&& bRval = std::move(b); // std::move is in fact just a cast
        Hash::Node nodeB("b", std::move(bRval));
        // Not copied - in fact it is not moved either (since std::any is moved which seems to swap),
        // but that is an implementation detail we better do not test against.
        CPPUNIT_ASSERT_EQUAL(0, TraceCopies::countCopyConstr);
        CPPUNIT_ASSERT_EQUAL(2, nodeB.getValue<TraceCopies>().value);
        // Next line would throw again due to access to moved-away b
        // CPPUNIT_ASSERT_EQUAL(-1, std::any_cast<TraceCopies>(b).value);

        TraceCopies::reset();
    }
}
