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
 * File:   HashBinarySerializer_Test.hh
 * Author: heisenb
 *
 * Created on February 25, 2013, 6:03 PM
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <karabo/log/Logger.hh>

#include "karabo/data/io/BinarySerializer.hh"
#include "karabo/data/io/HashBinarySerializer.hh"
#include "karabo/data/io/TextSerializer.hh"
#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/NDArray.hh"
#include "karabo/util/TimeProfiler.hh"


using namespace karabo::data;
using std::complex;
using std::string;
using std::vector;

static void hashContentTest(const Hash& innerHash, const std::string& serialisationType);


TEST(TestHashBinarySerializer, testSerialization) {
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
    Hash someHash;
    someHash.set<Hash>("hash", h);
    someHash.set<Hash::Pointer>("hash_ptr", std::make_shared<Hash>(h));
    someHash.set<vector<Hash>>("vec_hash", vector<Hash>(100, h));
    someHash.set<vector<Hash::Pointer>>("vec_hash_ptr", vector<Hash::Pointer>(10, std::make_shared<Hash>(h)));
    Schema s;
    s.setParameterHash(h);
    someHash.set<Schema>("schema", s);
    someHash.setAttribute("schema", "schema", s);

    BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Bin");
    vector<char> archive1;
    std::chrono::steady_clock::time_point tick = std::chrono::steady_clock::now();
    const int ntests = 1; // for measurements, better increase...
    for (int i = 0; i < ntests; ++i) {
        p->save(someHash, archive1);
    }
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick);
    float ave = diff.count() / static_cast<double>(ntests);
    KARABO_LOG_FRAMEWORK_DEBUG_C("TestHashBinarySerializer")
          << " Average serialization time: " << ave << " ms for Hash of size: " << archive1.size() / 10.e6 << " MB";

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
    KARABO_LOG_FRAMEWORK_DEBUG_C("TestHashBinarySerializer")
          << " Average serialization time schema only: " << ave << " ms";

    Hash hash;
    tick = std::chrono::steady_clock::now();
    for (int i = 0; i < ntestsSchema; ++i) {
        hash.clear();
        size_t size = p->load(hash, archiveSchema);
        EXPECT_EQ(archiveSchema.size(), size);
    }
    diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick);
    ave = diff.count() / static_cast<double>(ntestsSchema);
    KARABO_LOG_FRAMEWORK_DEBUG_C("TestHashBinarySerializer")
          << " Average de-serialization time schema only: " << ave << " ms";

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
        EXPECT_TRUE(hash.fullyEquals(schemaOnlyHash2));
    }

    EXPECT_EQ(bytes, archiveSchema.size());

    tick = std::chrono::steady_clock::now();
    for (int i = 0; i < ntests; ++i) {
        hash.clear();
        p->load(hash, archive1);
    }
    diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tick);
    ave = diff.count() / static_cast<double>(ntests);
    KARABO_LOG_FRAMEWORK_DEBUG_C("TestHashBinarySerializer") << " Average de-serialization time: " << ave << " ms";
    EXPECT_TRUE(karabo::data::similar(hash, someHash));
    EXPECT_NO_THROW(hashContentTest(hash.get<Hash>("hash"), "std::vector<char>"));
    EXPECT_NO_THROW(hashContentTest(*hash.get<Hash::Pointer>("hash_ptr"), "std::vector<char> ptr"));
    EXPECT_NO_THROW(hashContentTest(hash.get<Schema>("schema").getParameterHash(), "std::vector<char> Schema"));
    EXPECT_NO_THROW(hashContentTest(hash.getAttribute<Schema>("schema", "schema").getParameterHash(),
                                    "std::vector<char> Schema - Attribute"));
    const vector<Hash>& vecHash = hash.get<vector<Hash>>("vec_hash");
    EXPECT_EQ(100ul, vecHash.size());
    EXPECT_NO_THROW(hashContentTest(vecHash[0], "std::vector<char> vector<Hash>[0]")); // skip others...
    const vector<Hash::Pointer>& vecHashPtr = hash.get<vector<Hash::Pointer>>("vec_hash_ptr");
    EXPECT_EQ(10ul, vecHashPtr.size());
    EXPECT_NO_THROW(hashContentTest(*(vecHashPtr[0]), "std::vector<char> vector<Hash::Pointer>[0]")); // skip others...

    // serialising twice should give identical results:
    vector<char> archive2;
    p->save(hash, archive2);
    EXPECT_TRUE(string(&archive1[0], archive1.size()) == string(&archive2[0], archive2.size()));

    // return; // till here OK
    // Now content test with BufferSet - allCopy
    karabo::data::BufferSet archiveBuf1(true); // allCopy
    EXPECT_NO_THROW(p->save(someHash, archiveBuf1));

    // Check that it can be converted to boost buffers - and that there is one asio buffer per non-empty BufferSet
    // buffer
    vector<boost::asio::const_buffer> asioBuf1;
    EXPECT_NO_THROW(archiveBuf1.appendTo(asioBuf1));
    const std::vector<unsigned int> sizes1(archiveBuf1.sizes());
    EXPECT_EQ(sizes1.size() - std::count(sizes1.begin(), sizes1.end(), 0u), asioBuf1.size());

    Hash hashArchive1;
    EXPECT_NO_THROW(p->load(hashArchive1, archiveBuf1));
    EXPECT_TRUE(karabo::data::similar(hashArchive1, someHash));
    EXPECT_NO_THROW(hashContentTest(hashArchive1.get<Hash>("hash"), "BufferSet(true)"));
    EXPECT_NO_THROW(hashContentTest(*hashArchive1.get<Hash::Pointer>("hash_ptr"), "BufferSet(true) ptr"));
    EXPECT_NO_THROW(hashContentTest(hashArchive1.get<Schema>("schema").getParameterHash(), "BufferSet(true) Schema"));
    EXPECT_NO_THROW(hashContentTest(hashArchive1.getAttribute<Schema>("schema", "schema").getParameterHash(),
                                    "BufferSet(true) Schema - Attribute"));
    const vector<Hash>& vecHash1 = hashArchive1.get<vector<Hash>>("vec_hash");
    EXPECT_EQ(100ul, vecHash1.size());
    EXPECT_NO_THROW(hashContentTest(vecHash1[0], "BufferSet(true) vector<Hash>[0]")); // skip others...
    const vector<Hash::Pointer>& vecHashPtr1 = hashArchive1.get<vector<Hash::Pointer>>("vec_hash_ptr");
    EXPECT_EQ(10ul, vecHashPtr1.size());
    EXPECT_NO_THROW(hashContentTest(*(vecHashPtr1[0]), "BufferSet(true) vector<Hash::Pointer>[0]")); // skip others...

    // Now content test with BufferSet - skip some copies
    karabo::data::BufferSet archiveBuf2(false); // avoid copy if possible
    Hash hashArchive2;
    EXPECT_NO_THROW(p->save(someHash, archiveBuf2));

    // Check that it can be converted to boost buffers - and that there is one asio buffer per non-empty BufferSet
    // buffer
    vector<boost::asio::const_buffer> asioBuf2;
    EXPECT_NO_THROW(archiveBuf2.appendTo(asioBuf2));
    const std::vector<unsigned int> sizes2(archiveBuf2.sizes());
    EXPECT_EQ(sizes2.size() - std::count(sizes2.begin(), sizes2.end(), 0u), asioBuf2.size());

    EXPECT_NO_THROW(p->load(hashArchive2, archiveBuf2));
    EXPECT_TRUE(karabo::data::similar(hashArchive2, someHash));
    EXPECT_NO_THROW(hashContentTest(hashArchive2.get<Hash>("hash"), "BufferSet(false)"));
    EXPECT_NO_THROW(hashContentTest(*hashArchive2.get<Hash::Pointer>("hash_ptr"), "BufferSet(false) ptr"));
    EXPECT_NO_THROW(hashContentTest(hashArchive2.get<Schema>("schema").getParameterHash(), "BufferSet(false) Schema"));
    EXPECT_NO_THROW(hashContentTest(hashArchive2.getAttribute<Schema>("schema", "schema").getParameterHash(),
                                    "BufferSet(false) Schema - Attribute"));
    const vector<Hash>& vecHash2 = hashArchive2.get<vector<Hash>>("vec_hash");
    EXPECT_EQ(100ul, vecHash2.size());
    EXPECT_NO_THROW(hashContentTest(vecHash2[0], "BufferSet(false) vector<Hash>[0]")); // skip others...
    const vector<Hash::Pointer>& vecHashPtr2 = hashArchive2.get<vector<Hash::Pointer>>("vec_hash_ptr");
    EXPECT_EQ(10ul, vecHashPtr2.size());
    EXPECT_NO_THROW(hashContentTest(*(vecHashPtr2[0]), "BufferSet(false) vector<Hash::Pointer>[0]")); // skip others...
}


