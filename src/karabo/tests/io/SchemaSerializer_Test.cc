/* 
 * File:   SchemaSerializer_Test.cc
 * Author: boukhele
 * 
 * Created on July 10, 2013, 11:49 AM
 */

#include <karabo/io/SchemaXsdSerializer.hh>
#include <karabo/io/SchemaBinarySerializer.hh>
#include "karabo/io/BinarySerializer.hh"
#include "karabo/util/TimeProfiler.hh"
#include <karabo/io/TextFileOutput.hh>
#include <karabo/io/BinaryFileOutput.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/NodeElement.hh>
#include <karabo/util/ListElement.hh>
#include <karabo/util/VectorElement.hh>
#include <karabo/io/FileTools.hh>

#include "SchemaSerializer_Test.hh"

using namespace karabo::io;
using namespace karabo::util;


CPPUNIT_TEST_SUITE_REGISTRATION(SchemaSerializer_Test);

SchemaSerializer_Test::SchemaSerializer_Test() {
}

SchemaSerializer_Test::~SchemaSerializer_Test() {
}

void SchemaSerializer_Test::setUp() {
}

void SchemaSerializer_Test::tearDown() {
}

struct TestSchemaSerializer {
    KARABO_CLASSINFO(TestSchemaSerializer, "TestSchemaSerializer", "1.0");

    static void expectedParameters(karabo::util::Schema & expected) {

        STRING_ELEMENT(expected).key("exampleKey1")
                .tags("hardware, poll")
                .displayedName("Example key 1")
                .description("Example key 1 description")
                .options("Radio,Air Condition,Navigation", ",")
                .assignmentOptional().defaultValue("Navigation")
                .advanced()
                .commit();

        NODE_ELEMENT(expected).key("MyNodeElement")
                .tags("myNode")
                .displayedName("MyNodeElem")
                .description("Description of my node elem")
                .commit();

        DOUBLE_ELEMENT(expected).key("MyNodeElement.a")
                .tags("myNode")
                .displayedName("MyNodeElem_A")
                .description("Description of my node elem A")
                .assignmentMandatory()
                .commit();

        STRING_ELEMENT(expected).key("MyNodeElement.b")
                .tags("myNode")
                .displayedName("MyNodeElem_B")
                .description("Description of my node elem B")
                .assignmentMandatory()
                .commit();

        INT64_ELEMENT(expected).key("exampleKey5")
                .alias("exampleAlias5")
                .tags("h/w; d.m.y", ";")
                .displayedName("Example key 5")
                .description("Example key 5 description")
                .readOnly()
                .initialValue(1442244)
                .commit();

        INT64_ELEMENT(expected).key("exampleKeyINTERNAL")
                .displayedName("INTERNAL")
                .description("Example key INTERNAL")
                .assignmentInternal();

        //test for CHOICE element
        CHOICE_ELEMENT(expected).key("shapes")
                .displayedName("shapesAsChoice")
                .description("Description of Choice-element shapes")
                .assignmentOptional().defaultValue("circle")
                .commit();

        //or: test for LIST element (min/max appears only in 'annotation')
        //  LIST_ELEMENT(expected).key("shapes")
        //       .displayedName("shapesAsList")
        //       .description("Description of List-element shapes")
        //       .min(2)
        //       .max(5)
        //       .assignmentMandatory()
        //       .commit();

        NODE_ELEMENT(expected).key("shapes.circle")
                .tags("shape")
                .displayedName("Circle")
                .description("Description of circle")
                .commit();

        INT32_ELEMENT(expected).key("shapes.circle.radius")
                .tags("shape")
                .displayedName("radius")
                .description("Radius of circle")
                .minInc(5)
                .maxExc(10)
                .assignmentOptional().defaultValue(5)
                .commit();

        INT32_ELEMENT(expected).key("shapes.circle.color")
                .tags("shape")
                .displayedName("color")
                .description("Color of circle")
                .minExc(2)
                .maxInc(20)
                .assignmentOptional().defaultValue(5)
                .commit();

        NODE_ELEMENT(expected).key("shapes.circle.newnode")
                .tags("shape")
                .displayedName("NewNodeOfCircle")
                .description("Description of NEW NODE of circle")
                .commit();

        INT32_ELEMENT(expected).key("shapes.circle.newnode.mynewint")
                .tags("shape")
                .displayedName("MyNewInt")
                .description("Descr of shapes circle newnode MyNewInt")
                .assignmentOptional().defaultValue(555)
                .commit();

        NODE_ELEMENT(expected).key("shapes.rectangle")
                .tags("shape")
                .displayedName("rectangle")
                .description("Description of rectangle")
                .commit();

        DOUBLE_ELEMENT(expected).key("shapes.rectangle.square")
                .tags("shape")
                .displayedName("square")
                .description("Description of square of rectangle")
                .assignmentOptional().noDefaultValue()
                .commit();


        vector<string> vecStr;
        vecStr.push_back("first line");
        vecStr.push_back("second line");
        VECTOR_STRING_ELEMENT(expected).key("strVector")
                .displayedName("myVectorString")
                .assignmentOptional().defaultValue(vecStr)
                .reconfigurable()
                .commit();

        vector<int> vecInt;
        vecInt.push_back(5);
        vecInt.push_back(15);
        VECTOR_INT32_ELEMENT(expected).key("intVector")
                .displayedName("MyVectorInt")
                .minSize(2)
                .maxSize(5)
                .assignmentOptional().defaultValue(vecInt)
                .reconfigurable()
                .commit();

        INT32_ELEMENT(expected).key("SimpleElem")
                .displayedName("SimpleElem")
                .description("Description of SimpleElem")
                .unit(Unit::METER)
                .metricPrefix(MetricPrefix::MILLI)
                .readOnly()
                .alarmHigh(7).needsAcknowledging(false)
                .warnHigh(5).needsAcknowledging(false)
                .warnLow(0).needsAcknowledging(false)
                .alarmLow(-2).needsAcknowledging(false)
                .commit();

    }
};

