/* 
 * File:   HashBinarySerializer_Test.cc
 * Author: heisenb
 * 
 * Created on February 25, 2013, 6:03 PM
 */

#include <karabo/io/HashBinarySerializer.hh>
#include "HashBinarySerializer_Test.hh"
#include "karabo/io/BinarySerializer.hh"
#include "karabo/io/TextSerializer.hh"
#include "karabo/util/TimeProfiler.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(HashBinarySerializer_Test);

using namespace karabo::io;
using namespace karabo::util;


HashBinarySerializer_Test::HashBinarySerializer_Test() {
}


HashBinarySerializer_Test::~HashBinarySerializer_Test() {
}


void HashBinarySerializer_Test::setUp() {
    Hash h;
    h.set<bool>("bool", true);
    h.set<char>("char", 'c');
    h.set<unsigned char>("uint8", 8);
    h.set<signed char>("int8", -8);
    h.set<unsigned short>("uint16", 16);
    h.set<short>("int16", -16);
    h.set<unsigned int>("uint32", 32);
    h.set<int>("int32", -32);
    h.set<unsigned long long>("int64", 64);
    h.set<long long>("uint64", -64);
    h.set<float>("float", 3.141);
    h.set<double>("double", 3.14159265359);
    h.set<complex<float> >("cf", complex<float>(1., 1.));
    h.set<complex<double> >("cd", complex<float>(2., 2.));
    h.set<string>("str", "Hello Karabo");

    h.setAttribute<bool>("bool", "bool", true);
    h.setAttribute<char>("char", "char", 'c');
    h.setAttribute<unsigned char>("uint8", "uint8", 8);
    h.setAttribute<signed char>("int8", "int8", -8);
    h.setAttribute<unsigned short>("uint16", "uint16", 16);
    h.setAttribute<short>("int16", "int16", -16);
    h.setAttribute<unsigned int>("uint32", "uint32", 32);
    h.setAttribute<int>("int32", "int32", -32);
    h.setAttribute<unsigned long long>("uint64", "uint64", 64);
    h.setAttribute<long long>("int64", "int64", -64);
    h.setAttribute<float>("float", "float", 3.141);
    h.setAttribute<double>("double", "double", 3.14159265359);
    h.setAttribute<complex<float> >("cf", "cf", complex<float>(1., 1.));
    h.setAttribute<complex<double> >("cd", "cd", complex<float>(2., 2.));
    h.setAttribute<string>("str", "str", "Hello Karabo");

    h.set < vector<bool> >("vec_bool", vector<bool>(1000, true));
    h.set<vector<char> >("vec_char", vector<char>(1000, 'c'));
    h.set<vector<unsigned char> >("vec_uint8", vector<unsigned char>(1000, 8));
    h.set < vector<signed char> >("vec_int8", vector <signed char>(1000, -8));
    h.set<vector<unsigned short> >("vec_uint16", vector<unsigned short>(1000, 16));
    h.set<vector<short> >("vec_int16", vector<short>(1000, -16));
    h.set<vector<unsigned int> >("vec_uint32", vector<unsigned int>(1000, 32));
    h.set<vector<int> >("vec_int32", vector <int>(1000, -32));
    h.set<vector<unsigned long long> >("vec_uint64", vector<unsigned long long>(1000, 64));
    h.set<vector<long long> >("vec_int64", vector <long long>(1000, -64));
    h.set<vector<float> >("vec_float", vector<float>(1000, 3.141));
    h.set<vector<double> >("vec_double", vector<double>(1000, 3.14159265359));
    h.set<vector<complex<float> > >("vec_cf", vector<complex<float> >(1000, complex<float>(1., 2.)));
    h.set<vector<complex<double> > >("vec_cd", vector < complex<double> >(1000, complex<double>(3., 4.)));
    h.set<vector<string> > ("vec_str", vector<string>(1000, "Hello Karabo"));

    h.setAttribute < vector<bool> >("vec_bool", "vec_bool", vector<bool>(1000, true));
    h.setAttribute<vector<char> >("vec_char", "vec_char", vector<char>(1000, 'c'));
    h.setAttribute<vector<unsigned char> >("vec_uint8", "vec_uint8", vector<unsigned char>(1000, 8));
    h.setAttribute < vector<signed char> >("vec_int8", "vec_int8", vector <signed char>(1000, -8));
    h.setAttribute<vector<unsigned short> >("vec_uint16", "vec_uint16", vector<unsigned short>(1000, 16));
    h.setAttribute<vector<short> >("vec_int16", "vec_int16", vector<short>(1000, -16));
    h.setAttribute<vector<unsigned int> >("vec_uint32", "vec_uint32", vector<unsigned int>(1000, 32));
    h.setAttribute<vector<int> >("vec_int32", "vec_int32", vector <int>(1000, -32));
    h.setAttribute<vector<unsigned long long> >("vec_uint64", "vec_uint64", vector<unsigned long long>(1000, 64));
    h.setAttribute<vector<long long> >("vec_int64", "vec_int64", vector <long long>(1000, -64));
    h.setAttribute<vector<float> >("vec_float", "vec_float", vector<float>(1000, 3.141));
    h.setAttribute<vector<double> >("vec_double", "vec_double", vector<double>(1000, 3.14159265359));
    h.setAttribute<vector<complex<float> > >("vec_cf", "vec_cf", vector<complex<float> >(1000, complex<float>(1., 1.)));
    h.setAttribute<vector<complex<double> > >("vec_cd", "vec_cd", vector < complex<double> >(1000, complex< float>(2., 2.)));
    h.setAttribute<vector<string> > ("vec_str", "vec_str", vector<string>(1000, "Hello Karabo"));

    m_hash.set<Hash>("hash", h);
    m_hash.set<vector<Hash> >("vec_hash", vector<Hash >(100, h));

    Schema s;
    s.setParameterHash(h);
    m_hash.set<Schema>("schema", s);
}


void HashBinarySerializer_Test::testSerialization() {

    BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Bin");

    vector<char> archive1;
    vector<char> archive2;

    boost::posix_time::ptime tick = boost::posix_time::microsec_clock::local_time();

    for (int i = 0; i < 10; ++i) {
        p->save(m_hash, archive1);
    }

    boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::local_time() - tick;
    float ave = diff.total_milliseconds() / 10.0;
    clog << "\n Average serialization time: " << ave << " ms for Hash of size: " << archive1.size() / 10.e6 << " MB" << endl;

    Hash hash;
    tick = boost::posix_time::microsec_clock::local_time();
    for (int i = 0; i < 10; ++i) {
        p->load(hash, archive1);
    }
    diff = boost::posix_time::microsec_clock::local_time() - tick;
    ave = diff.total_milliseconds() / 10.0;
    clog << "\n Average de-serialization time: " << ave << " ms" << endl;

    CPPUNIT_ASSERT(karabo::util::similar(hash, m_hash));

    p->save(hash, archive2);

    CPPUNIT_ASSERT(string(archive1[0], archive1.size()) == string(archive2[0], archive2.size()));

}