void hashContentTest(const Hash& innerHash, const std::string& serialisationType) {
    // PODs and complex
    EXPECT_EQ(true, innerHash.get<bool>("bool")) << serialisationType;
    EXPECT_EQ('c', innerHash.get<char>("char")) << serialisationType;
    EXPECT_EQ(static_cast<unsigned char>(8), innerHash.get<unsigned char>("uint8")) << serialisationType;
    EXPECT_EQ(static_cast<signed char>(-8), innerHash.get<signed char>("int8")) << serialisationType;
    EXPECT_EQ(static_cast<unsigned short>(16), innerHash.get<unsigned short>("uint16")) << serialisationType;
    EXPECT_EQ(static_cast<short>(-16), innerHash.get<short>("int16")) << serialisationType;
    EXPECT_EQ(32u, innerHash.get<unsigned int>("uint32")) << serialisationType;
    EXPECT_EQ(-32, innerHash.get<int>("int32")) << serialisationType;
    EXPECT_EQ(64ull, innerHash.get<unsigned long long>("uint64")) << serialisationType;
    EXPECT_EQ(-64ll, innerHash.get<long long>("int64")) << serialisationType;
    EXPECT_NEAR(3.141f, innerHash.get<float>("float"), 1.e-7) << serialisationType;
    EXPECT_NEAR(3.14159265359, innerHash.get<double>("double"), 1.e-15) << serialisationType;
    const auto complexF = innerHash.get<complex<float>>("cf");
    EXPECT_NEAR(1.f, complexF.real(), 1.e-7) << serialisationType;
    EXPECT_NEAR(2.f, complexF.imag(), 1.e-7) << serialisationType;
    const auto complexD = innerHash.get<complex<double>>("cd");
    EXPECT_NEAR(3., complexD.real(), 1.e-15) << serialisationType;
    EXPECT_NEAR(4., complexD.imag(), 1.e-15) << serialisationType;
    EXPECT_STREQ("Hello Karabo", innerHash.get<string>("str").c_str()) << serialisationType;
    // Some selected NDArray value tests
    EXPECT_EQ(1, innerHash.get<NDArray>("ndarr").getData<int>()[42]) << serialisationType;
    EXPECT_EQ(10ull, innerHash.get<NDArray>("ndarr").getShape().x3()) << serialisationType;
    EXPECT_EQ(0ul, innerHash.get<NDArray>("ndarrEmpty").byteSize()) << serialisationType;
    EXPECT_EQ(0ul, innerHash.get<NDArray>("ndarrEmpty").size()) << serialisationType;

    // attributes
    EXPECT_EQ(true, innerHash.getAttribute<bool>("bool", "bool")) << serialisationType;
    EXPECT_EQ('c', innerHash.getAttribute<char>("char", "char")) << serialisationType;
    EXPECT_EQ(static_cast<unsigned char>(8), innerHash.getAttribute<unsigned char>("uint8", "uint8"))
          << serialisationType;
    EXPECT_EQ(static_cast<signed char>(-8), innerHash.getAttribute<signed char>("int8", "int8")) << serialisationType;
    EXPECT_EQ(static_cast<unsigned short>(16), innerHash.getAttribute<unsigned short>("uint16", "uint16"))
          << serialisationType;
    EXPECT_EQ(static_cast<short>(-16), innerHash.getAttribute<short>("int16", "int16")) << serialisationType;
    EXPECT_EQ(32u, innerHash.getAttribute<unsigned int>("uint32", "uint32")) << serialisationType;
    EXPECT_EQ(-32, innerHash.getAttribute<int>("int32", "int32")) << serialisationType;
    EXPECT_EQ(64ull, innerHash.getAttribute<unsigned long long>("uint64", "uint64")) << serialisationType;
    EXPECT_EQ(-64ll, innerHash.getAttribute<long long>("int64", "int64")) << serialisationType;
    EXPECT_NEAR(3.141f, innerHash.getAttribute<float>("float", "float"), 1.e-7) << serialisationType;
    EXPECT_NEAR(3.14159265359, innerHash.getAttribute<double>("double", "double"), 1.e-15) << serialisationType;
    const auto complexFattr = innerHash.getAttribute<complex<float>>("cf", "cf");
    EXPECT_NEAR(1.f, complexFattr.real(), 1.e-7) << serialisationType;
    EXPECT_NEAR(2.f, complexFattr.imag(), 1.e-7) << serialisationType;
    const auto complexDattr = innerHash.getAttribute<complex<double>>("cd", "cd");
    EXPECT_NEAR(3., complexDattr.real(), 1.e-15) << serialisationType;
    EXPECT_NEAR(4., complexDattr.imag(), 1.e-15) << serialisationType;
    EXPECT_STREQ("Hello Karabo", innerHash.getAttribute<string>("str", "str").c_str()) << serialisationType;
    // test here NDArray attribute?


    // vector values
    auto vecBool = innerHash.get<vector<bool>>("vec_bool");
    EXPECT_EQ(1000ul, vecBool.size()) << serialisationType;
    EXPECT_TRUE(vecBool[0]) << serialisationType;
    auto vecChar = innerHash.get<vector<char>>("vec_char");
    EXPECT_EQ(1000ul, vecChar.size()) << serialisationType;
    EXPECT_EQ('c', vecChar[0]) << serialisationType;
    auto vecUint8 = innerHash.get<vector<unsigned char>>("vec_uint8");
    EXPECT_EQ(1000ul, vecUint8.size()) << serialisationType;
    EXPECT_EQ(static_cast<unsigned char>(8), vecUint8[0]) << serialisationType;
    auto vecInt8 = innerHash.get<vector<signed char>>("vec_int8");
    EXPECT_EQ(1000ul, vecInt8.size()) << serialisationType;
    EXPECT_EQ(static_cast<signed char>(-8), vecInt8[0]) << serialisationType;
    auto vecUint16 = innerHash.get<vector<unsigned short>>("vec_uint16");
    EXPECT_EQ(1000ul, vecUint16.size()) << serialisationType;
    EXPECT_EQ(static_cast<unsigned short>(16), vecUint16[0]) << serialisationType;
    auto vecInt16 = innerHash.get<vector<short>>("vec_int16");
    EXPECT_EQ(1000ul, vecInt16.size()) << serialisationType;
    EXPECT_EQ(static_cast<short>(-16), vecInt16[0]) << serialisationType;
    auto vecUint32 = innerHash.get<vector<unsigned int>>("vec_uint32");
    EXPECT_EQ(1000ul, vecUint32.size()) << serialisationType;
    EXPECT_EQ(32u, vecUint32[0]) << serialisationType;
    auto vecInt32 = innerHash.get<vector<int>>("vec_int32");
    EXPECT_EQ(1000ul, vecInt32.size()) << serialisationType;
    EXPECT_EQ(-32, vecInt32[0]) << serialisationType;
    auto vecUint64 = innerHash.get<vector<unsigned long long>>("vec_uint64");
    EXPECT_EQ(1000ul, vecUint64.size());
    EXPECT_EQ(64ull, vecUint64[0]) << serialisationType;
    auto vecInt64 = innerHash.get<vector<long long>>("vec_int64");
    EXPECT_EQ(1000ul, vecInt64.size()) << serialisationType;
    EXPECT_EQ(-64ll, vecInt64[0]) << serialisationType;

    auto vecFloat = innerHash.get<vector<float>>("vec_float");
    EXPECT_EQ(1000ul, vecFloat.size()) << serialisationType;
    EXPECT_NEAR(3.141f, vecFloat[0], 1.e-7) << serialisationType;
    auto vecDouble = innerHash.get<vector<double>>("vec_double");
    EXPECT_EQ(1000ul, vecDouble.size());
    EXPECT_NEAR(3.14159265359, vecDouble[0], 1.e-15) << serialisationType;
    auto vecCf = innerHash.get<vector<complex<float>>>("vec_cf");
    EXPECT_EQ(1000ul, vecCf.size()) << serialisationType;
    EXPECT_NEAR(1., vecCf[0].real(), 1.e-7) << serialisationType;
    EXPECT_NEAR(2., vecCf[0].imag(), 1.e-7) << serialisationType;
    auto vecCd = innerHash.get<vector<complex<double>>>("vec_cd");
    EXPECT_EQ(1000ul, vecCd.size()) << serialisationType;
    EXPECT_NEAR(3., vecCd[0].real(), 1.e-15) << serialisationType;
    EXPECT_NEAR(4., vecCd[0].imag(), 1.e-15) << serialisationType;

    auto vecString = innerHash.get<vector<string>>("vec_str");
    EXPECT_EQ(1000ul, vecString.size()) << serialisationType;
    EXPECT_STREQ("Hello Karabo", vecString[0].c_str()) << serialisationType;

    // vector attributes
    vecBool = innerHash.getAttribute<vector<bool>>("vec_bool", "vec_bool");
    EXPECT_EQ(1000ul, vecBool.size()) << serialisationType;
    EXPECT_TRUE(vecBool[0]) << serialisationType;
    vecChar = innerHash.getAttribute<vector<char>>("vec_char", "vec_char");
    EXPECT_EQ(1000ul, vecChar.size()) << serialisationType;
    EXPECT_EQ('c', vecChar[0]) << serialisationType;
    vecUint8 = innerHash.getAttribute<vector<unsigned char>>("vec_uint8", "vec_uint8");
    EXPECT_EQ(1000ul, vecUint8.size()) << serialisationType;
    EXPECT_EQ(static_cast<unsigned char>(8), vecUint8[0]) << serialisationType;
    vecInt8 = innerHash.getAttribute<vector<signed char>>("vec_int8", "vec_int8");
    EXPECT_EQ(1000ul, vecInt8.size()) << serialisationType;
    EXPECT_EQ(static_cast<signed char>(-8), vecInt8[0]) << serialisationType;
    vecUint16 = innerHash.getAttribute<vector<unsigned short>>("vec_uint16", "vec_uint16");
    EXPECT_EQ(1000ul, vecUint16.size()) << serialisationType;
    EXPECT_EQ(static_cast<unsigned short>(16), vecUint16[0]) << serialisationType;
    vecInt16 = innerHash.getAttribute<vector<short>>("vec_int16", "vec_int16");
    EXPECT_EQ(1000ul, vecInt16.size()) << serialisationType;
    EXPECT_EQ(static_cast<short>(-16), vecInt16[0]) << serialisationType;
    vecUint32 = innerHash.getAttribute<vector<unsigned int>>("vec_uint32", "vec_uint32");
    EXPECT_EQ(1000ul, vecUint32.size()) << serialisationType;
    EXPECT_EQ(32u, vecUint32[0]) << serialisationType;
    vecInt32 = innerHash.getAttribute<vector<int>>("vec_int32", "vec_int32");
    EXPECT_EQ(1000ul, vecInt32.size()) << serialisationType;
    EXPECT_EQ(-32, vecInt32[0]) << serialisationType;
    vecUint64 = innerHash.getAttribute<vector<unsigned long long>>("vec_uint64", "vec_uint64");
    EXPECT_EQ(1000ul, vecUint64.size()) << serialisationType;
    EXPECT_EQ(64ull, vecUint64[0]) << serialisationType;
    vecInt64 = innerHash.getAttribute<vector<long long>>("vec_int64", "vec_int64");
    EXPECT_EQ(1000ul, vecInt64.size()) << serialisationType;
    EXPECT_EQ(-64ll, vecInt64[0]) << serialisationType;

    vecFloat = innerHash.getAttribute<vector<float>>("vec_float", "vec_float");
    EXPECT_EQ(1000ul, vecFloat.size()) << serialisationType;
    EXPECT_NEAR(3.141f, vecFloat[0], 1.e-7) << serialisationType;
    vecDouble = innerHash.getAttribute<vector<double>>("vec_double", "vec_double");
    EXPECT_EQ(1000ul, vecDouble.size()) << serialisationType;
    EXPECT_NEAR(3.14159265359, vecDouble[0], 1.e-15) << serialisationType;
    vecCf = innerHash.getAttribute<vector<complex<float>>>("vec_cf", "vec_cf");
    EXPECT_EQ(1000ul, vecCf.size()) << serialisationType;
    EXPECT_NEAR(1., vecCf[0].real(), 1.e-7) << serialisationType;
    EXPECT_NEAR(2., vecCf[0].imag(), 1.e-7) << serialisationType;
    vecCd = innerHash.getAttribute<vector<complex<double>>>("vec_cd", "vec_cd");
    EXPECT_EQ(1000ul, vecCd.size()) << serialisationType;
    EXPECT_NEAR(3., vecCd[0].real(), 1.e-15) << serialisationType;
    EXPECT_NEAR(4., vecCd[0].imag(), 1.e-15) << serialisationType;

    vecString = innerHash.getAttribute<vector<string>>("vec_str", "vec_str");
    EXPECT_EQ(1000ul, vecString.size()) << serialisationType;
    EXPECT_STREQ("Hello Karabo", vecString[0].c_str()) << serialisationType;
}


