/*
 * File:   Hash_Test.cc
 * Author: heisenb
 *
 * Created on Sep 18, 2012, 6:47:59 PM
 */

#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include <stack>
#include "Hash_Test.hh"
#include "karabo/util/ToLiteral.hh"

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

    }

    {
        Hash h("a.b.c", 1, "b.c", 2.0, "c", 3.f, "d.e", "4", "e.f.g.h", std::vector<unsigned long long> (5, 5), "F.f.f.f.f", Hash("x.y.z", 99));
        CPPUNIT_ASSERT(h.empty() == false);
        CPPUNIT_ASSERT(h.size() == 6);
        CPPUNIT_ASSERT(h.get<int>("a.b.c") == 1);
        CPPUNIT_ASSERT(h.get<double>("b.c") == 2.0);
        CPPUNIT_ASSERT(h.get<float>("c") == 3.0);
        CPPUNIT_ASSERT(h.get<string > ("d.e") == "4");
        CPPUNIT_ASSERT(h.get<std::vector<unsigned long long> >("e.f.g.h")[0] == 5);
        CPPUNIT_ASSERT(h.get<Hash > ("F.f.f.f.f").get<int>("x.y.z") == 99);
        CPPUNIT_ASSERT(h.get<int>("F.f.f.f.f.x.y.z") == 99);

        // Check 'flatten'
        Hash flat;
        Hash::flatten(h, flat);

        CPPUNIT_ASSERT(flat.empty() == false);
        CPPUNIT_ASSERT(flat.size() == 6);
        CPPUNIT_ASSERT(flat.get<int>("a.b.c", 0) == 1);
        CPPUNIT_ASSERT(flat.get<double>("b.c", 0) == 2.0);
        CPPUNIT_ASSERT(flat.get<float>("c", 0) == 3.0);
        CPPUNIT_ASSERT(flat.get<string > ("d.e", 0) == "4");
        CPPUNIT_ASSERT(flat.get<std::vector<unsigned long long> >("e.f.g.h", 0)[0] == 5);
        CPPUNIT_ASSERT(flat.get<int>("F.f.f.f.f.x.y.z", 0) == 99);

        Hash tree;
        flat.unflatten(tree);

        CPPUNIT_ASSERT(tree.empty() == false);
        CPPUNIT_ASSERT(tree.size() == 6);
        CPPUNIT_ASSERT(tree.get<int>("a.b.c") == 1);
        CPPUNIT_ASSERT(tree.get<double>("b.c") == 2.0);
        CPPUNIT_ASSERT(tree.get<float>("c") == 3.0);
        CPPUNIT_ASSERT(tree.get<string > ("d.e") == "4");
        CPPUNIT_ASSERT(tree.get<std::vector<unsigned long long> >("e.f.g.h")[0] == 5);
        CPPUNIT_ASSERT(tree.get<Hash > ("F.f.f.f.f").get<int>("x.y.z") == 99);
        CPPUNIT_ASSERT(tree.get<int>("F.f.f.f.f.x.y.z") == 99);

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
        CPPUNIT_ASSERT(h.has("a.b.c1.d") == true);
        CPPUNIT_ASSERT(h.get<Hash > ("a").has("b.c1") == true);

        h.set("a.b.c2.d", "1");
        CPPUNIT_ASSERT(h.get<Hash > ("a").has("b") == true);
        CPPUNIT_ASSERT(h.get<Hash > ("a.b").has("c1") == true);
        CPPUNIT_ASSERT(h.get<Hash > ("a.b").has("c2") == true);
        CPPUNIT_ASSERT(h.get<Hash > ("a.b").has("c2.d") == true);
        CPPUNIT_ASSERT(h.get<Hash > ("a.b").is<string > ("c2.d") == true);
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
        CPPUNIT_ASSERT(h.get<Hash>("a").has("b") == true);
        CPPUNIT_ASSERT(h.get<Hash>("a").size() == 1);
        CPPUNIT_ASSERT(h.is<std::vector<Hash> >("a.b") == true);
        CPPUNIT_ASSERT(h.has("a.b") == true);
        CPPUNIT_ASSERT(h.get<std::vector<Hash> >("a.b").size() == 3);
        CPPUNIT_ASSERT(h.get<int>("a.b[0].a") == 1);
        CPPUNIT_ASSERT(h.get<Hash>("a.b[1]").empty() == true);
        CPPUNIT_ASSERT(h.get<string>("a.b[2].a") == "1");
        CPPUNIT_ASSERT(h.get<std::vector<Hash> >("a.b")[0].is<int>("a") == true);
        CPPUNIT_ASSERT(h.get<std::vector<Hash> >("a.b")[1].empty() == true);
        CPPUNIT_ASSERT(h.get<std::vector<Hash> >("a.b")[2].is<string>("a") == true);

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
        CPPUNIT_ASSERT(h.get<Hash > ("a").is<Hash > ("b") == true);
        CPPUNIT_ASSERT(h.is<int>("a.b.c") == true);
        CPPUNIT_ASSERT(h.has("a.b") == true);
        CPPUNIT_ASSERT(h.has("a.b.c.d") == false);
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
        Hash h("a", "1");
        h.get("a", s);
        CPPUNIT_ASSERT(s == "1");
        h.get<string > ("a") = "2";
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


}


