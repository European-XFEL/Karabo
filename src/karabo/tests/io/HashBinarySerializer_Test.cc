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

    Hash rooted("a.b.c", 1, "a.b.d", vector<int>(5, 1), "a.b.e", vector<Hash > (2, Hash("a", 1)), "a.d", std::complex<double>(1.2, 4.2));
    rooted.setAttribute("a", "a1", true);
    rooted.setAttribute("a", "a2", 3.4);
    rooted.setAttribute("a.b", "b1", "3");
    rooted.setAttribute("a.b.c", "c1", 2);
    rooted.setAttribute("a.b.c", "c2", vector<string > (3, "bla"));
    m_rootedHash = rooted;

    TimeProfiler p("binary");
    p.open();
    p.startPeriod("create");
    m_data = std::vector<double>(20 * 1024 * 1024, 1.0);
    Hash big("a.b", std::pair<const double*, size_t>(&m_data[0], m_data.size()));
    p.stopPeriod("create");
    
//Old:    cout << "\nCreation time: " << std::fixed << karabo::util::HighResolutionTimer::time2double(p.getTime("create")) << endl;

    //p.startPeriod("ref");
    //const vector<double>& vect = big.get<vector<double> >("a.b");
   //p.stopPeriod("ref");
//Old:    cout << "\nReference time: " << std::fixed << karabo::util::HighResolutionTimer::time2double(p.getTime("ref")) << endl;

//    p.startPeriod("copy");
//    vector<double> vect1 = vect;
//    p.stopPeriod("copy");
    p.close();
    //cout << "\nCreation time: " << p.getPeriod("create").getDuration() << endl;
    //cout << "\nReference time: " << p.getPeriod(ref).getDuration() << endl;
    //cout << "\nCopy time: " << p.getPeriod("copy").getDuration() << endl;

//Old:    cout << "\nCopy time: " << std::fixed << karabo::util::HighResolutionTimer::time2double(p.getTime("copy")) << endl;

    vector<Hash>& tmp = big.bindReference<vector<Hash> >("a.c");
    tmp.resize(1);
    for (size_t i = 0; i < tmp.size(); ++i) {
        tmp[i] = m_rootedHash;
    }
    m_bigHash = big;


    Hash unrooted("a.b.c", 1, "b.c", 2.0, "c", 3.f, "d.e", "4", "e.f.g.h", std::vector<unsigned long long > (5, 5), "F.f.f.f.f", Hash("x.y.z", 99));
    unrooted.setAttribute("F.f.f", "attr1", true);
    m_unrootedHash = unrooted;

    for (size_t i = 0; i < 10; ++i) {
        m_vectorOfHashes.push_back(m_rootedHash);
    }
}


void HashBinarySerializer_Test::tearDown() {
}


void HashBinarySerializer_Test::testSerialization() {

    BinarySerializer<Hash>::Pointer p = BinarySerializer<Hash>::create("Bin");
    
    {
        Schema s = TextSerializer<Hash>::getSchema("Xml");
        Hash schemaIncluded("a1", 3.2, "a2", s);
        vector<char> archive;
        p->save(schemaIncluded, archive);
        Hash fresh;
        p->load(fresh, archive);
        //cout << "HASH: " << fresh.get<Schema>("a2") << endl;
    }

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

        //cout << "\nSerialize start ----------------------------\n";
        TimeProfiler pr("binary");
        pr.open();
        pr.startPeriod("serialize");

        p->save(m_bigHash, archive1);
        pr.stopPeriod("serialize");
        pr.close();
        //cout << "\nSerialize time: " << pr.getPeriod("serialize").getDuration() << endl;
        //cout << "\n\n Archive size: " << archive1.size() << " bytes" << endl;

        Hash h;
        p->load(h, archive1);

        // This is commented as the m_bigHash has value type pair<const T*, size_t>,
        // whilst the serialized h has a vector<T>, this is intended but will break the similar function
        //CPPUNIT_ASSERT(karabo::util::similar(m_bigHash, h) == true);

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

    {
        vector<char> archive1;
        vector<char> archive2;
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