TEST(TestHashBinarySerializer, testSpeedLargeArrays) {
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

    EXPECT_TRUE(karabo::data::similar(h, dh));
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
        EXPECT_NO_THROW(archive3.appendTo(buf));
        std::size_t idx = 0;
        for (auto it = buf.begin(); it != buf.end(); ++it, ++idx) {
            const boost::asio::const_buffer& b = *it;
            size_t size = boost::asio::buffer_size(b);
            std::clog << std::dec << "\tidx=" << idx << "\t size=" << std::setw(12) << std::setfill(' ') << size
                      << "  ->  0x" << std::hex;
            for (size_t i = 0; i != std::min(size, size_t(30)); ++i) {
                const char* data = static_cast<const char*>(b.data());
                std::clog << std::setw(2) << std::setfill('0') << int(data[i]);
            }
            std::clog << (size > 30 ? "..." : "") << std::endl;
        }

        EXPECT_TRUE(karabo::data::similar(h, dh2));
        bool all_same = true;

        // verify that we do not have any byte shifting in between serialization and deserialization
        NDArray tarr = dh2.get<NDArray>("ndarr");
        EXPECT_TRUE(ndarr.itemSize() == tarr.itemSize());
        EXPECT_TRUE(ndarr.byteSize() == tarr.byteSize());
        for (int i = 0; i != 100; ++i) {
            all_same &= (ndarr.getDataPtr().get()[i] == tarr.getDataPtr().get()[i]);
        }
        EXPECT_TRUE(all_same);

        for (int i = 0; i != 100; ++i) {
            all_same &= (ndarr.getDataPtr().get()[ndarr.byteSize() - i - 1] ==
                         tarr.getDataPtr().get()[ndarr.byteSize() - i - 1]);
        }
        EXPECT_TRUE(all_same);
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
        EXPECT_NO_THROW(archive3.appendTo(buf));
        std::size_t idx = 0;
        for (auto it = buf.begin(); it != buf.end(); ++it, ++idx) {
            const boost::asio::const_buffer& b = *it;
            size_t size = boost::asio::buffer_size(b);
            std::clog << std::dec << "\tidx=" << idx << "\t size=" << std::setw(12) << std::setfill(' ') << size
                      << "  ->  0x" << std::hex;
            for (size_t i = 0; i != std::min(size, size_t(30)); ++i) {
                const char* data = static_cast<const char*>(b.data());
                std::clog << std::setw(2) << std::setfill('0') << int(data[i]);
            }
            std::clog << (size > 30 ? "..." : "") << std::endl;
        }
        EXPECT_TRUE(karabo::data::similar(h, dh3));

        bool all_same = true;
        // verify that we do not have any byte shifting in between serialization and deserialization
        NDArray tarr = dh3.get<NDArray>("ndarr");
        EXPECT_TRUE(ndarr.itemSize() == tarr.itemSize());
        EXPECT_TRUE(ndarr.byteSize() == tarr.byteSize());
        for (int i = 0; i != 100; ++i) {
            all_same &= (ndarr.getDataPtr().get()[i] == tarr.getDataPtr().get()[i]);
        }
        EXPECT_TRUE(all_same);

        for (int i = 0; i != 100; ++i) {
            all_same &= (ndarr.getDataPtr().get()[ndarr.byteSize() - i - 1] ==
                         tarr.getDataPtr().get()[ndarr.byteSize() - i - 1]);
        }
        EXPECT_TRUE(all_same);
    }
}