void Hash_Test::testGetAs() {

    {
        Hash h("a", true);
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "1");
        CPPUNIT_ASSERT(h.getAs<int > ("a") == 1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, h.getAs<double > ("a"), 0.00001);
        CPPUNIT_ASSERT(h.getAs<char > ("a") == '1');
    }

    {
        Hash h("a", true);
        h.setAttribute("a", "a", true);
        CPPUNIT_ASSERT(h.getAttributeAs<string > ("a", "a") == "1");
        CPPUNIT_ASSERT(h.getAttributeAs<int > ("a", "a") == 1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, h.getAttributeAs<double > ("a", "a"), 0.00001);
        CPPUNIT_ASSERT(h.getAttributeAs<char > ("a", "a") == '1');
        boost::any& any = h.getAttributeAsAny("a", "a");
        CPPUNIT_ASSERT(boost::any_cast<bool>(any) == true);
        h.setAttribute("a", "b", 12);
        h.setAttribute("a", "c", 1.23);
        Hash::Attributes attrs = h.getAttributes("a");
        Hash g("Z.a.b.c", "Value");
        g.setAttributes("Z.a.b.c", attrs);
        CPPUNIT_ASSERT(g.getAttributeAs<string > ("Z.a.b.c", "a") == "1");
        CPPUNIT_ASSERT(g.getAttributeAs<int > ("Z.a.b.c", "a") == 1);
        CPPUNIT_ASSERT_DOUBLES_EQUAL(1.0, g.getAttributeAs<double > ("Z.a.b.c", "a"), 0.00001);
    }

    {
        Hash h("a", std::vector<bool>(4, false));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "0,0,0,0");
        int tmp = h.getAs<int, std::vector > ("a")[3];
        CPPUNIT_ASSERT(tmp == 0);
    }
    {
        Hash h("a", char('R'));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "R");
    }
    {
        // Assumes vector to contain binary data and does a base64 encode
        Hash h("a", std::vector<unsigned char>(3, '4'));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "NDQ0");
    }
    {
        // Assumes vector to contain binary data and does a base64 encode
        Hash h("a", std::vector<char>(3, '4'));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "NDQ0");
    }
    {
        // Assumes vector to contain printable (ASCII) characters
        Hash h("a", std::vector<signed char>(3, '4'));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "52,52,52");
    }
    {
        Hash h("a", static_cast<unsigned char> ('R'));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "82");
    }
    {
        Hash h("a", static_cast<signed char> ('R'));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "82");
    }
    {
        Hash h("a", std::vector<signed char>(4, '2'));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "50,50,50,50");
    }
    {
        Hash h("a", short(126));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "126");
    }
    {
        Hash h("a", std::vector<short>(4, 13));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "13,13,13,13");
    }
    {
        Hash h("a", int(-42));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "-42");
    }
    {
        Hash h("a", std::vector<int>(1, -42));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "-42");
    }
    {
        Hash h("a", static_cast<unsigned int> (42));
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "42");
    }
    {
        Hash h("a", std::vector<unsigned int>());
        CPPUNIT_ASSERT(h.getAs<string > ("a") == "");
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
    {
        try {
            const int arr[6] = {0, 1, 2, 3, 4, 5};
            const int* ptr = &arr[0];
            Hash h("a", std::pair<const int*, size_t>(ptr, 6));
            CPPUNIT_ASSERT(h.getAs<string > ("a") == "0,1,2,3,4,5");
            ostringstream oss;
            oss << h;            
        } catch (Exception& e) {
            cerr << e;
            KARABO_RETHROW(e);
        }
    }
}


