/* 
 * File:   HashBinarySerializer_Test.cc
 * Author: heisenb
 * 
 * Created on February 25, 2013, 6:03 PM
 */

#include <karabo/io/HashBinarySerializer.hh>
#include "HashBinarySerializer_Test.hh"
#include "karabo/io/BinarySerializer.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(HashBinarySerializer_Test);

using namespace karabo::io;
using namespace karabo::util;

HashBinarySerializer_Test::HashBinarySerializer_Test() {
}

HashBinarySerializer_Test::~HashBinarySerializer_Test() {
}

void HashBinarySerializer_Test::setUp() {

    Hash rooted("a.b.c", 1, "a.b.d", vector<int>(5, 1), "a.b.e", vector<Hash > (2, Hash("a", 1)), "a.d", std::complex<double>(1.2, 4.2));
    rooted.setAttribute("a", "a1", true);
    rooted.setAttribute("a", "a2", 3.4);
    rooted.setAttribute("a.b", "b1", "3");
    rooted.setAttribute("a.b.c", "c1", 2);
    rooted.setAttribute("a.b.c", "c2", vector<string > (3, "bla"));
    m_rootedHash = rooted;


    Hash big("a.b", std::vector<double>(10000000, 1.0));
    vector<Hash>& tmp = big.bindReference<vector<Hash> >("a.c");
    tmp.resize(1000);
    for (size_t i = 0; i < tmp.size(); ++i) {
        tmp[i] = m_rootedHash;
    }
    m_bigHash = big;


    Hash unrooted("a.b.c", 1, "b.c", 2.0, "c", 3.f, "d.e", "4", "e.f.g.h", std::vector<unsigned long long > (5, 5), "F.f.f.f.f", Hash("x.y.z", 99));
    unrooted.setAttribute("F.f.f", "attr1", true);
    m_unrootedHash = unrooted;
}

void HashBinarySerializer_Test::tearDown() {
}

void HashBinarySerializer_Test::testSerialization() {

    BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Binary");

    {
        vector<char> archive1;
        vector<char> archive2;

        p->save(m_rootedHash, archive1);

        //cout << "\n\n" << archive1 << endl;

        Hash h;
        p->load(h, archive1);
        //cout << endl << h << endl;

        CPPUNIT_ASSERT(karabo::util::similar(m_rootedHash, h) == true);

        p->save(h, archive2);

        //cout << "\n\n" << archive2 << endl;

       CPPUNIT_ASSERT(string(archive1[0], archive1.size()) == string(archive2[0], archive2.size()));
    }

    {
        vector<char> archive1;
        vector<char> archive2;

        p->save(m_bigHash, archive1);

        //cout << "\n\n Archive size: " << archive1.size() << " bytes" << endl;

        Hash h;
        p->load(h, archive1);

       CPPUNIT_ASSERT(karabo::util::similar(m_bigHash, h) == true);

        p->save(h, archive2);
        
        CPPUNIT_ASSERT(string(archive1[0], archive1.size()) == string(archive2[0], archive2.size()));

    }

    {
        vector<char> archive1;
        vector<char> archive2;

        p->save(m_unrootedHash, archive1);

        //cout << "\n\nXML string size: " << archive.size() / 1024 / 1024 << " MB" << endl;

        Hash h;
        p->load(h, archive1);

        CPPUNIT_ASSERT(karabo::util::similar(m_unrootedHash, h) == true);

        p->save(h, archive2);
        CPPUNIT_ASSERT(string(archive1[0], archive1.size()) == string(archive2[0], archive2.size()));

    }
}
