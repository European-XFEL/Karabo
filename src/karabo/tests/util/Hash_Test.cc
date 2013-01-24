/*
 * File:   Hash_Test.cc
 * Author: heisenb
 *
 * Created on Sep 18, 2012, 6:47:59 PM
 */

#include <karabo/util/Hash.hh>
#include "Hash_Test.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(Hash_Test);

using namespace karabo::util;
using namespace std;

Hash_Test::Hash_Test() {
}

Hash_Test::~Hash_Test() {
}

void Hash_Test::setUp() {
}

void Hash_Test::tearDown() {
}

void Hash_Test::testConstructors() {

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
        CPPUNIT_ASSERT(h.get<string > ("d") == "4");
    }

    {
        Hash h("a", 1, "b", 2.0, "c", 3.f, "d", "4", "e", std::vector<unsigned int>(5, 5));
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 5);
        CPPUNIT_ASSERT(h.get<int>("a") == 1);
        CPPUNIT_ASSERT(h.get<double>("b") == 2.0);
        CPPUNIT_ASSERT(h.get<float>("c") == 3.0);
        CPPUNIT_ASSERT(h.get<string > ("d") == "4");
        CPPUNIT_ASSERT(h.get<std::vector<unsigned int> > ("e")[0] == 5);
    }

    {
        Hash h("a", 1, "b", 2.0, "c", 3.f, "d", "4", "e", std::vector<unsigned int>(5, 5), "f", Hash("a", 6));
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 6);
        CPPUNIT_ASSERT(h.get<int>("a") == 1);
        CPPUNIT_ASSERT(h.get<double>("b") == 2.0);
        CPPUNIT_ASSERT(h.get<float>("c") == 3.0);
        CPPUNIT_ASSERT(h.get<string > ("d") == "4");
        CPPUNIT_ASSERT(h.get<std::vector<unsigned int> >("e")[0] == 5);
        CPPUNIT_ASSERT(h.get<Hash > ("f").get<int>("a") == 6);
        CPPUNIT_ASSERT(h.get<int > ("f.a") == 6);
        
        Hash h1("a.barsch.c", 1);
        h1.setAttribute("a", "attr1", 1);
        h1.setAttribute("a", "attr2", 2);
        h1.setAttribute("a.barsch.c", "color", "red");
        h.set("myArray[0]", h1);
        h.set("myArray[1]", h1);
        h.set("myArray[5]", h1);
        cerr << endl << endl;
        cerr << h;
        cerr << endl << endl;
    }
}

void Hash_Test::testGetSet() {

    {
        Hash h;
        h.set("a.b.c1.d", 1);
        CPPUNIT_ASSERT(h.get<Hash > ("a").has("b") == true);
        CPPUNIT_ASSERT(h.get<Hash > ("a.b").has("c1") == true);
        CPPUNIT_ASSERT(h.get<Hash > ("a.b.c1").has("d") == true);
        CPPUNIT_ASSERT(h.get<int>("a.b.c1.d") == 1);

        h.set("a.b.c2.d", "1");
        CPPUNIT_ASSERT(h.get<Hash > ("a").has("b") == true);
        CPPUNIT_ASSERT(h.get<Hash > ("a.b").has("c1") == true);
        CPPUNIT_ASSERT(h.get<Hash > ("a.b").has("c2") == true);
        CPPUNIT_ASSERT(h.get<Hash > ("a.b.c2").has("d") == true);
        CPPUNIT_ASSERT(h.get<string > ("a.b.c2.d") == "1");

        h.set("a.b[0]", Hash("a", 1));
        CPPUNIT_ASSERT(h.get<Hash > ("a").has("b") == true);
        CPPUNIT_ASSERT(h.get<Hash > ("a").size() == 1);
        CPPUNIT_ASSERT(h.is<std::vector<Hash> >("a.b") == true);
        CPPUNIT_ASSERT(h.get<std::vector<Hash> >("a.b").size() == 1);
        CPPUNIT_ASSERT(h.get<std::vector<Hash> >("a.b")[0].size() == 1);
        CPPUNIT_ASSERT(h.get<std::vector<Hash> >("a.b")[0].get<int>("a") == 1);
        CPPUNIT_ASSERT(h.get<int>("a.b[0].a") == 1);

        h.set("a.b[2]", Hash("a", "1"));
        CPPUNIT_ASSERT(h.get<Hash > ("a").has("b") == true);
        CPPUNIT_ASSERT(h.get<Hash > ("a").size() == 1);
        CPPUNIT_ASSERT(h.is<std::vector<Hash> >("a.b") == true);
        CPPUNIT_ASSERT(h.get<std::vector<Hash> >("a.b").size() == 3);
        CPPUNIT_ASSERT(h.get<int>("a.b[0].a") == 1);
        CPPUNIT_ASSERT(h.get<Hash > ("a.b[1]").empty() == true);
        CPPUNIT_ASSERT(h.get<string > ("a.b[2].a") == "1");
    }

    {
        Hash h;
        h.set("a.b.c", 1);
        h.set("a.b.c", 2);
        CPPUNIT_ASSERT(h.get<int>("a.b.c") == 2);
    }

    {
        Hash h("a[0]", Hash("a", 1), "a[1]", Hash("a", 1));
        CPPUNIT_ASSERT(h.get<int>("a[0].a") == 1);
        CPPUNIT_ASSERT(h.get<int>("a[1].a") == 1);
    }

    {
        Hash h;
        h.set("x[0].y[0]", Hash("a", 4.2, "b", "red", "c", true));
        h.set("x[1].y[0]", Hash("a", 4.0, "b", "green", "c", false));
        CPPUNIT_ASSERT(h.get<bool>("x[0].y[0].c") == true);
        CPPUNIT_ASSERT(h.get<bool>("x[1].y[0].c") == false);
        CPPUNIT_ASSERT(h.get<string > ("x[0].y[0].b") == "red");
        CPPUNIT_ASSERT(h.get<string > ("x[1].y[0].b") == "green");
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
        int i;
        Hash h("a", "1");
        h.get("a", s);
        CPPUNIT_ASSERT(s == "1");
        h.get<string > ("a") = "2";
        h.get("a", s);
        CPPUNIT_ASSERT(s == "2");
    }
}

