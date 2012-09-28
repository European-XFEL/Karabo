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
    
    h.set("vecString", vector<string>(3, "test"));
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