TEST(TestHashBinarySerializer, testMaxHashKeyLength) {
    BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Bin");
    Hash h;
    vector<char> archive;

    std::string key(254, 'a');
    h.set<char>(key, 'c');
    EXPECT_NO_THROW(p->save(h, archive));

    key += 'a';
    h.set<char>(key, 'c');
    EXPECT_NO_THROW(p->save(h, archive));

    key += 'a';
    h.set<char>(key, 'c');
    EXPECT_THROW(p->save(h, archive), karabo::data::IOException);
}

TEST(TestHashBinarySerializer, testReadVectorHashPointer) {
    BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Bin");

    std::vector<Hash::Pointer> ptrs;
    ptrs.push_back(Hash::Pointer(new Hash("a", 1)));
    ptrs.push_back(Hash::Pointer(new Hash("b", 2)));

    const Hash h("ptrs", ptrs);

    {
        // Test writing to/reading from vector<char>
        vector<char> archive;

        EXPECT_NO_THROW(p->save(h, archive));

        Hash hashRead;
        EXPECT_NO_THROW(p->load(hashRead, archive));

        // EXPECT_TRUE(hashRead.fullyEquals(h)); fullyEquals does not support VECTOR_HASH_POINTER
        EXPECT_EQ(1ul, hashRead.size());
        EXPECT_TRUE(hashRead.has("ptrs")) << toString(hashRead);
        EXPECT_TRUE(hashRead.is<std::vector<Hash::Pointer>>("ptrs")) << toString(hashRead);
        auto& vec = hashRead.get<std::vector<Hash::Pointer>>("ptrs");
        EXPECT_EQ(2ul, vec.size());
        EXPECT_EQ(1ul, vec[0]->size());
        EXPECT_TRUE(vec[0]->has("a"));
        EXPECT_TRUE(!vec[0]->has("b"));
        EXPECT_EQ(1, vec[0]->get<int>("a"));
        EXPECT_EQ(1ul, vec[1]->size());
        EXPECT_TRUE(!vec[1]->has("a"));
        EXPECT_TRUE(vec[1]->has("b"));
        EXPECT_EQ(2, vec[1]->get<int>("b"));
    }
    {
        // Test writing to/reading from BufferSet
        BufferSet archive;

        EXPECT_NO_THROW(p->save(h, archive));

        Hash hashRead;
        EXPECT_NO_THROW(p->load(hashRead, archive));

        // EXPECT_TRUE(hashRead.fullyEquals(h)); fullyEquals does not support VECTOR_HASH_POINTER
        EXPECT_EQ(1ul, hashRead.size());
        EXPECT_TRUE(hashRead.has("ptrs")) << toString(hashRead);
        EXPECT_TRUE(hashRead.is<std::vector<Hash::Pointer>>("ptrs")) << toString(hashRead);
        auto& vec = hashRead.get<std::vector<Hash::Pointer>>("ptrs");
        EXPECT_EQ(2ul, vec.size());
        EXPECT_EQ(1ul, vec[0]->size());
        EXPECT_TRUE(vec[0]->has("a"));
        EXPECT_TRUE(!vec[0]->has("b"));
        EXPECT_EQ(1, vec[0]->get<int>("a"));
        EXPECT_EQ(1ul, vec[1]->size());
        EXPECT_TRUE(!vec[1]->has("a"));
        EXPECT_TRUE(vec[1]->has("b"));
        EXPECT_EQ(2, vec[1]->get<int>("b"));
    }
}

