/* 
 * File:   Schema_Test.cc
 * Author: irinak
 * 
 * Created on September 28, 2012, 1:14 PM
 */

#include "Schema_Test.hh"

using namespace std;
using namespace karabo::util;

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
            .tag("s/w")
            .displayedName("Example key 4")
            .description("Example key 4 description")
            .assignmentOptional().defaultValue(0)
            .readOnly().noDefaultValue()
            .commit();

    INT64_ELEMENT(expected).key("exampleKey5").alias("exampleAlias5")
            .tag("s/w")
            .displayedName("Example key 5")
            .description("Example key 5 description")
            .assignmentOptional().defaultValue(0)
            .readOnly().noDefaultValue()
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
        schema.filteringSetAccessMode(READ|WRITE|INIT);
                
        Schema_Test::expectedParameters(schema);
        cout << schema << endl;  
    } catch (karabo::util::Exception e) {
        cout << e << endl;
    }
}
