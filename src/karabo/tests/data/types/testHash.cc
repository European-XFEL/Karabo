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

#include <gtest/gtest.h>

#include <climits>
#include <karabo/util/PackParameters.hh>
#include <stack>
#include <vector>

#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/types/Exception.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/NDArray.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/data/types/ToLiteral.hh"

using namespace karabo::data;
using namespace std;

// It's unclear if we need in GoogleTest something similar to what follows below...
// namespace CppUnit {
//     // Enable EXPECT_EQ for vectors
//     // (Note: vector<unsigned char> might need special casing, vector<Hash> even more!)
//     template <typename T>
//     struct assertion_traits<std::vector<T>> {
//         static bool equal(const std::vector<T>& a, const std::vector<T>& b) {
//             return a == b;
//         }

//         static std::string toString(const std::vector<T>& p) {
//             return karabo::data::toString(p);
//         }
//     };
// } // namespace CppUnit


TEST(TestHash, testConstructors) {
    {
        Hash h;
        h.set("h", Hash());
        Hash& i = h.get<Hash>("h");
        i.set("i", Hash("j", 5));
        EXPECT_TRUE(h.get<int>("h.i.j") == 5);
    }

    {
        Hash h;
        EXPECT_TRUE(h.empty() == true);
        EXPECT_TRUE(h.size() == 0);
    }

    {
        Hash h("a", 1);
        EXPECT_TRUE(h.empty() == false);
        EXPECT_TRUE(h.size() == 1);
        EXPECT_TRUE(h.get<int>("a") == 1);
    }

    {
        Hash h("a", 1, "b", 2.0);
        EXPECT_TRUE(h.empty() == false);
        EXPECT_TRUE(h.size() == 2);
        EXPECT_TRUE(h.get<int>("a") == 1);
        EXPECT_TRUE(h.get<double>("b") == 2.0);
    }

    {
        Hash h("a", 1, "b", 2.0, "c", 3.f);
        EXPECT_TRUE(h.empty() == false);
        EXPECT_TRUE(h.size() == 3);
        EXPECT_TRUE(h.get<int>("a") == 1);
        EXPECT_TRUE(h.get<double>("b") == 2.0);
        EXPECT_TRUE(h.get<float>("c") == 3.0);
    }

    {
        Hash h("a", 1, "b", 2.0, "c", 3.f, "d", "4");
        EXPECT_TRUE(h.empty() == false);
        EXPECT_TRUE(h.size() == 4);
        EXPECT_TRUE(h.get<int>("a") == 1);
        EXPECT_TRUE(h.get<double>("b") == 2.0);
        EXPECT_TRUE(h.get<float>("c") == 3.0);
        EXPECT_TRUE(h.get<string>("d") == "4");
    }

    {
        const Dims shape(2, 5);
        std::vector<float> data(10, 4.2);
        NDArray arr(&data[0], data.size(), shape);

        Hash h("arr", arr);
        EXPECT_TRUE(h.empty() == false);
        EXPECT_TRUE(h.get<NDArray>("arr").getShape().toVector() == shape.toVector());
    }

    {
        Hash h("a", 1, "b", 2.0, "c", 3.f, "d", "4", "e", std::vector<unsigned int>(5, 5));
        EXPECT_TRUE(h.empty() == false);
        EXPECT_TRUE(h.size() == 5);
        EXPECT_TRUE(h.get<int>("a") == 1);
        EXPECT_TRUE(h.get<double>("b") == 2.0);
        EXPECT_TRUE(h.get<float>("c") == 3.0);
        EXPECT_TRUE(h.get<string>("d") == "4");
        EXPECT_TRUE(h.get<std::vector<unsigned int>>("e")[0] == 5);
    }

    {
        Hash h("a", 1, "b", 2.0, "c", 3.f, "d", "4", "e", std::vector<unsigned int>(5, 5), "f", Hash("a", 6));
        EXPECT_TRUE(h.empty() == false);
        EXPECT_TRUE(h.size() == 6);
        EXPECT_TRUE(h.get<int>("a") == 1);
        EXPECT_TRUE(h.get<double>("b") == 2.0);
        EXPECT_TRUE(h.get<float>("c") == 3.0);
        EXPECT_TRUE(h.get<string>("d") == "4");
        EXPECT_TRUE(h.get<std::vector<unsigned int>>("e")[0] == 5);
        EXPECT_TRUE(h.get<Hash>("f").get<int>("a") == 6);
        EXPECT_TRUE(h.get<int>("f.a") == 6);
    }

    {
        Hash h("a", 1, "b", 2.0, "c", 3.f, "d", "4", "e", std::vector<unsigned int>(5, 5), "f",
               Hash::Pointer(new Hash("a", 6)));
        EXPECT_TRUE(h.empty() == false);
        EXPECT_TRUE(h.size() == 6);
        EXPECT_TRUE(h.get<int>("a") == 1);
        EXPECT_TRUE(h.get<double>("b") == 2.0);
        EXPECT_TRUE(h.get<float>("c") == 3.0);
        EXPECT_TRUE(h.get<string>("d") == "4");
        EXPECT_TRUE(h.get<std::vector<unsigned int>>("e")[0] == 5);
        EXPECT_TRUE(h.get<Hash::Pointer>("f")->get<int>("a") == 6);
    }


    {
        Hash h("a", 1, "b", 2.0, "c", 3.f, "d", "4", "e", std::vector<unsigned int>(5, 5), "f",
               std::vector<Hash::Pointer>(5, Hash::Pointer(new Hash("a", 6))));
        EXPECT_TRUE(h.empty() == false);
        EXPECT_TRUE(h.size() == 6);
        EXPECT_TRUE(h.get<int>("a") == 1);
        EXPECT_TRUE(h.get<double>("b") == 2.0);
        EXPECT_TRUE(h.get<float>("c") == 3.0);
        EXPECT_TRUE(h.get<string>("d") == "4");
        EXPECT_TRUE(h.get<std::vector<unsigned int>>("e")[0] == 5);
        EXPECT_TRUE(h.get<std::vector<Hash::Pointer>>("f")[3]->get<int>("a") == 6);
    }

    {
        Hash h("a.b.c", 1, "b.c", 2.0, "c", 3.f, "d.e", "4", "e.f.g.h", std::vector<unsigned long long>(5, 5),
               "F.f.f.f.f", Hash("x.y.z", 99));
        h.set("foo.array", NDArray(Dims(5, 5)));
        EXPECT_TRUE(h.empty() == false);
        EXPECT_TRUE(h.size() == 7);
        EXPECT_TRUE(h.get<int>("a.b.c") == 1);
        EXPECT_TRUE(h.get<double>("b.c") == 2.0);
        EXPECT_TRUE(h.get<float>("c") == 3.0);
        EXPECT_TRUE(h.get<string>("d.e") == "4");
        EXPECT_TRUE(h.get<std::vector<unsigned long long>>("e.f.g.h")[0] == 5);
        EXPECT_TRUE(h.get<Hash>("F.f.f.f.f").get<int>("x.y.z") == 99);
        EXPECT_TRUE(h.get<int>("F.f.f.f.f.x.y.z") == 99);
        // Internally, Hash-derived classes are stored as Hash
        EXPECT_TRUE(h.getType("foo.array") == karabo::data::Types::HASH);

        // Check 'flatten'
        Hash flat;
        Hash::flatten(h, flat);

        EXPECT_TRUE(flat.empty() == false);
        EXPECT_TRUE(flat.size() == 7);
        EXPECT_TRUE(flat.get<int>("a.b.c", 0) == 1);
        EXPECT_TRUE(flat.get<double>("b.c", 0) == 2.0);
        EXPECT_TRUE(flat.get<float>("c", 0) == 3.0);
        EXPECT_TRUE(flat.get<string>("d.e", 0) == "4");
        EXPECT_TRUE(flat.get<std::vector<unsigned long long>>("e.f.g.h", 0)[0] == 5);
        EXPECT_TRUE(flat.get<int>("F.f.f.f.f.x.y.z", 0) == 99);
        // Internally, Hash-derived classes are stored as Hash
        EXPECT_TRUE(flat.getType("foo.array", 0) == karabo::data::Types::HASH);

        Hash tree;
        flat.unflatten(tree);

        EXPECT_TRUE(tree.empty() == false);
        EXPECT_TRUE(tree.size() == 7);
        EXPECT_TRUE(tree.get<int>("a.b.c") == 1);
        EXPECT_TRUE(tree.get<double>("b.c") == 2.0);
        EXPECT_TRUE(tree.get<float>("c") == 3.0);
        EXPECT_TRUE(tree.get<string>("d.e") == "4");
        EXPECT_TRUE(tree.get<std::vector<unsigned long long>>("e.f.g.h")[0] == 5);
        EXPECT_TRUE(tree.get<Hash>("F.f.f.f.f").get<int>("x.y.z") == 99);
        EXPECT_TRUE(tree.get<int>("F.f.f.f.f.x.y.z") == 99);
        // Internally, Hash-derived classes are stored as Hash
        EXPECT_TRUE(flat.getType("foo.array", 0) == karabo::data::Types::HASH);
    }

    {
        // copy constructor
        Hash tmp("a", 1);
        Hash h(tmp);
        EXPECT_TRUE(h.empty() == false);
        EXPECT_TRUE(h.size() == 1);
        EXPECT_TRUE(h.get<int>("a") == 1);
        EXPECT_TRUE(tmp.empty() == false);
    }

    {
        // lvalue assignment
        Hash tmp("a", 1);
        Hash h;
        h = tmp;
        EXPECT_TRUE(h.empty() == false);
        EXPECT_TRUE(h.size() == 1);
        EXPECT_TRUE(h.get<int>("a") == 1);
        EXPECT_TRUE(tmp.empty() == false);
    }

    {
        // move constructor
        Hash tmp("a", 1);
        Hash h(std::move(tmp));
        EXPECT_TRUE(h.empty() == false);
        EXPECT_TRUE(h.size() == 1);
        EXPECT_TRUE(h.get<int>("a") == 1);
        EXPECT_TRUE(tmp.empty() == true);
    }

    {
        // rvalue assignment
        Hash h;
        Hash tmp("a", 1);
        h = std::move(tmp);
        EXPECT_TRUE(h.empty() == false);
        EXPECT_TRUE(h.size() == 1);
        EXPECT_TRUE(h.get<int>("a") == 1);
        EXPECT_TRUE(tmp.empty() == true);
    }
}


