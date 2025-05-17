/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   HashBinarySerializer_Test.cc
 * Author: heisenb
 *
 * Created on February 25, 2013, 6:03 PM
 */

#include "HashBinarySerializer_Test.hh"

#include <algorithm>

#include "karabo/data/io/BinarySerializer.hh"
#include "karabo/data/io/HashBinarySerializer.hh"
#include "karabo/data/io/TextSerializer.hh"
#include "karabo/data/types/NDArray.hh"
#include "karabo/util/TimeProfiler.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(HashBinarySerializer_Test);

using namespace karabo::data;
using std::complex;
using std::string;
using std::vector;


HashBinarySerializer_Test::HashBinarySerializer_Test() {
    // Uncomment for output, e.g. serialisaton speed measurements
    // karabo::log::Logger::configure(Hash("priority", "DEBUG"));
    // karabo::log::Logger::useConsole();
}


HashBinarySerializer_Test::~HashBinarySerializer_Test() {}


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
    h.set<complex<float>>("cf", complex<float>(1.f, 2.f));
    h.set<complex<double>>("cd", complex<double>(3.f, 4.f));
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
    h.setAttribute<complex<float>>("cf", "cf", complex<float>(1.f, 2.f));
    h.setAttribute<complex<double>>("cd", "cd", complex<double>(3., 4.));
    h.setAttribute<string>("str", "str", "Hello Karabo");
    h.set<vector<bool>>("vec_bool", vector<bool>(1000, true));
    h.set<vector<char>>("vec_char", vector<char>(1000, 'c'));
    h.set<vector<unsigned char>>("vec_uint8", vector<unsigned char>(1000, 8));
    h.set<vector<signed char>>("vec_int8", vector<signed char>(1000, -8));
    h.set<vector<unsigned short>>("vec_uint16", vector<unsigned short>(1000, 16));
    h.set<vector<short>>("vec_int16", vector<short>(1000, -16));
    h.set<vector<unsigned int>>("vec_uint32", vector<unsigned int>(1000, 32));
    h.set<vector<int>>("vec_int32", vector<int>(1000, -32));
    h.set<vector<unsigned long long>>("vec_uint64", vector<unsigned long long>(1000, 64));
    h.set<vector<long long>>("vec_int64", vector<long long>(1000, -64));
    h.set<vector<float>>("vec_float", vector<float>(1000, 3.141f));
    h.set<vector<double>>("vec_double", vector<double>(1000, 3.14159265359));
    h.set<vector<complex<float>>>("vec_cf", vector<complex<float>>(1000, complex<float>(1.f, 2.f)));
    h.set<vector<complex<double>>>("vec_cd", vector<complex<double>>(1000, complex<double>(3., 4.)));
    h.set<vector<string>>("vec_str", vector<string>(1000, "Hello Karabo"));
    NDArray ndarr(Dims(30, 20, 10), 1);
    h.set("ndarr", ndarr);
    // Also test an empty NDArray:
    const int noData[] = {};
    NDArray ndarrEmpty(noData, sizeof(noData) / sizeof(int));
    h.set("ndarrEmpty", ndarrEmpty);
    h.setAttribute<vector<bool>>("vec_bool", "vec_bool", vector<bool>(1000, true));
    h.setAttribute<vector<char>>("vec_char", "vec_char", vector<char>(1000, 'c'));
    h.setAttribute<vector<unsigned char>>("vec_uint8", "vec_uint8", vector<unsigned char>(1000, 8));
    h.setAttribute<vector<signed char>>("vec_int8", "vec_int8", vector<signed char>(1000, -8));
    h.setAttribute<vector<unsigned short>>("vec_uint16", "vec_uint16", vector<unsigned short>(1000, 16));
    h.setAttribute<vector<short>>("vec_int16", "vec_int16", vector<short>(1000, -16));
    h.setAttribute<vector<unsigned int>>("vec_uint32", "vec_uint32", vector<unsigned int>(1000, 32));
    h.setAttribute<vector<int>>("vec_int32", "vec_int32", vector<int>(1000, -32));
    h.setAttribute<vector<unsigned long long>>("vec_uint64", "vec_uint64", vector<unsigned long long>(1000, 64));
    h.setAttribute<vector<long long>>("vec_int64", "vec_int64", vector<long long>(1000, -64));
    h.setAttribute<vector<float>>("vec_float", "vec_float", vector<float>(1000, 3.141));
    h.setAttribute<vector<double>>("vec_double", "vec_double", vector<double>(1000, 3.14159265359));
    h.setAttribute<vector<complex<float>>>("vec_cf", "vec_cf", vector<complex<float>>(1000, complex<float>(1.f, 2.f)));
    h.setAttribute<vector<complex<double>>>("vec_cd", "vec_cd", vector<complex<double>>(1000, complex<double>(3., 4.)));
    h.setAttribute<vector<string>>("vec_str", "vec_str", vector<string>(1000, "Hello Karabo"));
    m_hash.set<Hash>("hash", h);
    m_hash.set<Hash::Pointer>("hash_ptr", std::make_shared<Hash>(h));
    m_hash.set<vector<Hash>>("vec_hash", vector<Hash>(100, h));
    m_hash.set<vector<Hash::Pointer>>("vec_hash_ptr", vector<Hash::Pointer>(10, std::make_shared<Hash>(h)));
    Schema s;
    s.setParameterHash(h);
    m_hash.set<Schema>("schema", s);
    m_hash.setAttribute("schema", "schema", s);

    BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Bin");
    vector<char> archive1;
    std::chrono::steady_clock::time_point tick = std::chrono::steady_clock::now();
    const int ntests = 1; // for measurements, better increase...
    for (int i = 0; i < ntests; ++i) {
        p->save(m_hash, archive1);
    }
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick);
    float ave = diff.count() / static_cast<double>(ntests);
    KARABO_LOG_FRAMEWORK_DEBUG << " Average serialization time: " << ave
                               << " ms for Hash of size: " << archive1.size() / 10.e6 << " MB";

    const Hash schemaOnlyHash("schema", s);
    tick = std::chrono::steady_clock::now();
    std::vector<char> archiveSchema;
    const int ntestsSchema = ntests * 10;
    for (int i = 0; i < ntestsSchema; ++i) {
        archiveSchema.clear();
        p->save(schemaOnlyHash, archiveSchema);
    }
    diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick);
    ave = diff.count() / static_cast<double>(ntestsSchema);
    KARABO_LOG_FRAMEWORK_DEBUG << " Average serialization time schema only: " << ave << " ms";

    Hash hash;
    tick = std::chrono::steady_clock::now();
    for (int i = 0; i < ntestsSchema; ++i) {
        hash.clear();
        size_t size = p->load(hash, archiveSchema);
        CPPUNIT_ASSERT_EQUAL(archiveSchema.size(), size);
    }
    diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick);
    ave = diff.count() / static_cast<double>(ntestsSchema);
    KARABO_LOG_FRAMEWORK_DEBUG << " Average de-serialization time schema only: " << ave << " ms";

    // Check how save2 and load work together
    archiveSchema.clear();
    Hash schemaOnlyHash2(schemaOnlyHash);
    for (int i = 0; i < ntestsSchema; ++i) {
        schemaOnlyHash2.set("counter", i);
        p->save2(schemaOnlyHash2, archiveSchema);
    }

    // Load back ...
    size_t bytes = 0;
    for (int i = 0; i < ntestsSchema; ++i) {
        hash.clear();
        schemaOnlyHash2.set("counter", i);
        bytes += p->load(hash, archiveSchema.data() + bytes, archiveSchema.size() - bytes);
        CPPUNIT_ASSERT(hash.fullyEquals(schemaOnlyHash2));
    }

    CPPUNIT_ASSERT_EQUAL(bytes, archiveSchema.size());

    tick = std::chrono::steady_clock::now();
    for (int i = 0; i < ntests; ++i) {
        hash.clear();
        p->load(hash, archive1);
    }
    diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick);
    ave = diff.count() / static_cast<double>(ntests);
    KARABO_LOG_FRAMEWORK_DEBUG << " Average de-serialization time: " << ave << " ms";
    CPPUNIT_ASSERT(karabo::data::similar(hash, m_hash));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hash.get<Hash>("hash"), "std::vector<char>"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(*hash.get<Hash::Pointer>("hash_ptr"), "std::vector<char> ptr"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hash.get<Schema>("schema").getParameterHash(), "std::vector<char> Schema"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hash.getAttribute<Schema>("schema", "schema").getParameterHash(),
                                            "std::vector<char> Schema - Attribute"));
    const vector<Hash>& vecHash = hash.get<vector<Hash>>("vec_hash");
    CPPUNIT_ASSERT_EQUAL(100ul, vecHash.size());
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(vecHash[0], "std::vector<char> vector<Hash>[0]")); // skip others...
    const vector<Hash::Pointer>& vecHashPtr = hash.get<vector<Hash::Pointer>>("vec_hash_ptr");
    CPPUNIT_ASSERT_EQUAL(10ul, vecHashPtr.size());
    CPPUNIT_ASSERT_NO_THROW(
          hashContentTest(*(vecHashPtr[0]), "std::vector<char> vector<Hash::Pointer>[0]")); // skip others...

    // serialising twice should give identical results:
    vector<char> archive2;
    p->save(hash, archive2);
    CPPUNIT_ASSERT(string(&archive1[0], archive1.size()) == string(&archive2[0], archive2.size()));

    // return; // till here OK
    // Now content test with BufferSet - allCopy
    karabo::data::BufferSet archiveBuf1(true); // allCopy
    CPPUNIT_ASSERT_NO_THROW(p->save(m_hash, archiveBuf1));

    // Check that it can be converted to boost buffers - and that there is one asio buffer per non-empty BufferSet
    // buffer
    vector<boost::asio::const_buffer> asioBuf1;
    CPPUNIT_ASSERT_NO_THROW(archiveBuf1.appendTo(asioBuf1));
    const std::vector<unsigned int> sizes1(archiveBuf1.sizes());
    CPPUNIT_ASSERT_EQUAL(sizes1.size() - std::count(sizes1.begin(), sizes1.end(), 0u), asioBuf1.size());

    Hash hashArchive1;
    CPPUNIT_ASSERT_NO_THROW(p->load(hashArchive1, archiveBuf1));
    CPPUNIT_ASSERT(karabo::data::similar(hashArchive1, m_hash));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hashArchive1.get<Hash>("hash"), "BufferSet(true)"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(*hashArchive1.get<Hash::Pointer>("hash_ptr"), "BufferSet(true) ptr"));
    CPPUNIT_ASSERT_NO_THROW(
          hashContentTest(hashArchive1.get<Schema>("schema").getParameterHash(), "BufferSet(true) Schema"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hashArchive1.getAttribute<Schema>("schema", "schema").getParameterHash(),
                                            "BufferSet(true) Schema - Attribute"));
    const vector<Hash>& vecHash1 = hashArchive1.get<vector<Hash>>("vec_hash");
    CPPUNIT_ASSERT_EQUAL(100ul, vecHash1.size());
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(vecHash1[0], "BufferSet(true) vector<Hash>[0]")); // skip others...
    const vector<Hash::Pointer>& vecHashPtr1 = hashArchive1.get<vector<Hash::Pointer>>("vec_hash_ptr");
    CPPUNIT_ASSERT_EQUAL(10ul, vecHashPtr1.size());
    CPPUNIT_ASSERT_NO_THROW(
          hashContentTest(*(vecHashPtr1[0]), "BufferSet(true) vector<Hash::Pointer>[0]")); // skip others...

    // Now content test with BufferSet - skip some copies
    karabo::data::BufferSet archiveBuf2(false); // avoid copy if possible
    Hash hashArchive2;
    CPPUNIT_ASSERT_NO_THROW(p->save(m_hash, archiveBuf2));

    // Check that it can be converted to boost buffers - and that there is one asio buffer per non-empty BufferSet
    // buffer
    vector<boost::asio::const_buffer> asioBuf2;
    CPPUNIT_ASSERT_NO_THROW(archiveBuf2.appendTo(asioBuf2));
    const std::vector<unsigned int> sizes2(archiveBuf2.sizes());
    CPPUNIT_ASSERT_EQUAL(sizes2.size() - std::count(sizes2.begin(), sizes2.end(), 0u), asioBuf2.size());

    CPPUNIT_ASSERT_NO_THROW(p->load(hashArchive2, archiveBuf2));
    CPPUNIT_ASSERT(karabo::data::similar(hashArchive2, m_hash));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hashArchive2.get<Hash>("hash"), "BufferSet(false)"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(*hashArchive2.get<Hash::Pointer>("hash_ptr"), "BufferSet(false) ptr"));
    CPPUNIT_ASSERT_NO_THROW(
          hashContentTest(hashArchive2.get<Schema>("schema").getParameterHash(), "BufferSet(false) Schema"));
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(hashArchive2.getAttribute<Schema>("schema", "schema").getParameterHash(),
                                            "BufferSet(false) Schema - Attribute"));
    const vector<Hash>& vecHash2 = hashArchive2.get<vector<Hash>>("vec_hash");
    CPPUNIT_ASSERT_EQUAL(100ul, vecHash2.size());
    CPPUNIT_ASSERT_NO_THROW(hashContentTest(vecHash2[0], "BufferSet(false) vector<Hash>[0]")); // skip others...
    const vector<Hash::Pointer>& vecHashPtr2 = hashArchive2.get<vector<Hash::Pointer>>("vec_hash_ptr");
    CPPUNIT_ASSERT_EQUAL(10ul, vecHashPtr2.size());
    CPPUNIT_ASSERT_NO_THROW(
          hashContentTest(*(vecHashPtr2[0]), "BufferSet(false) vector<Hash::Pointer>[0]")); // skip others...
}


