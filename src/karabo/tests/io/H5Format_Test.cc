/*
 * File:   H5Format_Test.cc
 * Author: wrona
 *
 * Created on Feb 13, 2013, 11:21:54 AM
 */

#include "H5Format_Test.hh"

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

#include <karabo/util/util.hh>
#include <karabo/io/TextSerializer.hh>

#include <karabo/io/h5/Format.hh>
#include <karabo/io/h5/Element.hh>



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

void H5Format_Test::testEmptyFormat() {
    Format::Pointer format = Format::createEmptyFormat();
    const Hash& config = format->getConfig();
    clog << endl << "Empty format config\n" << format->getConfig() << endl;
    CPPUNIT_ASSERT(config.has("Format") == true);
    CPPUNIT_ASSERT(config.is<Hash > ("Format") == true);

    CPPUNIT_ASSERT(config.get<vector<Hash> >("Format.elements").size() == 0);
    //clog << "Empty format config\n" << format->getConfig() << endl;    
}

void H5Format_Test::testManualFormat() {


    TextSerializer<Hash>::Pointer p = TextSerializer<Hash>::create("Xml");
    std::string archive1;
    std::string archive2;
    std::string archive3;



    Format::Pointer format = Format::createEmptyFormat();
    Hash c1(
            "h5path", "experimental",
            "h5name", "test23",
            "key", "instrument.test",
            "type", "UINT32",
            "compressionLevel", 9
            );

    h5::Element::Pointer e1 = h5::Element::create("UINT32", c1);
    format->addElement(e1);

    const Hash config1 = format->getConfig();
    clog << endl << "config1:" << endl << config1 << endl;
    p->save(config1, archive1);
    //clog << "archive1:" << endl << archive1 << endl;



    const vector<Hash>& vec = config1.get<vector<Hash> >("Format.elements");
    CPPUNIT_ASSERT(vec.size() == 1);
    CPPUNIT_ASSERT(vec[0].has("UINT32") == true);




    Hash c2(
            "h5path", "experimental2",
            "h5name", "test1000",
            "key", "instrument.test2",
            "type", "FLOAT",
            "compressionLevel", 0
            );
    h5::Element::Pointer e2 = h5::Element::create("FLOAT", c2);
    format->replaceElement("experimental.test23", e2);

    const Hash config2 = format->getConfig();
    CPPUNIT_ASSERT(config2.has("Format") == true);
    CPPUNIT_ASSERT(config2.is<Hash > ("Format") == true);
    CPPUNIT_ASSERT(config2.has("Format.elements") == true);
    CPPUNIT_ASSERT(config2.get<vector<Hash> >("Format.elements").size() == 1);
    CPPUNIT_ASSERT(config2.get<Hash > ("Format.elements[0]").has("UINT32") == false);
    CPPUNIT_ASSERT(config2.get<Hash > ("Format.elements[0]").has("FLOAT") == true);
    CPPUNIT_ASSERT(config2.get<Hash > ("Format.elements[0]").get<string > ("FLOAT.h5path") == "experimental2");

    clog << endl << "config2:" << endl << config2 << endl;
    p->save(config2, archive2);
    //clog << "archive2:" << endl << archive2 << endl;



    format->removeElement("experimental2.test1000");

    const Hash config3 = format->getConfig();
    CPPUNIT_ASSERT(config3.has("Format") == true);
    CPPUNIT_ASSERT(config3.is<Hash > ("Format") == true);
    CPPUNIT_ASSERT(config3.has("Format.elements") == true);
    CPPUNIT_ASSERT(config3.get<vector<Hash> >("Format.elements").size() == 0);

    clog << endl << "config3:" << endl << config3 << endl;
    p->save(config3, archive3);
    //clog << "archive3:" << endl << archive3 << endl;

}

void H5Format_Test::testDiscoverFromHash() {

    return;
    {

        #define _IO_TEST_HELPER_MACRO( data, type, value, size )\
data.set( "key-" + ToType<ToLiteral>::to(FromType<FromTypeInfo>::from(typeid(type) )), static_cast<type > (value));\
data.set( "key-" + ToType<ToLiteral>::to(FromType<FromTypeInfo>::from(typeid(vector<type>) )), vector<type>(size, value));

        try {
            clog << endl << "testDiscoverFromHash" << endl;
            Hash data, data1;

            vector<Hash> vecHash;
            vecHash.resize(3);

            vecHash[0] = data1;
            data1.set("b.c", 1);
            vecHash[1] = data1;
            data1.clear();
            data1.set("d.e", static_cast<short> (1));
            data1.set("f", 2.4f); //vector<int>(15, 2));
            vecHash[2] = data1;

            data.set("a.b.c.str", "astring");
            data.set("a.b.int", 24);

            //data.set("m", vecHash);

            //data.setAttribute("a","attr1",25);

            Hash config;
            Format::discoverFromHash(data, config);
            Format::Pointer format = Format::createNode("Format", "Format", config);


            clog << "data: \n" << data << endl;
            clog << "conf: \n" << format->getConfig() << endl;


        } catch (Exception e) {
            clog << e.detailedMsg() << endl;
            KARABO_RETHROW
        }


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
        #undef _IO_TEST_HELPER_MACRO
    }


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
}


