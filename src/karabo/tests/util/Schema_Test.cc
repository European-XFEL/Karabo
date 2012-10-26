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

void Schema_Test::oneElementExpectParams(Schema& expected) {

    STRING_ELEMENT(expected).key("testKey").alias("testAlias")
            .displayedName("sample Displayed Name")
            .description("sample Description")
            .assignmentOptional().defaultValue("Some default string")
            .reconfigurable()
            .commit();
}

void Schema_Test::setUp() {
    Schema& tmp = sch.initParameterDescription("test", READ | WRITE | INIT);
    settingExpectedParameters(tmp);
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

void Schema_Test::testGetAccessMode() {

    CPPUNIT_ASSERT(sch.getAccessMode() == 7);

}

void Schema_Test::testHasParameters() {

    CPPUNIT_ASSERT(sch.hasParameters());

}

void Schema_Test::testGetAllParameters() {

    vector<string> vecStr = sch.getAllParameters();
    //uncomment for check:
    //for (int i=0; i < vecStr.size(); i++) {
    // cout << "vecStr[" <<i<<"]=" << vecStr[i] <<endl;
    //}
    CPPUNIT_ASSERT(vecStr[0] == "exampleKey1");
    CPPUNIT_ASSERT(vecStr[5] == "image");
    CPPUNIT_ASSERT(vecStr[10] == "image.pixelArray");

}

void Schema_Test::testHasRoot() {

    CPPUNIT_ASSERT(sch.hasRoot());

}

void Schema_Test::testGetRoot() {

    CPPUNIT_ASSERT(sch.getRoot() == "test");

}

void Schema_Test::testIsAttribute() {

    CPPUNIT_ASSERT(sch.isAttribute());

}

void Schema_Test::testPerKeyFunctionality() {
    Schema testSch;
    Schema& tmp = testSch.initParameterDescription("testSchema", READ | WRITE | INIT);
    oneElementExpectParams(tmp);
   
    CPPUNIT_ASSERT(testSch.hasRoot());
    string node = testSch.getRoot();
    CPPUNIT_ASSERT(node == "testSchema");
    
    CPPUNIT_ASSERT(testSch.isNode());
    
    CPPUNIT_ASSERT(testSch.hasParameters());
    Schema schemaParams = testSch.getParameters();
    
    CPPUNIT_ASSERT(!schemaParams.isNode());
    
    for (Schema::const_iterator it = schemaParams.begin(); it != schemaParams.end(); it++) {

        //Fetch individual description
        const Schema& desc = schemaParams.get<Schema>(it);

        CPPUNIT_ASSERT(desc.hasKey());
        string key = desc.getKey();
        CPPUNIT_ASSERT(key == "testKey");
        
        CPPUNIT_ASSERT(desc.hasAssignment());
        Schema::AssignmentType assignment = desc.getAssignment();
        CPPUNIT_ASSERT(assignment == 0);
        CPPUNIT_ASSERT(assignment == karabo::util::Schema::OPTIONAL_PARAM);
        
        CPPUNIT_ASSERT(!desc.isCommand());
        CPPUNIT_ASSERT(desc.isAttribute());
        CPPUNIT_ASSERT(desc.isLeaf());
        CPPUNIT_ASSERT(!desc.isNode());
        
        CPPUNIT_ASSERT(desc.isAccessReconfigurable());
        CPPUNIT_ASSERT(desc.isAssignmentOptional());
        
        CPPUNIT_ASSERT(desc.isValueOfType<string>());

        Types::Type valueType = desc.getValueType();
        CPPUNIT_ASSERT(valueType == 14);
        CPPUNIT_ASSERT(valueType == Types::STRING);
        
        CPPUNIT_ASSERT(desc.getValueTypeAsString() == "STRING");
        
        CPPUNIT_ASSERT(desc.hasDisplayedName());
        CPPUNIT_ASSERT(desc.getDisplayedName() == "sample Displayed Name");
        
        CPPUNIT_ASSERT(!desc.hasDisplayType());
        
        CPPUNIT_ASSERT(desc.hasDescription());
        CPPUNIT_ASSERT(desc.getDescription() == "sample Description");
                
        CPPUNIT_ASSERT(!desc.hasValueOptions());
        
        CPPUNIT_ASSERT(!desc.hasAllowedStates());
        
        CPPUNIT_ASSERT(desc.hasAccess());
        AccessType access = desc.getAccess();
        CPPUNIT_ASSERT(access == 4);
        CPPUNIT_ASSERT(access == WRITE);
            
        CPPUNIT_ASSERT(desc.hasDefaultValue());
        CPPUNIT_ASSERT(desc.getDefaultValue<string>() == "Some default string");
        
        CPPUNIT_ASSERT(desc.hasAlias());
        string alias = desc.getAlias<string>();
        CPPUNIT_ASSERT(alias == "testAlias");
        
        CPPUNIT_ASSERT(!desc.hasUnitName());
        CPPUNIT_ASSERT(!desc.hasUnitSymbol());
    }
}