void HashBinarySerializer_Test::hashContentTest(const Hash& innerHash, const std::string& serialisationType) {
    // PODs and complex
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, true, innerHash.get<bool>("bool"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 'c', innerHash.get<char>("char"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned char>(8),
                                 innerHash.get<unsigned char>("uint8"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<signed char>(-8), innerHash.get<signed char>("int8"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned short>(16),
                                 innerHash.get<unsigned short>("uint16"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<short>(-16), innerHash.get<short>("int16"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 32u, innerHash.get<unsigned int>("uint32"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -32, innerHash.get<int>("int32"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 64ull, innerHash.get<unsigned long long>("uint64"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -64ll, innerHash.get<long long>("int64"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.141f, innerHash.get<float>("float"), 1.e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.14159265359, innerHash.get<double>("double"), 1.e-15);
    const auto complexF = innerHash.get<complex<float>>("cf");
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 1.f, complexF.real(), 1.e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 2.f, complexF.imag(), 1.e-7);
    const auto complexD = innerHash.get<complex<double>>("cd");
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3., complexD.real(), 1.e-15);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 4., complexD.imag(), 1.e-15);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, std::string("Hello Karabo"), innerHash.get<string>("str"));
    // Some selected NDArray value tests
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1, innerHash.get<NDArray>("ndarr").getData<int>()[42]);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 10ull, innerHash.get<NDArray>("ndarr").getShape().x3());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 0ul, innerHash.get<NDArray>("ndarrEmpty").byteSize());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 0ul, innerHash.get<NDArray>("ndarrEmpty").size());

    // attributes
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, true, innerHash.getAttribute<bool>("bool", "bool"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 'c', innerHash.getAttribute<char>("char", "char"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned char>(8),
                                 innerHash.getAttribute<unsigned char>("uint8", "uint8"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<signed char>(-8),
                                 innerHash.getAttribute<signed char>("int8", "int8"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned short>(16),
                                 innerHash.getAttribute<unsigned short>("uint16", "uint16"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<short>(-16),
                                 innerHash.getAttribute<short>("int16", "int16"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 32u, innerHash.getAttribute<unsigned int>("uint32", "uint32"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -32, innerHash.getAttribute<int>("int32", "int32"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 64ull,
                                 innerHash.getAttribute<unsigned long long>("uint64", "uint64"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -64ll, innerHash.getAttribute<long long>("int64", "int64"));
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.141f, innerHash.getAttribute<float>("float", "float"),
                                         1.e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.14159265359,
                                         innerHash.getAttribute<double>("double", "double"), 1.e-15);
    const auto complexFattr = innerHash.getAttribute<complex<float>>("cf", "cf");
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 1.f, complexFattr.real(), 1.e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 2.f, complexFattr.imag(), 1.e-7);
    const auto complexDattr = innerHash.getAttribute<complex<double>>("cd", "cd");
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3., complexDattr.real(), 1.e-15);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 4., complexDattr.imag(), 1.e-15);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, std::string("Hello Karabo"),
                                 innerHash.getAttribute<string>("str", "str"));
    // test here NDArray attribute?


    // vector values
    auto vecBool = innerHash.get<vector<bool>>("vec_bool");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecBool.size());
    CPPUNIT_ASSERT_MESSAGE(serialisationType, vecBool[0]);
    auto vecChar = innerHash.get<vector<char>>("vec_char");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecChar.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 'c', vecChar[0]);
    auto vecUint8 = innerHash.get<vector<unsigned char>>("vec_uint8");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint8.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned char>(8), vecUint8[0]);
    auto vecInt8 = innerHash.get<vector<signed char>>("vec_int8");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt8.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<signed char>(-8), vecInt8[0]);
    auto vecUint16 = innerHash.get<vector<unsigned short>>("vec_uint16");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint16.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned short>(16), vecUint16[0]);
    auto vecInt16 = innerHash.get<vector<short>>("vec_int16");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt16.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<short>(-16), vecInt16[0]);
    auto vecUint32 = innerHash.get<vector<unsigned int>>("vec_uint32");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint32.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 32u, vecUint32[0]);
    auto vecInt32 = innerHash.get<vector<int>>("vec_int32");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt32.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -32, vecInt32[0]);
    auto vecUint64 = innerHash.get<vector<unsigned long long>>("vec_uint64");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint64.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 64ull, vecUint64[0]);
    auto vecInt64 = innerHash.get<vector<long long>>("vec_int64");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt64.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -64ll, vecInt64[0]);

    auto vecFloat = innerHash.get<vector<float>>("vec_float");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecFloat.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.141f, vecFloat[0], 1.e-7);
    auto vecDouble = innerHash.get<vector<double>>("vec_double");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecDouble.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.14159265359, vecDouble[0], 1.e-15);
    auto vecCf = innerHash.get<vector<complex<float>>>("vec_cf");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecCf.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 1., vecCf[0].real(), 1.e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 2., vecCf[0].imag(), 1.e-7);
    auto vecCd = innerHash.get<vector<complex<double>>>("vec_cd");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecCd.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3., vecCd[0].real(), 1.e-15);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 4., vecCd[0].imag(), 1.e-15);

    auto vecString = innerHash.get<vector<string>>("vec_str");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecString.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, string("Hello Karabo"), vecString[0]);

    // vector attributes
    vecBool = innerHash.getAttribute<vector<bool>>("vec_bool", "vec_bool");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecBool.size());
    CPPUNIT_ASSERT_MESSAGE(serialisationType, vecBool[0]);
    vecChar = innerHash.getAttribute<vector<char>>("vec_char", "vec_char");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecChar.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 'c', vecChar[0]);
    vecUint8 = innerHash.getAttribute<vector<unsigned char>>("vec_uint8", "vec_uint8");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint8.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned char>(8), vecUint8[0]);
    vecInt8 = innerHash.getAttribute<vector<signed char>>("vec_int8", "vec_int8");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt8.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<signed char>(-8), vecInt8[0]);
    vecUint16 = innerHash.getAttribute<vector<unsigned short>>("vec_uint16", "vec_uint16");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint16.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<unsigned short>(16), vecUint16[0]);
    vecInt16 = innerHash.getAttribute<vector<short>>("vec_int16", "vec_int16");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt16.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, static_cast<short>(-16), vecInt16[0]);
    vecUint32 = innerHash.getAttribute<vector<unsigned int>>("vec_uint32", "vec_uint32");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint32.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 32u, vecUint32[0]);
    vecInt32 = innerHash.getAttribute<vector<int>>("vec_int32", "vec_int32");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt32.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -32, vecInt32[0]);
    vecUint64 = innerHash.getAttribute<vector<unsigned long long>>("vec_uint64", "vec_uint64");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecUint64.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 64ull, vecUint64[0]);
    vecInt64 = innerHash.getAttribute<vector<long long>>("vec_int64", "vec_int64");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecInt64.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, -64ll, vecInt64[0]);

    vecFloat = innerHash.getAttribute<vector<float>>("vec_float", "vec_float");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecFloat.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.141f, vecFloat[0], 1.e-7);
    vecDouble = innerHash.getAttribute<vector<double>>("vec_double", "vec_double");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecDouble.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3.14159265359, vecDouble[0], 1.e-15);
    vecCf = innerHash.getAttribute<vector<complex<float>>>("vec_cf", "vec_cf");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecCf.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 1., vecCf[0].real(), 1.e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 2., vecCf[0].imag(), 1.e-7);
    vecCd = innerHash.getAttribute<vector<complex<double>>>("vec_cd", "vec_cd");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecCd.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 3., vecCd[0].real(), 1.e-15);
    CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(serialisationType, 4., vecCd[0].imag(), 1.e-15);

    vecString = innerHash.getAttribute<vector<string>>("vec_str", "vec_str");
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, 1000ul, vecString.size());
    CPPUNIT_ASSERT_EQUAL_MESSAGE(serialisationType, string("Hello Karabo"), vecString[0]);
}