TEST(TestHash, testGetSet) {
    {
        Hash h;
        h.set("a.b.c1.d", 1);
        EXPECT_TRUE(h.get<Hash>("a").has("b") == true);
        EXPECT_TRUE(h.get<Hash>("a.b").has("c1") == true);
        EXPECT_TRUE(h.get<Hash>("a.b.c1").has("d") == true);
        EXPECT_TRUE(h.get<int>("a.b.c1.d") == 1);
        EXPECT_TRUE(h.has("a.b.c1.d") == true);
        EXPECT_TRUE(h.get<Hash>("a").has("b.c1") == true);

        h.set("a.b.c2.d", "1");
        EXPECT_TRUE(h.get<Hash>("a").has("b") == true);
        EXPECT_TRUE(h.get<Hash>("a.b").has("c1") == true);
        EXPECT_TRUE(h.get<Hash>("a.b").has("c2") == true);
        EXPECT_TRUE(h.get<Hash>("a.b").has("c2.d") == true);
        EXPECT_TRUE(h.get<Hash>("a.b").is<string>("c2.d") == true);
        EXPECT_TRUE(h.get<Hash>("a.b.c2").has("d") == true);
        EXPECT_TRUE(h.get<string>("a.b.c2.d") == "1");

        h.set("a.b[0]", Hash("a", 1));
        EXPECT_TRUE(h.get<Hash>("a").has("b") == true);
        EXPECT_TRUE(h.get<Hash>("a").size() == 1);
        EXPECT_TRUE(h.is<std::vector<Hash>>("a.b") == true);
        EXPECT_TRUE(h.get<std::vector<Hash>>("a.b").size() == 1);
        EXPECT_TRUE(h.get<std::vector<Hash>>("a.b")[0].size() == 1);
        EXPECT_TRUE(h.get<std::vector<Hash>>("a.b")[0].get<int>("a") == 1);
        EXPECT_TRUE(h.get<int>("a.b[0].a") == 1);

        h.set("a.b[2]", Hash("a", "1"));
        EXPECT_TRUE(h.get<Hash>("a").has("b") == true);
        EXPECT_TRUE(h.get<Hash>("a").size() == 1);
        EXPECT_TRUE(h.is<std::vector<Hash>>("a.b") == true);
        EXPECT_TRUE(h.has("a.b") == true);
        EXPECT_TRUE(h.get<std::vector<Hash>>("a.b").size() == 3);
        EXPECT_TRUE(h.get<int>("a.b[0].a") == 1);
        EXPECT_TRUE(h.get<Hash>("a.b[1]").empty() == true);
        EXPECT_TRUE(h.get<string>("a.b[2].a") == "1");
        EXPECT_TRUE(h.get<std::vector<Hash>>("a.b")[0].is<int>("a") == true);
        EXPECT_TRUE(h.get<std::vector<Hash>>("a.b")[1].empty() == true);
        EXPECT_TRUE(h.get<std::vector<Hash>>("a.b")[2].is<string>("a") == true);

        EXPECT_TRUE(h.get<Hash>("a").is<Hash>("b[0]") == true);
        EXPECT_TRUE(h.get<Hash>("a").is<Hash>("b[1]") == true);
        EXPECT_TRUE(h.get<Hash>("a").is<Hash>("b[2]") == true);
        EXPECT_TRUE(h.get<Hash>("a.b[0]").empty() == false);
        EXPECT_TRUE(h.get<Hash>("a.b[1]").empty() == true);
        EXPECT_TRUE(h.get<Hash>("a.b[2]").empty() == false);
    }

    {
        Hash h;
        h.set("a.b.c", 1);
        h.set("a.b.c", 2);
        EXPECT_TRUE(h.get<int>("a.b.c") == 2);
        EXPECT_TRUE(h.get<Hash>("a").is<Hash>("b") == true);
        EXPECT_TRUE(h.is<int>("a.b.c") == true);
        EXPECT_TRUE(h.has("a.b") == true);
        EXPECT_TRUE(h.has("a.b.c.d") == false);
    }

    {
        Hash h("a[0]", Hash("a", 1), "a[1]", Hash("a", 2));
        EXPECT_TRUE(h.get<int>("a[0].a") == 1);
        EXPECT_TRUE(h.get<int>("a[1].a") == 2);
    }

    {
        Hash h;
        h.set("x[0].y[0]", Hash("a", 4.2, "b", "red", "c", true));
        h.set("x[1].y[0]", Hash("a", 4.0, "b", "green", "c", false));
        EXPECT_TRUE(h.get<bool>("x[0].y[0].c") == true);
        EXPECT_TRUE(h.get<bool>("x[1].y[0].c") == false);
        EXPECT_TRUE(h.get<string>("x[0].y[0].b") == "red");
        EXPECT_TRUE(h.get<string>("x[1].y[0].b") == "green");
    }

    {
        Hash h1("a[0].b[0]", Hash("a", 1));
        Hash h2("a[0].b[0]", Hash("a", 2));

        h1.set("a[0]", h2);
        EXPECT_TRUE(h1.get<int>("a[0].a[0].b[0].a") == 2);
        h1.set("a", h2);
        EXPECT_TRUE(h1.get<int>("a.a[0].b[0].a") == 2);
    }

    {
        std::string s;
        Hash h("a", "1");
        h.get("a", s);
        EXPECT_TRUE(s == "1");
        h.get<string>("a") = "2";
        h.get("a", s);
        EXPECT_TRUE(s == "2");
    }

    {
        Hash h;
        bool a = true;
        h.set<int>("a", a);
        EXPECT_TRUE(h.getType("a") == Types::INT32);
        EXPECT_TRUE(h.is<int>("a") == true);
    }

    {
        // test that correct exceptions  are thrown
        Hash h("a", 77, "b[1].c", 88);
        // no exceptions:
        EXPECT_NO_THROW(h.get<int>("a"));
        EXPECT_NO_THROW(h.get<Hash>("b[0]"));
        EXPECT_NO_THROW(h.get<Hash>("b[1]"));
        EXPECT_NO_THROW(h.get<int>("b[1].c"));

        // non-existing "normal" path
        EXPECT_THROW(h.get<int>("c"), karabo::data::ParameterException);

        // non-existing index of vector that is last item
        EXPECT_TRUE(h.get<vector<Hash>>("b").size() == 2);
        bool caught2 = false;
        try {
            h.get<Hash>("b[2]");
        } catch (karabo::data::ParameterException const& e) {
            caught2 = true;
        }
        EXPECT_TRUE(caught2);

        // item under non-existing index of vector
        bool caught3 = false;
        try {
            h.get<int>("b[2].c");
        } catch (karabo::data::ParameterException const& e) {
            caught3 = true;
        }
        EXPECT_TRUE(caught3);
    }

    {
        // Checks implicit conversions between signed and unsigned integers.
        Hash h("uint32Prop", 30450u);
        EXPECT_TRUE(h.getType("uint32Prop") == Types::UINT32);
        EXPECT_TRUE(h.get<unsigned int>("uint32Prop") == 30450u);
        EXPECT_NO_THROW(h.set("uint32Prop", -1));
        // After the previous set, the node type becomes Types::INT32 and an
        // attempt to get it as Types::UINT32 will fail.
        EXPECT_THROW(h.get<unsigned int>("uint32Prop"), karabo::data::CastException);
        // Hash::getAs, on the other hand, will do the implicit conversion.
        EXPECT_TRUE(h.getAs<unsigned int>("uint32Prop") == UINT32_MAX);
    }

    {
        Hash h;
        h.set("c1"sv, "char A"sv);
        EXPECT_TRUE(h.get<std::string>("c1"s) == "char A"s);
        h.set("c2"sv, L"wchar_t ∀"sv);
        EXPECT_TRUE(h.get<std::wstring>("c2"s) == L"wchar_t ∀"s);
        h.set("c3"sv, u8"char8_t ∆"sv);
        EXPECT_TRUE(h.get<std::u8string>("c3"s) == u8"char8_t ∆"s);
        h.set("c4"sv, u"char16_t ∇"sv);
        EXPECT_TRUE(h.get<std::u16string>("c4"s) == u"char16_t ∇"s);
        h.set("c5"sv, U"char32_t ∃"sv);
        EXPECT_TRUE(h.get<std::u32string>("c5"s) == U"char32_t ∃"s);
        h.set("e1", "Tschüß"sv);
        EXPECT_TRUE(h.get<std::string>("e1"s) == "Tschüß"s);
        h.set("e2", L"Moin, Moin"sv);
        EXPECT_TRUE(h.get<std::wstring>("e2"s) == L"Moin, Moin"s);
        h.set("e3", u8"Привет"sv);
        EXPECT_TRUE(h.get<std::u8string>("e3"s) == u8"Привет"s);
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


TEST(TestHash, testSetMoveSemantics) {
    TraceCopies::reset(); // Ensure that nothing yet - e.g. when other test that ran before failed
    {
        // test Hash::set of normal non-const object
        TraceCopies ta(2);
        Hash h;
        // Normal set
        h.set("ta", ta);
        EXPECT_EQ(1, TraceCopies::countCopyConstr); // copied into Hash
        EXPECT_EQ(2, h.get<TraceCopies>("ta").value);
        // Normal set to the now existing node
        ta.value = 4;
        h.set("ta", ta);
        EXPECT_EQ(2, TraceCopies::countCopyConstr); // ta copied again into Hash
        EXPECT_EQ(4, h.get<TraceCopies>("ta").value);

        // 'moving' set
        h.set("tb", std::move(ta));
        EXPECT_EQ(2, TraceCopies::countCopyConstr); // unchanged
        EXPECT_EQ(4, h.get<TraceCopies>("tb").value);
        // 'moving' set to the now existing node
        ta.value = 8; // get back into a defined state after object "behind" was moved away from 'ta'
        h.set("tb", std::move(ta));
        EXPECT_EQ(2, TraceCopies::countCopyConstr); // again unchanged
        EXPECT_EQ(8, h.get<TraceCopies>("tb").value);

        // set of const
        const TraceCopies tc(3);
        h.set("tc", tc);
        EXPECT_EQ(3, TraceCopies::countCopyConstr); // copied...
        EXPECT_EQ(3, h.get<TraceCopies>("tc").value);
        // set of const to the now existing node
        h.get<TraceCopies>("tc").value = 42;
        EXPECT_EQ(42, h.get<TraceCopies>("tc").value);
        h.set("tc", tc);
        EXPECT_EQ(4, TraceCopies::countCopyConstr);
        EXPECT_EQ(3, h.get<TraceCopies>("tc").value);

        TraceCopies::reset(); // Start next round from zero
    }

    {
        // test set of Hash
        TraceCopies ta(11);
        Hash h;
        Hash hInner;
        hInner.set("ta", ta);
        EXPECT_EQ(1, TraceCopies::countCopyConstr);
        // We set a non-const Hash: It gets copied, so the contained TraceCopies does.
        h.set("h", hInner);
        EXPECT_EQ(2, TraceCopies::countCopyConstr);
        EXPECT_EQ(11, h.get<TraceCopies>("h.ta").value);
        // same again to now existing node
        h.get<TraceCopies>("h.ta").value = 22;
        EXPECT_EQ(22, h.get<TraceCopies>("h.ta").value);
        h.set("h", hInner);
        EXPECT_EQ(3, TraceCopies::countCopyConstr);
        EXPECT_EQ(11, h.get<TraceCopies>("h.ta").value);

        // We move-set a Hash: It gets empty and - since content is just moved - no copy/assignment of TraceCopies
        h.set("h2", std::move(hInner));
        EXPECT_TRUE(hInner.empty());
        EXPECT_EQ(11, h.get<TraceCopies>("h2.ta").value);
        // same again to now existing node
        hInner.set("ta", TraceCopies(17));
        h.set("h2", std::move(hInner));
        EXPECT_TRUE(hInner.empty());
        EXPECT_EQ(17, h.get<TraceCopies>("h2.ta").value);

        // We set a const Hash: As for the non-const, it gets copied, so the contained TraceCopies does.
        const Hash hInner2("ta2", ta);
        h.set("h3", hInner2);
        EXPECT_EQ(5, TraceCopies::countCopyConstr);
        // same again to now existing node
        h.get<TraceCopies>("h3.ta2").value = 22;
        EXPECT_EQ(22, h.get<TraceCopies>("h3.ta2").value);
        h.set("h3", hInner2);
        EXPECT_EQ(6, TraceCopies::countCopyConstr); // another copy
        EXPECT_EQ(11, h.get<TraceCopies>("h3.ta2").value);

        TraceCopies::reset();
    }

    {
        // test set of Hash, but now to path with index
        // same test as above, extended to set also to non-existing index
        TraceCopies ta(11);
        Hash h;
        Hash hInner;
        hInner.set("ta", ta);
        EXPECT_EQ(1, TraceCopies::countCopyConstr);
        // We set a non-const Hash: It gets copied, so the contained TraceCopies does.
        h.set("h[0]", hInner);
        EXPECT_EQ(2, TraceCopies::countCopyConstr);
        EXPECT_EQ(11, h.get<TraceCopies>("h[0].ta").value);
        // same again to now existing node
        h.get<TraceCopies>("h[0].ta").value = 22;
        EXPECT_EQ(22, h.get<TraceCopies>("h[0].ta").value);
        h.set("h[0]", hInner);
        EXPECT_EQ(3, TraceCopies::countCopyConstr);
        EXPECT_EQ(11, h.get<TraceCopies>("h[0].ta").value);
        // and now to non-existing index
        h.set("h[1]", hInner);
        EXPECT_EQ(4, TraceCopies::countCopyConstr); // 5 without noexcept in Hash::Hash(Hash&&) etc.!
        EXPECT_EQ(11, h.get<TraceCopies>("h[1].ta").value);

        // We move-set a Hash: It gets empty and - since content is just moved - no copy/assignment of TraceCopies
        h.set("h2[0]", std::move(hInner));
        EXPECT_TRUE(hInner.empty());
        EXPECT_EQ(11, h.get<TraceCopies>("h2[0].ta").value);
        // same again to now existing node
        hInner.set("ta", TraceCopies(18));
        h.set("h2[0]", std::move(hInner));
        EXPECT_TRUE(hInner.empty());
        EXPECT_EQ(18, h.get<TraceCopies>("h2[0].ta").value);
        // now to not yet existing index
        hInner.set("ta", TraceCopies(19));
        h.set("h2[1]", std::move(hInner));
        EXPECT_TRUE(hInner.empty());
        EXPECT_EQ(19, h.get<TraceCopies>("h2[1].ta").value);

        // We set a const Hash: As for the non-const, it gets copied, so the contained TraceCopies does.
        const Hash hInner2("ta2", ta);
        EXPECT_EQ(5, TraceCopies::countCopyConstr);
        h.set("h3[0]", hInner2);
        EXPECT_EQ(6, TraceCopies::countCopyConstr);
        // same again to now existing node
        h.get<TraceCopies>("h3[0].ta2").value = 22;
        EXPECT_EQ(22, h.get<TraceCopies>("h3[0].ta2").value);
        h.set("h3[0]", hInner2);
        EXPECT_EQ(7, TraceCopies::countCopyConstr);
        EXPECT_EQ(11, h.get<TraceCopies>("h3[0].ta2").value);
        // same now to non-existing index
        h.set("h3[1]", hInner2);
        EXPECT_EQ(8, TraceCopies::countCopyConstr);
        EXPECT_EQ(11, h.get<TraceCopies>("h3[1].ta2").value);

        TraceCopies::reset();
    }

    {
        // test Hash::set of Hash derived object like NDArray
        TraceCopiesHash ta(TraceCopies(2));
        EXPECT_EQ(1, TraceCopies::countCopyConstr); // TraceCopiesHash ctr. takes it by rerefence, so copies
        Hash h;
        // Normal set
        h.set("ta", ta);
        EXPECT_EQ(2, h.get<TraceCopiesHash>("ta").getValue().value);
        EXPECT_EQ(2, TraceCopies::countCopyConstr);
        // Normal set to the now existing node
        ta.setValue(4); // set inner value, no construction
        h.set("ta", ta);
        EXPECT_EQ(3, TraceCopies::countCopyConstr);
        EXPECT_EQ(4, h.get<TraceCopiesHash>("ta").getValue().value);

        // 'moving' set - since the TraceCopiesHash object is moved (as a Hash!),
        //                this leaves no trace, so we cannot really test :-(
        h.set("tb", std::move(ta));
        // Moving ta indeed leaves no trace of TraceCopies construction,
        // but to leave 'ta' in a valid state (with existing key "v"), a default constructed TraceCopiesHash
        // is assigned to it and the default construction move-assigns the TraceCopies object inside.
        EXPECT_EQ(4, h.get<TraceCopiesHash>("tb").getValue().value);
        EXPECT_NO_THROW(ta.getValue()); // Ensure that 'ta' is in a valid state after moving from it.
        ta.setValue(42);
        // 'moving' set to the now existing node
        h.set("tb", std::move(ta));
        // Same counting as above: one move construction expected
        EXPECT_EQ(3, TraceCopies::countCopyConstr);
        EXPECT_EQ(42, h.get<TraceCopiesHash>("tb").getValue().value) << toString(h);

        // set of const
        const TraceCopiesHash tc(TraceCopies(3));
        EXPECT_EQ(4, TraceCopies::countCopyConstr);
        h.set("tc", tc);
        EXPECT_EQ(5, TraceCopies::countCopyConstr);
        EXPECT_EQ(3, h.get<TraceCopiesHash>("tc").getValue().value);
        // set of const to the now existing node
        h.get<TraceCopiesHash>("tc").setValue(-42);
        EXPECT_EQ(-42, h.get<TraceCopiesHash>("tc").getValue().value);
        h.set("tc", tc);
        EXPECT_EQ(6, TraceCopies::countCopyConstr);
        EXPECT_EQ(3, h.get<TraceCopiesHash>("tc").getValue().value);

        TraceCopies::reset();
    }

    {
        // Test Hash::set(path, std::any)
        Hash h;
        std::any a(TraceCopies(4));
        h.set("a", a);
        EXPECT_EQ(1, TraceCopies::countCopyConstr); // a and thus its TraceCopies got copied
        EXPECT_EQ(4, h.get<TraceCopies>("a").value);

        const std::any& a2 = a;
        h.set("a2", a2);
        EXPECT_EQ(2, TraceCopies::countCopyConstr); // a and thus its TraceCopies get copied
        EXPECT_EQ(4, h.get<TraceCopies>("a2").value);

        h.set("a3", std::move(a));
        EXPECT_EQ(2, TraceCopies::countCopyConstr); // copied
        EXPECT_EQ(4, h.get<TraceCopies>("a3").value);

        TraceCopies::reset();
    }

    // The next tests are not fitting well into testSetMoveSemantics() since they have nothing
    // to do with move semantics - but with the special overloads of Element::setValue for 'char*' etc.,
    // so they are closely related.
    {
        // test Hash::set of various strings,
        Hash h;
        h.set("const_char_pointer", "a");
        EXPECT_STREQ("a", h.get<std::string>("const_char_pointer").c_str());

        char cText[] = "a2and3";
        h.set("char_array", cText);
        EXPECT_STREQ("a2and3", h.get<std::string>("char_array").c_str());

        char* cPtr = cText;
        h.set("char_ptr", cPtr);
        EXPECT_STREQ("a2and3", h.get<std::string>("char_ptr").c_str());

        h.set("tmp_string", std::string("b"));
        EXPECT_STREQ("b", h.get<std::string>("tmp_string").c_str());

        const std::string b1("b1");
        h.set("const_string", b1);
        EXPECT_STREQ("b1", h.get<std::string>("const_string").c_str());

        std::string b2("b2");
        h.set("string", b2);
        EXPECT_STREQ("b2", h.get<std::string>("string").c_str());
    }

    {
        // test wide characters
        // Shouldn't be used much in Karabo since there is no serialisation support,
        // but there is treament in Element::setValue to convert to wstring
        Hash h;
        h.set("const_wchart_pointer", L"a");
        // EXPECT_STREQ does not support std::wstring... (TODO: Check this statement!)
        EXPECT_TRUE(std::wstring(L"a") == h.get<std::wstring>("const_wchart_pointer"));

        wchar_t cText[] = L"a2and3";
        h.set("wchart_array", cText);
        EXPECT_TRUE(std::wstring(L"a2and3") == h.get<std::wstring>("wchart_array"));

        wchar_t* cPtr = cText;
        h.set("wchart_ptr", cPtr);
        EXPECT_TRUE(std::wstring(L"a2and3") == h.get<std::wstring>("wchart_ptr"));
    }

    // Some final checks
    {
        // Ensure that setting still works when type is not deduced, but explicitely specified
        // (as was allowed before introducing move semantics).
        Hash h;

        h.set<int>("int", 1);
        EXPECT_EQ(1, h.get<int>("int"));

        h.set<Hash>("hash", Hash("a", "b"));
        EXPECT_TRUE(h.get<Hash>("hash").fullyEquals(Hash("a", "b"))) << toString(h);

        h.set<NDArray>("ndarray", NDArray(Dims({20}), 5));
        EXPECT_EQ(20ul, h.get<NDArray>("ndarray").size()) << toString(h);

        h.set<TraceCopies>("trace", TraceCopies(77));
        EXPECT_EQ(77, h.get<TraceCopies>("trace").value);

        // Test also Element::setValue<typename>(..) directly
        h.getNode("int").setValue<int>(42);
        EXPECT_EQ(42, h.get<int>("int"));

        h.getNode("hash").setValue<Hash>(Hash("b", "c"));
        EXPECT_TRUE(h.get<Hash>("hash").fullyEquals(Hash("b", "c"))) << toString(h);

        h.getNode("ndarray").setValue<NDArray>(NDArray(Dims({10}), 6));
        EXPECT_EQ(10ul, h.get<NDArray>("ndarray").size()) << toString(h);

        h.getNode("trace").setValue<TraceCopies>(TraceCopies(88));
        EXPECT_EQ(88, h.get<TraceCopies>("trace").value);

        TraceCopies::reset();
    }
}


TEST(TestHash, testSetAttributeMoveSemantics) {
    TraceCopies::reset(); // Ensure that nothing yet - e.g. when other test that ran before failed
    {
        // test Hash::setAttribute of normal non-const object
        TraceCopies ta(2);
        Hash h("a", 1);
        // Normal set
        h.setAttribute("a", "ta", ta);
        EXPECT_EQ(1, TraceCopies::countCopyConstr); // copied into Hash
        EXPECT_EQ(2, h.getAttribute<TraceCopies>("a", "ta").value);
        // Normal set to the now existing node
        ta.value = 4;
        h.setAttribute("a", "ta", ta);
        EXPECT_EQ(2, TraceCopies::countCopyConstr); // again copied into Hash
        EXPECT_EQ(4, h.getAttribute<TraceCopies>("a", "ta").value);

        // 'moving' set
        h.setAttribute("a", "tb", std::move(ta));
        EXPECT_EQ(2, TraceCopies::countCopyConstr); // unchanged
        EXPECT_EQ(4, h.getAttribute<TraceCopies>("a", "tb").value);
        EXPECT_EQ(-1, ta.value); // the moved-from object gets -1 assigned to value in the move constructor
        // 'moving' set to the now existing node
        ta.value = 8; // in general, a moved-from object is in a valid state, but we do not necessarily know which...
        h.setAttribute("a", "tb", std::move(ta));
        EXPECT_EQ(2, TraceCopies::countCopyConstr); // again unchanged
        EXPECT_EQ(8, h.getAttribute<TraceCopies>("a", "tb").value);
        EXPECT_EQ(-1, ta.value); // as before: the moved-from object gets -1 assigned to value in the move constructor
        ta.value = 9;
        h.setAttribute<TraceCopies>("a", "tb", std::move(ta));
        EXPECT_EQ(2, TraceCopies::countCopyConstr); // again unchanged
        EXPECT_EQ(9, h.getAttribute<TraceCopies>("a", "tb").value);

        // set of const
        const TraceCopies tc(3);
        h.setAttribute("a", "tc", tc);
        EXPECT_EQ(3, TraceCopies::countCopyConstr); // copied...
        EXPECT_EQ(3, h.getAttribute<TraceCopies>("a", "tc").value);
        // set of const to the now existing node
        h.getAttribute<TraceCopies>("a", "tc").value = 42;
        EXPECT_EQ(42, h.getAttribute<TraceCopies>("a", "tc").value);
        h.setAttribute("a", "tc", tc);
        EXPECT_EQ(4, TraceCopies::countCopyConstr);
        EXPECT_EQ(3, h.getAttribute<TraceCopies>("a", "tc").value);
        h.setAttribute<TraceCopies>("a", "tc", tc);
        EXPECT_EQ(5, TraceCopies::countCopyConstr);
        EXPECT_EQ(3, h.getAttribute<TraceCopies>("a", "tc").value);

        TraceCopies::reset(); // Start next round from zero
    }

    {
        // Test Hash::setAttribute(path, attr, std::any)
        Hash h("a", 2);
        std::any a(TraceCopies(4));
        h.setAttribute("a", "attr", a);
        EXPECT_EQ(1, TraceCopies::countCopyConstr); // a and thus its TraceCopies got copied
        EXPECT_EQ(4, h.getAttribute<TraceCopies>("a", "attr").value);

        const std::any& a2 = a;
        h.setAttribute("a", "attr2", a2);
        EXPECT_EQ(2, TraceCopies::countCopyConstr); // a and thus its TraceCopies get copied
        EXPECT_EQ(4, h.getAttribute<TraceCopies>("a", "attr2").value);

        h.setAttribute("a", "attr3", std::move(a));
        EXPECT_EQ(2, TraceCopies::countCopyConstr);
        EXPECT_EQ(4, h.getAttribute<TraceCopies>("a", "attr3").value);

        TraceCopies::reset();
    }
    // test bulk setting of attributes
    {
        Hash::Attributes attrs;
        attrs.set("attr", TraceCopies(7));
        EXPECT_EQ(0, TraceCopies::countCopyConstr);
        Hash h("a", 1, "b", 2);

        // copy case
        h.setAttributes("a", attrs);
        EXPECT_EQ(7, h.getAttribute<TraceCopies>("a", "attr").value);
        EXPECT_EQ(1ul, h.getAttributes("a").size());
        EXPECT_EQ(1, TraceCopies::countCopyConstr);

        // move case
        h.setAttributes("b", std::move(attrs));
        EXPECT_EQ(7, h.getAttribute<TraceCopies>("b", "attr").value);
        EXPECT_EQ(1ul, h.getAttributes("b").size());
        // Neither moved nor copied since entire 'attrs' now moved inside the Hash
        EXPECT_EQ(1, TraceCopies::countCopyConstr);
        EXPECT_TRUE(attrs.empty()); // since entirely 'moved away'

        TraceCopies::reset();
    }
    // test setting of various strings as also at the end of testSetMoveSemantics
    {
        // test Hash::set of various strings,
        Hash h("a", 1);
        h.setAttribute("a", "const_char_pointer", "a");
        EXPECT_STREQ("a", h.getAttribute<std::string>("a", "const_char_pointer").c_str());

        char cText[] = "a2and3";
        h.setAttribute("a", "char_array", cText);
        EXPECT_STREQ("a2and3", h.getAttribute<std::string>("a", "char_array").c_str());

        char* cPtr = cText;
        h.setAttribute("a", "char_ptr", cPtr);
        EXPECT_STREQ("a2and3", h.getAttribute<std::string>("a", "char_ptr").c_str());

        h.setAttribute("a", "tmp_string", std::string("b"));
        EXPECT_STREQ("b", h.getAttribute<std::string>("a", "tmp_string").c_str());

        const std::string b1("b1");
        h.setAttribute("a", "const_string", b1);
        EXPECT_STREQ("b1", h.getAttribute<std::string>("a", "const_string").c_str());

        std::string b2("b2");
        h.setAttribute("a", "string", b2);
        EXPECT_STREQ("b2", h.getAttribute<std::string>("a", "string").c_str());
    }
}

TEST(TestHash, testConstructorMoveSemantics) {
    TraceCopies::reset(); // Clean start

    // First test setting single value as specially treated
    {
        // test ctr with normal non-const object
        TraceCopies ta(2);
        Hash h("ta", ta);
        EXPECT_EQ(1, TraceCopies::countCopyConstr); // copied into Hash
        EXPECT_EQ(2, h.get<TraceCopies>("ta").value);
    }
    {
        // 'moving' set
        Hash h("tb", TraceCopies(4));
        EXPECT_EQ(1, TraceCopies::countCopyConstr); // unchanged
        // EXPECT_EQ(1, TraceCopies::countMoveConstr); // since now it is moved
        EXPECT_EQ(4, h.get<TraceCopies>("tb").value);
    }
    {
        // set of const
        const TraceCopies tc(3);
        Hash h("tc", tc);
        EXPECT_EQ(2, TraceCopies::countCopyConstr); // copied...
        EXPECT_EQ(3, h.get<TraceCopies>("tc").value);
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

        EXPECT_EQ(8, TraceCopies::countCopyConstr); // ta, tb, 4x when ha and hb are copied into h and 2x
                                                    // when ta/tb are copied into hc

        EXPECT_EQ(1, h.get<TraceCopies>("ta").value);
        EXPECT_EQ(2, h.get<TraceCopies>("tb").value);
        EXPECT_EQ(3, h.get<TraceCopies>("tc").value);
        EXPECT_EQ(1, h.get<TraceCopies>("ha.a").value);
        EXPECT_EQ(2, h.get<TraceCopies>("ha.b").value);
        EXPECT_EQ(1, h.get<TraceCopies>("hb.a").value);
        EXPECT_EQ(2, h.get<TraceCopies>("hb.b").value);
        EXPECT_EQ(1, h.get<TraceCopies>("hc.a").value);
        EXPECT_EQ(2, h.get<TraceCopies>("hc.b").value);

        // Verify insertion order
        // (was wrong in first attempt to have move semantics in Hash constructor)
        Hash::const_iterator it = h.begin();
        EXPECT_STREQ("int", it->getKey().c_str());
        EXPECT_STREQ("ta", (++it)->getKey().c_str());
        EXPECT_STREQ("tb", (++it)->getKey().c_str());
        EXPECT_STREQ("tc", (++it)->getKey().c_str());
        EXPECT_STREQ("ha", (++it)->getKey().c_str());
        EXPECT_STREQ("hb", (++it)->getKey().c_str());
        EXPECT_STREQ("hc", (++it)->getKey().c_str());
        EXPECT_TRUE(++it == h.end());
    }

    TraceCopies::reset(); // Start next round from zero
}

TEST(TestHash, testGetAs) {
    {
        Hash h("a", true);
        EXPECT_TRUE(h.getAs<string>("a") == "1");
        EXPECT_TRUE(h.getAs<int>("a") == 1);
        EXPECT_NEAR(1.0, h.getAs<double>("a"), 0.00001);
        EXPECT_TRUE(h.getAs<char>("a") == '1');
    }

    {
        Hash h("a", true);
        h.setAttribute("a", "a", true);
        EXPECT_TRUE(h.getAttributeAs<string>("a", "a") == "1");
        EXPECT_TRUE(h.getAttributeAs<int>("a", "a") == 1);
        EXPECT_NEAR(1.0, h.getAttributeAs<double>("a", "a"), 0.00001);
        EXPECT_TRUE(h.getAttributeAs<char>("a", "a") == '1');
        std::any& any = h.getAttributeAsAny("a", "a");
        EXPECT_TRUE(std::any_cast<bool>(any) == true);
        h.setAttribute("a", "b", 12);
        h.setAttribute("a", "c", 1.23);
        Hash::Attributes attrs = h.getAttributes("a");
        Hash g("Z.a.b.c", "Value");
        g.setAttributes("Z.a.b.c", attrs);
        EXPECT_TRUE(g.getAttributeAs<string>("Z.a.b.c", "a") == "1");
        EXPECT_TRUE(g.getAttributeAs<int>("Z.a.b.c", "a") == 1);
        EXPECT_NEAR(1.0, g.getAttributeAs<double>("Z.a.b.c", "a"), 0.00001);
        h.set("a.b", "cardinal");
        h.setAttribute("a.b", "Q", 1.8e-06);
        EXPECT_TRUE(h.getAttribute<double>("a.b", "Q") == 1.8e-06);
        EXPECT_TRUE(h.getAttributeAs<int>("a.b", "Q") == 0);
    }

    {
        Hash h("a", std::vector<bool>(4, false));
        EXPECT_TRUE(h.getAs<string>("a") == "0,0,0,0");
        int tmp = h.getAs<int, std::vector>("a")[3];
        EXPECT_TRUE(tmp == 0);
    }
    {
        Hash h("a", char('R'));
        EXPECT_TRUE(h.getAs<string>("a") == "R");
    }
    {
        // Assumes vector to contain printable (ASCII) characters
        Hash h("a", std::vector<unsigned char>(3, '4'));
        EXPECT_STREQ("52,52,52", h.getAs<string>("a").c_str());
    }
    {
        // Assumes vector to contain binary data and does a base64 encode
        Hash h("a", std::vector<char>(3, '4'));
        EXPECT_TRUE(h.getAs<string>("a") == "NDQ0");
    }
    {
        // Assumes vector to contain printable (ASCII) characters
        Hash h("a", std::vector<signed char>(3, '4'));
        EXPECT_TRUE(h.getAs<string>("a") == "52,52,52");
    }
    {
        Hash h("a", static_cast<unsigned char>('R'));
        EXPECT_TRUE(h.getAs<string>("a") == "82");
    }
    {
        Hash h("a", static_cast<signed char>('R'));
        EXPECT_TRUE(h.getAs<string>("a") == "82");
    }
    {
        Hash h("a", std::vector<signed char>(4, '2'));
        EXPECT_TRUE(h.getAs<string>("a") == "50,50,50,50");
    }
    {
        Hash h("a", short(126));
        EXPECT_TRUE(h.getAs<string>("a") == "126");
    }
    {
        Hash h("a", std::vector<short>(4, 13));
        EXPECT_TRUE(h.getAs<string>("a") == "13,13,13,13");
    }
    {
        Hash h("a", int(-42));
        EXPECT_TRUE(h.getAs<string>("a") == "-42");
    }
    {
        Hash h("a", std::vector<int>(1, -42));
        EXPECT_TRUE(h.getAs<string>("a") == "-42");
    }
    {
        Hash h("a", static_cast<unsigned int>(42));
        EXPECT_TRUE(h.getAs<string>("a") == "42");
    }
    {
        Hash h("a", std::vector<unsigned int>());
        EXPECT_TRUE(h.getAs<string>("a") == "");
    }
    {
        Hash h("a", static_cast<long long>(-2147483647));
        EXPECT_TRUE(h.getAs<string>("a") == "-2147483647");
    }
    {
        Hash h("a", static_cast<unsigned long long>(0));
        EXPECT_TRUE(h.getAs<string>("a") == "0");
    }
    {
        Hash h("a", static_cast<float>(0.1234567));
        EXPECT_TRUE(h.getAs<string>("a") == "0.1234567");
    }
    {
        Hash h("a", 0.123456789123456);
        EXPECT_TRUE(h.getAs<string>("a") == "0.123456789123456");
    }
    {
        Hash h("a", std::complex<float>(1.2, 0.5));
        EXPECT_TRUE(h.getAs<string>("a") == "(1.2,0.5)");
    }
    {
        Hash h("a", std::complex<double>(1.2, 0.5));
        EXPECT_TRUE(h.getAs<string>("a") == "(1.2,0.5)");
    }
    {
        // getAs as a container
        Hash h("a", std::vector<unsigned short>({2, 3, 5, 7, 11}));
        const auto result = h.getAs<std::string, std::vector>("a");
        EXPECT_TRUE(result == std::vector<std::string>({"2", "3", "5", "7", "11"}))
              << "Result is " + karabo::data::toString(result);
    }
    {
        // There is some extra treatment of STRING as source in Element::getValueAs<T>
        Hash h("a", "5");
        EXPECT_EQ(5, h.getAs<int>("a"));
    }
    {
        // There is some extra treatment of STRING as source in Element::getValueAs<CONT<T>>
        Hash h("a", "5,6, 7 ");
        const auto result = h.getAs<int, std::vector>("a");
        EXPECT_TRUE(std::vector<int>({5, 6, 7}) == result) << "Result is: " + karabo::data::toString(result);
    }
    {
        // There is some extra treatment of empty string as source for containers
        Hash h("a", std::string());
        const auto result = h.getAs<std::string, std::vector>("a");
        // Empty string becomes empty vector of strings and not vector with a single empty string
        EXPECT_EQ(0ul, result.size());
    }
}


TEST(TestHash, testFind) {
    // First test non-const version of Hash::find(..).
    {
        Hash h("a.b.c1.d", 1, "b[2].c.d", "some");
        // Check existing node and its value.
        boost::optional<Hash::Node&> node = h.find("a.b.c1.d");
        EXPECT_TRUE(!node == false);
        EXPECT_TRUE(node->getValue<int>() == 1);

        // Test that other separator fails
        node = h.find("a.b.c1.d", '/');
        EXPECT_TRUE(!node == true);

        // Check existence of first level node.
        node = h.find("a");
        EXPECT_TRUE(!node == false);

        // Check non-existence of first level node.
        node = h.find("nee");
        EXPECT_TRUE(!node == true);

        // Check non-existence of last level node.
        node = h.find("a.b.c1.f");
        EXPECT_TRUE(!node == true);

        // Check non-existence of middle level node.
        node = h.find("a.b.c2.d");
        EXPECT_TRUE(!node == true);

        // Check existence with index as last but two.
        node = h.find("b[2].c.d");
        EXPECT_TRUE(!node == false);

        // Check existence with index as last but one.
        node = h.find("b[2].c");
        EXPECT_TRUE(!node == false);

        // Index at end is not allowed - would be Hash, not Node.
        node = h.find("b[2]");
        EXPECT_TRUE(!node == true);

        // Same check, but with invalid index.
        node = h.find("b[3]");
        EXPECT_TRUE(!node == true);

        // Check non-existence with invalid index as last but one.
        node = h.find("b[3].c");
        EXPECT_TRUE(!node == true);

        // Check non-existence with invalid index as last but two.
        node = h.find("b[3].c.d");
        EXPECT_TRUE(!node == true);
    }

    // Now test Hash::find(..) const.
    // (Same code as above except adding twice 'const'.)
    {
        const Hash h("a.b.c1.d", 1, "b[2].c.d", "some");
        // Check existing node and its value.
        boost::optional<const Hash::Node&> node = h.find("a.b.c1.d");
        EXPECT_TRUE(!node == false);
        EXPECT_TRUE(node->getValue<int>() == 1);

        // Test that other separator fails
        node = h.find("a.b.c1.d", '/');
        EXPECT_TRUE(!node == true);

        // Check existence of first level node.
        node = h.find("a");
        EXPECT_TRUE(!node == false);

        // Check non-existence of first level node.
        node = h.find("nee");
        EXPECT_TRUE(!node == true);

        // Check non-existence of last level node.
        node = h.find("a.b.c1.f");
        EXPECT_TRUE(!node == true);

        // Check non-existence of middle level node.
        node = h.find("a.b.c2.d");
        EXPECT_TRUE(!node == true);

        // Check existence with index as last but two.
        node = h.find("b[2].c.d");
        EXPECT_TRUE(!node == false);

        // Check existence with index as last but one.
        node = h.find("b[2].c");
        EXPECT_TRUE(!node == false);

        // Index at end is not allowed - would be Hash, not Node.
        node = h.find("b[2]");
        EXPECT_TRUE(!node == true);

        // Same check, but with invalid index.
        node = h.find("b[3]");
        EXPECT_TRUE(!node == true);

        // Check non-existence with invalid index as last but one.
        node = h.find("b[3].c");
        EXPECT_TRUE(!node == true);

        // Check non-existence with invalid index as last but two.
        node = h.find("b[3].c.d");
        EXPECT_TRUE(!node == true);
    }
}


TEST(TestHash, testAttributes) {
    {
        Hash h("a.b.a.b", 42);
        h.setAttribute("a", "attrKey", "1, 2, 3, 4, 5");
        h.setAttribute("a", "attr1", "someValue");

        EXPECT_TRUE(h.getNode("a").getAttributes().is<std::string>("attrKey"sv));
        EXPECT_TRUE(h.getNode("a").getAttributes().is<std::string>("attr1"s));
        EXPECT_TRUE(h.getAttribute<std::string>("a", "attr1") == "someValue");

        h.setAttribute("a", "attr2", 42);
        EXPECT_TRUE(h.getNode("a").getAttributes().is<int>("attr2"sv));
        EXPECT_TRUE(h.getAttribute<std::string>("a", "attr1") == "someValue");
        EXPECT_TRUE(h.getAttribute<int>("a", "attr2") == 42);

        h.setAttribute("a", "attr2", 43);
        EXPECT_TRUE(h.getAttribute<std::string>("a", "attr1") == "someValue");
        EXPECT_TRUE(h.getAttribute<int>("a", "attr2") == 43);

        h.setAttribute("a.b.a.b", "attr1", true);
        EXPECT_TRUE(h.getAttribute<bool>("a.b.a.b", "attr1") == true);

        const Hash::Attributes& attrs = h.getAttributes("a");
        EXPECT_TRUE(attrs.size() == 3);
        EXPECT_TRUE(attrs.get<std::string>("attr1") == "someValue");
        EXPECT_TRUE(attrs.get<int>("attr2") == 43);
        EXPECT_TRUE(attrs.is<int>("attr2"));

        Hash::Attributes::Node node = attrs.getNode("attr2");
        EXPECT_TRUE(node.getType() == Types::INT32);

        EXPECT_TRUE((h.getNode("a").getAttributes().getAs<int, std::vector>("attrKey"sv)[0]) == 1);
        EXPECT_TRUE((attrs.getAs<int, std::vector>("attrKey"sv)[2]) == 3);
    }
    {
        Hash h("a", 1);
        bool b = true;
        h.getNode("a").setAttribute<int>("a", b);
        EXPECT_TRUE(h.getNode("a").getType() == Types::INT32);
    }
    {
        Hash h("a", 442);
        Hash::Attributes& attrs = h.getAttributes("a");
        attrs.set("a1", "char A"sv);
        EXPECT_TRUE(attrs.get<std::string>("a1") == "char A"s);
        attrs.set("a2", L"wchar_t ∀"sv);
        EXPECT_TRUE(attrs.get<std::wstring>("a2"s) == L"wchar_t ∀"s);
        attrs.set("a3", u8"char8_t ∆"sv);
        EXPECT_TRUE(attrs.get<std::u8string>("a3"s) == u8"char8_t ∆"s);
        attrs.set("a4", u"char16_t ∇"sv);
        EXPECT_TRUE(attrs.get<std::u16string>("a4"s) == u"char16_t ∇"s);
        attrs.set("a5", U"char32_t ∃"sv);
        EXPECT_TRUE(attrs.get<std::u32string>("a5"s) == U"char32_t ∃"s);
    }
}


TEST(TestHash, testIteration) {
    Hash h("should", 1, "be", 2, "iterated", 3, "in", 4, "correct", 5, "order", 6);
    Hash::Attributes a("should", 1, "be", 2, "iterated", 3, "in", 4, "correct", 5, "order", 6);

    {
        std::vector<std::string> insertionOrder;
        for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
            insertionOrder.push_back(it->getKey());
        }
        EXPECT_TRUE(insertionOrder[0] == "should");
        EXPECT_TRUE(insertionOrder[1] == "be");
        EXPECT_TRUE(insertionOrder[2] == "iterated");
        EXPECT_TRUE(insertionOrder[3] == "in");
        EXPECT_TRUE(insertionOrder[4] == "correct");
        EXPECT_TRUE(insertionOrder[5] == "order");
    }

    {
        std::vector<std::string> alphaNumericOrder;
        for (Hash::const_map_iterator it = h.mbegin(); it != h.mend(); ++it) {
            alphaNumericOrder.push_back(it->second.getKey());
        }
        EXPECT_TRUE(alphaNumericOrder[0] == "be");
        EXPECT_TRUE(alphaNumericOrder[1] == "correct");
        EXPECT_TRUE(alphaNumericOrder[2] == "in");
        EXPECT_TRUE(alphaNumericOrder[3] == "iterated");
        EXPECT_TRUE(alphaNumericOrder[4] == "order");
        EXPECT_TRUE(alphaNumericOrder[5] == "should");
    }

    h.set("be", "2"); // Has no effect on order

    {
        std::vector<std::string> insertionOrder;
        for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
            insertionOrder.push_back(it->getKey());
        }
        EXPECT_TRUE(insertionOrder[0] == "should");
        EXPECT_TRUE(insertionOrder[1] == "be");
        EXPECT_TRUE(insertionOrder[2] == "iterated");
        EXPECT_TRUE(insertionOrder[3] == "in");
        EXPECT_TRUE(insertionOrder[4] == "correct");
        EXPECT_TRUE(insertionOrder[5] == "order");
    }

    {
        std::vector<std::string> alphaNumericOrder;
        for (Hash::const_map_iterator it = h.mbegin(); it != h.mend(); ++it) {
            alphaNumericOrder.push_back(it->second.getKey());
        }
        EXPECT_TRUE(alphaNumericOrder[0] == "be");
        EXPECT_TRUE(alphaNumericOrder[1] == "correct");
        EXPECT_TRUE(alphaNumericOrder[2] == "in");
        EXPECT_TRUE(alphaNumericOrder[3] == "iterated");
        EXPECT_TRUE(alphaNumericOrder[4] == "order");
        EXPECT_TRUE(alphaNumericOrder[5] == "should");
    }

    h.erase("be");    // Remove
    h.set("be", "2"); // Must be last element in sequence now

    {
        std::vector<std::string> insertionOrder;
        for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
            insertionOrder.push_back(it->getKey());
        }
        EXPECT_TRUE(insertionOrder[0] == "should");
        EXPECT_TRUE(insertionOrder[1] == "iterated");
        EXPECT_TRUE(insertionOrder[2] == "in");
        EXPECT_TRUE(insertionOrder[3] == "correct");
        EXPECT_TRUE(insertionOrder[4] == "order");
        EXPECT_TRUE(insertionOrder[5] == "be");
    }

    {
        std::vector<std::string> alphaNumericOrder;
        for (Hash::const_map_iterator it = h.mbegin(); it != h.mend(); ++it) {
            alphaNumericOrder.push_back(it->second.getKey());
        }
        EXPECT_TRUE(alphaNumericOrder[0] == "be");
        EXPECT_TRUE(alphaNumericOrder[1] == "correct");
        EXPECT_TRUE(alphaNumericOrder[2] == "in");
        EXPECT_TRUE(alphaNumericOrder[3] == "iterated");
        EXPECT_TRUE(alphaNumericOrder[4] == "order");
        EXPECT_TRUE(alphaNumericOrder[5] == "should");
    }
    {
        // erase during map iteration
        Hash h2(h); // local copy
        for (auto it = h2.mbegin(); it != h2.mend();) {
            if (it->first == "be" || it->first == "correct") {
                it = h2.erase(it);
            } else {
                ++it;
            }
        }
        std::vector<std::string> insertionOrder;
        for (auto it = h2.begin(); it != h2.end(); ++it) {
            insertionOrder.push_back(it->getKey());
        }
        EXPECT_TRUE(std::vector<std::string>({"should", "iterated", "in", "order"}) == insertionOrder);
    }

    //  getKeys(...) to ...
    //         "set"
    {
        std::set<std::string> tmp; // create empty set
        h.getKeys(tmp);            // fill set by keys
        std::set<std::string>::const_iterator it = tmp.begin();
        EXPECT_TRUE(*it++ == "be");
        EXPECT_TRUE(*it++ == "correct");
        EXPECT_TRUE(*it++ == "in");
        EXPECT_TRUE(*it++ == "iterated");
        EXPECT_TRUE(*it++ == "order");
        EXPECT_TRUE(*it++ == "should");
    }

    //         "vector"
    {
        std::vector<std::string> tmp; // create empty vector
        h.getKeys(tmp);               // fill vector by keys
        std::vector<std::string>::const_iterator it = tmp.begin();
        EXPECT_TRUE(*it++ == "should");
        EXPECT_TRUE(*it++ == "iterated");
        EXPECT_TRUE(*it++ == "in");
        EXPECT_TRUE(*it++ == "correct");
        EXPECT_TRUE(*it++ == "order");
        EXPECT_TRUE(*it++ == "be");

        EXPECT_TRUE(tmp == h.getKeys());
    }

    //         "list"
    {
        std::list<std::string> tmp; // create empty list
        h.getKeys(tmp);             // fill list by keys
        std::list<std::string>::const_iterator it = tmp.begin();
        EXPECT_TRUE(*it++ == "should");
        EXPECT_TRUE(*it++ == "iterated");
        EXPECT_TRUE(*it++ == "in");
        EXPECT_TRUE(*it++ == "correct");
        EXPECT_TRUE(*it++ == "order");
        EXPECT_TRUE(*it++ == "be");
    }

    //         "deque"
    {
        std::deque<std::string> tmp; // create empty queue
        h.getKeys(tmp);              // fill deque by keys
        std::deque<std::string>::const_iterator it = tmp.begin();
        EXPECT_TRUE(*it++ == "should");
        EXPECT_TRUE(*it++ == "iterated");
        EXPECT_TRUE(*it++ == "in");
        EXPECT_TRUE(*it++ == "correct");
        EXPECT_TRUE(*it++ == "order");
        EXPECT_TRUE(*it++ == "be");
    }
}


