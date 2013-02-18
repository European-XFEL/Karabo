/*
 * File:   H5Format_Test.cc
 * Author: wrona
 *
 * Created on Feb 13, 2013, 11:21:54 AM
 */

#include "H5Format_Test.hh"
#include <karabo/util/Hash.hh>
#include <karabo/util/FromInt.hh>
#include <karabo/io/h5/Format.hh>
#include <karabo/util/FromLiteral.hh>
#include <karabo/util/FromType.hh>

#include "../../io/h5/Element.hh"

#include <boost/shared_ptr.hpp>

#define _TRACER 2
#include "../../io/h5/Tracer.hh"

using namespace karabo::util;
using namespace karabo::io::h5;
using namespace boost;

CPPUNIT_TEST_SUITE_REGISTRATION(H5Format_Test);

H5Format_Test::H5Format_Test() {
}

H5Format_Test::~H5Format_Test() {
}

void H5Format_Test::setUp() {
}

void H5Format_Test::tearDown() {
}

void H5Format_Test::testDiscoverFromHash() {

    {

#define _IO_TEST_HELPER_MACRO( data, type, value, size )\
data.set( "key-" + ToType<ToLiteral>::to(FromType<FromTypeInfo>::from(typeid(type) )), static_cast<type > (value));\
data.set( "key-" + ToType<ToLiteral>::to(FromType<FromTypeInfo>::from(typeid(vector<type>) )), vector<type>(size, value));


        try {
            Hash data, config, data1, empty;

            vector<Hash> vecHash;
            vecHash.resize(3);

            vecHash[0] = data1;
            data1.set("key-HASH.key-INT32", 1);
            vecHash[1] = data1;
            data1.clear();
            data1.set("key-HASH.key-INT16", static_cast<short> (1));
            data1.set("key-VECTOR_INT32", vector<int>(15, 2));
            vecHash[2] = data1;

            size_t vecSize = 12;

            // order must be the same as for Types enum definition
            _IO_TEST_HELPER_MACRO(data, bool, 1, vecSize)
            _IO_TEST_HELPER_MACRO(data, char, 49, vecSize)
            _IO_TEST_HELPER_MACRO(data, signed char, 1, vecSize)
            _IO_TEST_HELPER_MACRO(data, unsigned char, 1, vecSize)
            _IO_TEST_HELPER_MACRO(data, signed short, 1, vecSize)
            _IO_TEST_HELPER_MACRO(data, unsigned short, 1, vecSize)
            _IO_TEST_HELPER_MACRO(data, signed int, 1, vecSize)
            _IO_TEST_HELPER_MACRO(data, unsigned int, 1, vecSize)
            _IO_TEST_HELPER_MACRO(data, signed long long, 1, vecSize)
            _IO_TEST_HELPER_MACRO(data, unsigned long long, 1, vecSize)
            _IO_TEST_HELPER_MACRO(data, float, 1, vecSize)
            _IO_TEST_HELPER_MACRO(data, double, 1, vecSize)
            _IO_TEST_HELPER_MACRO(data, complex<float>, 1, vecSize)
            _IO_TEST_HELPER_MACRO(data, complex<double>, 1, vecSize)
            _IO_TEST_HELPER_MACRO(data, string, "1", vecSize)
            _IO_TEST_HELPER_MACRO(data, Hash, data1, vecSize)

            _IO_TEST_HELPER_MACRO(data, Hash, empty, vecSize)

            data.set("a.b.c.c.e.ff.asasas.adsds", static_cast<int> (100));
            data.setAttribute("a.b.c.c.e", "iii", 25);

            data.set("a.vh", vecHash);

            Format::discoverFromHash(data, config);
            //clog << data << endl << endl;
            //clog << "Config: " << endl << config << endl;


            //clog << endl;
            vector<Hash>& vec = config.get<vector<Hash> >("Format");
            for (int i = 0; i <= int(Types::VECTOR_STRING); i++) {
                //  clog << "i = " << i << " type: " <<  ToType<ToLiteral>::to(FromType<FromInt>::from(i)) << endl;
                //  clog << vec[i].get<string > ("key") << endl;
                CPPUNIT_ASSERT(vec[i].get<string > ("key") == "key-" + ToType<ToLiteral>::to(FromType<FromInt>::from(i)));
                CPPUNIT_ASSERT(vec[i].get<string > ("type") == ToType<ToLiteral>::to(FromType<FromInt>::from(i)));
                if (Types::category(i) == Types::SEQUENCE) {
                    CPPUNIT_ASSERT(vec[i].get<unsigned long long > ("size") == vecSize);
                }

            }


            /*
                    int hashId = int(Types::HASH);
                    int vecHashId = int(Types::VECTOR_HASH);
                    _IO_TEST_HELPER_MACRO_TEST1(hashId);
                    _IO_TEST_HELPER_MACRO_TEST1(vecHashId);
                    // TODO: _IO_TEST_HELPER_MACRO_TEST2(vecHashId, vecSize);


                    //            CPPUNIT_ASSERT(config.get<string > ("a.type") == "HASH");
                    //            CPPUNIT_ASSERT(config.get<string > ("a/b.type") == "INT32");
             */

        } catch (Exception& e) {
            clog << e << endl;
            KARABO_RETHROW
        }

#undef _IO_TEST_HELPER_MACRO
    }


    trace(1) << "trace 1" << endl;
    trace(2) << "trace 2" << endl;
    trace(3) << "trace 3" << endl;

    {

        Hash data;
        data.set("a", 23);
        data.set("b", static_cast<float> (1));

        clog << ToType<ToLiteral>::to(FromType<FromTypeInfo>::from(typeid (int))) << endl;
        clog << Scalar<int>::classInfo().getClassId() << endl;
        clog << Scalar<float>::classInfo().getClassId() << endl;
        clog << Scalar<short>::classInfo().getClassId() << endl;

        shared_ptr<karabo::io::h5::Element> el1 = Factory<karabo::io::h5::Element>::create("INT32");
        shared_ptr<karabo::io::h5::Element> el2 = Factory<karabo::io::h5::Element>::create("FLOAT");

        el1->write(data.getNode("a"), 0);
        el2->write(data.getNode("b"), 0);


    }


    {



        try {


            Hash data, config;

            vector<unsigned long long> dims;
            dims.push_back(100);
            dims.push_back(200);
            Hash::Attributes attr("dims", dims, "a2", "abc", "a3", static_cast<float> (0.75));
            data.set("a", 1).setAttributes(attr);

            data.set("b", 2).setAttribute("m", "easy");

            data.set("c", static_cast<long> (100));

            data.set("c", static_cast<long> (100));

            unsigned short d[20];
            for (size_t i = 0; i < 20; ++i) {
                d[i] = 100 + i;
            }

            vector<unsigned long long> dimsD;
            dimsD.push_back(10);
            dimsD.push_back(2);
            data.set("d", &d[0]).setAttribute("dims", dimsD);

            clog << "About to discover the format" << endl;

            Format::discoverFromHash(data, config);

            clog << endl << "==================" << endl;
            //clog << data << endl;
            clog << endl << config << endl;

            clog << "fromType: int = " << ToType<ToLiteral>::to(Types::from<int>()) << endl;
            clog << "fromType: unsigned short*  = " << ToType<ToLiteral>::to(Types::from<unsigned short*>()) << endl;



        } catch (Exception& e) {
            cerr << e << endl;
            KARABO_RETHROW
        }

    }


}


