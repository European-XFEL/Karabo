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
            .assignmentOptional().defaultValue("Some default string")
            .reconfigurable()
            .commit();

    INT32_ELEMENT(expected).key("exampleKey2").alias(10)
            .displayedName("Example key 2")
            .description("Example key 2 description")
            .assignmentOptional().defaultValue(10)
            .commit();

    UINT32_ELEMENT(expected).key("exampleKey3").alias(5.5)
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

    INT64_ELEMENT(expected).key("exampleKey5").alias("exampleAlias5")
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

void Schema_Test::setUp() {
    Schema& tmp = sch.initParameterDescription("test", READ | WRITE | INIT);
    settingExpectedParameters(tmp);
    cout << "Schema sch: \n" << sch << endl;
}

void Schema_Test::testHasKey() {

    CPPUNIT_ASSERT(sch.hasKey("exampleKey1"));
    CPPUNIT_ASSERT(sch.hasKey("exampleKey2"));
    CPPUNIT_ASSERT(sch.hasKey("image"));

    CPPUNIT_ASSERT(!sch.hasKey("abcd"));

}

void Schema_Test::testKeyHasAlias() {

    CPPUNIT_ASSERT(!sch.keyHasAlias("exampleKey1"));
    CPPUNIT_ASSERT(!sch.keyHasAlias("image"));

    CPPUNIT_ASSERT(sch.keyHasAlias("exampleKey2"));
    CPPUNIT_ASSERT(sch.keyHasAlias("exampleKey3"));
    CPPUNIT_ASSERT(sch.keyHasAlias("exampleKey4"));
    CPPUNIT_ASSERT(sch.keyHasAlias("exampleKey5"));
}

void Schema_Test::testHasAlias() {

    CPPUNIT_ASSERT(sch.hasAlias(10));
    CPPUNIT_ASSERT(sch.hasAlias(5.5));
    CPPUNIT_ASSERT(sch.hasAlias("exampleAlias4"));
    CPPUNIT_ASSERT(sch.hasAlias("exampleAlias5"));
}

void Schema_Test::testAliasIsOfType() {

    CPPUNIT_ASSERT(sch.aliasIsOfType<int>("exampleKey2"));
    CPPUNIT_ASSERT(sch.aliasIsOfType<double>("exampleKey3"));
    CPPUNIT_ASSERT(sch.aliasIsOfType<std::string > ("exampleKey4"));

}

void Schema_Test::testParameterIsOfType() {

    CPPUNIT_ASSERT(sch.parameterIsOfType<std::string > ("exampleKey1"));
    CPPUNIT_ASSERT(sch.parameterIsOfType<int>("exampleKey2"));
    CPPUNIT_ASSERT(sch.parameterIsOfType<unsigned int>("exampleKey3"));
    CPPUNIT_ASSERT(sch.parameterIsOfType<float>("exampleKey4"));
    CPPUNIT_ASSERT(sch.parameterIsOfType<long long>("exampleKey5"));

}

void Schema_Test::testKey2Alias() {

    CPPUNIT_ASSERT(sch.key2alias<int>("exampleKey2") == 10);
    CPPUNIT_ASSERT(sch.key2alias<double>("exampleKey3") == 5.5);
    CPPUNIT_ASSERT(sch.key2alias<std::string > ("exampleKey4") == "exampleAlias4");

}

void Schema_Test::testAlias2Key() {

    CPPUNIT_ASSERT(sch.alias2key(10) == "exampleKey2");
    CPPUNIT_ASSERT(sch.alias2key(5.5) == "exampleKey3");
    CPPUNIT_ASSERT(sch.alias2key("exampleAlias4") == "exampleKey4");

}