TEST(TestHash, testGetPaths) {
    {
        // getPaths for vector
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
        EXPECT_TRUE(paths.size() == 7);

        std::vector<std::string>::const_iterator it = paths.begin();
        EXPECT_TRUE(*it++ == "a");
        EXPECT_TRUE(*it++ == "b.c");
        EXPECT_TRUE(*it++ == "array");
        EXPECT_TRUE(*it++ == "vector.hash.one[0].a.b");
        EXPECT_TRUE(*it++ == "vector.hash.one[1]");
        EXPECT_TRUE(*it++ == "empty.vector.hash");
        EXPECT_TRUE(*it++ == "empty.hash");

        EXPECT_TRUE(paths == h.getPaths());
    }

    {
        // getDeepPaths for vector
        Hash h;
        h.set("a", 1);
        h.set("b.c", "foo");
        h.set("b.array", NDArray(Dims(10, 10)));
        h.set("emptyhash", Hash());
        std::vector<std::string> paths;
        h.getDeepPaths(paths);
        EXPECT_EQ(7ul, paths.size()) << toString(paths) << "\n" << toString(h);
        std::vector<std::string>::const_iterator it = paths.begin();
        EXPECT_STREQ((*it++).c_str(), "a");
        EXPECT_STREQ((*it++).c_str(), "b.c");
        EXPECT_STREQ((*it++).c_str(), "b.array.data");
        EXPECT_STREQ((*it++).c_str(), "b.array.type");
        EXPECT_STREQ((*it++).c_str(), "b.array.shape");
        EXPECT_STREQ((*it++).c_str(), "b.array.isBigEndian");
        EXPECT_STREQ((*it++).c_str(), "emptyhash");

        EXPECT_TRUE(paths == h.getDeepPaths());
    }
}