void Hash_Test::testFind() {

    {
        Hash h("a.b.c1.d", 1);
        boost::optional<Hash::Node&> node = h.find("a.b.c1.d");
        if (node) CPPUNIT_ASSERT(node->getValue<int>() == 1);
        else CPPUNIT_ASSERT(false);
        node = h.find("a.b.c1.f");
        if (node) CPPUNIT_ASSERT(false);
        else CPPUNIT_ASSERT(true);
    }

    {
        Hash h("a.b.c", "1");
        boost::optional<Hash::Node&> node = h.find("a.b.c");
        if (node) node->setValue(2);
        CPPUNIT_ASSERT(h.get<int>("a.b.c") == 2);
        node = h.find("a.b.c", '/');
        if (node) CPPUNIT_ASSERT(false);
    }
}


void Hash_Test::testAttributes() {

    {
        Hash h("a.b.a.b", 42);
        h.setAttribute("a", "attr1", "someValue");
        CPPUNIT_ASSERT(h.getAttribute<std::string > ("a", "attr1") == "someValue");

        h.setAttribute("a", "attr2", 42);
        CPPUNIT_ASSERT(h.getAttribute<std::string > ("a", "attr1") == "someValue");
        CPPUNIT_ASSERT(h.getAttribute<int>("a", "attr2") == 42);

        h.setAttribute("a", "attr2", 43);
        CPPUNIT_ASSERT(h.getAttribute<std::string > ("a", "attr1") == "someValue");
        CPPUNIT_ASSERT(h.getAttribute<int>("a", "attr2") == 43);

        h.setAttribute("a.b.a.b", "attr1", true);
        CPPUNIT_ASSERT(h.getAttribute<bool>("a.b.a.b", "attr1") == true);

        const Hash::Attributes& attrs = h.getAttributes("a");
        CPPUNIT_ASSERT(attrs.size() == 2);
        CPPUNIT_ASSERT(attrs.get<std::string > ("attr1") == "someValue");
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

    h.erase("be"); // Remove
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
        h.getKeys(tmp); // fill set by keys
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
        h.getKeys(tmp); // fill vector by keys
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
        h.getKeys(tmp); // fill list by keys
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
        h.getKeys(tmp); // fill deque by keys
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
        std::vector<std::string> paths;
        h.getPaths(paths);
        CPPUNIT_ASSERT(paths.size() == 0);
    }
}


void Hash_Test::testMerge() {

    Hash h1("a", 1,
            "b", 2,
            "c.b[0].g", 3,
            "c.c[0].d", 4,
            "c.c[1]", Hash("a.b.c", 6),
            "d.e", 7
            );

    Hash h2("a", 21,
            "b.c", 22,
            "c.b[0]", Hash("key", "value"),
            "c.b[1].d", 24,
            "e", 27
            );

    h1 += h2;

    CPPUNIT_ASSERT(h1.has("a"));
    CPPUNIT_ASSERT(h1.get<int>("a") == 21);
    CPPUNIT_ASSERT(h1.has("b"));
    CPPUNIT_ASSERT(!h1.has("c.b.d"));
    CPPUNIT_ASSERT(h1.has("c.b[0]"));
    CPPUNIT_ASSERT(h1.has("c.b[1]"));
    CPPUNIT_ASSERT(h1.has("c.b[2]"));
    CPPUNIT_ASSERT(h1.get<int>("c.b[2].d") == 24);
    CPPUNIT_ASSERT(h1.has("c.c[0].d"));
    CPPUNIT_ASSERT(h1.has("c.c[1].a.b.c"));
    CPPUNIT_ASSERT(h1.has("d.e"));
    CPPUNIT_ASSERT(h1.has("e"));

    Hash h3 = h1;

    CPPUNIT_ASSERT(similar(h1, h3));
}


