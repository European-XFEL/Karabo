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
 * File:   SchemaSerializer_Test.cc
 * Author: boukhele
 *
 * Created on July 10, 2013, 11:49 AM
 */


#include "SchemaSerializer_Test.hh"

#include <karabo/io/BinaryFileOutput.hh>
#include <karabo/io/FileTools.hh>
#include <karabo/io/SchemaBinarySerializer.hh>
#include <karabo/io/TextFileOutput.hh>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/NodeElement.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/VectorElement.hh>

#include "karabo/io/BinarySerializer.hh"
#include "karabo/util/TimeProfiler.hh"

using namespace karabo::io;
using namespace karabo::util;
using std::vector;

CPPUNIT_TEST_SUITE_REGISTRATION(SchemaSerializer_Test);


SchemaSerializer_Test::SchemaSerializer_Test() {}


SchemaSerializer_Test::~SchemaSerializer_Test() {}


void SchemaSerializer_Test::setUp() {}


void SchemaSerializer_Test::tearDown() {}


struct TestSchemaSerializer {
    KARABO_CLASSINFO(TestSchemaSerializer, "TestSchemaSerializer", "1.0");


    static void expectedParameters(karabo::util::Schema& expected) {
        STRING_ELEMENT(expected)
              .key("exampleKey1")
              .tags("hardware, poll")
              .displayedName("Example key 1")
              .description("Example key 1 description")
              .options("Radio,Air Condition,Navigation", ",")
              .assignmentOptional()
              .defaultValue("Navigation")
              .commit();

        NODE_ELEMENT(expected)
              .key("MyNodeElement")
              .tags("myNode")
              .displayedName("MyNodeElem")
              .description("Description of my node elem")
              .commit();

        DOUBLE_ELEMENT(expected)
              .key("MyNodeElement.a")
              .tags("myNode")
              .displayedName("MyNodeElem_A")
              .description("Description of my node elem A")
              .assignmentMandatory()
              .commit();

        STRING_ELEMENT(expected)
              .key("MyNodeElement.b")
              .tags("myNode")
              .displayedName("MyNodeElem_B")
              .description("Description of my node elem B")
              .assignmentMandatory()
              .commit();

        INT64_ELEMENT(expected)
              .key("exampleKey5")
              .alias("exampleAlias5")
              .tags("h/w; d.m.y", ";")
              .displayedName("Example key 5")
              .description("Example key 5 description")
              .readOnly()
              .initialValue(1442244)
              .commit();

        INT64_ELEMENT(expected)
              .key("exampleKeyINTERNAL")
              .displayedName("INTERNAL")
              .description("Example key INTERNAL")
              .assignmentInternal();

        // test for CHOICE element
        CHOICE_ELEMENT(expected)
              .key("shapes")
              .displayedName("shapesAsChoice")
              .description("Description of Choice-element shapes")
              .assignmentOptional()
              .defaultValue("circle")
              .commit();

        NODE_ELEMENT(expected)
              .key("shapes.circle")
              .tags("shape")
              .displayedName("Circle")
              .description("Description of circle")
              .commit();

        INT32_ELEMENT(expected)
              .key("shapes.circle.radius")
              .tags("shape")
              .displayedName("radius")
              .description("Radius of circle")
              .minInc(5)
              .maxExc(10)
              .assignmentOptional()
              .defaultValue(5)
              .commit();

        INT32_ELEMENT(expected)
              .key("shapes.circle.color")
              .tags("shape")
              .displayedName("color")
              .description("Color of circle")
              .minExc(2)
              .maxInc(20)
              .assignmentOptional()
              .defaultValue(5)
              .commit();

        NODE_ELEMENT(expected)
              .key("shapes.circle.newnode")
              .tags("shape")
              .displayedName("NewNodeOfCircle")
              .description("Description of NEW NODE of circle")
              .commit();

        INT32_ELEMENT(expected)
              .key("shapes.circle.newnode.mynewint")
              .tags("shape")
              .displayedName("MyNewInt")
              .description("Descr of shapes circle newnode MyNewInt")
              .assignmentOptional()
              .defaultValue(555)
              .commit();

        NODE_ELEMENT(expected)
              .key("shapes.rectangle")
              .tags("shape")
              .displayedName("rectangle")
              .description("Description of rectangle")
              .commit();

        DOUBLE_ELEMENT(expected)
              .key("shapes.rectangle.square")
              .tags("shape")
              .displayedName("square")
              .description("Description of square of rectangle")
              .assignmentOptional()
              .noDefaultValue()
              .commit();


        vector<std::string> vecStr;
        vecStr.push_back("first line");
        vecStr.push_back("second line");
        VECTOR_STRING_ELEMENT(expected)
              .key("strVector")
              .displayedName("myVectorString")
              .assignmentOptional()
              .defaultValue(vecStr)
              .reconfigurable()
              .commit();

        vector<int> vecInt;
        vecInt.push_back(5);
        vecInt.push_back(15);
        VECTOR_INT32_ELEMENT(expected)
              .key("intVector")
              .displayedName("MyVectorInt")
              .minSize(2)
              .maxSize(5)
              .assignmentOptional()
              .defaultValue(vecInt)
              .reconfigurable()
              .commit();

        INT32_ELEMENT(expected)
              .key("SimpleElem")
              .displayedName("SimpleElem")
              .description("Description of SimpleElem")
              .unit(Unit::METER)
              .metricPrefix(MetricPrefix::MILLI)
              .readOnly()
              .commit();
    }
};


