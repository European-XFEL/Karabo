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

void Schema_Test::settingExpectedParameters(Schema& expected) {

    STRING_ELEMENT(expected).key("exampleKey1")
            .displayedName("Example key 1")
            .description("Example key 1 description")
            .alias("myAlias")
            .assignmentOptional().defaultValue("Some default string")
            .reconfigurable()
            .commit();

    UINT32_ELEMENT(expected).key("exampleKey2")
            .displayedName("Example key 2")
            .description("Example key 2 description")
            .assignmentOptional().defaultValue(10)
            .commit();

    UINT32_ELEMENT(expected).key("exampleKey3")
            .displayedName("Example key 3")
            .description("Example key 3 description")
            .assignmentOptional().defaultValue(20)
            .reconfigurable()
            .commit();

    FLOAT_ELEMENT(expected).key("exampleKey4").alias("exampleAlias4")
            .displayedName("Example key 4")
            .description("Example key 4 description")
            .assignmentOptional().defaultValue(0)
            .readOnly()
            .commit();

    INT32_ELEMENT(expected).key("exampleKey5").alias("exampleAlias5")
            .displayedName("Example key 5")
            .description("Example key 5 description")
            .assignmentOptional().defaultValue(0)
            .readOnly()
            .commit();

    FLOAT_IMAGE_ELEMENT(expected).key("image")
            .displayedName("Image")
            .description("Image")
            .commit();

}

void Schema_Test::testHasKey() {
    Schema sch;
    Schema& tmp = sch.initParameterDescription("test", READ | WRITE | INIT);
    settingExpectedParameters(tmp);
    CPPUNIT_ASSERT(sch.hasKey("exampleKey1"));
    CPPUNIT_ASSERT(sch.keyHasAlias("exampleKey1"));
}