void Hash_Test::testSubtract() {
    Hash h1("a", 1,
            "b", 2,
            "c.b[0].g", 3,
            "c.c[0].d", 4,
            "c.c[1]", Hash("a.b.c", 6),
            "d.e", 7
            );

    Hash h2("a", 21,
            "b.c", 22,
            "c.b[0]", Hash("key", "value"),
            "c.b[1].d", 24,
            "e", 27
            );
    h1 += h2;
    h1 -= h2;
    CPPUNIT_ASSERT(h1.has("a") == false);
    CPPUNIT_ASSERT(h1.get<Hash>("b").empty() == true);
    CPPUNIT_ASSERT(h1.get<int>("c.b[0].g") == 3);
    CPPUNIT_ASSERT(h1.get<string>("c.b[1].key") == "value");
    CPPUNIT_ASSERT(h1.get<int>("c.b[2].d") == 24);
    CPPUNIT_ASSERT(h1.get<int>("c.c[0].d") == 4);
    CPPUNIT_ASSERT(h1.get<int>("c.c[1].a.b.c") == 6);
    CPPUNIT_ASSERT(h1.get<int>("d.e") == 7);

    Hash h3("a.b.c", 1,
            "a.b.d", 2,
            "a.c.d", 22,
            "b.c.d", 33,
            "c.d.e", 44,
            "c.e.f", 55
            );
    Hash h4("a.b", Hash(),
            "c", Hash());
    h3 -= h4;
    CPPUNIT_ASSERT(h3.has("a.b") == false);
    CPPUNIT_ASSERT(h3.has("c") == false);
    CPPUNIT_ASSERT(h3.get<int>("a.c.d") == 22);
    CPPUNIT_ASSERT(h3.get<int>("b.c.d") == 33);
}

namespace helper {

    class Helper {

    public:


        Helper() {
        };


        virtual ~Helper() {
        };


        bool operator()(const karabo::util::Hash::Node& node) {
            return eval(node);
        }

        virtual bool eval(const karabo::util::Hash::Node& node) = 0;
    };

    bool dfs(const karabo::util::Hash& hash, Helper& helper);
    bool dfs(const std::vector<karabo::util::Hash>& hash, Helper& helper);
    bool dfs(const karabo::util::Hash::Node& node, Helper& helper);


    bool dfs(const karabo::util::Hash& hash, Helper& helper) {
        if (hash.empty())
            return false;

        for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
            if (!dfs(*it, helper)) return false;
        }
        return true;
    }


    bool dfs(const std::vector<karabo::util::Hash>& hash, Helper& helper) {
        if (hash.empty())
            return false;

        for (size_t i = 0; i < hash.size(); ++i) {
            if (!dfs(hash[i], helper)) return false;
        }
        return true;
    }


    bool dfs(const karabo::util::Hash::Node& node, Helper& helper) {
        helper(node);

        if (node.getType() == Types::HASH) {
            return dfs(node.getValue<Hash > (), helper);
        }
        if (node.getType() == Types::VECTOR_HASH) {
            return dfs(node.getValue<std::vector<Hash> >(), helper);
        }
        return true;
    }

    template<class V, class E>
    class Visitor {

    public:


        Visitor() {
        };


        ~Visitor() {
        };


        E operator()(V& visitable) {
            E evaluator;
            visitable.visit(evaluator);
            return evaluator;
        }


        bool operator()(V& visitable, E& evaluator) {
            return visitable.visit(evaluator);
        }


        template<class Helper>
        static bool visit__(const karabo::util::Hash& hash, Helper& helper) {
            if (hash.empty())
                return false;

            for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                if (!visit__(*it, helper)) return false;
            }
            return true;
        }


        template<class Helper>
        static bool visit__(const std::vector<karabo::util::Hash>& hash, Helper& helper) {
            if (hash.empty())
                return false;

            for (size_t i = 0; i < hash.size(); ++i) {
                if (!visit__(hash[i], helper)) return false;
            }
            return true;
        }


        template<class Helper>
        static bool visit__(const karabo::util::Hash::Node& node, Helper& helper) {
            helper(node);

            if (node.getType() == Types::HASH) {
                return visit__(node.getValue<Hash > (), helper);
            }
            if (node.getType() == Types::VECTOR_HASH) {
                return visit__(node.getValue<std::vector<Hash> >(), helper);
            }
            return true;
        }
    };


}

