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
 * File:   SchemaSerializer_Test.hh
 * Author: boukhele
 *
 * Created on July 10, 2013, 11:49 AM
 */
#include <gtest/gtest.h>

#include "karabo/data/io/BinaryFileOutput.hh"
#include "karabo/data/io/BinarySerializer.hh"
#include "karabo/data/io/FileTools.hh"
#include "karabo/data/io/SchemaBinarySerializer.hh"
#include "karabo/data/io/TextFileOutput.hh"
#include "karabo/data/schema/NodeElement.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/VectorElement.hh"
#include "karabo/util/TimeProfiler.hh"

using namespace karabo::data;
using std::vector;

struct TestSchemaExample {
    KARABO_CLASSINFO(TestSchemaExample, "TestSchemaExample", "1.0");


    static void expectedParameters(karabo::data::Schema& expected) {
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

        // test for top NODE element (former CHOICE_ELEMENT)
        STRING_ELEMENT(expected)
              .key("shapes")
              .displayedName("shapesAsChoice")
              .description("Description of Choice-element shapes")
              .assignmentOptional()
              .defaultValue("circle")
              .commit();

        NODE_ELEMENT(expected)
              .key("circle")
              .tags("shape")
              .displayedName("Circle")
              .description("Description of circle")
              .commit();

        INT32_ELEMENT(expected)
              .key("circle.radius")
              .tags("shape")
              .displayedName("radius")
              .description("Radius of circle")
              .minInc(5)
              .maxExc(10)
              .assignmentOptional()
              .defaultValue(5)
              .commit();

        INT32_ELEMENT(expected)
              .key("circle.color")
              .tags("shape")
              .displayedName("color")
              .description("Color of circle")
              .minExc(2)
              .maxInc(20)
              .assignmentOptional()
              .defaultValue(5)
              .commit();

        NODE_ELEMENT(expected)
              .key("circle.newnode")
              .tags("shape")
              .displayedName("NewNodeOfCircle")
              .description("Description of NEW NODE of circle")
              .commit();

        INT32_ELEMENT(expected)
              .key("circle.newnode.mynewint")
              .tags("shape")
              .displayedName("MyNewInt")
              .description("Descr of shapes circle newnode MyNewInt")
              .assignmentOptional()
              .defaultValue(555)
              .commit();

        NODE_ELEMENT(expected)
              .key("rectangle")
              .tags("shape")
              .displayedName("rectangle")
              .description("Description of rectangle")
              .commit();

        DOUBLE_ELEMENT(expected)
              .key("rectangle.square")
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


TEST(TestSchemaSerializer, testBinarySerializer) {
    Schema testSchema("TestSchema", Schema::AssemblyRules(READ | WRITE | INIT));
    TestSchemaExample::expectedParameters(testSchema);
    // cout << "Schema : \n" << testSchema << endl;

    BinarySerializer<Schema>::Pointer p = BinarySerializer<Schema>::create("Bin");

    std::vector<char> archive1;

    p->save(testSchema, archive1);

    Schema inputSchema;

    p->load(inputSchema, &archive1[0], archive1.size());

    // Check whether alias maps got re-established
    EXPECT_TRUE(inputSchema.keyHasAlias("exampleKey5") == true);
    EXPECT_TRUE(inputSchema.aliasHasKey("exampleAlias5") == true);
    EXPECT_TRUE(inputSchema.getKeyFromAlias("exampleAlias5") == "exampleKey5");
    EXPECT_TRUE(inputSchema.getAliasFromKey<std::string>("exampleKey5") == "exampleAlias5");


    std::vector<char> archive2;

    p->save(inputSchema, archive2);

    // std::clog << "\nOriginal:\n" << testSchema << std::endl;
    // std::clog << "\nSerialized:\n" << inputSchema << std::endl;

    EXPECT_TRUE(archive2.size() == archive1.size());

    EXPECT_TRUE(memcmp(&archive1[0], &archive2[0], archive1.size()) == 0);

    TextSerializer<Schema>::Pointer p2 = TextSerializer<Schema>::create("Xml");

    std::string archive4;
    p2->save(testSchema, archive4);

    // std::clog << "Xml:\n" << archive4 << std::endl;

    //    std::clog << "Binary: " << archive2.size()   << " bytes" << std::endl;
    //    std::clog << "Xml   : " << archive4.length() << " bytes" << std::endl;
    //    std::clog << "Xsd   : " << archive3.length() << " bytes" << std::endl;
}


TEST(TestSchemaSerializer, testLoadLastFromSequence) {
    BinarySerializer<Schema>::Pointer ser = BinarySerializer<Schema>::create("Bin");

    Schema testSchema("TestSchema", Schema::AssemblyRules(READ | WRITE | INIT));
    TestSchemaExample::expectedParameters(testSchema);
    EXPECT_STREQ("Navigation", testSchema.getDefaultValue<std::string>("exampleKey1").c_str());

    Schema testSchemaMod("TestSchemaMod", Schema::AssemblyRules(READ | WRITE | INIT));
    TestSchemaExample::expectedParameters(testSchemaMod);
    testSchemaMod.setDefaultValue("exampleKey1", std::string{"Orientation"});
    EXPECT_STREQ("Orientation", testSchemaMod.getDefaultValue<std::string>("exampleKey1").c_str());

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
    EXPECT_NO_THROW(ser->loadLastFromSequence(loadedSch, archSingleSer, archSingle.size()));
    EXPECT_TRUE(testSchema.getParameterHash().fullyEquals(loadedSch.getParameterHash()));
    // Checks that the second element gets loaded for the sequence with 2 elements.
    EXPECT_NO_THROW(ser->loadLastFromSequence(loadedSch, archTwoSer, archTwo.size()));
    EXPECT_TRUE(testSchemaMod.getParameterHash().fullyEquals(loadedSch.getParameterHash()));
}
