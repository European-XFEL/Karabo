/* 
 * File:   HashXmlSerializer_Test.cc
 * Author: heisenb
 * 
 * Created on February 25, 2013, 6:03 PM
 */

#include <vector>


#include <karabo/io/HashXmlSerializer.hh>
#include "HashXmlSerializer_Test.hh"
#include "karabo/io/BinarySerializer.hh"
#include "karabo/util/GenericElement.hh"
#include "karabo/util/SimpleElement.hh"

#include <iostream>

CPPUNIT_TEST_SUITE_REGISTRATION(HashXmlSerializer_Test);

using namespace karabo::io;
using namespace karabo::util;
using std::vector;
using std::string;

HashXmlSerializer_Test::HashXmlSerializer_Test() {
}


HashXmlSerializer_Test::~HashXmlSerializer_Test() {
}


void HashXmlSerializer_Test::setUp() {

    Schema sch("schema_attr");
    INT32_ELEMENT(sch).key("metric").assignmentOptional().defaultValue(12).commit();

    Hash rooted("a.b.c", 1, "a.b.d", vector<int>(5, 1), "a.b.e", vector<Hash > (2, Hash("a", 1)), "a.d", std::complex<double>(1.2, 4.2));
    rooted.setAttribute("a", "a1", true);
    rooted.setAttribute("a", "a2", 3.4);
    rooted.setAttribute("a", "a3", vector<Hash>{Hash("row1", "value1"), Hash("row2", "value2")});
    rooted.setAttribute("a", "a4", sch);
    rooted.setAttribute("a.b", "b1", "3");
    rooted.setAttribute("a.b.c", "c1", 2);
    rooted.setAttribute("a.b.c", "c2", vector<string > (3, "bla"));
    rooted.setAttribute("a.b.c", "c3", vector<Hash>{Hash("row1", 1), Hash("row2", 2)});
    rooted.setAttribute("a.b.e", "myAttr", "Hallo");
    rooted.setAttribute("a.b.e", "eAttr", vector<string > (2, "abc"));
    rooted.set("an/element/with/slashes", true);
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
        string serialized;
        p->save(schemaIncluded, serialized);
        Hash deserialized;
        p->load(deserialized, serialized);

        CPPUNIT_ASSERT_MESSAGE("'schemaIncluded' and 'deserialized' hashes should be fullyEquals.",
                               schemaIncluded.fullyEquals(deserialized));
        CPPUNIT_ASSERT_MESSAGE("Hashes for original schema 's' and the deserialized schema node in key 'a2' should be fullyEquals.",
                               s.getParameterHash().fullyEquals(deserialized.get<Schema>("a2").getParameterHash()));
      }


    {
        std::string archive1;
        std::string archive2;

        p->save(m_rootedHash, archive1);

        Hash h;
        p->load(h, archive1);

        CPPUNIT_ASSERT_MESSAGE("Deserialized version of m_rootedHash, 'h', and 'm_rootedHash' itself should be fullyEquals.",
                               h.fullyEquals(m_rootedHash));

        // Checks serialization of attribute of type vector<string>.
        const vector<std::string> serVectStrAttr = h.getAttribute<vector < std::string >> ("a.b.e", "eAttr");
        const vector<std::string> origVectStrAttr = m_rootedHash.getAttribute<vector < std::string >> ("a.b.e", "eAttr");
        CPPUNIT_ASSERT_EQUAL(origVectStrAttr.size(), serVectStrAttr.size());
        for (vector<std::string>::size_type i = 0; i < serVectStrAttr.size(); i++) {
            CPPUNIT_ASSERT_EQUAL(origVectStrAttr[i], serVectStrAttr[i]);
        }

        // Checks serialization of attribute of type vector<Hash>.
        const vector<Hash> serVectHashAttr = h.getAttribute<vector < Hash >> ("a", "a3");
        const vector<Hash> origVectHashAttr = m_rootedHash.getAttribute<vector < Hash >> ("a", "a3");
        CPPUNIT_ASSERT_EQUAL(origVectStrAttr.size(), serVectStrAttr.size());
        for (vector<std::string>::size_type i = 0; i < serVectHashAttr.size(); i++) {
            CPPUNIT_ASSERT_MESSAGE("Hashes at vector<Hash> attribute 'a.a3' of 'origVectHashAttr' and 'serVectHashAttr' are not fullyEquals.",
                                   origVectHashAttr[i].fullyEquals(serVectHashAttr[i]));
        }

        // Checks serialization of attribute of type Schema.
        const Schema serSchemaAttr = h.getAttribute<Schema>("a", "a4");
        const Schema origSchemaAttr = m_rootedHash.getAttribute<Schema>("a", "a4");
        CPPUNIT_ASSERT_MESSAGE("Hashes corresponding to original schema attribute, 'origSchemaAttr', and its serialized version, 'serSchemaAttr', are not fullyEquals.",
                               origSchemaAttr.getParameterHash().fullyEquals(serSchemaAttr.getParameterHash()));

        p->save(h, archive2);

        CPPUNIT_ASSERT(archive1 == archive2);
    }


    {
        // Checks that a specially crafted hash node won't cause a name clash between the xml element for the
        // hash node and the xml element created to contain the serialized form of the vector<Hash> attribute.
        std::string archive1;
        std::string archive2;

        Hash rooted("a._attr_a_a3", "something");
        rooted.setAttribute("a", "a3", vector<Hash>{Hash("row1", "value1"), Hash("row2", "value2")});

        p->save(rooted, archive1);

        Hash h;
        p->load(h, archive1);

        CPPUNIT_ASSERT_MESSAGE("Specially crafted hash, 'rooted', with node named to try to provoque collision, and its serialized form, 'h', should be fullyEquals.",
                               rooted.fullyEquals(h));

        p->save(h, archive2);

        CPPUNIT_ASSERT(archive1 == archive2);
    }


    {
        std::string archive1;
        std::string archive2;

        p->save(m_bigHash, archive1);

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

        Hash h;
        p->load(h, archive1);

        CPPUNIT_ASSERT_MESSAGE("UnrootedHash, 'm_unrootedHash', and its serialized form, 'h', should be fullyEquals.",
                               m_unrootedHash.fullyEquals(h));

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
            CPPUNIT_ASSERT_MESSAGE("Serialized hash in vector of hashes, hs[i] should be fullyEquals to its original form, 'm_rootedHash'.",
                                   m_rootedHash.fullyEquals(hs[i]));
        }

        p->save(hs, archive2);
        CPPUNIT_ASSERT(archive1 == archive2);
    }

}
