/* 
 * File:   HashXmlSerializer_Test.cc
 * Author: heisenb
 * 
 * Created on February 25, 2013, 6:03 PM
 */

#include <karabo/io/HashXmlSerializer.hh>
#include "HashXmlSerializer_Test.hh"
#include "karabo/io/BinarySerializer.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(HashXmlSerializer_Test);

using namespace karabo::io;
using namespace karabo::util;


HashXmlSerializer_Test::HashXmlSerializer_Test() {
}


HashXmlSerializer_Test::~HashXmlSerializer_Test() {
}


void HashXmlSerializer_Test::setUp() {

    Hash rooted("a.b.c", 1, "a.b.d", vector<int>(5, 1), "a.b.e", vector<Hash > (2, Hash("a", 1)), "a.d", std::complex<double>(1.2, 4.2));
    rooted.setAttribute("a", "a1", true);
    rooted.setAttribute("a", "a2", 3.4);
    rooted.setAttribute("a.b", "b1", "3");
    rooted.setAttribute("a.b.c", "c1", 2);
    rooted.setAttribute("a.b.c", "c2", vector<string > (3, "bla"));
    rooted.setAttribute("a.b.e", "myAttr", "Hallo");
    rooted.setAttribute("a.b.e", "eAttr", vector<string > (2, "abc"));
    rooted.set("vec", std::vector<float>());
    m_rootedHash = rooted;

    Hash big("a.b", std::vector<double>(10000, 1.0));
    vector<Hash>& tmp = big.bindReference<vector<Hash> >("a.c");
    tmp.resize(1000);
    for (size_t i = 0; i < tmp.size(); ++i) {
        tmp[i] = m_rootedHash;
    }
    m_bigHash = big;

    Hash unrooted("a.b.c", 1, "b.c", 2.0, "c", 3.f, "d.e", "4", "e.f.g.h", std::vector<unsigned long long > (5, 5), "F.f.f.f.f", Hash("x.y.z", 99));
    unrooted.setAttribute("F.f.f", "attr1", true);
    unrooted.set("a1", string());
    m_unrootedHash = unrooted;
    
    for (size_t i = 0; i < 10; ++i) {
        m_vectorOfHashes.push_back(m_rootedHash);
    }
}


void HashXmlSerializer_Test::tearDown() {
}


void HashXmlSerializer_Test::testSerialization() {

    TextSerializer<Hash>::Pointer p = TextSerializer<Hash>::create("Xml");

    {

        Schema s = TextSerializer<Hash>::getSchema("Xml");
        Hash schemaIncluded("a1", 3.2, "a2", s);
        string garbage;
        p->save(schemaIncluded, garbage);
        //cout << "GARBAGE: " << garbage << endl;
        Hash fresh;
        p->load(fresh, garbage);
        //cout << "HASH: " << fresh.get<Schema>("a2") << endl;

    }

    {
        std::string archive1;
        std::string archive2;

        p->save(m_rootedHash, archive1);
        
        //cout << "\n\n  archive1: \n" << archive1 << endl;
        
        Hash h;
        p->load(h, archive1);
        //cout << "\n\n  h: \n" << h << endl;
        
        CPPUNIT_ASSERT(karabo::util::similar(m_rootedHash, h) == true);

        p->save(h, archive2);

        //cout << "\n\n archive2: \n" << archive2 << endl;

        CPPUNIT_ASSERT(archive1 == archive2);


    }

    {
        std::string archive1;
        std::string archive2;

        p->save(m_bigHash, archive1);

        //cout << "\n\nXML string size: " << archive.size() / 1024 / 1024 << " MB" << endl;

        Hash h;
        p->load(h, archive1);

        CPPUNIT_ASSERT(karabo::util::similar(m_bigHash, h) == true);

        p->save(h, archive2);
        CPPUNIT_ASSERT(archive1 == archive2);

    }

    {
        std::string archive1;
        std::string archive2;
        p->save(m_unrootedHash, archive1);
        //cout << "\n\nXML string size: " << archive.size() / 1024 / 1024 << " MB" << endl;

        Hash h;
        p->load(h, archive1);

        CPPUNIT_ASSERT(karabo::util::similar(m_unrootedHash, h) == true);

        p->save(h, archive2);
        CPPUNIT_ASSERT(archive1 == archive2);

    }
    
    {
        std::string archive1;
        std::string archive2;
        p->save(m_vectorOfHashes, archive1);
        vector<Hash> hs;
        p->load(hs, archive1);
        for (size_t i = 0; i < 10; ++i) {
            CPPUNIT_ASSERT(karabo::util::similar(m_rootedHash, hs[i]) == true);
        }
        
        p->save(hs, archive2);
        CPPUNIT_ASSERT(archive1 == archive2);
    }
}
