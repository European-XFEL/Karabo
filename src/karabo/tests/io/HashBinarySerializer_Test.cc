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
#include "karabo/util/NDArray.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(HashBinarySerializer_Test);

using namespace karabo::io;
using namespace karabo::util;
using std::complex;
using std::string;
using std::vector;


HashBinarySerializer_Test::HashBinarySerializer_Test() {
}


HashBinarySerializer_Test::~HashBinarySerializer_Test() {
}


void HashBinarySerializer_Test::testSerialization() {

    Hash h;
    h.set<bool>("bool", true);
    h.set<char>("char", 'c');
    h.set<unsigned char>("uint8", 8);
    h.set<signed char>("int8", -8);
    h.set<unsigned short>("uint16", 16);
    h.set<short>("int16", -16);
    h.set<unsigned int>("uint32", 32);
    h.set<int>("int32", -32);
    h.set<unsigned long long>("uint64", 64);
    h.set<long long>("int64", -64);
    h.set<float>("float", 3.141);
    h.set<double>("double", 3.14159265359);
    h.set<complex<float> >("cf", complex<float>(1.f, 2.f));
    h.set<complex<double> >("cd", complex<double>(3.f, 4.f));
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
    h.setAttribute<complex<float> >("cf", "cf", complex<float>(1.f, 2.f));
    h.setAttribute<complex<double> >("cd", "cd", complex<double>(3., 4.));
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
    h.set<vector<float> >("vec_float", vector<float>(1000, 3.141f));
    h.set<vector<double> >("vec_double", vector<double>(1000, 3.14159265359));
    h.set<vector<complex<float> > >("vec_cf", vector<complex<float> >(1000, complex<float>(1.f, 2.f)));
    h.set<vector<complex<double> > >("vec_cd", vector < complex<double> >(1000, complex<double>(3., 4.)));
    h.set<vector<string> > ("vec_str", vector<string>(1000, "Hello Karabo"));
    NDArray ndarr(Dims(30, 20, 10), 1);
    h.set("ndarr", ndarr);
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
    h.setAttribute<vector<complex<float> > >("vec_cf", "vec_cf", vector<complex<float> >(1000, complex<float>(1.f, 2.f)));
    h.setAttribute<vector<complex<double> > >("vec_cd", "vec_cd", vector < complex<double> >(1000, complex<double>(3., 4.)));
    h.setAttribute<vector<string> > ("vec_str", "vec_str", vector<string>(1000, "Hello Karabo"));
    m_hash.set<Hash>("hash", h);
    m_hash.set<Hash::Pointer>("hash_ptr", boost::make_shared<Hash>(h));
    m_hash.set<vector<Hash> >("vec_hash", vector<Hash >(100, h));
    m_hash.set<vector<Hash::Pointer> >("vec_hash_ptr", vector<Hash::Pointer>(10, boost::make_shared<Hash>(h)));
    Schema s;
    s.setParameterHash(h);
    m_hash.set<Schema>("schema", s);    
    m_hash.setAttribute("schema", "schema", s);

    BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Bin");
    vector<char> archive1;
    boost::posix_time::ptime tick = boost::posix_time::microsec_clock::local_time();
    for (int i = 0; i < 10; ++i) {
        p->save(m_hash, archive1);
    }
    boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::local_time() - tick;
    float ave = diff.total_milliseconds() / 10.0;
    KARABO_LOG_FRAMEWORK_DEBUG << "\n Average serialization time: " << ave << " ms for Hash of size: " << archive1.size() / 10.e6 << " MB";
    Hash hash;
    tick = boost::posix_time::microsec_clock::local_time();
    for (int i = 0; i < 10; ++i) {
        p->load(hash, archive1);
    }
    diff = boost::posix_time::microsec_clock::local_time() - tick;
    ave = diff.total_milliseconds() / 10.0;
    KARABO_LOG_FRAMEWORK_DEBUG << "\n Average de-serialization time: " << ave << " ms";
    CPPUNIT_ASSERT(karabo::util::similar(hash, m_hash));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hash.get<Hash>("hash"), "std::vector<char>"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(*hash.get<Hash::Pointer>("hash_ptr"), "std::vector<char> ptr"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hash.get<Schema>("schema").getParameterHash(), "std::vector<char> Schema"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hash.getAttribute<Schema>("schema", "schema").getParameterHash(), "std::vector<char> Schema - Attribute"));
    const vector<Hash>& vecHash = hash.get<vector<Hash> >("vec_hash");
    CPPUNIT_ASSERT_EQUAL(100ul, vecHash.size());
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(vecHash[0], "std::vector<char> vector<Hash>[0]")); // skip others...
    const vector<Hash::Pointer>& vecHashPtr = hash.get<vector<Hash::Pointer> >("vec_hash_ptr");
    CPPUNIT_ASSERT_EQUAL(10ul, vecHashPtr.size());
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(*(vecHashPtr[0]), "std::vector<char> vector<Hash::Pointer>[0]")); // skip others...

    // serialising twice should give identical results:
    vector<char> archive2;
    p->save(hash, archive2);
    CPPUNIT_ASSERT(string(&archive1[0], archive1.size()) == string(&archive2[0], archive2.size()));

    // return; // till here OK
    // Now content test with BufferSet - allCopy
    karabo::io::BufferSet archiveBuf1(true); // allCopy
    CPPUNIT_ASSERT_NO_THROW(p->save(m_hash, archiveBuf1));
    Hash hashArchive1;
    CPPUNIT_ASSERT_NO_THROW(p->load(hashArchive1, archiveBuf1));
    CPPUNIT_ASSERT(karabo::util::similar(hashArchive1, m_hash));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hashArchive1.get<Hash>("hash"), "BufferSet(true)"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(*hashArchive1.get<Hash::Pointer>("hash_ptr"), "BufferSet(true) ptr"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hashArchive1.get<Schema>("schema").getParameterHash(), "BufferSet(true) Schema"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hashArchive1.getAttribute<Schema>("schema", "schema").getParameterHash(), "BufferSet(true) Schema - Attribute"));
    const vector<Hash>& vecHash1 = hashArchive1.get<vector<Hash> >("vec_hash");
    CPPUNIT_ASSERT_EQUAL(100ul, vecHash1.size());
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(vecHash1[0], "BufferSet(true) vector<Hash>[0]")); // skip others...
    const vector<Hash::Pointer>& vecHashPtr1 = hashArchive1.get<vector<Hash::Pointer> >("vec_hash_ptr");
    CPPUNIT_ASSERT_EQUAL(10ul, vecHashPtr1.size());
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(*(vecHashPtr1[0]), "BufferSet(true) vector<Hash::Pointer>[0]")); // skip others...

    // Now content test with BufferSet - skip some copies
    karabo::io::BufferSet archiveBuf2(false); // avoid copy if possible
    Hash hashArchive2;
    CPPUNIT_ASSERT_NO_THROW(p->save(m_hash, archiveBuf2));
    CPPUNIT_ASSERT_NO_THROW(p->load(hashArchive2, archiveBuf2));
    CPPUNIT_ASSERT(karabo::util::similar(hashArchive2, m_hash));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hashArchive2.get<Hash>("hash"), "BufferSet(false)"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(*hashArchive2.get<Hash::Pointer>("hash_ptr"), "BufferSet(false) ptr"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hashArchive2.get<Schema>("schema").getParameterHash(), "BufferSet(false) Schema"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hashArchive2.getAttribute<Schema>("schema", "schema").getParameterHash(), "BufferSet(false) Schema - Attribute"));
    const vector<Hash>& vecHash2 = hashArchive2.get<vector<Hash> >("vec_hash");
    CPPUNIT_ASSERT_EQUAL(100ul, vecHash2.size());
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(vecHash2[0], "BufferSet(false) vector<Hash>[0]")); // skip others...
    const vector<Hash::Pointer>& vecHashPtr2 = hashArchive2.get<vector<Hash::Pointer> >("vec_hash_ptr");
    CPPUNIT_ASSERT_EQUAL(10ul, vecHashPtr2.size());
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(*(vecHashPtr2[0]), "BufferSet(false) vector<Hash::Pointer>[0]")); // skip others...
}


