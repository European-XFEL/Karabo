/* 
 * File:   Schema_Test.cc
 * Author: irinak
 * 
 * Created on September 28, 2012, 1:14 PM
 */

#include "Schema_Test.hh"
#include "Configurator_Test.hh"

using namespace std;
using namespace karabo::util;
using namespace schemaTest;

CPPUNIT_TEST_SUITE_REGISTRATION(Schema_Test);




Schema_Test::Schema_Test() {
}
 
Schema_Test::~Schema_Test() {
}

void Schema_Test::expectedParameters(Schema& expected) { 

    STRING_ELEMENT(expected).key("exampleKey1") 
            .tag("h/w")
            .displayedName("Example key 1")
            .description("Example key 1 description")
            .assignmentOptional().defaultValue("Some default string")
            .reconfigurable()
            .commit();

    INT32_ELEMENT(expected).key("exampleKey2").alias(10)
            .tag("h/w")
            .displayedName("Example key 2")
            .description("Example key 2 description")
            .assignmentOptional().defaultValue(10)
            .commit();

    UINT32_ELEMENT(expected).key("exampleKey3").alias(5.5)
            .tag("h/w")
            .displayedName("Example key 3")
            .description("Example key 3 description")
            .assignmentOptional().defaultValue(20)
            .reconfigurable()
            .commit();

    FLOAT_ELEMENT(expected).key("exampleKey4").alias("exampleAlias4")
            .tag("hardware")
            .displayedName("Example key 4")
            .description("Example key 4 description")
            .readOnly()
            .commit();
            

    INT64_ELEMENT(expected).key("exampleKey5").alias("exampleAlias5")
            .tag("hardware")
            .displayedName("Example key 5")
            .description("Example key 5 description")
            .readOnly()
            .initialValue(1442244)
            .commit();
//
//    IMAGE_ELEMENT(expected).key("image")
//            .displayedName("Image")
//            .description("Image")
//            .commit();

}


void Schema_Test::testBuildUp() {
    cout << endl << endl;
    try {
        
        Schema schema("MyClass");
        GraphicsRenderer1::expectedParameters(schema);
        cout << schema << endl;
        
        
        GraphicsRenderer::Pointer p = GraphicsRenderer::create("GraphicsRenderer", Hash("shapes.Circle.radius", 0.1,"color", "red", "antiAlias", true));
        
        
        
        
    } catch (karabo::util::Exception e) {
        cout << e << endl;
    }
 }