void Hash_Test::testGetAs() {

    {
        Hash h("a", true);
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "1");
        CPPUNIT_ASSERT(h.getAs<int > ("a") == 1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, h.getAs<double > ("a"), 0.00001);
        CPPUNIT_ASSERT(static_cast<unsigned int> (h.getAs<char > ("a")) == 1);
    }
    {
        Hash h("a", std::vector<bool>(4, false));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "[0,0,0,0]");
        int tmp = h.getAs<int, std::vector > ("a")[3];
        CPPUNIT_ASSERT(tmp == 0);
    }
    {
        Hash h("a", char('R'));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "R");
    }
    {
        Hash h("a", std::vector<char>(3, '4'));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "[4,4,4]");
    }
    {
        Hash h("a", static_cast<unsigned char> ('R'));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "82");
    }
    {
        Hash h("a", std::vector<unsigned char>(4, '2'));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "[50,50,50,50]");
    }
    {
        Hash h("a", static_cast<signed char> ('R'));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "82");
    }
    {
        Hash h("a", std::vector<signed char>(4, '2'));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "[50,50,50,50]");
    }
    {
        Hash h("a", short(126));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "126");
    }
    {
        Hash h("a", std::vector<short>(4, 13));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "[13,13,13,13]");
    }
    {
        Hash h("a", int(-42));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "-42");
    }
    {
        Hash h("a", std::vector<int>(1, -42));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "[-42]");
    }
    {
        Hash h("a", static_cast<unsigned int> (42));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "42");
    }
    {
        Hash h("a", std::vector<unsigned int>());
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "[]");
    }
    {
        Hash h("a", static_cast<long long> (-2147483647));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "-2147483647");
    }
    {
        Hash h("a", static_cast<unsigned long long> (0));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "0");
    }
    {
        Hash h("a", static_cast<float> (0.1234567));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "0.1234567");
    }
    {
        Hash h("a", 0.123456789123456);
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "0.123456789123456");
    }
    {
        Hash h("a", std::complex<float>(1.2, 0.5));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "(1.2000000,0.5000000)");
    }
    {
        Hash h("a", std::complex<double>(1.2, 0.5));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "(1.200000000000000,0.500000000000000)");
    }
}

void Hash_Test::testFind() {

    Hash h("a.b.c1.d", 1);
    boost::optional<Hash::Node&> node = h.find("a.b.c1.d");
    if (node) CPPUNIT_ASSERT(node->getValue<int>() == 1);
    else CPPUNIT_ASSERT(true == false);
    node = h.find("a.b.c1.f");
    if (node) CPPUNIT_ASSERT(true == false);
    else CPPUNIT_ASSERT(true == true);
}

void Hash_Test::testAttributes() {
    Hash h("This.is.a.test", 42);
    h.setAttribute("This", "attr1", "someValue");
    CPPUNIT_ASSERT(h.getAttribute<std::string > ("This", "attr1") == "someValue");
    h.setAttribute("This", "attr2", 42);
    CPPUNIT_ASSERT(h.getAttribute<std::string > ("This", "attr1") == "someValue");
    CPPUNIT_ASSERT(h.getAttribute<int>("This", "attr2") == 42);
    h.setAttribute("This", "attr2", 43);
    CPPUNIT_ASSERT(h.getAttribute<std::string > ("This", "attr1") == "someValue");
    CPPUNIT_ASSERT(h.getAttribute<int>("This", "attr2") == 43);
    h.setAttribute("This.is.a.test", "attr1", true);
    CPPUNIT_ASSERT(h.getAttribute<bool>("This.is.a.test", "attr1") == true);
    const Hash::Attributes& attrs = h.getAttributes("This");
    CPPUNIT_ASSERT(attrs.size() == 2);
    CPPUNIT_ASSERT(attrs.get<std::string > ("attr1") == "someValue");
    CPPUNIT_ASSERT(attrs.get<int>("attr2") == 43);
    Hash::Attributes::Node node = attrs.getNode("attr2");
    CPPUNIT_ASSERT(node.getType() == Types::INT32);
}

void Hash_Test::testIteration() {
    Hash h("This", 1, "should", 2, "be", 3, "iterated", 4, "in", 5, "order", 6);
    std::vector<std::string> inOrder;
    for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
        inOrder.push_back(it->getKey());
    }
    CPPUNIT_ASSERT(inOrder[0] == "This");
    CPPUNIT_ASSERT(inOrder[1] == "should");
    CPPUNIT_ASSERT(inOrder[2] == "be");
    CPPUNIT_ASSERT(inOrder[3] == "iterated");
    CPPUNIT_ASSERT(inOrder[4] == "in");
    CPPUNIT_ASSERT(inOrder[5] == "order");
}
