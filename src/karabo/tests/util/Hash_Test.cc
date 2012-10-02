/*
 * File:   Hash_Test.cc
 * Author: heisenb
 *
 * Created on Sep 18, 2012, 6:47:59 PM
 */

#include <karabo/util/Hash.hh>

#include "Hash_Test.hh"

using namespace karabo::util;
using namespace std;

CPPUNIT_TEST_SUITE_REGISTRATION(Hash_Test);

Hash_Test::Hash_Test() {
}

Hash_Test::~Hash_Test() {
}

void Hash_Test::testGet() {

    Hash h;
    int* i = h.bindPointer<int>("int");
    *i = 25456;
    int j = h.get<int>("int");
    CPPUNIT_ASSERT(*i == j);
    CPPUNIT_ASSERT(j = 25456);

    vector<int>& v = h.bindReference<vector<int> >("vec");
    v.resize(10);
    for (size_t i = 0; i < v.size(); ++i) {
        v[i] = i;
    }
    vector<int> v1 = h.get<vector<int> >("vec");
    CPPUNIT_ASSERT(v1.size() == 10);
    for (size_t i = 0; i < v1.size(); ++i) {
        CPPUNIT_ASSERT(v1[i] == (int) i);
    }

}

void Hash_Test::testGetTypeAsStringOrId() {

    Hash h;
    h.set("char", 'a');
    CPPUNIT_ASSERT(string("CHAR") == h.getTypeAsString("char"));
    CPPUNIT_ASSERT(Types::CHAR == h.getTypeAsId("char"));

    h.set("unsignedChar", (unsigned char) '6');
    CPPUNIT_ASSERT(string("UINT8") == h.getTypeAsString("unsignedChar"));
    CPPUNIT_ASSERT(Types::UINT8 == h.getTypeAsId("unsignedChar"));

    h.set<signed char>("signedChar", '7');
    CPPUNIT_ASSERT(string("INT8") == h.getTypeAsString("signedChar"));

    h.set("bool", true);
    CPPUNIT_ASSERT(string("BOOL") == h.getTypeAsString("bool"));

    h.set("int", 1);
    CPPUNIT_ASSERT(string("INT32") == h.getTypeAsString("int"));

    h.set("float", 1.0f);
    CPPUNIT_ASSERT(string("FLOAT") == h.getTypeAsString("float"));

    h.set("double", 1.0);
    CPPUNIT_ASSERT(string("DOUBLE") == h.getTypeAsString("double"));

    h.set("string", "Hello World");
    CPPUNIT_ASSERT(string("STRING") == h.getTypeAsString("string"));

    h.set("custom", vector<int>(5, 1));
    CPPUNIT_ASSERT(string("VECTOR_INT32") == h.getTypeAsString("custom"));

    h.set("vecUChar", vector<unsigned char>(5, 255));
    CPPUNIT_ASSERT(string("VECTOR_UINT8") == h.getTypeAsString("vecUChar"));

    h.set("vecChar", vector<char>(5, 'c'));
    CPPUNIT_ASSERT(string("VECTOR_CHAR") == h.getTypeAsString("vecChar"));

    h.set("vecDouble", vector<double>(5, 1.7E+308));
    CPPUNIT_ASSERT(string("DOUBLE") == h.getTypeAsString("double"));

    h.set("vecBool", deque<bool>(5, true));
    CPPUNIT_ASSERT(string("VECTOR_BOOL") == h.getTypeAsString("vecBool"));

    h.set("vecString", vector<string > (3, "test"));
    CPPUNIT_ASSERT(string("VECTOR_STRING") == h.getTypeAsString("vecString"));
}

void Hash_Test::testGetFromPath() {

    Hash h("This.is.a.test", 5);
    CPPUNIT_ASSERT(h.getFromPath<int>("This.is.a.test") == 5);

    Hash hash("a.b.c.myDouble", 5.5);
    CPPUNIT_ASSERT(hash.getFromPath<double>("a.b.c.myDouble") == 5.5);

}

void Hash_Test::testHasFromPath() {

    Hash hash;
    hash.setFromPath("a.b.c.myFloat", -5.7f);

    CPPUNIT_ASSERT(hash.hasFromPath("a") == true);
    CPPUNIT_ASSERT(hash.hasFromPath("a.b") == true);
    CPPUNIT_ASSERT(hash.hasFromPath("a.b.c") == true);
    CPPUNIT_ASSERT(hash.hasFromPath("a.b.c.myFloat") == true);
    CPPUNIT_ASSERT(hash.hasFromPath("a.x.c.myFloat") == false);

}

