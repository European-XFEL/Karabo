/*
 * $Id: testHashConvertFromString.cc 5057 2012-02-07 15:52:59Z esenov $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <iostream>
#include <cassert>
#include <string>
#include "../Test.hh"
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/any.hpp>
#include <typeinfo>
#include <cmath>
#include <complex>

#include "../Hash.hh"


using namespace std;
using namespace exfel::util;

int testHashConvertFromString(int argc, char** argv) {
    //  open issues:
    //  decide how to handle char
    //  At he moment unsigned char or signed char is supported
    //
    // how to use const char* ?
    // h.convertFromString("a", Types::CONST_CHAR_PTR);
    // throws exception

    // convertFromString throws boost::bad_lexical_cast if the value is out of range
    // should not be this the CastException?

    try {

        Test t;
        TEST_INIT(t, argc, argv);

        cout << t << endl;
        // use t.file("filename"); to access file


        int32_t i = 22;
        int8_t a = boost::numeric_cast<int8_t > (boost::lexical_cast<int>("127"));
        cout << "typeid.name() " << typeid (int8_t).name() << endl;
        assert(a == 127);

        boost::any value = a;

        double dd = boost::numeric_cast<double > (boost::any_cast<int > (i));
        cout << "dd=" << dd << endl;
        double ee = boost::numeric_cast<double > (boost::any_cast<int8_t > (value));
        cout << "typeid.neme: " << typeid (a).name() << " ee=" << ee << endl;


        {
            complex<float> cf1(12.1, 4.5);
            cout << "complex float: cf1 = " << cf1 << endl;
            complex<float> cf2 = boost::lexical_cast<complex<float> >("(12.1,4.5)");
            cout << "complex float: cf2 = " << cf2 << endl;

            try {
                complex<float> cf3 = boost::lexical_cast<complex<float> >("12.1,4.5");
                cout << "complex float: cf3 = " << cf3 << endl;
                assert(false);
            } catch (boost::bad_lexical_cast e) {
            }
            try {
                complex<float> cf3 = boost::lexical_cast<complex<float> >("(12.1, 4.5)");
                cout << "complex float: cf3 = " << cf3 << endl;
                assert(false);
            } catch (boost::bad_lexical_cast e) {
            }
        }
        {
            const char* s = "abcde";
            string str = boost::lexical_cast<string > (s);
        }

        Hash h;
        {
            std::string nStr[] = {"0", "n", "no", "false"};
            for (int i = 0; i < 4; i++) {
                h.set("a", nStr[i]);
                h.convertFromString("a", Types::BOOL);
                bool a = h.get<bool > ("a");
                assert(a == false);
                cout << "Testing bool " << nStr[i] << ": a = " << a << " fine" << endl;
            }
        }
        {
            std::string nStr[] = {"1", "y", "yes", "true"};
            for (int i = 0; i < 4; i++) {
                h.set("a", nStr[i]);
                h.convertFromString("a", Types::BOOL);
                bool a = h.get<bool > ("a");
                assert(a == true);
                cout << "Testing bool " << nStr[i] << ": a = " << a << " fine" << endl;
            }
        }
        {
            std::string nStr = "60";
            h.set("a", nStr);
            h.convertFromString("a", Types::INT8);
            int8_t a = h.get<int8_t > ("a");
            assert(a == 60);
            cout << "Testing int8_t a = " << (int) a << " - fine" << endl;
        }
        {
            std::string nStr = "60";
            h.set("a", nStr);
            h.convertFromString("a", Types::INT8);
            signed char a = h.get<signed char>("a");
            assert(a == 60);
            cout << "Testing signed char a = " << (int) a << " - fine" << endl;
        }
        {
            std::string nStr = "60";
            h.set("a", nStr);
            h.convertFromString("a", Types::CHAR);
            char a = h.get<char>("a");
            assert(a == 60);
            cout << "Testing char a = " << (int) a << " - fine" << endl;
        }
        {
            std::string nStr = "60";
            h.set("a", nStr);
            h.convertFromString("a", Types::INT16);
            short a = h.get<short>("a");
            assert(a == 60);
            cout << "Testing short a = " << a << " - fine" << endl;
        }
        {
            std::string nStr = "60";
            h.set("a", nStr);
            h.convertFromString("a", Types::INT32);
            int a = h.get<int>("a");
            assert(a == 60);
            cout << "Testing int a = " << a << " - fine" << endl;
        }
        {
            std::string nStr = "60";
            h.set("a", nStr);
            h.convertFromString("a", Types::INT64);
            long long a = h.get<long long>("a");
            assert(a == 60);
            cout << "Testing long long a = " << a << " - fine" << endl;
        }
        {
            std::string nStr = "3.40282e+38";
            float max = 3.40282e+38;
            h.set("a", nStr);
            h.convertFromString("a", Types::FLOAT);
            float a = h.get<float>("a");
            cout << "abs(a - max)/max " << abs(a - max) / max << endl;
            assert((abs(a) - max) / max < 1e-5);
            cout << "Testing float a = " << a << " - fine" << endl;
            cout << "float limits: (" << std::numeric_limits<float>::min() << ", "
                    << std::numeric_limits<float>::max() << ")" << endl;
            float b = 0;
            try {
                std::string nStr = "3.40283e+38";
                h.set("b", nStr);
                h.convertFromString("b", Types::FLOAT);
                b = h.get<float>("b");
                cout << "b=" << b << endl;
                cout << "Testing float (out of range): boost::bad_lexical_cast not thrown - test failed" << endl;
                assert(false);
            } catch (boost::bad_lexical_cast e) {
                //cout << e.what() << endl;
                cout << "Testing float (out of range) - fine" << endl;
            } catch (Exception e) {
                cout << e;
                assert(false);
            }
        }
        {
            std::string nStr = "(60.0,80.0)";
            h.set("a", nStr);
            h.convertFromString("a", Types::COMPLEX_FLOAT);
            complex<float> a = h.get<complex<float> >("a");
            assert((abs(a) - 100.0) < 1e-5);
            cout << "Testing complex float a = " << a << " - fine" << endl;
            //cout << "abs(a) - 100 " << abs(a) - 100.0 << endl;
        }
        {
            std::string nStr = "60.1";
            h.set("a", nStr);
            h.convertFromString("a", Types::DOUBLE);
            double a = h.get<double>("a");
            assert((abs(a) - 60.1) < 1e-10);
            cout << "Testing double a = " << a << " - fine" << endl;
            //cout << "abs(a - 60.1) " << abs(a - 60.1) << endl;
            cout << "double limits: (" << std::numeric_limits<double>::min() << ", "
                    << std::numeric_limits<double>::max() << ")" << endl;

            double b = 0;
            try {
                std::string nStr = "1.7977e+308";
                h.set("b", nStr);
                h.convertFromString("b", Types::DOUBLE);
                b = h.get<double>("b");
                cout << "b=" << b << endl;
                cout << "Testing double (out of range): boost::bad_lexical_cast not thrown - test failed" << endl;
                assert(false);
            } catch (boost::bad_lexical_cast e) {
                //cout << e.what() << endl;
                cout << "Testing double (out of range) - fine" << endl;
            } catch (Exception e) {
                cout << e;
                assert(false);
            }

        }
        {
            std::string nStr = "(-60.0,80.0)";
            h.set("a", nStr);
            h.convertFromString("a", Types::COMPLEX_DOUBLE);
            complex<double> a = h.get<complex<double> >("a");
            assert((abs(a) - 100.0) < 1e-5);
            cout << "Testing complex double a = " << a << " - fine" << endl;
            //cout << "abs(a) - 100 " << abs(a) - 100.0 << endl;
        }
        {
            std::string nStr = "Hello World!!!";
            h.set("a", nStr);
            h.convertFromString("a", Types::STRING);
            string a = h.get<string > ("a");
            assert(a == "Hello World!!!");
            cout << "Testing string a = " << a << " - fine" << endl;
        }
        //    {
        //      std::string nStr = "Hello World!!!";
        //      h.set("a", nStr);
        //      h.convertFromString("a", Types::CONST_CHAR_PTR);
        //      const char* a = h.get<const char* >("a");
        //      cout << "const char* a = " << a << endl;
        //      assert(strcmp(a, "Hello World!!!") == 0);
        //    }
        {
            std::string nStr = "129";
            h.set("a", nStr);
            h.convertFromString("a", Types::UINT8);
            unsigned char a = h.get<unsigned char>("a");
            assert(a == 129);
            cout << "Testing unsigned char a = " << (int) a << " - fine" << endl;
            try {
                unsigned char b = 0;
                h.set("b", "256");
                b = h.get<unsigned char>("b");
                assert(false);
                cout << "unsigned char inside range - too bad: b = " << b << endl;
            } catch (CastException e) {
                cout << "Testing unsigned char (out of range) - fine" << endl;
            } catch (...) {
                assert(false);
            }
        }
        {
            std::string nStr = "60";
            h.set("a", nStr);
            h.convertFromString("a", Types::UINT16);
            unsigned short a = h.get<unsigned short>("a");
            assert(a == 60);
            cout << "Testing unsigned short a = " << a << " - fine" << endl;
            try {
                unsigned short b = 0;
                h.set("b", "-1");
                b = h.get<unsigned short>("b");
                assert(false);
                cout << "unsigned short inside range - too bad: b = " << b << endl;
            } catch (CastException e) {
                cout << "Testing unsigned short (out of range) - fine" << endl;
            } catch (...) {
                assert(false);
            }
        }
        {
            std::string nStr = "60";
            h.set("a", nStr);
            h.convertFromString("a", Types::UINT32);
            unsigned int a = h.get<unsigned int>("a");
            assert(a == 60);
            cout << "Testing unsigned int a = " << a << " - fine" << endl;
            try {
                unsigned int b = 0;
                h.set("b", "-1");
                b = h.get<unsigned int>("b");
                assert(false);
                cout << "unsigned int inside range - too bad: b = " << b << endl;
            } catch (CastException e) {
                cout << "Testing unsigned int (out of range) - fine" << endl;
            } catch (...) {
                assert(false);
            }
        }
        {
            std::string nStr = "60";
            h.set("a", nStr);
            h.convertFromString("a", Types::UINT64);
            unsigned long long a = h.get<unsigned long long>("a");
            assert(a == 60);
            cout << "Testing unsigned long long a = " << a << " - fine" << endl;
            try {
                unsigned long long b = 0;
                h.set("b", "-1");
                b = h.get<unsigned long long>("b");
                assert(false);
                cout << "unsigned long long inside range - too bad: b = " << b << endl;
            } catch (CastException e) {
                cout << "Testing unsigned long long (out of range) - fine" << endl;
            } catch (...) {
                assert(false);
            }
        }
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
                assert(a[i] == ref[i]);
            }
            Hash h2;
            h2.set("vectStr", ref);
            cout << "Show vector of strings :\n" << h2 << endl;
            cout << "Testing vector<string> - fine" << endl;
        }





    } catch (Exception e) {
        cout << e;
        RETHROW
    }

    return 0;
}