TEST(TestHash, testMerge) {
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

    EXPECT_TRUE(similar(h1, h1b)) << "Replace or merge attributes influenced resulting paths";
    EXPECT_TRUE(similar(h1, h1d)) << "merge and += don't do the same";

    EXPECT_TRUE(h1.has("a"));
    EXPECT_TRUE(h1.get<int>("a") == 21); // new value
    // Attribute kept, but value overwritten:
    EXPECT_TRUE(h1.hasAttribute("a", "attrKey")) << "Attribute on node not kept";
    EXPECT_STREQ("Really just a number", h1.getAttribute<std::string>("a", "attrKey").c_str())
          << "Attribute not overwritten";
    EXPECT_EQ(1ul, h1.getAttributes("a").size()) << "Attribute added out of nothing";

    EXPECT_TRUE(h1b.hasAttribute("a", "attrKey")) << "Attribute on node not kept (MERGE)";
    EXPECT_STREQ("Really just a number", h1b.getAttribute<std::string>("a", "attrKey").c_str())
          << "Attribute not overwritten (MERGE)";
    EXPECT_EQ(1ul, h1b.getAttributes("a").size()) << "Attribute added out of nothing (MERGE)";

    EXPECT_TRUE(h1.has("b"));
    EXPECT_TRUE(h1.is<Hash>("b")); // switch to new type...
    EXPECT_TRUE(h1.has("b.c"));    // ...and as Hash can hold a child

    // Attributes overwritten by nothing or kept
    EXPECT_EQ(0ul, h1.getAttributes("c.b").size()) << "Attributes on node kept";

    EXPECT_EQ(1ul, h1b.getAttributes("c.b").size()) << "Number of attributes on node changed (MERGE)";
    EXPECT_TRUE(h1b.hasAttribute("c.b", "attrKey2")) << "Attribute on node not kept (MERGE)";
    EXPECT_EQ(3, h1b.getAttribute<int>("c.b", "attrKey2")) << "Attribute on node changed (MERGE)";

    EXPECT_TRUE(!h1.has("c.b.d"));
    EXPECT_TRUE(h1.has("c.b[0]"));
    EXPECT_TRUE(h1.has("c.b[1]")) << toString(h1);
    EXPECT_TRUE(!h1.has("c.b[2]"));
    EXPECT_TRUE(h1.get<int>("c.b[1].d") == 24);
    EXPECT_TRUE(h1.has("c.c[0].d"));
    EXPECT_TRUE(h1.has("c.c[1].a.b.c"));
    EXPECT_TRUE(h1.has("d.e"));
    EXPECT_TRUE(h1.has("e"));
    EXPECT_TRUE(h1.has("g.h.i"));
    EXPECT_TRUE(h1.has("g.h.j"));
    EXPECT_TRUE(h1.has("h.i"));
    EXPECT_TRUE(h1.has("h.j"));
    EXPECT_TRUE(h1.has(".i[1].j"));
    EXPECT_TRUE(h1.has(".i[2].k.l"));
    EXPECT_TRUE(h1.has(".i[3]"));
    EXPECT_TRUE(h1.has("j.k"));
    EXPECT_TRUE(h1.has("array")) << toString(h1);
    EXPECT_TRUE(h1.has("array.data"));
    EXPECT_TRUE(h1.has("array2"));
    EXPECT_TRUE(h1.has("array2.data"));

    EXPECT_EQ(25ull, h1.get<NDArray>("array2").getShape().size()) << "Array size changed through merge";

    // Just add attributes with leaf (identical for REPLACE or MERGE)
    EXPECT_EQ(2ul, h1.getAttributes("e").size()) << "Not all attributes on leaf added";
    EXPECT_EQ(-1, h1.getAttribute<int>("e", "attrKey4")) << "Int attribute value incorrect";
    EXPECT_FLOAT_EQ(-11.f, h1.getAttribute<float>("e", "attrKey5")) << "Float attribute value incorrect";
    EXPECT_EQ(2ul, h1b.getAttributes("e").size()) << "Not all attributes on leaf added (MERGE)";
    EXPECT_EQ(-1, h1b.getAttribute<int>("e", "attrKey4")) << "Int attribute value incorrect (MERGE)";
    EXPECT_FLOAT_EQ(-11.f, h1b.getAttribute<float>("e", "attrKey5")) << "Float attribute value incorrect (MERGE)";
    // Just add attributes for new Hash/vector<Hash> (identical for REPLACE or MERGE)
    EXPECT_EQ(1ul, h1.getAttributes(".i").size()) << "Not all attributes on vector<Hash> added";
    EXPECT_EQ(123ll, h1.getAttribute<long long>(".i", "attrKey8")) << "Int64 attributes on vector<Hash> wrong";
    EXPECT_EQ(1ul, h1.getAttributes("j").size()) << "Not all attributes on Hash added";
    EXPECT_DOUBLE_EQ(12.3, h1.getAttribute<double>("j", "attrKey9")) << "Double attributes on Hash wrong";

    EXPECT_EQ(1ul, h1b.getAttributes(".i").size()) << "Not all attributes on vector<Hash> added (MERGE)";
    EXPECT_EQ(123ll, h1b.getAttribute<long long>(".i", "attrKey8"))
          << "Int64 attributes on vector<Hash> wrong  (MERGE)";
    EXPECT_EQ(1ul, h1b.getAttributes("j").size()) << "Not all attributes on Hash added (MERGE)";
    EXPECT_DOUBLE_EQ(12.3, h1b.getAttribute<double>("j", "attrKey9")) << "Double attributes on Hash wrong (MERGE)";

    EXPECT_TRUE(h1b.hasAttribute("c.b", "attrKey2")) << "Attribute on node not kept (MERGE)";


    EXPECT_TRUE(h1.has("f"));
    EXPECT_TRUE(h1.has("f.g")); // merging does not overwrite h1["f"] with empty Hash

    EXPECT_EQ(1ul, h1.getAttributes("f").size()) << "Attributes not replaced";
    EXPECT_EQ(77u, h1.getAttribute<unsigned int>("f", "attrKey7")) << "UInt attribute value incorrect";
    // += is merge with REPLACE_ATTRIBUTES
    EXPECT_EQ(1ul, h1d.getAttributes("f").size()) << "Attributes not replaced (+=)";
    EXPECT_EQ(77u, h1d.getAttribute<unsigned int>("f", "attrKey7")) << "UInt attribute value incorrect (+=)";
    // here is MERGE_ATTRIBUTES
    EXPECT_EQ(2ul, h1b.getAttributes("f").size()) << "Attributes not merged";
    EXPECT_STREQ("buaah!", h1b.getAttribute<std::string>("f", "attrKey6").c_str())
          << "UInt attribute value incorrect (MERGE)";
    EXPECT_EQ(77u, h1b.getAttribute<unsigned int>("f", "attrKey7")) << "UInt attribute value incorrect (MERGE)";

    // Now check the 'selectedPaths' feature (no extra test for attribute merging needed):
    std::set<std::string> selectedPaths;
    selectedPaths.insert("a");
    selectedPaths.insert("b.c");
    selectedPaths.insert("g.h.i");
    selectedPaths.insert("h.i");
    selectedPaths.insert(".i[2]");
    selectedPaths.insert(".i[5]"); // check that we tolerate to select path with invalid index
    EXPECT_NO_THROW(h1c.merge(h2, Hash::MERGE_ATTRIBUTES, selectedPaths));

    // Keep everything it had before merging:
    EXPECT_TRUE(h1c.has("a"));
    EXPECT_TRUE(h1c.has("b"));
    EXPECT_TRUE(h1c.has("c.b[0].g"));
    EXPECT_TRUE(h1c.has("c.c[0].d"));
    EXPECT_TRUE(h1c.has("c.c[1].a.b.c"));
    EXPECT_TRUE(h1c.has("d.e"));
    EXPECT_TRUE(h1c.has("f.g"));
    // The additionally selected ones from h2:
    EXPECT_TRUE(h1c.has("b.c"));
    EXPECT_TRUE(h1c.has("g.h.i"));
    EXPECT_TRUE(h1c.has("h.i"));
    EXPECT_TRUE(h1c.has(".i[0].k.l")) << toString(h1c); // only row 2 (i[2]) selected, which becomes row 0
    // But not the other ones from h2:
    EXPECT_TRUE(!h1c.has("c.b[0].key")); // neither at old position of h2
    EXPECT_TRUE(!h1c.has("c.b[2]"));     // nor an extended vector<Hash> at all
    EXPECT_TRUE(!h1c.has("e"));
    // Take care that adding path "g.h.i" does not trigger that other children of "g.h" in h2 are taken as well:
    EXPECT_TRUE(!h1c.has("g.h.j"));
    EXPECT_TRUE(!h1c.has("h.j"));
    // Adding .i[2] should not trigger to add children of .i[1] nor .i[3]]
    EXPECT_TRUE(!h1c.has(".i[1].j"));
    EXPECT_TRUE(!h1c.has(".i[3]"));

    // Some further small tests for so far untested cases with selected paths...
    Hash hashTarget(".b", 1, ".c", Hash(), "c", "so so!");
    const Hash hashSource(".d", 8., "e..e[0]", Hash("f", 0), "e..e[1]", Hash("g", 1), "ha", 9);
    selectedPaths.clear();
    selectedPaths.insert(""); // trigger merging '.d'
    selectedPaths.insert("e..e[1]");
    EXPECT_NO_THROW(hashTarget.merge(hashSource, Hash::MERGE_ATTRIBUTES, selectedPaths));
    EXPECT_TRUE(hashTarget.has(".d"));
    EXPECT_TRUE(hashTarget.has("e..e[0]"));
    EXPECT_TRUE(hashTarget.has("e..e[0].g"));  // the selected e[1] becomes e[0]
    EXPECT_TRUE(!hashTarget.has("e..e[0].f")); // no children of e[0] since e[0] not selected (see test above)
    EXPECT_TRUE(!hashTarget.has("e..e[1]"));

    Hash hashTargetB("a[1].b", 1, "c", "Does not matter");
    Hash hashTargetC(hashTargetB);
    Hash hashTargetD(hashTargetB);
    const Hash hashSourceBCD("a[2]", Hash("a", 33, "c", 4.4), "ha", 9, "c[1]", Hash("k", 5, "l", 6), "c[2]",
                             Hash("b", -3), "d[2].b", 66, "e[1]", Hash("1", 1, "2", 2, "3", 3));
    selectedPaths.clear();
    selectedPaths.insert("a"); // trigger merging full vector
    // trigger selecting first HashVec item overwriting what was not a hashVec before, but only keep selected items
    selectedPaths.insert("c[1].l"); // for table rows one cannot select keys, i.e. '.l' is ignored
    selectedPaths.insert("d");      // trigger adding full new vector
    selectedPaths.insert("e[1].2"); // tabel row 1 is selected - the following '.2' is ignored
    EXPECT_NO_THROW(hashTargetB.merge(hashSourceBCD, Hash::MERGE_ATTRIBUTES, selectedPaths));
    EXPECT_TRUE(hashTargetB.has("a[0]")); // the empty one merged into it
    EXPECT_TRUE(!hashTargetB.has("a[0].b"));
    EXPECT_TRUE(hashTargetB.has("a[1]"));    // dito
    EXPECT_TRUE(!hashTargetB.has("a[1].b")); // target table a got replaced
    EXPECT_TRUE(hashTargetB.has("a[2].a"));
    EXPECT_TRUE(hashTargetB.has("a[2].c"));
    EXPECT_TRUE(!hashTargetB.has("a[3]"));
    EXPECT_TRUE(hashTargetB.has("c[0]"));
    EXPECT_TRUE(hashTargetB.has("c[0].k")) << toString(hashTargetB);
    EXPECT_TRUE(hashTargetB.has("c[0].l"));
    EXPECT_TRUE(hashTargetB.has("d[2].b"));
    EXPECT_TRUE(!hashTargetB.has("d[3]"));
    EXPECT_TRUE(hashTargetB.has("e[0]"));
    EXPECT_TRUE(hashTargetB.has("e[0].1"));
    EXPECT_TRUE(hashTargetB.has("e[0].2"));
    EXPECT_TRUE(hashTargetB.has("e[0].3"));

    selectedPaths.clear();
    selectedPaths.insert("a[0]");
    selectedPaths.insert("a[2].b"); // trigger selective vector items
    selectedPaths.insert("c");      // trigger overwriting with complete vector
    hashTargetC.merge(hashSourceBCD, Hash::MERGE_ATTRIBUTES, selectedPaths);
    EXPECT_TRUE(!hashTargetC.has("a[1].b")); // all table rows are overwritten
    EXPECT_TRUE(hashTargetC.has("a[1].a"));
    EXPECT_TRUE(hashTargetC.has("a[1].c"));
    EXPECT_TRUE(!hashTargetC.has("a[2]"));
    EXPECT_TRUE(hashTargetC.has("c[1].k"));
    EXPECT_TRUE(hashTargetC.has("c[1].l"));
    EXPECT_TRUE(hashTargetC.has("c[2].b"));
    EXPECT_TRUE(!hashTargetC.has("c[3]"));

    // Now select only invalid indices - nothing should be added
    selectedPaths.clear();
    selectedPaths.insert("a[10]"); // to existing vector
    selectedPaths.insert("c[10]"); // where there was another node
    selectedPaths.insert("d[10]"); // where there was no node at all
    selectedPaths.insert("ha[0]"); // for leaves, all indices are invalid
    Hash copyD(hashTargetD);
    hashTargetD.merge(hashSourceBCD, Hash::MERGE_ATTRIBUTES, selectedPaths);
    EXPECT_TRUE(similar(copyD, hashTargetD)) << "Selecting only invalid indices changed something";

    ////////////////////////////////////////////////////////////////////////////////////
    // Few more tests for a table
    const Hash targetTemplate("table", std::vector<Hash>({Hash("a", 1, "b", "1"), Hash("a", 12, "b", "12")}));
    Hash source("table", std::vector<Hash>(
                               {Hash("a", 101, "b", "101"), Hash("a", 102, "b", "102"), Hash("a", 103, "b", "103")}));

    Hash target1(targetTemplate);
    target1.merge(source);
    EXPECT_TRUE(target1.fullyEquals(source)) << toString(target1);

    // But we can select to use some rows only
    Hash target2(targetTemplate);
    // Keep only first and last rows of source
    target2.merge(source, Hash::MERGE_ATTRIBUTES, {"table[0]", "table[2]"});
    const auto mergedTable = target2.get<std::vector<Hash>>("table");
    EXPECT_EQ(2ul, mergedTable.size());
    const Hash& row0 = mergedTable[0];
    EXPECT_TRUE(row0.fullyEquals(source.get<std::vector<Hash>>("table")[0])) << toString(row0);
    const Hash& row1 = mergedTable[1];
    EXPECT_TRUE(row1.fullyEquals(source.get<std::vector<Hash>>("table")[2])) << toString(row1);
}


