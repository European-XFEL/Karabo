/*
 * $Id: testHash.cc 6750 2012-07-11 16:02:01Z esenov $
 *
 * File:   testHash.hh
 * Author: <your.email@xfel.eu>
 *
 * Created on August 13, 2010, 3:28 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <cstdlib>
#include <iostream>
#include <assert.h>
#include <string>
#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

#include "../Exception.hh"
#include "../Hash.hh"

using namespace std;
using namespace exfel::util;
using namespace boost::assign;

int testHash(int argc, char** argv) {

    cout << "Running Test: testHash..." << endl;
    {
        Hash h;

        int* i = h.bindPointer<int>("int");

        *i = 25456;

        cout << h.get<int>("int") << endl;
        int j = h.get<int>("int");
        assert(*i == j);
        assert(j = 25456);

        std::vector<int>& v = h.bindReference<std::vector<int> >("vec");
        v.resize(10);
        for (size_t i = 0; i < v.size(); ++i) {
            v[i] = i;
        }
        vector<int> v1 = h.get<vector<int> >("vec");
        assert(v1.size() == 10);
        for (size_t i = 0; i < v1.size(); ++i) {
            assert(v1[i] == (int) i);
        }
        cout << "SEQENCE: " << String::sequenceToString(h.get<vector<int> >("vec")) << endl;
    }
    /***********************************
     *           Setters               *
     ***********************************/

    Hash h;
    h.set("char", 'a');
    h.set("unsignedChar", (unsigned char) '6');
    h.set<signed char>("signedChar", '7');
    h.set("bool", true);
    h.set("int", 1);
    h.set("float", 1.0f);
    h.set("double", 1.0);
    h.set("string", "Hello World");
    h.set("custom", vector<int>(5, 1));
    h.set("vuchar", vector<unsigned char>(5, 255));
    h.set("vecChar", vector<char>(5, 'c'));

    vector<double> vecDouble;
    vecDouble += 1.7E-308, 1.7E+308;
    h.set("vecDouble", vecDouble);

    h.set("vecbool", deque<bool>(5, true));

    vector<signed short> signShortVec;
    signShortVec += -1, -123, 0, 12678, +10000;
    h.set("vecInt16", signShortVec);

    vector<unsigned short> unsignShortVec;
    unsignShortVec += 0, 12678, 10000;
    h.set("vecUInt16", unsignShortVec);

    vector<unsigned int> unsignIntVec;
    unsignIntVec += 0, 1, 100000, 0xffffffff;
    h.set("vecUInt32", unsignIntVec);

    vector<float> floatVec;
    floatVec += 3.4E-38, 3.4E+38;
    h.set("vecFloat", floatVec);

    boost::filesystem::path pathFile("/path/to/file.txt");
    h.set("pathToFile", pathFile);

    h.set("self", h);

    try {
        h.set("boolArray", vector<bool>(10, true)); // This is unsupported
        assert(false == true);
    } catch (const NotSupportedException& e) {
        // Ok, expected
    }

    assert(string("BOOL") == h.getTypeAsString("bool"));
    assert(string("INT32") == h.getTypeAsString("int"));
    assert(string("FLOAT") == h.getTypeAsString("float"));
    assert(string("DOUBLE") == h.getTypeAsString("double"));
    assert(string("STRING") == h.getTypeAsString("string"));
    assert(string("CHAR") == h.getTypeAsString("char"));
    assert(string("INT8") == h.getTypeAsString("signedChar"));
    assert(string("UINT8") == h.getTypeAsString("unsignedChar"));
    assert(string("VECTOR_INT32") == h.getTypeAsString("custom"));
    assert(string("VECTOR_DOUBLE") == h.getTypeAsString("vecDouble"));
    assert(string("VECTOR_FLOAT") == h.getTypeAsString("vecFloat"));
    assert(string("VECTOR_BOOL") == h.getTypeAsString("vecbool"));
    assert(string("VECTOR_INT16") == h.getTypeAsString("vecInt16"));
    assert(string("VECTOR_UINT16") == h.getTypeAsString("vecUInt16"));
    assert(string("VECTOR_UINT32") == h.getTypeAsString("vecUInt32"));
    assert(string("VECTOR_CHAR") == h.getTypeAsString("vecChar"));
    assert(string("HASH") == h.getTypeAsString("self"));

    cout << h << endl;
    { // getTypeAsId, getTypeAsString

        Hash h1;
        char ch = 'b';
        int8_t a = 1;
        signed char b = 2;
        unsigned char c = 3;
        short d = 4;
        unsigned short e = 5;
        int f = 6;
        unsigned int g = 7;
        long long h = 8;
        unsigned long long i = 9;
        float j = 10;
        double k = 11;
        complex<float> l(12, 12);
        complex<double> m(13, 13);
        bool n = true;
        boost::filesystem::path pathtofile("a/b/some_dir/file.txt");

        h1.set("ch", ch);
        h1.set("a", a);
        h1.set("b", b);
        h1.set("c", c);
        h1.set("d", d);
        h1.set("e", e);
        h1.set("f", f);
        h1.set("g", g);
        h1.set("h", h);
        h1.set("i", i);
        h1.set("j", j);
        h1.set("k", k);
        h1.set("l", l);
        h1.set("m", m);
        h1.set("n", n);
        h1.set("p", pathtofile);

        cout << "h1: " << h1 << endl;

        assert(h1.getTypeAsId("ch") == Types::CHAR);
        assert(h1.getTypeAsId("a") == Types::INT8);
        assert(h1.getTypeAsString("a") == "INT8");
        assert(h1.getTypeAsId("b") == Types::INT8);
        assert(h1.getTypeAsString("b") == "INT8");
        assert(h1.getTypeAsId("c") == Types::UINT8);
        assert(h1.getTypeAsString("c") == "UINT8");
        assert(h1.getTypeAsId("d") == Types::INT16);
        assert(h1.getTypeAsString("d") == "INT16");
        assert(h1.getTypeAsId("e") == Types::UINT16);
        assert(h1.getTypeAsString("e") == "UINT16");
        assert(h1.getTypeAsId("f") == Types::INT32);
        assert(h1.getTypeAsString("f") == "INT32");
        assert(h1.getTypeAsId("g") == Types::UINT32);
        assert(h1.getTypeAsString("g") == "UINT32");
        assert(h1.getTypeAsId("h") == Types::INT64);
        assert(h1.getTypeAsString("h") == "INT64");
        assert(h1.getTypeAsId("i") == Types::UINT64);
        assert(h1.getTypeAsString("i") == "UINT64");
        assert(h1.getTypeAsId("j") == Types::FLOAT);
        assert(h1.getTypeAsString("j") == "FLOAT");
        assert(h1.getTypeAsId("k") == Types::DOUBLE);
        assert(h1.getTypeAsString("k") == "DOUBLE");
        assert(h1.getTypeAsId("l") == Types::COMPLEX_FLOAT);
        assert(h1.getTypeAsString("l") == "COMPLEX_FLOAT");
        assert(h1.getTypeAsId("m") == Types::COMPLEX_DOUBLE);
        assert(h1.getTypeAsString("m") == "COMPLEX_DOUBLE");
        assert(h1.getTypeAsId("n") == Types::BOOL);
        assert(h1.getTypeAsString("n") == "BOOL");
        assert(h1.getTypeAsId("p") == Types::PATH);
        assert(h1.getTypeAsString("p") == "PATH");
    }

    char char1;
    unsigned char char2;
    signed char char3;
    bool u1;
    int u2;
    float u3;
    double u4;
    string u5;
    vector<int> u6;
    vector <unsigned char> u7;
    h.get("char", char1);
    h.get("unsignedChar", char2);
    h.get("signedChar", char3);
    h.get("bool", u1);
    h.get("int", u2);
    h.get("float", u3);
    h.get("double", u4);
    h.get("string", u5);
    h.get("custom", u6);
    h.get("vuchar", u7);
    assert(char1 == 'a');
    assert(char2 == '6');
    assert(char3 == '7');
    assert(u1 == true);
    assert(u2 == 1);
    assert(u3 == 1.0);
    assert(u4 == 1.0);
    assert(u5 == "Hello World");
    assert(u6[4] == 1);
    assert(u7[0] == 255);

    //bool& gasd = h.get<bool>("bool");
    u2 = h.get<int>("int");

    // This must throw
    //    try {
    //        double tmp = h.get<double>("int");
    ////        cout << tmp;
    ////        assert(true == false);
    //    } catch (const exfel::util::CastException& e) {
    //        cout << "The following exception is thrown by purpose:" << endl;
    //        cout << e;
    //    }

    u3 = h.get<float>("float");
    u4 = h.get<double>("double");
    u5 = h.get<string > ("string");
    u6 = h.get<vector<int> >("custom");
    assert(h.get<bool>("bool") == true);
    assert(u2 == 1);
    assert(u3 == 1.0);
    assert(u4 == 1.0);
    assert(u5 == "Hello World");
    assert(u6[4] == 1);

    {// test append function
        Hash h1, h2;
        h1.set("a", 1);
        h1.set("b", 2);
        h1.set("c", 3);

        h2.set("b", 33);
        h2.set("d", 4);

        h2.append(h1);

        assert(h2.get<int>("a") == 1);
        assert(h2.get<int>("b") == 2);
        assert(h2.get<int>("c") == 3);
        assert(h2.get<int>("d") == 4);
    }

    {// test 'append' and 'update' functions
        cout << "TEST 'append' and 'update' functions" << endl;
        Hash h1, h2;
        h1.setFromPath("a.b.c", 1);
        h1.setFromPath("a.b.d", 2);
        h1.setFromPath("a.b.e", 3);

        h2.setFromPath("a.b.f", 4);
        h2.setFromPath("a.b.d", 22);

        h1.append(h2);

        assert(h1.getFromPath<int>("a.b.d") == 22); //value of key a.b.d changed to 22
        assert(h1.getFromPath<int>("a.b.f") == 4); //new key a.b.f added, with value 4

        //leaves a.b.c and a.b.e do not exist any more in h1 (as key a.b from h2 overwrites it)
        try {
            h1.getFromPath<int>("a.b.c");
            return 1;
        } catch (...) {
            cout << "Exception as expected: there is no key 'a.b.c' in h1" << endl;
        }
        try {
            h1.getFromPath<int>("a.b.e");
            return 1;
        } catch (...) {
            cout << "Exception as expected: there is no key 'a.b.e' in h1" << endl;
        }

        //testing update function (consider hash h3 to be the same as original h1) 
        Hash h3;
        h3.setFromPath("a.b.c", 1);
        h3.setFromPath("a.b.d", 2);
        h3.setFromPath("a.b.e", 3);

        h2.setFromPath("a.b.f", 4);
        h2.setFromPath("a.b.d", 22);

        h3.update(h2);

        assert(h3.getFromPath<int>("a.b.c") == 1); //key a.b.c remains unchanged
        assert(h3.getFromPath<int>("a.b.d") == 22); //value of key a.b.d changed to 22
        assert(h3.getFromPath<int>("a.b.e") == 3); //key a.b.e remains unchanged
        assert(h3.getFromPath<int>("a.b.f") == 4); //new key a.b.f added, with value 4
        cout << endl;
    }


    // Try transforming from string
    //    {
    //        Hash h("aDouble", "5.32156", "aBoolean", "true", "anInt", "4", "A vector of boolean", "y,yes,1,true,n,no,false,0");
    //        double d = h.convertFromString<double>("aDouble");
    //        assert(String::toString(d) == String::toString(5.32156));
    //        deque<bool> deq = h.convertFromString < deque<bool> >("A vector of boolean");
    //        for (size_t i = 0; i < deq.size(); ++i) {
    //            if (i <= 3) {
    //                assert(deq[i] == true);
    //            } else {
    //                assert(deq[i] == false);
    //            }
    //        }
    //    }

    // Test setFromPath/getFromPath stuff
    {
        try {

            // Setting up from constructor
            {
                Hash hash("Simple", 3.2);
                // C++ interprets the 3.2 above intrinsically as "double".
                // If we want to get it back with get<> we have to be accurate in the type.
                assert(hash.get<double>("Simple") == 3.2);
                // This does not work
                try {
                    hash.get<float>("Simple");
                    assert(true == false);
                } catch (...) {
                    // Ok, expecting this exception here
                    //RETHROW
                }
                // If you want a more intelligent behaviour use this
                try {
                    float aFloat = hash.getNumeric<float>("Simple");
                    cout << aFloat << endl;
                    //assert(aFloat == 3.2);
                } catch (...) {
                    RETHROW
                }
                Hash hash1("A.b", "la", "B.i.g.Brother", 2, "C", vector<double>(5, 2.5));
                cout << "TEST hash1 :" << endl;
                cout << hash1;

                vector<int> vecint;
                vecint += 1, 5, 33, 100, 200, 555;
                vector<double> vecdouble;
                vecdouble += 1.5, 33.1, 200.2, 555.5;
                deque<bool> vecbool;
                vecbool += true, 1, false, 0;
                Hash hash2("VectInt", vecint, "VectDouble", vecdouble, "VectBool", vecbool);
                cout << "TEST hash2 :" << endl;
                cout << hash2;

            }

            cout << "\n*** START  testing hasFromHash, isFromPath, eraseFromPath ***\n";
            Hash hash;

            // Creating section as needed (like mkdir -p)
            hash.setFromPath("a.b.c.myInt", 44);
            assert(hash.getFromPath<int>("a.b.c.myInt") == 44);

            assert(hash.hasFromPath("a") == true);
            assert(hash.hasFromPath("a.b") == true);
            assert(hash.hasFromPath("a.b.c") == true);
            assert(hash.hasFromPath("a.b.c.myInt") == true);

            hash.setFromPath("a.b.c.myDouble", 5.0);
            hash.getFromPath<double>("a.b.c.myDouble");

            hash.setFromPath("a.b.c.myFloat", -5.7f);
            hash.getFromPath<float>("a.b.c.myFloat");

            hash.setFromPath("a.b.c.myFloatNext", 10.0e11f);
            hash.getFromPath<float>("a.b.c.myFloatNext");
            assert(hash.hasFromPath("a.b.c.myFloatNext") == true);
            assert(hash.hasFromPath("a.b.qqq.myFloatNext") == false);
            assert(hash.isFromPath("a.b.c.myFloatNext", Types::FLOAT) == true);
            assert(hash.isFromPath("a.b.c", Types::HASH) == true);
            assert(hash.isFromPath("a.b.qqq", Types::HASH) == false);



#if defined(_WIN32)
            hash.setFromPath("a.b.c.myFloatInfin", 10.0e37f);
#else
            hash.setFromPath("a.b.c.myFloatInfin", float(10.0e123));
#endif
            hash.getFromPath<float>("a.b.c.myFloatInfin");

            // Changing separator in address
            assert(hash.getFromPath<int>("a&b&c&myInt", "&") == 44);
            assert(hash.getFromPath<int>("a/b/c/myInt", "/") == 44);
            assert(hash.hasFromPath("a/b/c/myInt", "/") == true);
            assert(hash.hasFromPath("a/b/qqq/myInt", "/") == false);
            assert(hash.isFromPath("a/b/c/myInt", Types::INT32, "/") == true);
            assert(hash.isFromPath("a/b/qqq/myInt", Types::INT32, "/") == false);


            // Any values can be assigned, behavior is always map like -> no overwrite for different keys
            hash.setFromPath("a.b.c.myString", "Parrot");
            const Hash& c = hash.getFromPath<Hash > ("a.b.c");
            assert(c.has("myInt"));
            assert(c.has("myString"));
            assert(c.get<string > ("myString") == "Parrot");
            // Put empty hash c1 under a.b
            hash.setFromPath("a.b.c1"); // => setFromPath("a.b.c1", Hash());
            assert(c.has("myInt"));
            assert(c.has("myString"));
            assert(c.get<string > ("myString") == "Parrot");
            // Copy of sub-tree
            hash.setFromPath<Hash > ("a.b.c1", hash.getFromPath<Hash > ("a.b.c"));
            // Can use all std::map functionality
            hash.getFromPath<Hash > ("a.b").erase("c");
            const Hash& b = hash.getFromPath<Hash > ("a.b");
            assert(!b.has("c"));
            const Hash& c1 = hash.getFromPath<Hash > ("a.b.c1");
            assert(c1.has("myInt"));
            assert(c1.has("myString"));
            assert(c1.get<string > ("myString") == "Parrot");
            
            assert(hash.eraseFromPath("a&b&c1&myInt", "&") == 1);
            assert(hash.eraseFromPath("a/b/qqq", "/") == 0);

            {
                Hash hash;
                // Array stuff
                hash.setFromPath("shapes[0].circle.color", "blue");
                assert(hash.hasFromPath("shapes[0]"));
                assert(hash.hasFromPath("shapes[0].circle"));
                assert(hash.hasFromPath("shapes[0].circle.color"));
                hash.setFromPath("shapes[ 1 ].circle.color", "red");
                assert(hash.hasFromPath("shapes[1]"));
                assert(hash.hasFromPath("shapes[1].circle"));
                assert(hash.hasFromPath("shapes[1].circle.color"));
                assert(hash.isFromPath("shapes[1]", Types::HASH));
                assert(hash.isFromPath("shapes[1].circle", Types::HASH));
                assert(hash.isFromPath("shapes[1].circle.color", Types::STRING));


                hash.setFromPath("shapes[next].circle.color", "green");
                const vector<Hash>& shapes = hash.get<vector<Hash> >("shapes");
                assert(shapes.size() == 3);
                assert(shapes[0].has("circle"));
                assert(shapes[0].getFromPath<string > ("circle.color") == "blue");
                assert(hash.getFromPath<string > ("shapes[ 0  ].circle.color") == "blue");
                assert(hash.getFromPath<string > ("shapes[1].circle.color") == "red");
                assert(hash.getFromPath<string > ("shapes[last].circle.color") == "green");
                hash.setFromPath("shapes[0].circle.color", "black");
                assert(hash.getFromPath<string > ("shapes[0].circle.color") == "black");
                hash.setFromPath("shapes[0].pi", 3.14);
                assert(hash.getFromPath<double>("shapes[0].pi") == 3.14);
                assert(hash.hasFromPath("shapes[0]"));
                assert(hash.hasFromPath("shapes[0].pi"));
                assert(!hash.hasFromPath("shapes[0].pic"));
                assert(hash.hasFromPath("shapes[2]"));
                assert(!hash.hasFromPath("shapes[3]"));

                assert(hash.isFromPath("shapes[0]", Types::HASH) == true);
                assert(hash.isFromPath<Hash > ("shapes[0]") == true);
                assert(hash.isFromPath("shapes[0].pi", Types::DOUBLE) == true);
                assert(hash.isFromPath<double>("shapes[0].pi") == true);
                assert(hash.isFromPath("shapes[0].pi", Types::FLOAT) == false);
                assert(hash.isFromPath<float>("shapes[0].pi") == false);
                assert(hash.isFromPath("shapes[0].pic", Types::HASH) == false);
                assert(hash.isFromPath<Hash > ("shapes[0].pic") == false);
                assert(hash.isFromPath("shapes[2]", Types::HASH) == true);
                assert(hash.isFromPath<Hash > ("shapes[2]") == true);
                assert(hash.isFromPath("shapes[3]", Types::HASH) == false);
                assert(hash.isFromPath<Hash > ("shapes[3]") == false);

                vector<int> numbers;

                int my_numbers[3] = {1, 2, 3};
                numbers.assign(my_numbers, my_numbers + 3);

                hash.setFromPath("i.set.vector", numbers);

                assert(hash.getFromPath<int>("i.set.vector[1]") == 2);
                cout << "We expect 'not supported' exception now" << endl;
                try {
                    // we are expecting exception here
                    assert(hash.hasFromPath("i.set.vector[1]"));
                } catch (const NotSupportedException& e) {
                    cout << e << endl;
                }
                assert(hash.isFromPath<int>("i.set.vector[1]") == true);


                assert(hash.getFromPath<int>("i.set.vector[last]") == 3);
                cout << "We expect 'not supported' exception again" << endl;
                try {
                    // we expect here an exception
                    assert(hash.hasFromPath("i.set.vector[last]"));
                } catch (const NotSupportedException& e) {
                    cout << e << endl;
                }
                assert(hash.isFromPath<int>("i.set.vector[last]") == true);
                assert(hash.isFromPath<float>("i.set.vector[last]") == false);

                assert(hash.getFromPath<string > ("shapes[0].circle.color") == "black");
                
                hash.setFromPath("shapes[0]", Hash("bla", "ALARM"));
                cout << hash << endl;
                assert(hash.getFromPath<Hash > ("shapes[0]").has("pi") == false);
                hash.setFromPath("shapes[last].circle.color", "black");
                assert(hash.getFromPath<string > ("shapes[last].circle.color") == "black");
                assert(hash.getFromPath<string > ("shapes[2].circle.color") == "black");
                assert(hash.getFromPath<string > ("shapes[ ].circle.color") == "black");
                assert(hash.getFromPath<string > ("shapes[ LAST ].circle.color") == "black");
                assert(hash.hasFromPath("shapes[ LAST ]"));
                assert(hash.hasFromPath("shapes[ LAST ].circle"));
                assert(hash.hasFromPath("shapes[ LAST ].circle.color"));
                assert(hash.isFromPath("shapes[ LAST ]", Types::HASH) == true);
                assert(hash.isFromPath("shapes[ LAST ].circle", Types::HASH) == true);
                assert(hash.isFromPath("shapes[ LAST ].circle.color", Types::STRING) == true);
                assert(hash.isFromPath<string > ("shapes[last].circle.color") == true);
                assert(hash.isFromPath<string > ("shapes[2].circle.color") == true);
                assert(hash.isFromPath<string > ("shapes[ ].circle.color") == true);
                assert(hash.isFromPath<string > ("shapes[ LAST ].circle.color") == true);


                cout << "Boost foreach: keys ...\n";
                typedef pair<string, boost::any> HashPair;

                BOOST_FOREACH(HashPair p, hash) {
                    cout << "\t" << p.first << endl;
                }
                
                //======================= ERASE
                assert(hash.eraseFromPath("i"));
                
                cout << "Structure of 'shapes' before erasure ...\n" << hash << endl;
                
                assert(hash.eraseFromPath("shapes[0].bla") == 1);   // erased 1 leaf
                assert(hash.eraseFromPath("shapes[0].bla") == 0);   // no erasure
                cout << "Erase \"shapes[0].bla\" ...\n" << hash << endl;
                
                assert(hash.eraseFromPath("shapes[0].pic") == 0);
                assert(hash.eraseFromPath("shapes[last]") == 1);
                cout << "Erase \"shapes[last]\" ...\n" << hash << endl;
                
                assert(hash.eraseFromPath("shapes[0]") == 1);
                cout << "Erase \"shapes[0]\" ...\n" << hash << endl;
                
            }
            cout << "*** END of testing hasFromPath, isFromPath, eraseFromPath ***\n" << endl;

            Hash tree;
            tree.setFromPath("1.1.1.1", 1);
            tree.setFromPath("1.1.1.2", 2);
            assert(tree.has("1.1.1.1") == false);
            assert(tree.has("1.1.1.2") == false);
            vector<string> leaves = tree.getLeavesAsVector(",");
            assert(leaves[0] == "1,1,1,1");
            assert(leaves[1] == "1,1,1,2");
            Hash flat = tree.flatten();
            assert(flat.has("1.1.1.1") == true);
            assert(flat.has("1.1.1.2") == true);
            assert(flat.get<int>("1.1.1.1") == 1);
            assert(flat.get<int>("1.1.1.2") == 2);
            tree.empty();
            tree = flat.unflatten();
            assert(tree.has("1.1.1.1") == false);
            assert(tree.has("1.1.1.2") == false);
            assert(tree.getFromPath<int>("1.1.1.1") == 1);


            { // Dynamic pathing...
                Hash h("a.b", "Some string");
                assert(h.getFromPath<string > ("a.b") == "Some string");
                h.setFromPath("a.b.c", "Some other string"); // Overwrites "b"
                assert(h.getFromPath<string > ("a.b.c") == "Some other string");
            }


            { // Dynamic pathing...
                Hash h("a.b", "Some string");
                assert(h.getFromPath<string > ("a.b") == "Some string");
                h.setFromPath("a.b.c", "Some other string"); // Overwrites "b"
                assert(h.getFromPath<string > ("a.b.c") == "Some other string");
            }


            {
                // test for overloaded set functions. Also show how the functions work for char* and const char*

                cout << "testing set( char*) " << endl;
                char* ch = new char[10];
                for (int i = 0; i < 10; ++i) {
                    ch[i] = i + 48;
                }
                ch[9] = '\0';

                Hash h;
                h.set<char*>("ch1", ch);
                h.set<char*>("ch2", ch);
                h.set("ch3", ch);
                h.set<string > ("ch4", ch);

                ch[4] = 'z';


                char* ch1 = h.get<char*>("ch1");
                char* ch2 = h.get<char*>("ch2");
                char* ch3 = h.get<char*>("ch3");

                assert(ch1[4] == 'z');
                assert(ch2[4] == 'z');
                assert(ch3[4] == 'z');

                cout << "char* (ch1):" << ch1 << endl;
                cout << "char* (ch2):" << ch2 << endl;
                cout << "char* (ch3):" << ch3 << endl;

                string ch4 = h.get<string > ("ch4");
                cout << "string (ch4):" << ch4 << endl;


                h.set("b", "aaaaa");
                string s = h.get<string > ("b");
                cout << "string :" << s << endl;

                const char* cch = "abcdefg";
                h.set("cch1", cch);
                string cch1String = h.get<string > ("cch1");
                cout << "string converted from const char* (cch1):" << cch1String << endl;
                cout << endl;

                h.set<const char*>("cch2", cch);
                const char* cch2 = h.get<const char*>("cch2");
                cout << "const char* (cch2):" << cch2 << endl;

                try {
                    h.set<const char*>("cch3", cch);
                    string cch3 = h.get<string > ("cch3");
                    // here we expect exception. If the exception is not raised this is considered an error.
                    cout << "Exception not caught for cch3. Indicates error." << endl;
                    return 1;
                } catch (CastException ex) {
                    cout << "exception is properly caught for cch3" << endl;
                } catch (...) {
                    cout << "Wrong exception for cch3. Indicates error." << endl;
                    return 1;
                }

                delete [] ch;
            }

            {
                // test for overloaded setFromPath functions. Also show how the functions work for char* and const char*

                cout << "testing setFromPath( char*) " << endl;
                char* ch = new char[10];
                for (int i = 0; i < 10; ++i) {
                    ch[i] = i + 48;
                }
                ch[9] = '\0';

                Hash h;
                h.setFromPath<char*>("a.ch1", ch);
                h.setFromPath<char*>("a.ch2", ch);
                h.setFromPath("a.ch3", ch);
                h.setFromPath<string > ("a.ch4", ch);

                ch[4] = 'z';


                char* ch1 = h.getFromPath<char*>("a.ch1");
                char* ch2 = h.getFromPath<char*>("a.ch2");
                char* ch3 = h.getFromPath<char*>("a.ch3");

                assert(ch1[4] == 'z');
                assert(ch2[4] == 'z');
                assert(ch3[4] == 'z');

                cout << "char* (a.ch1):" << ch1 << endl;
                cout << "char* (a.ch2):" << ch2 << endl;
                cout << "char* (a.ch3):" << ch3 << endl;

                string ch4 = h.getFromPath<string > ("a.ch4");
                cout << "string (a.ch4):" << ch4 << endl;


                h.setFromPath("a.b", "aaaaa");
                string s = h.getFromPath<string > ("a.b");
                cout << "string :" << s << endl;

                const char* cch = "abcdefg";
                h.setFromPath("a.cch1", cch);
                string cch1String = h.getFromPath<string > ("a.cch1");
                cout << "string converted from const char* (a.cch1):" << cch1String << endl;

                h.setFromPath<const char*>("a.cch2", cch);
                const char* cch2 = h.getFromPath<const char*>("a.cch2");
                cout << "const char* (a.cch2):" << cch2 << endl;

                try {
                    h.setFromPath<const char*>("a.cch3", cch);
                    string cch3 = h.getFromPath<string > ("a.cch3");
                    // here we expect exception. If the exception is not raised this is considered an error.
                    cout << "Exception not caught for a.cch3. Indicates error." << endl;
                    return 1;
                } catch (CastException ex) {
                    cout << "exception is properly caught for a.cch3" << endl;
                } catch (...) {
                    cout << "Wrong exception for a.cch3. Indicates error." << endl;
                    return 1;
                }

                delete [] ch;
            }

            {

                Hash hash;
                hash.set("a", 123);
                hash.set("b", 456);
                hash.set("c", 789);
                hash.setFromPath("d.e.f", 555);
                hash.setFromPath("d.e.g", 777);
                Hash::const_iterator it = hash.find("b");
                if (it == hash.end()) {
                    return 1;
                }
                boost::any any = hash.getAny(it);
                Hash hash2;
                hash2.set("x", any);

                int number = hash2.get<int>("x");
                assert(number == 456);


                // this shows how to use for nested hashes
                // at the moment no recursion is possible
                // other option would be to flatten the hash
                const Hash& nestedHash = hash.getFromPath<Hash > ("d.e");
                Hash::const_iterator it1 = nestedHash.find("g");
                if (it1 == hash.end()) {
                    return 1;
                }
                boost::any any1 = hash.getAny(it1);
                hash2.set("y", any1);
                number = hash2.get<int>("y");
                assert(number == 777);




            }


        } catch (const Exception& e) {
            cout << e;
        }
    }


    return 0;
}