void HashBinarySerializer_Test::testSpeedLargeArrays() {
    Hash h;
    NDArray ndarr(Dims(256, 256, 512), karabo::data::Types::DOUBLE);
    double* dptr = reinterpret_cast<double*>(ndarr.getDataPtr().get());
    for (size_t i = 0; i != ndarr.size(); ++i) {
        dptr[i] = i % 100;
    }

    h.set("ndarr", ndarr);

    int numTries = 10;
    auto printSerializationTime = [&numTries](const std::chrono::duration<long long, std::milli>& diff,
                                              size_t sizeInBytes) {
        const float ave = diff.count() / static_cast<float>(numTries);
        std::clog << " --- Average serialization time: " << ave << " ms for Hash of size: " << sizeInBytes * 1.e-6
                  << " MB" << std::endl;
    };
    auto printDeserializationTime = [&numTries](const std::chrono::duration<long long, std::milli>& diff) {
        const float ave = diff.count() / static_cast<float>(numTries);
        std::clog << " --- Average de-serialization time: " << ave << " ms" << std::endl;
    };

    BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Bin");

    //////////////////////////////////////////////////
    //////////////////////////////////////////////////
    //////////////////////////////////////////////////
    std::clog << "\nvector<char> copy -- allocate always...\n";
    auto tick = std::chrono::steady_clock::now();

    size_t totalSize = 0;
    for (int i = 0; i < numTries; ++i) {
        // To count also the time needed for space allocation for the target vector during serialisation, we always
        // start with a fresh vector. To get the last result out of the loop, we just swap, i.e. copy pointers.
        vector<char> vecInLoop;
        p->save(h, vecInLoop);
        if (i == numTries - 1) {
            totalSize = vecInLoop.size();
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick);
            printSerializationTime(dur, totalSize);
        }
    }

    //////////////////////////////////////////////////
    //////////////////////////////////////////////////
    //////////////////////////////////////////////////
    std::clog << "\nvector<char> copy -- re-use memory...\n";
    vector<char> archive1;
    archive1.reserve(totalSize); // pre-allocate capacity
    tick = std::chrono::steady_clock::now();

    for (int i = 0; i < numTries; ++i) {
        p->save(h, archive1);
    }

    {
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick);
        printSerializationTime(dur, archive1.size());
    }
    Hash dh;
    tick = std::chrono::steady_clock::now();
    for (int i = 0; i < numTries; ++i) {
        Hash hInternal;
        p->load(hInternal, archive1);
        if (i == numTries - 1) {
            dh = std::move(hInternal);
        }
    }
    {
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick);
        printDeserializationTime(dur);
    }

    CPPUNIT_ASSERT(karabo::data::similar(h, dh));
    //////////////////////////////////////////////////
    //////////////////////////////////////////////////
    //////////////////////////////////////////////////
    {
        std::clog << "\nBufferSet copy ...\n";
        karabo::data::BufferSet archive3(true);
        tick = std::chrono::steady_clock::now();

        for (int i = 0; i < numTries; ++i) {
            p->save(h, archive3);
        }

        {
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick);
            printSerializationTime(dur, archive3.totalSize());
        }
        archive3.rewind();
        Hash dh2;
        tick = std::chrono::steady_clock::now();
        for (int i = 0; i < numTries; ++i) {
            Hash hInternal;
            p->load(hInternal, archive3);
            if (i == numTries - 1) {
                dh2 = std::move(hInternal);
            }
        }
        {
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick);
            printDeserializationTime(dur);
        }
        std::clog << "------ " << archive3 << std::endl;

        // Check the content of boost::asio::buffer ....
        std::clog << "\tListing of Boost ASIO buffers ...\n";
        vector<boost::asio::const_buffer> buf;
        CPPUNIT_ASSERT_NO_THROW(archive3.appendTo(buf));
        std::size_t idx = 0;
        for (auto it = buf.begin(); it != buf.end(); ++it, ++idx) {
            size_t size = boost::asio::buffer_size(*it);
            std::clog << std::dec << "\tidx=" << idx << "\t size=" << std::setw(12) << std::setfill(' ') << size
                      << "  ->  0x" << std::hex;
            for (size_t i = 0; i != std::min(size, size_t(30)); ++i) {
                std::clog << std::setw(2) << std::setfill('0') << int(boost::asio::buffer_cast<const char*>(*it)[i]);
            }
            std::clog << (size > 30 ? "..." : "") << std::endl;
        }

        CPPUNIT_ASSERT(karabo::data::similar(h, dh2));
        bool all_same = true;

        // verify that we do not have any byte shifting in between serialization and deserialization
        NDArray tarr = dh2.get<NDArray>("ndarr");
        CPPUNIT_ASSERT(ndarr.itemSize() == tarr.itemSize());
        CPPUNIT_ASSERT(ndarr.byteSize() == tarr.byteSize());
        for (int i = 0; i != 100; ++i) {
            all_same &= (ndarr.getDataPtr().get()[i] == tarr.getDataPtr().get()[i]);
        }
        CPPUNIT_ASSERT(all_same);

        for (int i = 0; i != 100; ++i) {
            all_same &= (ndarr.getDataPtr().get()[ndarr.byteSize() - i - 1] ==
                         tarr.getDataPtr().get()[ndarr.byteSize() - i - 1]);
        }
        CPPUNIT_ASSERT(all_same);
    }

    //////////////////////////////////////////////////
    //////////////////////////////////////////////////
    //////////////////////////////////////////////////
    {
        std::clog << "\n--- BufferSet no copy...\n";
        numTries = 1000; // This is so fast that we can afford much more tries to get a nice average.
        karabo::data::BufferSet archive3(false);
        tick = std::chrono::steady_clock::now();

        for (int i = 0; i < numTries; ++i) {
            p->save(h, archive3);
        }
        {
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick);
            printSerializationTime(dur, archive3.totalSize());
        }

        archive3.rewind();
        Hash dh3;
        tick = std::chrono::steady_clock::now();
        for (int i = 0; i < numTries; ++i) {
            Hash hInternal;
            p->load(hInternal, archive3);
            if (i == numTries - 1) {
                dh3 = std::move(hInternal);
            }
        }
        {
            auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick);
            printDeserializationTime(dur);
        }
        std::clog << "------ " << archive3 << std::endl;

        // Check the content of boost::asio::buffer ....
        std::clog << "\tListing of Boost ASIO buffers ...\n";
        vector<boost::asio::const_buffer> buf;
        CPPUNIT_ASSERT_NO_THROW(archive3.appendTo(buf));
        std::size_t idx = 0;
        for (auto it = buf.begin(); it != buf.end(); ++it, ++idx) {
            size_t size = boost::asio::buffer_size(*it);
            std::clog << std::dec << "\tidx=" << idx << "\t size=" << std::setw(12) << std::setfill(' ') << size
                      << "  ->  0x" << std::hex;
            for (size_t i = 0; i != std::min(size, size_t(30)); ++i) {
                std::clog << std::setw(2) << std::setfill('0') << int(boost::asio::buffer_cast<const char*>(*it)[i]);
            }
            std::clog << (size > 30 ? "..." : "") << std::endl;
        }
        CPPUNIT_ASSERT(karabo::data::similar(h, dh3));

        bool all_same = true;
        // verify that we do not have any byte shifting in between serialization and deserialization
        NDArray tarr = dh3.get<NDArray>("ndarr");
        CPPUNIT_ASSERT(ndarr.itemSize() == tarr.itemSize());
        CPPUNIT_ASSERT(ndarr.byteSize() == tarr.byteSize());
        for (int i = 0; i != 100; ++i) {
            all_same &= (ndarr.getDataPtr().get()[i] == tarr.getDataPtr().get()[i]);
        }
        CPPUNIT_ASSERT(all_same);

        for (int i = 0; i != 100; ++i) {
            all_same &= (ndarr.getDataPtr().get()[ndarr.byteSize() - i - 1] ==
                         tarr.getDataPtr().get()[ndarr.byteSize() - i - 1]);
        }
        CPPUNIT_ASSERT(all_same);
    }
}