TEST(TestHash, testSubtract) {
    Hash h1("a", 1, "b", 2, "c.b[0].g", 3, "c.c[0].d", 4, "c.c[1]", Hash("a.b.c", 6), "d.e", 7);

    Hash h2("a", 21, "b.c", 22, "c.b[0]", Hash("key", "value"), "c.b[1].d", 24, "e", 27);
    h1 += h2;
    h1 -= h2;
    EXPECT_TRUE(h1.has("a") == false);
    EXPECT_TRUE(h1.get<Hash>("b").empty() == true);
    EXPECT_TRUE(!h1.has("c.b[0].g"));
    EXPECT_TRUE(!h1.has("c.b[1]"));
    EXPECT_TRUE(h1.get<int>("c.c[0].d") == 4);
    EXPECT_TRUE(h1.get<int>("c.c[1].a.b.c") == 6);
    EXPECT_TRUE(h1.get<int>("d.e") == 7);

    Hash h3("a.b.c", 1, "a.b.d", 2, "a.c.d", 22, "b.c.d", 33, "c.d.e", 44, "c.e.f", 55);
    Hash h4("a.b", Hash(), "c", Hash());
    h3 -= h4;
    EXPECT_TRUE(h3.has("a.b") == true);
    EXPECT_TRUE(h3.has("c") == true);
    EXPECT_TRUE(h3.get<int>("a.c.d") == 22);
    EXPECT_TRUE(h3.get<int>("b.c.d") == 33);
}