void SchemaSerializer_Test::testBinarySerializer() {
    Schema testSchema("TestSchema", Schema::AssemblyRules(READ | WRITE | INIT));
    TestSchemaSerializer::expectedParameters(testSchema);
    // cout << "Schema : \n" << testSchema << endl;

    BinarySerializer<Schema>::Pointer p = BinarySerializer<Schema>::create("Bin");

    std::vector<char> archive1;

    p->save(testSchema, archive1);

    Schema inputSchema;

    p->load(inputSchema, &archive1[0], archive1.size());

    // Check whether alias maps got re-established
    CPPUNIT_ASSERT(inputSchema.keyHasAlias("exampleKey5") == true);
    CPPUNIT_ASSERT(inputSchema.aliasHasKey("exampleAlias5") == true);
    CPPUNIT_ASSERT(inputSchema.getKeyFromAlias("exampleAlias5") == "exampleKey5");
    CPPUNIT_ASSERT(inputSchema.getAliasFromKey<std::string>("exampleKey5") == "exampleAlias5");


    std::vector<char> archive2;

    p->save(inputSchema, archive2);

    // std::clog << "\nOriginal:\n" << testSchema << std::endl;
    // std::clog << "\nSerialized:\n" << inputSchema << std::endl;

    CPPUNIT_ASSERT(archive2.size() == archive1.size());

    CPPUNIT_ASSERT(memcmp(&archive1[0], &archive2[0], archive1.size()) == 0);

    TextSerializer<Schema>::Pointer p2 = TextSerializer<Schema>::create("Xml");

    std::string archive4;
    p2->save(testSchema, archive4);

    // std::clog << "Xml:\n" << archive4 << std::endl;

    //    std::clog << "Binary: " << archive2.size()   << " bytes" << std::endl;
    //    std::clog << "Xml   : " << archive4.length() << " bytes" << std::endl;
    //    std::clog << "Xsd   : " << archive3.length() << " bytes" << std::endl;
}


void SchemaSerializer_Test::testLoadLastFromSequence() {
    BinarySerializer<Schema>::Pointer ser = BinarySerializer<Schema>::create("Bin");

    Schema testSchema("TestSchema", Schema::AssemblyRules(READ | WRITE | INIT));
    TestSchemaSerializer::expectedParameters(testSchema);
    CPPUNIT_ASSERT_EQUAL(std::string{"Navigation"}, testSchema.getDefaultValue<std::string>("exampleKey1"));

    Schema testSchemaMod("TestSchemaMod", Schema::AssemblyRules(READ | WRITE | INIT));
    TestSchemaSerializer::expectedParameters(testSchemaMod);
    testSchemaMod.setDefaultValue("exampleKey1", std::string{"Orientation"});
    CPPUNIT_ASSERT_EQUAL(std::string{"Orientation"}, testSchemaMod.getDefaultValue<std::string>("exampleKey1"));

    // CAVEAT: from the BinarySerializer<T>::save(T&, std::vector<char>&) documentation it can be seen that some
    // specializations of BinarySerializer may clear the receiving vector before adding the bytes of the serialized
    // form. That is not the case of the Schema specialization; if that changes, this test will break. It is intentional
    // that this test breaks if the Schema specialization changes as such a change would, most likely, introduce
    // undesired (and unknown) behavior changes in the Framework.

    // Generate two binary sequences: one with the serialized binary form of the
    // testSchema and another with the serialized binary form of testSchema and
    // testSchemaMod concatenated.
    std::vector<char> archSingle;
    ser->save(testSchema, archSingle);
    const char* archSingleSer = reinterpret_cast<const char*>(archSingle.data());
    std::vector<char> archTwo = archSingle;
    ser->save(testSchemaMod, archTwo);
    const char* archTwoSer = reinterpret_cast<const char*>(archTwo.data());

    Schema loadedSch;
    // Checks that the first element gets loaded for the sequence with just 1 element.
    CPPUNIT_ASSERT_NO_THROW(ser->loadLastFromSequence(loadedSch, archSingleSer, archSingle.size()));
    CPPUNIT_ASSERT(testSchema.getParameterHash().fullyEquals(loadedSch.getParameterHash()));
    // Checks that the second element gets loaded for the sequence with 2 elements.
    CPPUNIT_ASSERT_NO_THROW(ser->loadLastFromSequence(loadedSch, archTwoSer, archTwo.size()));
    CPPUNIT_ASSERT(testSchemaMod.getParameterHash().fullyEquals(loadedSch.getParameterHash()));
}


void SchemaSerializer_Test::testXmlSerializer() {
    CPPUNIT_ASSERT(true);
}