class Counter : public helper::Helper {

public:


    Counter() : m_counter(0) {

    }


    bool eval(const karabo::util::Hash::Node& node) {
        if (node.getType() == Types::VECTOR_HASH) {
            m_counter += node.getValue<std::vector<Hash> >().size();
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


    Concat() : m_concat("") {

    }


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
                m_stream << " " << ait->getKey() << "=\"" << ait->getValueAs<string>() /*<< " " << Types::to<ToLiteral>(ait->getType())*/ << "\"";
            }
        }

        switch (node.getType()) {
            case Types::HASH:
            case Types::VECTOR_HASH:
                fill[indent] = ' ';
                indent += 2;
                fill[indent] = 0;
                break;
        };
    }


    bool eval(const karabo::util::Hash::Node& node) {
        Types::ReferenceType type = node.getType();
        switch (type) {
            case Types::HASH: m_stream << " +";
                indices.push(-1);
                break;
            case Types::VECTOR_HASH: m_stream << " @";
                indices.push(0);
                break;
            case Types::SCHEMA: m_stream << " => " << node.getValue<karabo::util::Schema>();
                break;
            default:
                if (Types::isPointer(type)) {// TODO Add pointer types
                    m_stream << " => xxx " << Types::to<ToLiteral>(type);
                } else {
                    m_stream << " => " << node.getValueAs<string>() << " " << Types::to<ToLiteral>(type);
                }
        }
        m_stream << '\n';
        return true;
    }


    void post(const karabo::util::Hash::Node & node) {
        switch (node.getType()) {
            case Types::HASH:
            case Types::VECTOR_HASH:
                fill[indent] = ' ';
                indent -= 2;
                fill[indent] = 0;
                indices.pop();
                break;
        };
    }


    const std::ostringstream & getResult() {
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


    ~Flatten() {
    };


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
        }
        return true;
    }


    void post(const karabo::util::Hash::Node & node) {
        switch (node.getType()) {
            case Types::HASH:
            case Types::VECTOR_HASH:
                prefix.pop();
                indices.pop();
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


    ~Paths() {
    };


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
        }
        return true;
    }


    void post(const karabo::util::Hash::Node & node) {
        switch (node.getType()) {
            case Types::HASH:
            case Types::VECTOR_HASH:
                prefix.pop();
                indices.pop();
                break;
        };
    }


    const std::vector<std::string> & getResult() {
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
        Hash h3("a", 21,
                "b.c", 22,
                "c.b[0]", Hash("key", "value"),
                "c.b[1].d", 24,
                "e", 23
                );
        h3.setAttribute("a", "at0", "value0");

        Hash h2("a", 21,
                "b.c", 22,
                "c.b[0]", Hash("key", "value"),
                "c.b[1].d", h3,
                "e", 27
                );
        h2.setAttribute("a", "at1", "value1");

        Hash h1("a", 1,
                "b", 2,
                "c.b[0].g", h2,
                "c.c[0].d", h2,
                "c.c[1]", Hash("a.b.c", h2),
                "d.e", 7
                );

        h1.setAttribute("a", "at2", "value2");

        Counter counter;
        helper::dfs(h1, counter);

        Concat concat;
        helper::dfs(h1, concat);

        Serializer serializer;
        helper::dfs(h1, serializer);

        //std::clog << "Count    1: " << counter.getResult() << std::endl;
        //std::clog << "Concate  1: " << concat.getResult() << std::endl;
        //std::clog << "Serialize1: " << serializer.getResult().str() << std::endl;

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

        //helper::Visitor<Hash, Counter> count;
        //std::clog << "Count 2 : " << count(h1).getResult() << std::endl;
        //std::clog << "Count 3 : " << helper::Visitor<Hash, Counter>()(h1).getResult() << std::endl;
        //std::clog << "Count H : " << karabo::util::counter(h1) << std::endl;
        //std::clog << "Count V : " << c2.getResult() << std::endl;
    }
}