TEST(TestHash, testErase) {
    // prepare two identical hashes
    Hash h1("a", 1, "b", 2, "c.d", 31, "e.f.g", 411, "e.f.h", 412, "e.i", 42);
    Hash h2(h1);

    // Start testing Hash::erase on h1
    EXPECT_TRUE(h1.size() == 4);

    // erase existing key on first level => size decreases
    EXPECT_TRUE(h1.erase("a") == true);
    EXPECT_TRUE(h1.has("a") == false);
    EXPECT_TRUE(h1.size() == 3);

    // non-existing key - return false and keep size:
    EXPECT_TRUE(h1.erase("a") == false);
    EXPECT_TRUE(h1.size() == 3);

    // "c.d": composite key without siblings
    EXPECT_TRUE(h1.erase("c.d") == true);
    EXPECT_TRUE(h1.has("c.d") == false);
    EXPECT_TRUE(h1.has("c") == true);
    EXPECT_TRUE(h1.size() == 3); // "c" still in!

    // "e.f": composite key with two children and a sibling
    EXPECT_TRUE(h1.erase("e.f") == true);
    EXPECT_TRUE(h1.has("e.f.g") == false);
    EXPECT_TRUE(h1.has("e.f.h") == false);
    EXPECT_TRUE(h1.has("e.f") == false);
    EXPECT_TRUE(h1.has("e") == true); // stays
    EXPECT_TRUE(h1.size() == 3);

    // now testing Hash::erasePath on h2
    EXPECT_TRUE(h2.size() == 4);

    // erase existing key on first level => size decreases
    h2.erasePath("a");
    EXPECT_TRUE(h2.has("a") == false);
    EXPECT_TRUE(h2.size() == 3);

    // non-existing key: size just stays as it is
    h2.erasePath("a");
    EXPECT_TRUE(h2.size() == 3);


    // "c.d": composite key without siblings
    h2.erasePath("c.d");
    EXPECT_TRUE(h2.has("c.d") == false);
    EXPECT_TRUE(h2.has("c") == false); // removed since nothing left
    EXPECT_TRUE(h2.size() == 2);

    // "e.f": composite key with two children and a sibling
    h2.erasePath("e.f");
    EXPECT_TRUE(h2.has("e.f.g") == false);
    EXPECT_TRUE(h2.has("e.f.h") == false);
    EXPECT_TRUE(h2.has("e.f") == false);
    EXPECT_TRUE(h2.has("e") == true); // stays since there is "e.i"
    EXPECT_TRUE(h2.size() == 2);

    // Now testing erasure of elements in a vector<Hash>.
    Hash hVector("a[2].b", 111);
    EXPECT_TRUE(hVector.get<vector<Hash>>("a").size() == 3);
    EXPECT_TRUE(hVector.erase("a[3]") == false);
    EXPECT_TRUE(hVector.get<vector<Hash>>("a").size() == 3);
    EXPECT_TRUE(hVector.erase("a[0]") == true);
    EXPECT_TRUE(hVector.get<vector<Hash>>("a").size() == 2);
    EXPECT_TRUE(hVector.get<int>("a[1].b") == 111);
    // index on non-existing key
    EXPECT_TRUE(hVector.erase("c[2]") == false);
    EXPECT_TRUE(hVector.erase("a.c[2]") == false);
    EXPECT_TRUE(hVector.erase("a[0].c[1]") == false);

    // Now testing erasePath for paths containing indices.
    Hash hVector2("a[2].b", 111);
    EXPECT_TRUE(hVector2.get<vector<Hash>>("a").size() == 3);
    Hash copy = hVector2;
    hVector2.erasePath("a[3]"); // nothing happens (not even an exception)
    EXPECT_TRUE(hVector2 == copy);
    hVector2.erasePath("a[3].b"); // nothing happens (not even an exception)
    EXPECT_TRUE(hVector2 == copy);
    hVector2.erasePath("a[0]"); // shrunk
    EXPECT_TRUE(hVector2.get<vector<Hash>>("a").size() == 2);
    EXPECT_TRUE(hVector2.get<int>("a[1].b") == 111);
    hVector2.erasePath("a[1].b"); // erase a[1] as well since b is only daughter
    EXPECT_TRUE(hVector2.get<vector<Hash>>("a").size() == 1);
    // index for non-existing key must neither throw nor touch the content
    copy = hVector2;
    hVector2.erasePath("c[2]");
    EXPECT_TRUE(hVector2 == copy);
    hVector2.erasePath("a.c[2]");
    EXPECT_TRUE(hVector2 == copy);
    hVector2.erasePath("a[0].c[1]");
    EXPECT_TRUE(hVector2 == copy);
    // single element vector<Hash>: vector is removed completely
    hVector2.erasePath("a[0]");
    EXPECT_TRUE(hVector2.has("a") == false);

    // Test erase with empty keys at various places of the path
    Hash hEmptyKey("", 1, "a.", 2, "a1.", 3, "b..", 31, "c..d", 32, "e..f", 33);
    Hash hEmptyKey2(hEmptyKey); // for next test section
    // only empty key
    EXPECT_EQ(6ul, hEmptyKey.size());
    EXPECT_TRUE(hEmptyKey.has(""));
    EXPECT_TRUE(hEmptyKey.erase("")); // only empty key
    EXPECT_EQ(5ul, hEmptyKey.size());

    EXPECT_TRUE(hEmptyKey.has("a"));
    EXPECT_TRUE(hEmptyKey.has("a."));
    EXPECT_TRUE(hEmptyKey.erase("a.")); // empty key at end
    EXPECT_TRUE(!hEmptyKey.has("a."));
    EXPECT_TRUE(hEmptyKey.has("a"));

    EXPECT_TRUE(hEmptyKey.has("a1"));
    EXPECT_TRUE(hEmptyKey.has("a1."));
    EXPECT_TRUE(hEmptyKey.erase("a1"));
    EXPECT_TRUE(!hEmptyKey.has("a1."));
    EXPECT_TRUE(!hEmptyKey.has("a1"));

    EXPECT_TRUE(hEmptyKey.has("b"));
    EXPECT_TRUE(hEmptyKey.has("b."));
    EXPECT_TRUE(hEmptyKey.has("b.."));
    Hash& b = hEmptyKey.get<Hash>("b");
    EXPECT_TRUE(b.has("."));
    EXPECT_TRUE(b.erase(".")); // empty key at begin and end
    EXPECT_TRUE(!hEmptyKey.has("b.."));
    EXPECT_TRUE(hEmptyKey.has("b."));

    EXPECT_TRUE(hEmptyKey.has("c"));
    EXPECT_TRUE(hEmptyKey.has("c."));
    EXPECT_TRUE(hEmptyKey.has("c..d"));
    Hash& c = hEmptyKey.get<Hash>("c");
    EXPECT_TRUE(c.erase(".d")); // empty key at begin
    EXPECT_TRUE(!hEmptyKey.has("c..d"));
    EXPECT_TRUE(hEmptyKey.has("c."));

    EXPECT_TRUE(hEmptyKey.has("e"));
    EXPECT_TRUE(hEmptyKey.has("e."));
    EXPECT_TRUE(hEmptyKey.has("e..f"));
    EXPECT_TRUE(hEmptyKey.erase("e..f")); // empty key in middle
    EXPECT_TRUE(!hEmptyKey.has("e..f"));
    EXPECT_TRUE(hEmptyKey.has("e."));

    // Test erasePath with empty keys at various places of the path.
    // Same test cases as for erase, but sometimes other result!

    // only empty key
    EXPECT_EQ(6ul, hEmptyKey2.size());
    EXPECT_TRUE(hEmptyKey2.has(""));
    hEmptyKey2.erasePath("");
    EXPECT_EQ(5ul, hEmptyKey2.size());

    EXPECT_TRUE(hEmptyKey2.has("a"));
    EXPECT_TRUE(hEmptyKey2.has("a."));
    hEmptyKey2.erasePath("a."); // empty key an end
    EXPECT_TRUE(!hEmptyKey2.has("a."));
    EXPECT_TRUE(!hEmptyKey2.has("a"));

    EXPECT_TRUE(hEmptyKey2.has("a1"));
    EXPECT_TRUE(hEmptyKey2.has("a1."));
    hEmptyKey2.erasePath("a1");
    EXPECT_TRUE(!hEmptyKey2.has("a1."));
    EXPECT_TRUE(!hEmptyKey2.has("a1"));

    EXPECT_TRUE(hEmptyKey2.has("b"));
    EXPECT_TRUE(hEmptyKey2.has("b."));
    EXPECT_TRUE(hEmptyKey2.has("b.."));
    Hash& b2 = hEmptyKey2.get<Hash>("b");
    EXPECT_TRUE(b2.has("."));
    b2.erasePath("."); // empty key at begin and end
    EXPECT_TRUE(!hEmptyKey2.has("b.."));
    EXPECT_TRUE(!hEmptyKey2.has("b."));
    EXPECT_TRUE(hEmptyKey2.has("b"));

    EXPECT_TRUE(hEmptyKey2.has("c"));
    EXPECT_TRUE(hEmptyKey2.has("c."));
    EXPECT_TRUE(hEmptyKey2.has("c..d"));
    Hash& c2 = hEmptyKey2.get<Hash>("c");
    c2.erasePath(".d"); // empty key at begin
    EXPECT_TRUE(!hEmptyKey2.has("c..d"));
    EXPECT_TRUE(!hEmptyKey2.has("c."));
    EXPECT_TRUE(hEmptyKey2.has("c"));

    EXPECT_TRUE(hEmptyKey2.has("e"));
    EXPECT_TRUE(hEmptyKey2.has("e."));
    EXPECT_TRUE(hEmptyKey2.has("e..f"));
    hEmptyKey2.erasePath("e..f"); // empty key in middle
    EXPECT_TRUE(!hEmptyKey2.has("e..f"));
    EXPECT_TRUE(!hEmptyKey2.has("e."));
    EXPECT_TRUE(!hEmptyKey2.has("e"));

    // Check vector treatment, i.e. erasePath("a.v[0]") where v was - either size 1 or longer
    // Test erasePath where it acts differently than erase
    Hash hEmptyKey3("a.vec[1]", Hash(), ".vecb[1]", Hash());
    hEmptyKey3.erasePath("a.vec[1]");
    EXPECT_TRUE(hEmptyKey3.has("a.vec[0]"));
    hEmptyKey3.erasePath("a.vec[0]");
    EXPECT_TRUE(!hEmptyKey3.has("a.vec"));
    EXPECT_TRUE(!hEmptyKey3.has("a"));
    // Now empty key instead of "a":
    hEmptyKey3.erasePath(".vecb[1]");
    EXPECT_TRUE(hEmptyKey3.has(".vecb[0]"));
    hEmptyKey3.erasePath(".vecb[0]");
    EXPECT_TRUE(!hEmptyKey3.has(".vecb"));
    EXPECT_TRUE(!hEmptyKey3.has(""));
    EXPECT_TRUE(hEmptyKey3.empty());
}


