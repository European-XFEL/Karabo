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
 * File:   HashXmlSerializer_Test.cc
 * Author: heisenb
 *
 * Created on February 25, 2013, 6:03 PM
 */

#include "HashXmlSerializer_Test.hh"

#include <ctime>
#include <iostream>
#include <vector>

#include "karabo/data/io/BinarySerializer.hh"
#include "karabo/data/io/HashXmlSerializer.hh"
#include "karabo/data/schema/BaseElement.hh"
#include "karabo/data/schema/SimpleElement.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(HashXmlSerializer_Test);

using namespace karabo::data;
using std::string;
using std::vector;

HashXmlSerializer_Test::HashXmlSerializer_Test() {}


HashXmlSerializer_Test::~HashXmlSerializer_Test() {}


void HashXmlSerializer_Test::setUp() {
    Schema sch("schema_attr");
    INT32_ELEMENT(sch).key("metric").assignmentOptional().defaultValue(12).commit();

    Hash rooted("a.b.c", 1, "a.b.d", vector<int>(5, 1), "a.b.e", vector<Hash>(2, Hash("a", 1)), "a.d",
                std::complex<double>(1.2, 4.2));
    rooted.setAttribute("a", "a1", true);
    rooted.setAttribute("a", "a2", 3.4);
    rooted.setAttribute("a", "a3", vector<Hash>{Hash("row1", "value1"), Hash("row2", "value2")});
    rooted.setAttribute("a", "a4", sch);
    rooted.setAttribute("a.b", "b1", "3");
    rooted.setAttribute("a.b.c", "c1", 2);
    rooted.setAttribute("a.b.c", "c2", vector<string>(3, "bla"));
    rooted.setAttribute("a.b.c", "c3", vector<Hash>{Hash("row1", 1), Hash("row2", 2)});
    rooted.setAttribute("a.b.e", "myAttr", "Hallo");
    rooted.setAttribute("a.b.e", "eAttr", vector<string>(2, "abc"));
    rooted.set("an/element/with/slashes", true);
    rooted.set("vec", std::vector<float>());
    m_rootedHash = rooted;

    Hash big("a.b", std::vector<double>(10000, 1.0));
    vector<Hash>& tmp = big.bindReference<vector<Hash>>("a.c");
    tmp.resize(1000);
    for (size_t i = 0; i < tmp.size(); ++i) {
        tmp[i] = m_rootedHash;
    }
    m_bigHash = big;

    Hash unrooted("a.b.c", 1, "b.c", 2.0, "c", 3.f, "d.e", "4", "e.f.g.h", std::vector<unsigned long long>(5, 5),
                  "F.f.f.f.f", Hash("x.y.z", 99));
    unrooted.setAttribute("F.f.f", "attr1", true);
    unrooted.set("a1", string());
    m_unrootedHash = unrooted;

    for (size_t i = 0; i < 10; ++i) {
        m_vectorOfHashes.push_back(m_rootedHash);
    }
}


void HashXmlSerializer_Test::tearDown() {}


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
        CPPUNIT_ASSERT_MESSAGE(
              "Hashes for original schema 's' and the deserialized schema node in key 'a2' should be fullyEquals.",
              s.getParameterHash().fullyEquals(deserialized.get<Schema>("a2").getParameterHash()));
    }


    {
        std::string archive1;
        std::string archive2;

        p->save(m_rootedHash, archive1);

        Hash h;
        p->load(h, archive1);

        CPPUNIT_ASSERT_MESSAGE(
              "Deserialized version of m_rootedHash, 'h', and 'm_rootedHash' itself should be fullyEquals.",
              h.fullyEquals(m_rootedHash));

        // Checks serialization of attribute of type vector<string>.
        const vector<std::string> serVectStrAttr = h.getAttribute<vector<std::string>>("a.b.e", "eAttr");
        const vector<std::string> origVectStrAttr = m_rootedHash.getAttribute<vector<std::string>>("a.b.e", "eAttr");
        CPPUNIT_ASSERT_EQUAL(origVectStrAttr.size(), serVectStrAttr.size());
        for (vector<std::string>::size_type i = 0; i < serVectStrAttr.size(); i++) {
            CPPUNIT_ASSERT_EQUAL(origVectStrAttr[i], serVectStrAttr[i]);
        }

        // Checks serialization of attribute of type vector<Hash>.
        const vector<Hash> serVectHashAttr = h.getAttribute<vector<Hash>>("a", "a3");
        const vector<Hash> origVectHashAttr = m_rootedHash.getAttribute<vector<Hash>>("a", "a3");
        CPPUNIT_ASSERT_EQUAL(origVectStrAttr.size(), serVectStrAttr.size());
        for (vector<std::string>::size_type i = 0; i < serVectHashAttr.size(); i++) {
            CPPUNIT_ASSERT_MESSAGE(
                  "Hashes at vector<Hash> attribute 'a.a3' of 'origVectHashAttr' and 'serVectHashAttr' are not "
                  "fullyEquals.",
                  origVectHashAttr[i].fullyEquals(serVectHashAttr[i]));
        }

        // Checks serialization of attribute of type Schema.
        const Schema serSchemaAttr = h.getAttribute<Schema>("a", "a4");
        const Schema origSchemaAttr = m_rootedHash.getAttribute<Schema>("a", "a4");
        CPPUNIT_ASSERT_MESSAGE(
              "Hashes corresponding to original schema attribute, 'origSchemaAttr', and its serialized version, "
              "'serSchemaAttr', are not fullyEquals.",
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

        CPPUNIT_ASSERT_MESSAGE(
              "Specially crafted hash, 'rooted', with node named to try to provoque collision, and its serialized "
              "form, 'h', should be fullyEquals.",
              rooted.fullyEquals(h));

        p->save(h, archive2);

        CPPUNIT_ASSERT(archive1 == archive2);
    }


    {
        const int nSave = 1;                      // increase for measurements, minimum 1
        std::vector<std::string> archives(nSave); // an individial archive for each save
        const std::string& archive1 = archives[0];
        std::string archive2;

        std::clock_t c_start = std::clock();
        for (int i = 0; i < nSave; ++i) {
            p->save(m_bigHash, archives[i]);
        }
        std::clock_t c_end = std::clock();

        double time_elapsed_ms = 1000.0 / nSave * (c_end - c_start) / CLOCKS_PER_SEC;
        KARABO_LOG_FRAMEWORK_DEBUG << "Average serialization big Hash: " << time_elapsed_ms << " ms";

        std::vector<Hash> vecH(nSave); // A new Hash for each deserialisation
        c_start = std::clock();
        for (int i = 0; i < nSave; ++i) {
            p->load(vecH[i], archive1);
        }
        c_end = std::clock();

        time_elapsed_ms = 1000.0 / nSave * (c_end - c_start) / CLOCKS_PER_SEC;
        KARABO_LOG_FRAMEWORK_DEBUG << "Average de-serialization big Hash: " << time_elapsed_ms << " ms";

        Hash& h = vecH[0];
        CPPUNIT_ASSERT(karabo::data::similar(m_bigHash, h) == true);

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
            CPPUNIT_ASSERT_MESSAGE(
                  "Serialized hash in vector of hashes, hs[i] should be fullyEquals to its original form, "
                  "'m_rootedHash'.",
                  m_rootedHash.fullyEquals(hs[i]));
        }

        p->save(hs, archive2);
        CPPUNIT_ASSERT(archive1 == archive2);
    }
}