void HashBinarySerializer_Test::testMaxHashKeyLength() {
    BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Bin");
    Hash h;
    vector<char> archive;

    std::string key(254, 'a');
    h.set<char>(key, 'c');
    CPPUNIT_ASSERT_NO_THROW(p->save(h, archive));

    key += 'a';
    h.set<char>(key, 'c');
    CPPUNIT_ASSERT_NO_THROW(p->save(h, archive));

    key += 'a';
    h.set<char>(key, 'c');
    CPPUNIT_ASSERT_THROW(p->save(h, archive), karabo::data::IOException);
}

void HashBinarySerializer_Test::testReadVectorHashPointer() {
    BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Bin");

    std::vector<Hash::Pointer> ptrs;
    ptrs.push_back(Hash::Pointer(new Hash("a", 1)));
    ptrs.push_back(Hash::Pointer(new Hash("b", 2)));

    const Hash h("ptrs", ptrs);

    {
        // Test writing to/reading from vector<char>
        vector<char> archive;

        CPPUNIT_ASSERT_NO_THROW(p->save(h, archive));

        Hash hashRead;
        CPPUNIT_ASSERT_NO_THROW(p->load(hashRead, archive));

        // CPPUNIT_ASSERT(hashRead.fullyEquals(h)); fullyEquals does not support VECTOR_HASH_POINTER
        CPPUNIT_ASSERT_EQUAL(1ul, hashRead.size());
        CPPUNIT_ASSERT_MESSAGE(toString(hashRead), hashRead.has("ptrs"));
        CPPUNIT_ASSERT_MESSAGE(toString(hashRead), hashRead.is<std::vector<Hash::Pointer>>("ptrs"));
        auto& vec = hashRead.get<std::vector<Hash::Pointer>>("ptrs");
        CPPUNIT_ASSERT_EQUAL(2ul, vec.size());
        CPPUNIT_ASSERT_EQUAL(1ul, vec[0]->size());
        CPPUNIT_ASSERT(vec[0]->has("a"));
        CPPUNIT_ASSERT(!vec[0]->has("b"));
        CPPUNIT_ASSERT_EQUAL(1, vec[0]->get<int>("a"));
        CPPUNIT_ASSERT_EQUAL(1ul, vec[1]->size());
        CPPUNIT_ASSERT(!vec[1]->has("a"));
        CPPUNIT_ASSERT(vec[1]->has("b"));
        CPPUNIT_ASSERT_EQUAL(2, vec[1]->get<int>("b"));
    }
    {
        // Test writing to/reading from BufferSet
        BufferSet archive;

        CPPUNIT_ASSERT_NO_THROW(p->save(h, archive));

        Hash hashRead;
        CPPUNIT_ASSERT_NO_THROW(p->load(hashRead, archive));

        // CPPUNIT_ASSERT(hashRead.fullyEquals(h)); fullyEquals does not support VECTOR_HASH_POINTER
        CPPUNIT_ASSERT_EQUAL(1ul, hashRead.size());
        CPPUNIT_ASSERT_MESSAGE(toString(hashRead), hashRead.has("ptrs"));
        CPPUNIT_ASSERT_MESSAGE(toString(hashRead), hashRead.is<std::vector<Hash::Pointer>>("ptrs"));
        auto& vec = hashRead.get<std::vector<Hash::Pointer>>("ptrs");
        CPPUNIT_ASSERT_EQUAL(2ul, vec.size());
        CPPUNIT_ASSERT_EQUAL(1ul, vec[0]->size());
        CPPUNIT_ASSERT(vec[0]->has("a"));
        CPPUNIT_ASSERT(!vec[0]->has("b"));
        CPPUNIT_ASSERT_EQUAL(1, vec[0]->get<int>("a"));
        CPPUNIT_ASSERT_EQUAL(1ul, vec[1]->size());
        CPPUNIT_ASSERT(!vec[1]->has("a"));
        CPPUNIT_ASSERT(vec[1]->has("b"));
        CPPUNIT_ASSERT_EQUAL(2, vec[1]->get<int>("b"));
    }
}