TEST(TestHashBinarySerializer, testSpecialSeparator) {
    BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Bin");

    // Create Hash where one key contains the default seperator
    Hash h("a", 1, "b.c", 2);
    const char separator = '\0';
    EXPECT_TRUE(separator != Hash::k_defaultSep);
    h.set("e.f", 3, separator); // "e.f" will be a first level key, not a path

    {
        // Serialize to and deserialze from vector<char> archive
        std::vector<char> archive;
        EXPECT_NO_THROW(p->save(h, archive));
        Hash deserializedHash;
        EXPECT_NO_THROW(p->load(deserializedHash, archive));

        std::ostringstream str;
        str << "Before serialisation: " << h << "After deserialisation:" << deserializedHash;
        EXPECT_TRUE(h.fullyEquals(deserializedHash)) << str.str();
    }
    {
        // Serialize to and deserialze from BufferSet archive
        BufferSet bufferArchive;
        EXPECT_NO_THROW(p->save(h, bufferArchive));
        Hash deserializedHash;
        EXPECT_NO_THROW(p->load(deserializedHash, bufferArchive));

        std::ostringstream str;
        str << "Before serialisation: " << h << "After deserialisation:" << deserializedHash;
        EXPECT_TRUE(h.fullyEquals(deserializedHash)) << str.str();
    }
}