void HashXmlSerializer_Test::testLegacyDeserialization() {
    std::string legacyXml =
          "<?xml version=\"1.0\"?>"
          "<root KRB_Artificial=\"\" KRB_Type=\"HASH\">"
          "<table displayedName=\"KRB_STRING:Table property\" description=\"KRB_STRING:Table containing one node.\" "
          "assignment=\"KRB_INT32:0\" "
          "defaultValue=\"KRB_VECTOR_HASH:'e1' =&gt; abc STRING&#10;'e2' alarmCondition=&quot;none&quot; =&gt; 1 "
          "BOOL&#10;'e3' alarmCondition=&quot;none&quot; =&gt; 12 INT32&#10;'e4' alarmCondition=&quot;none&quot; =&gt; "
          "0.9837 FLOAT&#10;'e5' alarmCondition=&quot;none&quot; =&gt; 1.2345 DOUBLE&#10;,'e1' =&gt; xyz "
          "STRING&#10;'e2' alarmCondition=&quot;none&quot; =&gt; 0 BOOL&#10;'e3' alarmCondition=&quot;none&quot; =&gt; "
          "42 INT32&#10;'e4' alarmCondition=&quot;none&quot; =&gt; 2.33333 FLOAT&#10;'e5' "
          "alarmCondition=&quot;none&quot; =&gt; 7.77777 DOUBLE&#10;\" "
          "accessMode=\"KRB_INT32:4\" nodeType=\"KRB_INT32:0\" "
          "displayType=\"KRB_STRING:Table\" valueType=\"KRB_STRING:VECTOR_HASH\" "
          "rowSchema=\"KRB_SCHEMA:Schema Object\" requiredAccessLevel=\"KRB_INT32:1\" "
          "overwriteRestrictions=\"KRB_VECTOR_BOOL:0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0\" "
          "KRB_Type=\"INT32\">0"
          "</table>"
          "</root>";

    TextSerializer<Hash>::Pointer p = TextSerializer<Hash>::create("Xml");
    Hash deserialized;
    CPPUNIT_ASSERT_NO_THROW(p->load(deserialized, legacyXml));
    CPPUNIT_ASSERT_EQUAL(0, deserialized.get<int>("table"));
    auto attrs = deserialized.getNode("table").getAttributes();
    CPPUNIT_ASSERT_EQUAL(std::string("Table property"), attrs.get<std::string>("displayedName"));
    CPPUNIT_ASSERT_EQUAL(std::string("Table containing one node."), attrs.get<std::string>("description"));
    CPPUNIT_ASSERT_EQUAL(0, attrs.get<int>("assignment"));

    // Reading this legacy XML with original release 2.5.0 led to 'attrs.has("defaultValue") == true',
    // but accessing it with getAttribute<vector<hash> >>("table", "defaultValue") threw an exception.
    CPPUNIT_ASSERT(!attrs.has("defaultValue"));

    CPPUNIT_ASSERT_EQUAL(4, attrs.get<int>("accessMode"));
    CPPUNIT_ASSERT_EQUAL(0, attrs.get<int>("nodeType"));
    CPPUNIT_ASSERT_EQUAL(std::string("Table"), attrs.get<std::string>("displayType"));
    CPPUNIT_ASSERT_EQUAL(std::string("VECTOR_HASH"), attrs.get<std::string>("valueType"));

    // Reading this legacy XML with original release 2.5.0 led to 'attrs.has("rowSchema") == true',
    // but accessing it with getAttribute<Schema>("table", "rowSchema") threw an exception.
    CPPUNIT_ASSERT(!attrs.has("rowSchema"));

    CPPUNIT_ASSERT(attrs.has("overwriteRestrictions"));
}