TEST(TestHash, testHas) {
    // Hash::has(path) is already used a lot in other tests, but some corner cases
    // are missing, e.g. non-existing array indices at different positions in path.
    Hash h1("a.b[2]", Hash(), "b[1]", Hash());
    EXPECT_TRUE(h1.has("a") == true);
    EXPECT_TRUE(h1.has("a.b") == true);
    EXPECT_TRUE(h1.has("a.b[0]") == true);
    EXPECT_TRUE(h1.has("a.b[1]") == true);
    EXPECT_TRUE(h1.has("a.b[2]") == true);
    EXPECT_TRUE(h1.has("a.b[2].some") == false);
    EXPECT_TRUE(h1.has("a.b[2].some.other") == false);
    EXPECT_TRUE(h1.has("a.b[3]") == false);
    EXPECT_TRUE(h1.has("a.b[3].some") == false);
    EXPECT_TRUE(h1.has("a.b[3].some.other") == false);
    // Test also vector<Hash> on first level:
    EXPECT_TRUE(h1.has("b") == true);
    EXPECT_TRUE(h1.has("b[2]") == false);
    // And now some index on a non-existing vector<Hash>:
    EXPECT_TRUE(h1.has("c[0]") == false);
}


TEST(TestHash, testIs) {
    // Test different cases: paths without indices, with index at end or in the middle.
    Hash h("a", 77, "b[1].d", 88.8, "b[2].c", 88);
    EXPECT_TRUE(h.is<int>("a") == true);
    EXPECT_TRUE(h.is<vector<Hash>>("b") == true);
    EXPECT_TRUE(h.is<Hash>("b[0]") == true);
    EXPECT_TRUE(h.is<double>("b[1].d") == true);
    EXPECT_TRUE(h.is<Hash>("b[2]") == true);
    EXPECT_TRUE(h.is<int>("b[2].c") == true);

    // Check for false on wrong type - cannot test all wrong types...
    EXPECT_TRUE(h.is<float>("a") == false);
    EXPECT_TRUE(h.is<Hash>("b") == false);
    EXPECT_TRUE(h.is<int>("b[0]") == false);
    EXPECT_TRUE(h.is<float>("b[1].d") == false);
    EXPECT_TRUE(h.is<vector<Hash>>("b[2]") == false);
    EXPECT_TRUE(h.is<double>("b[2].c") == false);

    // Check exceptions on bad paths:
    // 1) non-existing "normal" path
    bool caught1 = false;
    try {
        h.is<int>("c");
    } catch (karabo::data::ParameterException const& e) {
        caught1 = true;
    }
    EXPECT_TRUE(caught1 == true);

    // 2) non-existing index of vector that is last item
    bool caught2 = false;
    try {
        h.is<Hash>("b[3]");
    } catch (karabo::data::ParameterException const& e) {
        caught2 = true;
    }
    EXPECT_TRUE(caught2 == true);

    // 3) item under non-existing index of vector
    bool caught3 = false;
    try {
        h.is<int>("b[3].d");
    } catch (karabo::data::ParameterException const& e) {
        caught3 = true;
    }
    EXPECT_TRUE(caught3 == true);

    // 4) non-existing item under existing index of vector
    bool caught4 = false;
    try {
        h.is<int>("b[0].a");
    } catch (karabo::data::ParameterException const& e) {
        caught4 = true;
    }
    EXPECT_TRUE(caught4 == true);
}


namespace helper {
    class Helper {
       public:
        Helper(){};


        virtual ~Helper(){};


        bool operator()(const karabo::data::Hash::Node& node) {
            return eval(node);
        }

        virtual bool eval(const karabo::data::Hash::Node& node) = 0;
    };

    bool dfs(const karabo::data::Hash& hash, Helper& helper);
    bool dfs(const std::vector<karabo::data::Hash>& hash, Helper& helper);
    bool dfs(const karabo::data::Hash::Node& node, Helper& helper);