void HashBinarySerializer_Test::testSpecialSeparator() {
    BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Bin");

    // Create Hash where one key contains the default seperator
    Hash h("a", 1, "b.c", 2);
    const char separator = '\0';
    CPPUNIT_ASSERT(separator != Hash::k_defaultSep);
    h.set("e.f", 3, separator); // "e.f" will be a first level key, not a path

    {
        // Serialize to and deserialze from vector<char> archive
        std::vector<char> archive;
        CPPUNIT_ASSERT_NO_THROW(p->save(h, archive));
        Hash deserializedHash;
        CPPUNIT_ASSERT_NO_THROW(p->load(deserializedHash, archive));

        std::ostringstream str;
        str << "Before serialisation: " << h << "After deserialisation:" << deserializedHash;
        CPPUNIT_ASSERT_MESSAGE(str.str(), h.fullyEquals(deserializedHash));
    }
    {
        // Serialize to and deserialze from BufferSet archive
        BufferSet bufferArchive;
        CPPUNIT_ASSERT_NO_THROW(p->save(h, bufferArchive));
        Hash deserializedHash;
        CPPUNIT_ASSERT_NO_THROW(p->load(deserializedHash, bufferArchive));

        std::ostringstream str;
        str << "Before serialisation: " << h << "After deserialisation:" << deserializedHash;
        CPPUNIT_ASSERT_MESSAGE(str.str(), h.fullyEquals(deserializedHash));
    }
}