void HashBinarySerializer_Test::hashContentTest(const Hash& innerHash, const std::string& serialisationType) {

    // PODs and complex
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, true, innerHash.get<bool>("bool"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 'c', innerHash.get<char>("char"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned char> (8), innerHash.get<unsigned char>("uint8"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<signed char> (-8), innerHash.get<signed char>("int8"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned short> (16), innerHash.get<unsigned short>("uint16"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<short> (-16), innerHash.get<short>("int16"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 32u, innerHash.get<unsigned int>("uint32"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -32, innerHash.get<int>("int32"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 64ull, innerHash.get<unsigned long long>("uint64"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -64ll, innerHash.get<long long>("int64"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.141f, innerHash.get<float>("float"), 1.e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.14159265359, innerHash.get<double>("double"), 1.e-15);
    const auto complexF = innerHash.get<complex<float> >("cf");
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 1.f, complexF.real(), 1.e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 2.f, complexF.imag(), 1.e-7);
    const auto complexD = innerHash.get<complex<double> >("cd");
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3., complexD.real(), 1.e-15);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 4., complexD.imag(), 1.e-15);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, std::string("Hello Karabo"), innerHash.get<string>("str"));
    // Some selected NDArray value tests
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1, innerHash.get<NDArray>("ndarr").getData<int>()[42]);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 10ull, innerHash.get<NDArray>("ndarr").getShape().x3());

    // attributes
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, true, innerHash.getAttribute<bool>("bool", "bool"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 'c', innerHash.getAttribute<char>("char", "char"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned char> (8), innerHash.getAttribute<unsigned char>("uint8", "uint8"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<signed char> (-8), innerHash.getAttribute<signed char>("int8", "int8"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned short> (16), innerHash.getAttribute<unsigned short>("uint16", "uint16"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<short> (-16), innerHash.getAttribute<short>("int16", "int16"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 32u, innerHash.getAttribute<unsigned int>("uint32", "uint32"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -32, innerHash.getAttribute<int>("int32", "int32"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 64ull, innerHash.getAttribute<unsigned long long>("uint64", "uint64"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -64ll, innerHash.getAttribute<long long>("int64", "int64"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.141f, innerHash.getAttribute<float>("float", "float"), 1.e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.14159265359, innerHash.getAttribute<double>("double", "double"), 1.e-15);
    const auto complexFattr = innerHash.getAttribute<complex<float> >("cf", "cf");
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 1.f, complexFattr.real(), 1.e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 2.f, complexFattr.imag(), 1.e-7);
    const auto complexDattr = innerHash.getAttribute<complex<double> >("cd", "cd");
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3., complexDattr.real(), 1.e-15);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 4., complexDattr.imag(), 1.e-15);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, std::string("Hello Karabo"), innerHash.getAttribute<string>("str", "str"));
    // test here NDArray attribute?
    

    // vector values
    auto vecBool = innerHash.get < vector<bool> >("vec_bool");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecBool.size());
    CPPUNIT_ASSERT_MESSAGE(serialisationType, vecBool[0]);
    auto vecChar = innerHash.get <vector<char> >("vec_char");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecChar.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 'c', vecChar[0]);
    auto vecUint8 = innerHash.get <vector<unsigned char> >("vec_uint8");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint8.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned char> (8), vecUint8[0]);
    auto vecInt8 = innerHash.get < vector<signed char> >("vec_int8");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt8.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<signed char> (-8), vecInt8[0]);
    auto vecUint16 = innerHash.get <vector<unsigned short> >("vec_uint16");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint16.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned short> (16), vecUint16[0]);
    auto vecInt16 = innerHash.get <vector<short> >("vec_int16");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt16.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<short> (-16), vecInt16[0]);
    auto vecUint32 = innerHash.get <vector<unsigned int> >("vec_uint32");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint32.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 32u, vecUint32[0]);
    auto vecInt32 = innerHash.get <vector<int> >("vec_int32");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt32.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -32, vecInt32[0]);
    auto vecUint64 = innerHash.get <vector<unsigned long long> >("vec_uint64");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint64.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 64ull, vecUint64[0]);
    auto vecInt64 = innerHash.get <vector<long long> >("vec_int64");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt64.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -64ll, vecInt64[0]);

    auto vecFloat = innerHash.get <vector<float> >("vec_float");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecFloat.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.141f, vecFloat[0], 1.e-7);
    auto vecDouble = innerHash.get <vector<double> >("vec_double");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecDouble.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.14159265359, vecDouble[0], 1.e-15);
    auto vecCf = innerHash.get <vector<complex<float> > >("vec_cf");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecCf.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 1., vecCf[0].real(), 1.e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 2., vecCf[0].imag(), 1.e-7);
    auto vecCd = innerHash.get <vector<complex<double> > >("vec_cd");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecCd.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3., vecCd[0].real(), 1.e-15);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 4., vecCd[0].imag(), 1.e-15);

    auto vecString = innerHash.get <vector<string> >("vec_str");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecString.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, string("Hello Karabo"), vecString[0]);

    // vector attributes
    vecBool = innerHash.getAttribute < vector<bool> >("vec_bool", "vec_bool");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecBool.size());
    CPPUNIT_ASSERT_MESSAGE(serialisationType, vecBool[0]);
    vecChar = innerHash.getAttribute <vector<char> >("vec_char", "vec_char");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecChar.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 'c', vecChar[0]);
    vecUint8 = innerHash.getAttribute <vector<unsigned char> >("vec_uint8", "vec_uint8");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint8.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned char> (8), vecUint8[0]);
    vecInt8 = innerHash.getAttribute < vector<signed char> >("vec_int8", "vec_int8");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt8.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<signed char> (-8), vecInt8[0]);
    vecUint16 = innerHash.getAttribute <vector<unsigned short> >("vec_uint16", "vec_uint16");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint16.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned short> (16), vecUint16[0]);
    vecInt16 = innerHash.getAttribute <vector<short> >("vec_int16", "vec_int16");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt16.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<short> (-16), vecInt16[0]);
    vecUint32 = innerHash.getAttribute <vector<unsigned int> >("vec_uint32", "vec_uint32");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint32.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 32u, vecUint32[0]);
    vecInt32 = innerHash.getAttribute <vector<int> >("vec_int32", "vec_int32");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt32.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -32, vecInt32[0]);
    vecUint64 = innerHash.getAttribute <vector<unsigned long long> >("vec_uint64", "vec_uint64");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint64.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 64ull, vecUint64[0]);
    vecInt64 = innerHash.getAttribute <vector<long long> >("vec_int64", "vec_int64");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt64.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -64ll, vecInt64[0]);

    vecFloat = innerHash.getAttribute <vector<float> >("vec_float", "vec_float");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecFloat.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.141f, vecFloat[0], 1.e-7);
    vecDouble = innerHash.getAttribute <vector<double> >("vec_double", "vec_double");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecDouble.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.14159265359, vecDouble[0], 1.e-15);
    vecCf = innerHash.getAttribute <vector<complex<float> > >("vec_cf", "vec_cf");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecCf.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 1., vecCf[0].real(), 1.e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 2., vecCf[0].imag(), 1.e-7);
    vecCd = innerHash.getAttribute <vector<complex<double> > >("vec_cd", "vec_cd");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecCd.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3., vecCd[0].real(), 1.e-15);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 4., vecCd[0].imag(), 1.e-15);

    vecString = innerHash.getAttribute <vector<string> >("vec_str", "vec_str");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecString.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, string("Hello Karabo"), vecString[0]);
}