void SchemaSerializer_Test::testBinarySerializer() {
    Schema testSchema("TestSchema", Schema::AssemblyRules(READ | WRITE | INIT));
    TestSchemaSerializer::expectedParameters(testSchema);
    //cout << "Schema : \n" << testSchema << endl;

    BinarySerializer<Schema>::Pointer p = BinarySerializer<Schema>::create("Bin");

    std::vector<char> archive1;

    p->save(testSchema, archive1);

    Schema inputSchema;

    p->load(inputSchema, &archive1[0], archive1.size());
    
    // Check whether alias maps got re-established
    CPPUNIT_ASSERT(inputSchema.keyHasAlias("exampleKey5") == true);
    CPPUNIT_ASSERT(inputSchema.aliasHasKey("exampleAlias5") == true);
    CPPUNIT_ASSERT(inputSchema.getKeyFromAlias("exampleAlias5") == "exampleKey5");
    CPPUNIT_ASSERT(inputSchema.getAliasFromKey<string>("exampleKey5") == "exampleAlias5");
            

    std::vector<char> archive2;

    p->save(inputSchema, archive2);

    //std::clog << "\nOriginal:\n" << testSchema << std::endl;
    //std::clog << "\nSerialized:\n" << inputSchema << std::endl;

    CPPUNIT_ASSERT(archive2.size() == archive1.size());

    CPPUNIT_ASSERT(memcmp(&archive1[0], &archive2[0], archive1.size()) == 0);

    TextSerializer<Schema>::Pointer p2 = TextSerializer<Schema>::create("Xsd");

    std::string archive3;
    p2->save(testSchema, archive3);

    p2 = TextSerializer<Schema>::create("Xml");

    std::string archive4;
    p2->save(testSchema, archive4);
    
    //std::clog << "Xml:\n" << archive4 << std::endl;

//    std::clog << "Binary: " << archive2.size()   << " bytes" << std::endl;
//    std::clog << "Xml   : " << archive4.length() << " bytes" << std::endl;
//    std::clog << "Xsd   : " << archive3.length() << " bytes" << std::endl;
}

void SchemaSerializer_Test::testXmlSerializer() {
    CPPUNIT_ASSERT(true);
}