    bool dfs(const karabo::data::Hash& hash, Helper& helper) {
        if (hash.empty()) return false;

        for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
            if (!dfs(*it, helper)) return false;
        }
        return true;
    }


    bool dfs(const std::vector<karabo::data::Hash>& hash, Helper& helper) {
        if (hash.empty()) return false;

        for (size_t i = 0; i < hash.size(); ++i) {
            if (!dfs(hash[i], helper)) return false;
        }
        return true;
    }


    bool dfs(const karabo::data::Hash::Node& node, Helper& helper) {
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
        static bool visit__(const karabo::data::Hash& hash, Helper& helper) {
            if (hash.empty()) return false;

            for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                if (!visit__(*it, helper)) return false;
            }
            return true;
        }


        template <class Helper>
        static bool visit__(const std::vector<karabo::data::Hash>& hash, Helper& helper) {
            if (hash.empty()) return false;

            for (size_t i = 0; i < hash.size(); ++i) {
                if (!visit__(hash[i], helper)) return false;
            }
            return true;
        }


        template <class Helper>
        static bool visit__(const karabo::data::Hash::Node& node, Helper& helper) {
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


    bool eval(const karabo::data::Hash::Node& node) {
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


    bool eval(const karabo::data::Hash::Node& node) {
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


    void pre(const karabo::data::Hash::Node& node) {
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


    bool eval(const karabo::data::Hash::Node& node) {
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
                m_stream << " => " << node.getValue<karabo::data::Schema>();
                break;
            default:
                m_stream << " => " << node.getValueAs<string>() << " " << Types::to<ToLiteral>(type);
        }
        m_stream << '\n';
        return true;
    }


    void post(const karabo::data::Hash::Node& node) {
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


    void pre(const karabo::data::Hash::Node& node) {
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


    bool eval(const karabo::data::Hash::Node& node) {
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


    void post(const karabo::data::Hash::Node& node) {
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


    void pre(const karabo::data::Hash::Node& node) {
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


    bool eval(const karabo::data::Hash::Node& node) {
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


    void post(const karabo::data::Hash::Node& node) {
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


TEST(TestHash, testHelper) {
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
        // std::clog << "Count H : " << karabo::data::counter(h1) << std::endl;
        // std::clog << "Count V : " << c2.getResult() << std::endl;
    }
}


TEST(TestHash, testPack) {
    using karabo::util::pack;
    Hash h;
    pack(h);
    EXPECT_TRUE(h.size() == 0);
    pack(h, 3);
    EXPECT_TRUE(h.size() == 1);
    EXPECT_TRUE(h.get<int>("a1") == 3);
    pack(h, 3, 2);
    pack(h, string("bla"), 2.5);
    EXPECT_TRUE(h.size() == 2);
    EXPECT_TRUE(h.get<string>("a1") == "bla");
    EXPECT_TRUE(h.get<double>("a2") == 2.5);

    string s;
    double x;

    karabo::util::unpack(h, s, x);
    EXPECT_TRUE(s == "bla");
    EXPECT_TRUE(x == 2.5);
}

TEST(TestHash, testCounter) {
    Hash h("a", true, "b", int(0), "c", NDArray(Dims(5, 5)), "d", std::vector<int>(3, 0));
    h.set("e", std::vector<NDArray>(3, NDArray(Dims(5, 5))));
    // if counter were not to skip over Hash derived classes the ND-Array internal reference type of type
    // INT32 would be counted leading to a count of 8
    EXPECT_TRUE(karabo::data::counter(h, karabo::data::Types::INT32) == 4);
    // if counter were not to skip over Hash derived classes the ND-Array internal is big endian of type
    // BOOL would be counted leading to a count of 5
    EXPECT_TRUE(karabo::data::counter(h, karabo::data::Types::BOOL) == 1);
    EXPECT_TRUE(karabo::data::counter(h, karabo::data::Types::HASH) == 1);
}


TEST(TestHash, testKeys) {
    // Test various funny keys/paths
    Hash h(" ", true, "", false, ".", 0, ".b", 1, "a.", 2, "c..b", 3);

    EXPECT_TRUE(h.has(" "));
    EXPECT_TRUE(h.has(""));
    EXPECT_TRUE(h.has("a"));
    EXPECT_TRUE(h.has("c"));
    EXPECT_EQ(4ul, h.size()); // no other 1st level keys!

    const Hash& g = h.get<Hash>("");
    EXPECT_TRUE(g.has(""));
    EXPECT_TRUE(g.has("b"));
    EXPECT_EQ(2ul, g.size()); // dito

    const Hash& a = h.get<Hash>("a");
    EXPECT_TRUE(a.has(""));
    EXPECT_EQ(1ul, a.size()); // dito

    const Hash& c = h.get<Hash>("c");
    EXPECT_TRUE(a.has(""));
    EXPECT_EQ(1ul, c.size()); // dito

    const Hash& c1 = c.get<Hash>("");
    EXPECT_TRUE(c1.has("b"));
    EXPECT_EQ(1ul, c1.size()); // dito
}


void testSimilarIsNotFullyEqualByOrder(bool orderMatters) {
    std::clog << "testSimilarIsNotFullyEqualByOrder starts with orderMatters = " << orderMatters << std::endl;

    Hash h1("a.b", "value", "a.c", true);
    Hash h2("a1.b", "value", "a1.c", true);

    // Checks that hashes with elements with different keys of the same type and in the same order
    // are still similar.
    EXPECT_EQ(h1, h2); // 'Hash::operator==' actually checks for similarity.
    // But are not fullyEqual
    EXPECT_TRUE(!h1.fullyEquals(h2, orderMatters)) << "h1 and h2 shouldn't be fullyEquals - they differ in key names.";

    Hash h3("a1", 1, "a1.b", "value", "a1.c", false);
    // Checks that hashes with elements with different values of the same type and in the same
    // order are still similar.
    EXPECT_EQ(h2, h3); // 'Hash::operator==' actually checks for similarity.
    // But are not fullyEqual
    EXPECT_TRUE(!h2.fullyEquals(h3, orderMatters)) << "h2 and h3 shouldn't be fullyEquals - they differ in key values.";

    Hash h4("a1", 1, "a1.b", "value", "a1.c", true);
    h4.setAttribute("a1", "attr", true);
    h2.setAttribute("a1", "attr", 4);
    // Checks that hashes with elements with different attributes, with the same value, of the
    // same type and in the same order are still similar.
    EXPECT_EQ(h2, h4); // 'Hash::operator==' actually checks for similarity.
    // But are not fullyEqual
    EXPECT_TRUE(!h2.fullyEquals(h4, orderMatters))
          << "h4 and h2 shouldn't be fullyEquals - they differ in element attributes.";

    Hash h5("a", 13.14159, "b[0]", Hash("hKey_0", "hValue_0"), "b[1]", Hash("hKey_1", "hValue_1"), "c",
            "1, 1, 2, 3, 5, 8, 11, 19, 30");
    Hash h6("a", 13.14159, "b[0]", Hash("hKey_0", "hValue_0"), "b[1]", Hash("hKey_1", "hValue_1"), "c",
            "1, 1, 2, 3, 5, 8, 11, 19, 30, 49, 79");
    // Repeats the test for hashes differing in node value, but this time with one
    // complex node, of type vector of hashes, that matches. The hashes are similar ...
    EXPECT_EQ(h5, h6); // 'Hash::operator==' actually checks for similarity.
    // But are not fullyEqual
    EXPECT_TRUE(!h5.fullyEquals(h6, orderMatters))
          << "h5 and h6 shouldn't be fullyEquals - they differ in element values.";

    vector<Hash> vhAttr{Hash("key_0", "val_0"), Hash("key_1", "val_1")};
    h5.setAttribute("a", "attr", vhAttr);
    h6.setAttribute("a", "attr", 2);
    h6.set<std::string>("c", "1, 1, 2, 3, 5, 8, 11, 19, 30");
    EXPECT_TRUE(!h5.fullyEquals(h6, orderMatters))
          << "h5 and h6 shouldn't be fullyEquals - they differ in vector of hash attribute";

    // A case where two hashes with complex attributes and nodes are fullyEquals.
    h6.setAttribute("a", "attr", vhAttr);
    EXPECT_TRUE(h5.fullyEquals(h6, orderMatters)) << "h5 and h6 should be fullyEquals!";

    Hash h7("a", 1, "b", 2, "c", 3);
    Hash h8("b", 1, "a", 2, "c", 3);
    // Checks that hashes with keys in different order are still similar.
    EXPECT_EQ(h7, h8);
    // But are not fullyEqual.
    EXPECT_TRUE(!h7.fullyEquals(h8, orderMatters))
          << "h7 and h8 shouldn't be fullyEquals - they differ in the order of their elements.";

    Hash h9("a", 1, "b", 2, "c", "3");
    // Checks that hashes with different value types for values that have the same string representation form are
    // neither similar nor fullyEquals.
    EXPECT_TRUE(h7 != h9) << "h7 and h9 should not be similar, as their 'c' elements differ in type.";
    EXPECT_TRUE(!h7.fullyEquals(h9, orderMatters))
          << "h7 and h9 should not be fullyEquals, as their 'c' elements differ in type.";

    // Check VECTOR_STRING treatment
    Hash h11("vecStr", std::vector<std::string>({"with,comma", "with space", "onlyChar"}));
    Hash h12("vecStr", std::vector<std::string>({"with,comma", "with space"}));
    EXPECT_TRUE(!h11.fullyEquals(h12, orderMatters)) << "Differ in number of elements in vector";
    h12.get<std::vector<std::string>>("vecStr").push_back("onlychar");
    EXPECT_TRUE(!h11.fullyEquals(h12, orderMatters)) << "Differ in one character of last element in vector";
    h12.get<std::vector<std::string>>("vecStr").back() = "onlyChar"; // now make fully equal
    EXPECT_TRUE(h11.fullyEquals(h12, orderMatters));
    // Now VECTOR_STRING as attribute
    h11.setAttribute("vecStr", "vecStrOpt", std::vector<std::string>({"With,comma", "With space", "OnlyChar"}));
    h12.setAttribute("vecStr", "vecStrOpt", std::vector<std::string>({"With,comma", "With space"}));
    EXPECT_TRUE(!h11.fullyEquals(h12, orderMatters)) << "Differ in number of elements in vector attribute";
    h12.getAttribute<std::vector<std::string>>("vecStr", "vecStrOpt").push_back("Onlychar");
    EXPECT_TRUE(!h11.fullyEquals(h12, orderMatters)) << "Differ in one character of last element in vector attribute";
    h12.getAttribute<std::vector<std::string>>("vecStr", "vecStrOpt").back() = "OnlyChar";
    EXPECT_TRUE(h11.fullyEquals(h12, orderMatters));

    Schema sch("hashSchema");
    INT32_ELEMENT(sch).key("a").tags("prop").assignmentOptional().defaultValue(10).commit();
    Hash h10("b", 2, "a", 1, "c", 3);
    h10.setAttribute("c", "schema", sch);
    h8.setAttribute("c", "schema", Schema("test"));
    // Checks that hashes with different attributes of type schema are similar
    EXPECT_EQ(h8, h10);
    // But are not fullyEquals
    EXPECT_TRUE(!h8.fullyEquals(h10, orderMatters))
          << "h8 and h10 should not be fullyEquals, as they have different values for attributes of type Schema ";
}


TEST(TestHash, testSimilarIsNotFullyEqual) {
    testSimilarIsNotFullyEqualByOrder(true);
    testSimilarIsNotFullyEqualByOrder(false);
}


TEST(TestHash, testFullyEqualUnordered) {
    // Just two keys are swapped: hashes differ if order matters, otherwise not
    Hash h1("a.b", "value", "a.c", true, "1", 1);
    Hash h2("a.c", true, "a.b", "value", "1", 1);

    EXPECT_TRUE(!h1.fullyEquals(h2, true));
    EXPECT_TRUE(h1.fullyEquals(h2, false));

    // Just order of attributes is swapped: hashes differ if order matters, otherwise not
    Hash h3(h1);
    h3.setAttribute("1", "A", 1);
    h3.setAttribute("1", "B", 2);
    h1.setAttribute("1", "B", 2);
    h1.setAttribute("1", "A", 1);

    EXPECT_TRUE(!h1.fullyEquals(h3, true)) << toString(h1) << " vs " << toString(h3);
    EXPECT_TRUE(h1.fullyEquals(h3, false)) << toString(h1) << " vs " << toString(h3);
}


TEST(TestHash, testNode) {
    // Hash::Node::setValue
    {
        Hash h1, h2;
        Hash::Node& node1 = h1.set("a", 1);
        Hash::Node& node2 = h2.set("a", 1);

        // setValue: Template specialization for Hash and the overload for Hash must have the same effect
        //           concerning __classId attribute:
        node1.setValue(Hash("1", 2));
        node2.setValue<Hash>(Hash("1", 2));
        EXPECT_TRUE(!node1.hasAttribute("__classId"));
        EXPECT_TRUE(!node2.hasAttribute("__classId"));

        EXPECT_EQ(0ul, node1.getAttributes().size());
        EXPECT_EQ(0ul, node2.getAttributes().size());
    }
    {
        // Test Hash::Node::setValue and the possible type change introduced by that.
        Hash h("a.b.c", "1");
        EXPECT_TRUE(h.get<std::string>("a.b.c") == "1");
        EXPECT_TRUE(h.getAs<int>("a.b.c") == 1);
        boost::optional<Hash::Node&> node = h.find("a.b.c");
        if (node) node->setValue(2);
        EXPECT_TRUE(h.get<int>("a.b.c") == 2);
        EXPECT_TRUE(h.getAs<std::string>("a.b.c") == "2");
    }
    {
        // Setting a Hash::Node is setting its value (due to Element::setValue(..) template specification).
        // Question arising: Why? h.set(node.getValueAsAny()) already does that...
        // But we keep backward compatibility here, i.e. this test succeeds in 2.11.4, but fails in 2.12.0,
        // see also https://git.xfel.eu/Karabo/Framework/-/merge_requests/5940.
        Hash::Node node("a", 1);
        EXPECT_EQ(static_cast<int>(Types::ReferenceType::INT32), static_cast<int>(node.getType()));
        const Hash::Node constNode(node);

        // Test setting for all cases: normal object, rvalue reference and const object
        Hash h;
        h.set("normal", node);
        h.set("moved", std::move(node));
        h.set("const", constNode);

        EXPECT_EQ(static_cast<int>(Types::ReferenceType::INT32), static_cast<int>(h.getType("moved")));
        EXPECT_EQ(1, h.get<int>("moved"));

        EXPECT_EQ(static_cast<int>(Types::ReferenceType::INT32), static_cast<int>(h.getType("const")));
        EXPECT_EQ(1, h.get<int>("const"));

        EXPECT_EQ(static_cast<int>(Types::ReferenceType::INT32), static_cast<int>(h.getType("normal")));
        EXPECT_EQ(1, h.get<int>("normal"));
    }
    {
        // Similar as before, but now testing also move semantics (i.e. would not succeed in 2.11.4).
        Hash::Node node("a", TraceCopies(2));
        const Hash::Node constNode("a", TraceCopies(3));
        TraceCopies::reset();

        // Test setting for all cases: normal object, rvalue reference and const object
        Hash h;
        h.set("normal", node);
        EXPECT_EQ(1, TraceCopies::countCopyConstr);
        EXPECT_EQ(2, h.get<TraceCopies>("normal").value);
        EXPECT_EQ(2, node.getValue<TraceCopies>().value); // i.e. not -1 as for a 'moved away' TraceCopies instance

        // Moving the node means move-assignment of its m_value member (which is a std::any) to the new node inside h.
        // That leaves node's m_value in a valid, but undefined state, so better do not use it (e.g. by node.getValue)
        // anymore.
        h.set("moved", std::move(node));
        EXPECT_EQ(2, h.get<TraceCopies>("moved").value);
        // Next line would throw (see comment above) since node::m_value has an unknown type (void?) that cannot be cast
        // to TraceCopies EXPECT_EQ(-1, node.getValue<TraceCopies>().value);
        EXPECT_EQ(1, TraceCopies::countCopyConstr); // There was no copy!
        // Next line succeeds, i.e. no move assignment either. Looks like since the move-assignment of std::any just
        // swaps between source and target instead of moving. But that is an implementation detail we should not tests.
        // EXPECT_EQ(0, TraceCopies::countMoveConstr);

        h.set("const", constNode);
        EXPECT_EQ(2, TraceCopies::countCopyConstr); // another copy now
        EXPECT_EQ(3, h.get<TraceCopies>("const").value);
        EXPECT_EQ(3, constNode.getValue<TraceCopies>().value); // i.e. not -1 as for a 'moved away' TraceCopies instance

        TraceCopies::reset();
    }
    {
        // Tests of Hash::Node constructors with move semantics from ValueType
        const TraceCopies a(1);
        Hash::Node nodeA("a", a);
        EXPECT_EQ(1, TraceCopies::countCopyConstr);
        EXPECT_EQ(1, nodeA.getValue<TraceCopies>().value);
        EXPECT_EQ(1, a.value); // not -1 as for a moved away object

        TraceCopies b(2);
        TraceCopies&& bRval = std::move(b); // std::move is in fact just a cast
        Hash::Node nodeB("b", std::move(bRval));
        EXPECT_EQ(1, TraceCopies::countCopyConstr);
        EXPECT_EQ(2, nodeB.getValue<TraceCopies>().value);
        EXPECT_EQ(-1, b.value); // -1 as for a moved away object

        TraceCopies::reset();
    }
    {
        // Tests of Hash::Node constructors with move semantics from std::any
        const std::any a(TraceCopies(1));
        TraceCopies::reset(); // Whatever the line before did does not matter...
        Hash::Node nodeA("a", a);
        EXPECT_EQ(1, TraceCopies::countCopyConstr);
        EXPECT_EQ(1, nodeA.getValue<TraceCopies>().value);
        EXPECT_EQ(1, std::any_cast<TraceCopies>(a).value); // not -1 as for a moved away object

        std::any b(TraceCopies(2));
        TraceCopies::reset();            // Whatever the line before did does not matter...
        std::any&& bRval = std::move(b); // std::move is in fact just a cast
        Hash::Node nodeB("b", std::move(bRval));
        // Not copied - in fact it is not moved either (since std::any is moved which seems to swap),
        // but that is an implementation detail we better do not test against.
        EXPECT_EQ(0, TraceCopies::countCopyConstr);
        EXPECT_EQ(2, nodeB.getValue<TraceCopies>().value);
        // Next line would throw again due to access to moved-away b
        // EXPECT_EQ(-1, std::any_cast<TraceCopies>(b).value);

        TraceCopies::reset();
    }
}
