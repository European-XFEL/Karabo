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

#include <karabo/util/Dims.hh>
#include <karabo/io/h5/Element.hh>


#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

#define _TRACER 2
#include "../../io/h5/Tracer.hh"

using namespace karabo::util;
using namespace karabo::io;
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

    //{
//
//#define _IO_TEST_HELPER_MACRO( data, type, value, size )
//data.set( "key-" + ToType<ToLiteral>::to(FromType<FromTypeInfo>::from(typeid(type) )), static_cast<type > (value));
//data.set( "key-" + ToType<ToLiteral>::to(FromType<FromTypeInfo>::from(typeid(vector<type>) )), vector<type>(size, value));
//
//
//        try {
//            Hash data, config, data1, empty;
//
//            vector<Hash> vecHash;
//            vecHash.resize(3);
//
//            vecHash[0] = data1;
//            data1.set("key-HASH.key-INT32", 1);
//            vecHash[1] = data1;
//            data1.clear();
//            data1.set("key-HASH.key-INT16", static_cast<short> (1));
//            data1.set("key-VECTOR_INT32", vector<int>(15, 2));
//            vecHash[2] = data1;
//
//            size_t vecSize = 12;
//
//            // order must be the same as for Types enum definition
//            _IO_TEST_HELPER_MACRO(data, bool, 1, vecSize)
//            _IO_TEST_HELPER_MACRO(data, char, 49, vecSize)
//            _IO_TEST_HELPER_MACRO(data, signed char, 1, vecSize)
//            _IO_TEST_HELPER_MACRO(data, unsigned char, 1, vecSize)
//            _IO_TEST_HELPER_MACRO(data, signed short, 1, vecSize)
//            _IO_TEST_HELPER_MACRO(data, unsigned short, 1, vecSize)
//            _IO_TEST_HELPER_MACRO(data, signed int, 1, vecSize)
//            _IO_TEST_HELPER_MACRO(data, unsigned int, 1, vecSize)
//            _IO_TEST_HELPER_MACRO(data, signed long long, 1, vecSize)
//            _IO_TEST_HELPER_MACRO(data, unsigned long long, 1, vecSize)
//            _IO_TEST_HELPER_MACRO(data, float, 1, vecSize)
//            _IO_TEST_HELPER_MACRO(data, double, 1, vecSize)
//            _IO_TEST_HELPER_MACRO(data, complex<float>, 1, vecSize)
//            _IO_TEST_HELPER_MACRO(data, complex<double>, 1, vecSize)
//            _IO_TEST_HELPER_MACRO(data, string, "1", vecSize)
//            _IO_TEST_HELPER_MACRO(data, Hash, data1, vecSize)
//
//            _IO_TEST_HELPER_MACRO(data, Hash, empty, vecSize)
//
//            data.set("a.b.c.c.e.ff.asasas.adsds", static_cast<int> (100));
//            data.setAttribute("a.b.c.c.e", "iii", 25);
//
//            data.set("a.vh", vecHash);
//
//            Format::discoverFromHash(data, config);
//            //clog << data << endl << endl;
//            //clog << "Config: " << endl << config << endl;
//
//
//            //clog << endl;
//            vector<Hash>& vec = config.get<vector<Hash> >("Format");
//            for (int i = 0; i <= int(Types::VECTOR_STRING); i++) {
//                //  clog << "i = " << i << " type: " <<  ToType<ToLiteral>::to(FromType<FromInt>::from(i)) << endl;
//                //  clog << vec[i].get<string > ("key") << endl;
//                CPPUNIT_ASSERT(vec[i].get<string > ("key") == "key-" + ToType<ToLiteral>::to(FromType<FromInt>::from(i)));
//                CPPUNIT_ASSERT(vec[i].get<string > ("type") == ToType<ToLiteral>::to(FromType<FromInt>::from(i)));
//                if (Types::category(i) == Types::SEQUENCE) {
//                    CPPUNIT_ASSERT(vec[i].get<unsigned long long > ("size") == vecSize);
//                }
//
//            }
//
//
//            /*
//                    int hashId = int(Types::HASH);
//                    int vecHashId = int(Types::VECTOR_HASH);
//                    _IO_TEST_HELPER_MACRO_TEST1(hashId);
//                    _IO_TEST_HELPER_MACRO_TEST1(vecHashId);
//                    // TODO: _IO_TEST_HELPER_MACRO_TEST2(vecHashId, vecSize);
//
//
//                    //            CPPUNIT_ASSERT(config.get<string > ("a.type") == "HASH");
//                    //            CPPUNIT_ASSERT(config.get<string > ("a/b.type") == "INT32");
//             */
//
//        } catch (Exception& e) {
//            clog << e << endl;
//            KARABO_RETHROW
//        }
//
//#undef _IO_TEST_HELPER_MACRO
//    }
//
//
//    trace(1) << "trace 1" << endl;
//    trace(2) << "trace 2" << endl;
//    trace(3) << "trace 3" << endl;
//
//    {
//
//        Hash data;
//        data.set("a", 23);
//        data.set("b", static_cast<float> (1));
//
//        clog << ToType<ToLiteral>::to(FromType<FromTypeInfo>::from(typeid (int))) << endl;
//        clog << Scalar<int>::classInfo().getClassId() << endl;
//        clog << Scalar<float>::classInfo().getClassId() << endl;
//        clog << Scalar<short>::classInfo().getClassId() << endl;
//
//        shared_ptr<karabo::io::h5::Element> el1 = Factory<karabo::io::h5::Element>::create("INT32");
//        shared_ptr<karabo::io::h5::Element> el2 = Factory<karabo::io::h5::Element>::create("FLOAT");
//
//        el1->write(data.getNode("a"), 0);
//        el2->write(data.getNode("b"), 0);
//
//
//    }
//
//
//    {
//
//
//
//        try {
//
//
//            Hash data, config;
//
//            vector<unsigned long long> dims;
//            dims.push_back(100);
//            dims.push_back(200);
//            Hash::Attributes attr("dims", dims, "a2", "abc", "a3", static_cast<float> (0.75));
//            data.set("a", 1).setAttributes(attr);
//
//            data.set("b", 2).setAttribute("m", "easy");
//
//            vector<unsigned long long> ccc;
//            ccc.push_back(10);
//            ccc.push_back(20);
//            ccc.push_back(30);
//            ccc.push_back(40);
//            ccc.push_back(50);
//            ccc.push_back(60);
//
//            vector<unsigned long long> dimsCcc;
//            dimsCcc.push_back(2);
//            dimsCcc.push_back(3);
//
//            data.set("ccc", ccc).setAttribute("dims", dimsCcc);
//
//
//
//            
//            
//            Dims dimsD(10, 6);
//            boost::shared_array<unsigned short> d = shared_array<unsigned short>(new unsigned short[dimsD.getNumElements()]);
//            for (size_t i = 0; i < dimsD.getNumElements(); ++i) {
//                d[i] = 100 + i;                
//            }
//
//
//
//            
//            
//            unsigned short* dPtr = d.get();
//            data.set("d", dPtr).setAttribute("dims", dimsD.toVector());
//
//            
//            // even better but requires additions to Hash
//            data.set("k", dPtr, Dims(1, 60));
//
//
//
//            clog << "About to discover the format" << endl;
//
//            Format::discoverFromHash(data, config);
//
//            clog << endl << "==================" << endl;
//
//            //clog << data << endl;
//
//            clog << endl << config << endl;
//
////            clog << "fromType: int = " << ToType<ToLiteral>::to(Types::from<int>()) << endl;
////            clog << "fromType: unsigned short*  = " << ToType<ToLiteral>::to(Types::from<unsigned short*>()) << endl;
//
//
//            
//            
//            
//            
//            unsigned short* dd = data.get<unsigned short*>("d");
//            const Dims& dimsDD = data.getAttribute<vector<unsigned long long> >("d", "dims");
//
//            if (dimsDD.getRank() == 2) {
//                for (size_t i = 0; i < dimsDD[0]; ++i) {
//                    for (size_t j = 0; j < dimsDD[1]; ++j) {
//                        clog << "dd[" << i * dimsDD[1] + j << "]: " << dd[i * dimsDD[1] + j] << " ";
//                        CPPUNIT_ASSERT( dd[i * dimsDD[1] + j] == (100 + i * dimsDD[1] + j) );
//                    }
//                    clog << endl;
//                }
//                clog << endl;
//            }
//
//
//            
//            
//            {
//                unsigned short* dd = data.get<unsigned short*>("k");
//                const Dims& dimsDD = data.getAttribute<vector<unsigned long long> >("k", "dims");
//
//                if (dimsDD.getRank() == 2) {
//                    for (size_t i = 0; i < dimsDD[0]; ++i) {
//                        for (size_t j = 0; j < dimsDD[1]; ++j) {
//                            clog << "dd[" << i * dimsDD[1] + j << "]: " << dd[i * dimsDD[1] + j] << " ";
//                            CPPUNIT_ASSERT( dd[i * dimsDD[1] + j] == (100 + i * dimsDD[1] + j) );
//                        }
//                        clog << endl;
//                    }
//                    clog << endl;
//                }
//
//            }
//
//            
//            
//            
//            
//            
//            
////            {
////                
////                Hash data1;
////                
////                data1 = data;
////                
////                unsigned short* dd;
////                Dims dims22;
////                //dd = data1.getPtr<unsigned short>("k", dims22);
////                data1.getPtr<unsigned short>("k", dd, dims22);
////                
////                
////                if (dimsDD.getRank() == 2) {
////                    for (size_t i = 0; i < dimsDD[0]; ++i) {
////                        for (size_t j = 0; j < dimsDD[1]; ++j) {
////                            clog << "dd[" << i * dimsDD[1] + j << "]: " << dd[i * dimsDD[1] + j] << " ";
////                            CPPUNIT_ASSERT( dd[i * dimsDD[1] + j] == (100 + i * dimsDD[1] + j) );
////                        }
////                        clog << endl;
////                    }
////                    clog << endl;
////                }
////
////            }
//// 
////            Dims d(1)
////
//
//        } catch (Exception& e) {
//            cerr << e << endl;
//            KARABO_RETHROW
//        }
//
//    }
//
//    {
//
//        Dims a(2, 12);
//
//        clog << "rank : " << a.getRank() << endl;
//        CPPUNIT_ASSERT(a.getRank() == 2);
//        clog << "NumEl: " << a.getNumElements() << endl;
//        CPPUNIT_ASSERT(a.getNumElements() == 24);
//
//        Dims b = a;
//
//        clog << "rank : " << b.getRank() << endl;
//        CPPUNIT_ASSERT(b.getRank() == 2);
//        clog << "NumEl: " << b.getNumElements() << endl;
//        CPPUNIT_ASSERT(b.getNumElements() == 24);
//
//
//        Dims c(a);
//        clog << "rank : " << c.getRank() << endl;
//        CPPUNIT_ASSERT(c.getRank() == 2);
//        clog << "NumEl: " << c.getNumElements() << endl;
//        CPPUNIT_ASSERT(c.getNumElements() == 24);
//
//        vector<unsigned long long> vec(5, 2);
//        Dims d(vec);
//        clog << "rank : " << d.getRank() << endl;
//        CPPUNIT_ASSERT(d.getRank() == 5);
//        clog << "NumEl: " << d.getNumElements() << endl;
//        CPPUNIT_ASSERT(d.getNumElements() == 32);
//
//        Hash h;
//
//
//        h.set("dims", d.toVector());
//
//        vector<unsigned long long>& vec1 = h.get<vector<unsigned long long> >("dims");
//        for (size_t i = 0; i < vec1.size(); ++i) {
//            clog << "vec1[" << i << "] = " << vec1[i] << endl;
//            CPPUNIT_ASSERT(vec1[i] == 2);
//        }
//
//
//        const Dims& e = h.get<vector<unsigned long long> >("dims");
//        clog << "rank e : " << e.getRank() << endl;
//        CPPUNIT_ASSERT(e.getRank() == 5);
//        clog << "NumEl e : " << e.getNumElements() << endl;
//        CPPUNIT_ASSERT(e.getNumElements() == 32);
//
//        for (size_t i = 0; i < vec1.size(); ++i) {
//            clog << "e[" << i << "] = " << e[i] << endl;
//            CPPUNIT_ASSERT(e[i] == 2);
//        }
//
//    }


}