void HashBinarySerializer_Test::testSpeedLargeArrays() {
    Hash h;
    NDArray ndarr(Dims(256, 256, 512), karabo::util::Types::DOUBLE);
    double* dptr = reinterpret_cast<double*>(ndarr.getDataPtr().get());
    for(size_t i = 0; i != ndarr.size(); ++i) {
        dptr[i] = i % 100;
    }
    
    h.set("ndarr", ndarr);
    
    vector<char> archive1;
    vector<char> archive2;
    
    BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Bin");
    
    boost::posix_time::ptime tick = boost::posix_time::microsec_clock::local_time();

    for (int i = 0; i < 10; ++i) {
        p->save(h, archive1);
    }

    boost::posix_time::time_duration diff = boost::posix_time::microsec_clock::local_time() - tick;
    float ave = diff.total_milliseconds() / 10.0;
    std::clog << "\n Average serialization time: " << ave << " ms for Hash of size: " << archive1.size() / 10.e6 << " MB"<<std::endl;

    Hash dh;
    tick = boost::posix_time::microsec_clock::local_time();
    for (int i = 0; i < 10; ++i) {
        p->load(dh, archive1);
    }
    diff = boost::posix_time::microsec_clock::local_time() - tick;
    ave = diff.total_milliseconds() / 10.0;
    std::clog << "\n Average de-serialization time: " << ave << " ms" << std::endl;
    
        
    CPPUNIT_ASSERT(karabo::util::similar(h, dh));
    
    {
        std::clog << "\nBufferSet copy ...\n";
        karabo::io::BufferSet archive3(true);
        tick = boost::posix_time::microsec_clock::local_time();

        for (int i = 0; i < 10; ++i) {
            p->save(h, archive3);
        }
        
        
        diff = boost::posix_time::microsec_clock::local_time() - tick;
        ave = diff.total_milliseconds() / 10.0;
        std::clog << "--- Average serialization time: " << ave << " ms for Hash of size: " << archive1.size() / 10.e6 << " MB"<<std::endl;
        
        std::clog << "------ " << archive3 << std::endl;

        // Check the content of boost::asio::buffer ....
        std::clog << "\tListing of Boost ASIO buffers ...\n";
        vector<boost::asio::const_buffer> buf;
        archive3.appendTo(buf);
        std::size_t idx = 0;
        for(auto it = buf.begin(); it != buf.end(); ++it, ++idx){
            size_t size = boost::asio::buffer_size(*it);
            std::clog << std::dec << "\tidx=" << idx << "\t size=" << std::setw(12) << std::setfill(' ') << size << "  ->  0x" << std::hex;
            for(size_t i = 0; i != std::min(size, size_t(30)); ++i) {
                std::clog << std::setw(2) << std::setfill('0') << int(boost::asio::buffer_cast<const char*>(*it)[i]);
            }
            std::clog << (size > 30 ? "..." : "") << std::endl;
        }

        archive3.rewind();    
        Hash dh2;
        tick = boost::posix_time::microsec_clock::local_time();
        for (int i = 0; i < 10; ++i) {
            p->load(dh2, archive3);
        }
        diff = boost::posix_time::microsec_clock::local_time() - tick;
        ave = diff.total_milliseconds() / 10.0;
        std::clog << "\n--- Average de-serialization time: " << ave << " ms" << std::endl;
        CPPUNIT_ASSERT(karabo::util::similar(h, dh2));
        bool all_same =  true;

        // verify that we do not have any byte shifting in between serialization and deserialization
        NDArray tarr = dh2.get<NDArray>("ndarr");
        CPPUNIT_ASSERT(ndarr.itemSize() == tarr.itemSize());
        CPPUNIT_ASSERT(ndarr.byteSize() == tarr.byteSize());
        for(int i = 0; i != 100; ++i) {
            all_same &= (ndarr.getDataPtr().get()[i] == tarr.getDataPtr().get()[i]);
        }
        CPPUNIT_ASSERT(all_same);

        for(int i = 0; i != 100; ++i) {
            all_same &= (ndarr.getDataPtr().get()[ndarr.byteSize()-i-1] == tarr.getDataPtr().get()[ndarr.byteSize()-i-1]);
        }
        CPPUNIT_ASSERT(all_same);
    }
    
    {
        std::clog << "\n--- BufferSet no copy...\n";
        karabo::io::BufferSet archive3(false);
        tick = boost::posix_time::microsec_clock::local_time();

        for (int i = 0; i < 1000; ++i) {
            p->save(h, archive3);
        }

        diff = boost::posix_time::microsec_clock::local_time() - tick;
        ave = diff.total_milliseconds() / 1000.0;
        std::clog << "--- Average serialization time: " << ave << " ms for Hash of size: " << archive1.size() / 10.e6 << " MB"<<std::endl;

        std::clog << "------ " << archive3 << std::endl;
        archive3.rewind();    
        Hash dh2;
        tick = boost::posix_time::microsec_clock::local_time();
        for (int i = 0; i < 1000; ++i) {
            p->load(dh2, archive3);
        }
        diff = boost::posix_time::microsec_clock::local_time() - tick;
        ave = diff.total_milliseconds() / 1000.0;
        std::clog << "--- Average de-serialization time: " << ave << " ms" << std::endl;
        CPPUNIT_ASSERT(karabo::util::similar(h, dh2));
        bool all_same =  true;

        // verify that we do not have any byte shifting in between serialization and deserialization
        NDArray tarr = dh2.get<NDArray>("ndarr");
        CPPUNIT_ASSERT(ndarr.itemSize() == tarr.itemSize());
        CPPUNIT_ASSERT(ndarr.byteSize() == tarr.byteSize());
        for(int i = 0; i != 100; ++i) {
            all_same &= (ndarr.getDataPtr().get()[i] == tarr.getDataPtr().get()[i]);
        }
        CPPUNIT_ASSERT(all_same);

        for(int i = 0; i != 100; ++i) {
            all_same &= (ndarr.getDataPtr().get()[ndarr.byteSize()-i-1] == tarr.getDataPtr().get()[ndarr.byteSize()-i-1]);
        }
        CPPUNIT_ASSERT(all_same);
    }
}
