/* 
 * File:   SchemaXsdSerializer_Test.cc
 * Author: irinak
 * 
 * Created on March 15, 2013, 10:30 AM
 */

#include <karabo/io/SchemaXsdSerializer.hh>
#include "SchemaXsdSerializer_Test.hh"
#include "TestPathSetup.hh"
#include <karabo/io/TextFileOutput.hh>
#include <karabo/io/BinaryFileOutput.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/NodeElement.hh>
#include <karabo/util/ListElement.hh>
#include <karabo/util/VectorElement.hh>
#include <karabo/io/FileTools.hh>

CPPUNIT_TEST_SUITE_REGISTRATION(SchemaXsdSerializer_Test);

using namespace karabo::io;
using namespace karabo::util;


SchemaXsdSerializer_Test::SchemaXsdSerializer_Test() {
}


SchemaXsdSerializer_Test::~SchemaXsdSerializer_Test() {
}


void SchemaXsdSerializer_Test::setUp() {
}


void SchemaXsdSerializer_Test::tearDown() {

}


struct TestSchemaXsdSerializer {
    KARABO_CLASSINFO(TestSchemaXsdSerializer, "TestXsd", "1.0");


    static void expectedParameters(karabo::util::Schema & expected) {

        STRING_ELEMENT(expected).key("exampleKey1")
                .tags("hardware, poll")
                .displayedName("Example key 1")
                .description("Example key 1 description")
                .options("Radio,Air Condition,Navigation", ",")
                .assignmentOptional().defaultValue("Navigation")
                .expertAccess()
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
                .warnLow(0).needsAcknowledging(false)
                .warnHigh(2).needsAcknowledging(true)
                .alarmLow(-2).needsAcknowledging(false)
                .alarmHigh(7).needsAcknowledging(false)
                .commit();

    }
};


void SchemaXsdSerializer_Test::testXsdSerialization() {

    Schema testSchema("TestSchema", Schema::AssemblyRules(READ | WRITE | INIT));
    TestSchemaXsdSerializer::expectedParameters(testSchema);
    //cout << "Schema : \n" << testSchema << endl;

    TextSerializer<Schema>::Pointer p = TextSerializer<Schema>::create("Xsd");
     
    std::string archive;
    p->save(testSchema, archive);
    //cout << "\n XSD representation: \n" << archive << endl;
}    

void SchemaXsdSerializer_Test::testTextFileOutputSchema() {
    
    Schema testSchema("TestSchema", Schema::AssemblyRules(READ | WRITE | INIT));
    TestSchemaXsdSerializer::expectedParameters(testSchema);
    
    string fileName = resourcePath("testSchemaFile.xsd");
    
    saveToFile(testSchema, fileName, Hash("format.Xsd.indentation", 2));
    
    //or:
    //Output<Schema>::Pointer out = Output<Schema>::create("TextFile", Hash("filename", fileName, "format.Xsd.indentation", 2));
    //out->write(testSchema);
    
    ifstream newFile(fileName.c_str());

    string fnameEtalon = resourcePath("testSchemaFile_readonly.xsd");
    ifstream etalonFile(fnameEtalon.c_str());

    if (etalonFile.is_open()) {
        if (newFile.is_open()) {

            while (etalonFile.good()) {
                while (newFile.good()) {
                    string etalonLine, newLine;

                    getline(etalonFile, etalonLine);

                    getline(newFile, newLine);

                    if (!etalonLine.compare(newLine) == 0) {
                        CPPUNIT_ASSERT(false);
                    }
                }
            }
            etalonFile.close();
            newFile.close();
        } else {
            cout << "Error opening file " << fileName << endl;
        }
    } else {
        cout << "Error opening etalon file " << fnameEtalon << endl;
    }    
}  