void Hash_Test::testConvertFromString() {
    Hash h;
    {
        std::string nStr = "Hi, this  ,is, an array of, 6, strings";
        vector<string> ref;
        ref.push_back("Hi");
        ref.push_back("this");
        ref.push_back("is");
        ref.push_back("an array of");
        ref.push_back("6");
        ref.push_back("strings");
        h.set("a", nStr);
        h.convertFromString("a", Types::VECTOR_STRING);
        vector<string> a = h.get<vector<string> >("a");
        for (size_t i = 0; i < a.size(); i++) {
            CPPUNIT_ASSERT(a[i] == ref[i]);
        }
    }
    {
        std::string nStr[] = {"0", "n", "no", "false"};
        for (int i = 0; i < 4; i++) {
            h.set("a", nStr[i]);
            h.convertFromString("a", Types::BOOL);
            bool a = h.get<bool > ("a");
            CPPUNIT_ASSERT(a == false);
        }
    }
    {
        std::string nStr[] = {"1", "y", "yes", "true"};
        for (int i = 0; i < 4; i++) {
            h.set("a", nStr[i]);
            h.convertFromString("a", Types::BOOL);
            bool a = h.get<bool > ("a");
            CPPUNIT_ASSERT(a == true);
        }
    }
    {
        std::string nStr = "60";
        h.set("a", nStr);
        h.convertFromString("a", Types::INT8);
        int8_t a = h.get<int8_t > ("a");
        CPPUNIT_ASSERT(a == 60);
    }
    {
        std::string nStr = "60";
        h.set("a", nStr);
        h.convertFromString("a", Types::INT8);
        signed char a = h.get<signed char>("a");
        CPPUNIT_ASSERT(a == 60);
    }
    {
        std::string nStr = "60";
        h.set("a", nStr);
        h.convertFromString("a", Types::CHAR);
        char a = h.get<char>("a");
        CPPUNIT_ASSERT(a == 60);
    }
    {
        std::string nStr = "60";
        h.set("a", nStr);
        h.convertFromString("a", Types::INT16);
        short a = h.get<short>("a");
        CPPUNIT_ASSERT(a == 60);
    }
    {
        std::string nStr = "60";
        h.set("a", nStr);
        h.convertFromString("a", Types::INT32);
        int a = h.get<int>("a");
        CPPUNIT_ASSERT(a == 60);
    }
    {
        std::string nStr = "60";
        h.set("a", nStr);
        h.convertFromString("a", Types::INT64);
        long long a = h.get<long long>("a");
        CPPUNIT_ASSERT(a == 60);
    }
    {
        std::string nStr = "3.40282e+38";
        float max = 3.40282e+38;
        h.set("a", nStr);
        h.convertFromString("a", Types::FLOAT);
        float a = h.get<float>("a");
        CPPUNIT_ASSERT((abs(a) - max) / max < 1e-5);
    }
    {
        std::string nStr = "(60.0,80.0)";
        h.set("a", nStr);
        h.convertFromString("a", Types::COMPLEX_FLOAT);
        complex<float> a = h.get<complex<float> >("a");
        CPPUNIT_ASSERT((abs(a) - 100.0) < 1e-5);
    }
    {
        std::string nStr = "60.1";
        h.set("a", nStr);
        h.convertFromString("a", Types::DOUBLE);
        double a = h.get<double>("a");
        CPPUNIT_ASSERT((abs(a) - 60.1) < 1e-10);
    }
    {
        std::string nStr = "(-60.0,80.0)";
        h.set("a", nStr);
        h.convertFromString("a", Types::COMPLEX_DOUBLE);
        complex<double> a = h.get<complex<double> >("a");
        CPPUNIT_ASSERT((abs(a) - 100.0) < 1e-5);
    }
    {
        std::string nStr = "Hello World!!!";
        h.set("a", nStr);
        h.convertFromString("a", Types::STRING);
        string a = h.get<string > ("a");
        CPPUNIT_ASSERT(a == "Hello World!!!");
    }
    {
        std::string nStr = "125";
        h.set("a", nStr);
        h.convertFromString("a", Types::UINT8);
        unsigned char a = h.get<unsigned char>("a");
        CPPUNIT_ASSERT(a == 125);
    }
    {
        std::string nStr = "60";
        h.set("a", nStr);
        h.convertFromString("a", Types::UINT16);
        unsigned short a = h.get<unsigned short>("a");
        CPPUNIT_ASSERT(a == 60);
    }
    {
        std::string nStr = "60";
        h.set("a", nStr);
        h.convertFromString("a", Types::UINT32);
        unsigned int a = h.get<unsigned int>("a");
        CPPUNIT_ASSERT(a == 60);
    }
    {
        std::string nStr = "60";
        h.set("a", nStr);
        h.convertFromString("a", Types::UINT64);
        unsigned long long a = h.get<unsigned long long>("a");
        CPPUNIT_ASSERT(a == 60);
    }

}//end testConvertFromString()

void Hash_Test::testGetCastException1() {
    Hash h;
    unsigned long long b = 0;
    h.set("b", "-1");
    b = h.get<unsigned long long>("b");
}

void Hash_Test::testGetCastException2() {
    Hash h;
    unsigned int b = 0;
    h.set("b", "-1");
    b = h.get<unsigned int>("b");
}

void Hash_Test::testGetCastException3() {
    Hash h;
    unsigned short b = 0;
    h.set("b", "-1");
    b = h.get<unsigned short>("b");
}

void Hash_Test::testGetCastException4() {
    Hash h;
    unsigned char b = 0;
    h.set("b", "256");
    b = h.get<unsigned char>("b");
}

void Hash_Test::testConvertFromStringBadLexicalCast1() {
    cout << "double limits: (" << std::numeric_limits<double>::min() << ", "
            << std::numeric_limits<double>::max() << ")" << endl;
    Hash h;
    std::string nStr = "1.7977e+308";
    h.set("b", nStr);
    h.convertFromString("b", Types::DOUBLE);
}

void Hash_Test::testConvertFromStringBadLexicalCast2() {
    cout << "float limits: (" << std::numeric_limits<float>::min() << ", "
            << std::numeric_limits<float>::max() << ")" << endl;
    Hash h;
    std::string nStr = "3.40283e+38";
    h.set("b", nStr);
    h.convertFromString("b", Types::FLOAT);
}

void Hash_Test::testConvertFromStringCastException() {
    Hash h;
    h.set("b", "-5");
    h.convertFromString("b", Types::BOOL);
}