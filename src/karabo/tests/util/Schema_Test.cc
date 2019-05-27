/* 
 * File:   Schema_Test.cc
 * Author: irinak
 * 
 * Created on September 28, 2012, 1:14 PM
 */

#include <cppunit/TestAssert.h>

#include "Schema_Test.hh"

using namespace std;
using namespace karabo::util;
using namespace configurationTest;

CPPUNIT_TEST_SUITE_REGISTRATION(Schema_Test);


Schema_Test::Schema_Test() {
}


Schema_Test::~Schema_Test() {
}


void Schema_Test::testBuildUp() {

    try {
        {
            Schema schema = Configurator<Shape>::getSchema("Circle");
            CPPUNIT_ASSERT(schema.isAccessInitOnly("shadowEnabled") == true);
            CPPUNIT_ASSERT(schema.isAccessInitOnly("radius") == true);
            CPPUNIT_ASSERT(schema.isLeaf("radius") == true);
        }
        {
            Schema schema("test");
            GraphicsRenderer1::expectedParameters(schema);
            CPPUNIT_ASSERT(schema.isAccessInitOnly("shapes.circle.radius") == true);
            CPPUNIT_ASSERT(schema.isLeaf("shapes.circle.radius") == true);
        }
        GraphicsRenderer::Pointer p = GraphicsRenderer::create("GraphicsRenderer", Hash("shapes.Circle.radius", 0.5, "color", "red", "antiAlias", "true"));
        //cout << Configurator<GraphicsRenderer>::getSchema("GraphicsRenderer"); 

    } catch (karabo::util::Exception e) {
        KARABO_LOG_FRAMEWORK_DEBUG << e;
        CPPUNIT_ASSERT(false);
    }
}


void Schema_Test::testPaths() {
    Schema schema("test");
    GraphicsRenderer::expectedParameters(schema);

    vector<string> paths = schema.getPaths();
    CPPUNIT_ASSERT(paths[0] == "antiAlias");
    CPPUNIT_ASSERT(paths[1] == "color");
    CPPUNIT_ASSERT(paths[2] == "bold");
    CPPUNIT_ASSERT(paths[3] == "shapes.Circle.shadowEnabled");
    CPPUNIT_ASSERT(paths[4] == "shapes.Circle.radius");
    CPPUNIT_ASSERT(paths[5] == "shapes.EditableCircle.shadowEnabled");
    CPPUNIT_ASSERT(paths[6] == "shapes.EditableCircle.radius");
    CPPUNIT_ASSERT(paths[7] == "shapes.Rectangle.shadowEnabled");
    CPPUNIT_ASSERT(paths[8] == "shapes.Rectangle.a");
    CPPUNIT_ASSERT(paths[9] == "shapes.Rectangle.b");
}


void Schema_Test::testGetRequiredAccessLevel() {

    Schema schema = GraphicsRenderer::getSchema("GraphicsRenderer");
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes") == Schema::EXPERT);
    //all sub-elements of Node-element 'shapes' will have EXPERT level: 
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Circle.shadowEnabled") == Schema::EXPERT);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Circle") == Schema::EXPERT);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Rectangle.b") == Schema::EXPERT);

    //but sub-element 'shapes.Rectangle.a' with higher level will keep its ADMIN level
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Rectangle.a") == Schema::ADMIN);

    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("antiAlias") == Schema::EXPERT);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("color") == Schema::USER);

    //check requiredAccesLevel set on leaves-elements in expectedParameters
    CPPUNIT_ASSERT(m_schema.getRequiredAccessLevel("exampleKey1") == Schema::USER);
    CPPUNIT_ASSERT(m_schema.getRequiredAccessLevel("exampleKey2") == Schema::OPERATOR);
    CPPUNIT_ASSERT(m_schema.getRequiredAccessLevel("exampleKey3") == Schema::EXPERT);
    CPPUNIT_ASSERT(m_schema.getRequiredAccessLevel("exampleKey4") == Schema::ADMIN);

    //default for readOnly element - OBSERVER
    CPPUNIT_ASSERT(m_schema.getRequiredAccessLevel("exampleKey5") == Schema::OBSERVER);

    //default for reconfigurable element - USER
    CPPUNIT_ASSERT(m_schema.getRequiredAccessLevel("sampleKey") == Schema::USER);

    Schema ose("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(ose);

    //check default requiredAccessLevel by elements : slot, path, vector, image
    CPPUNIT_ASSERT(ose.getRequiredAccessLevel("slotTest") == Schema::USER); //SLOT
    CPPUNIT_ASSERT(ose.getRequiredAccessLevel("filename") == Schema::USER); //reconfigurable PATH
    CPPUNIT_ASSERT(ose.getRequiredAccessLevel("testfile") == Schema::OBSERVER); //readOnly PATH
    CPPUNIT_ASSERT(ose.getRequiredAccessLevel("vecIntReconfig") == Schema::USER); //reconfigurable VECTOR_INT32
    CPPUNIT_ASSERT(ose.getRequiredAccessLevel("vecInt") == Schema::OBSERVER); //readOnly VECTOR_INT32
    CPPUNIT_ASSERT(ose.getRequiredAccessLevel("vecBool") == Schema::USER); //init VECTOR_BOOL
    CPPUNIT_ASSERT(ose.getRequiredAccessLevel("image") == Schema::OBSERVER); //IMAGE
}


void Schema_Test::testSetRequiredAccessLevel() {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    CPPUNIT_ASSERT(sch.getRequiredAccessLevel("image") == Schema::OBSERVER); //IMAGE (default level OBSERVER)
    sch.setRequiredAccessLevel("image", Schema::ADMIN);
    CPPUNIT_ASSERT(sch.getRequiredAccessLevel("image") == Schema::ADMIN); //IMAGE (changed by 'set' to ADMIN)

    Schema schema = GraphicsRenderer::getSchema("GraphicsRenderer");
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes") == Schema::EXPERT);
    //all sub-elements of Node-element 'shapes' will have EXPERT level: 
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Circle.shadowEnabled") == Schema::EXPERT);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Circle") == Schema::EXPERT);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Rectangle.a") == Schema::ADMIN);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Rectangle.b") == Schema::EXPERT);


    //set top-Node to lower level 'Observer' and check that sub-elements keep previous higher level
    schema.setRequiredAccessLevel("shapes", Schema::OBSERVER);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes") == Schema::OBSERVER);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Circle.shadowEnabled") == Schema::USER); //default level for init-elem is 'user'
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Circle") == Schema::OBSERVER);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Rectangle.a") == Schema::ADMIN);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Rectangle.b") == Schema::USER); //default level for init-elem is 'user'


}


void Schema_Test::setUp() {
    try {
        m_schema = Schema("MyTest", Schema::AssemblyRules(READ | WRITE | INIT));
        TestStruct1::expectedParameters(m_schema);
    } catch (karabo::util::Exception e) {
        KARABO_LOG_FRAMEWORK_DEBUG << e;
    }
}


void Schema_Test::testGetRootName() {
    CPPUNIT_ASSERT(m_schema.getRootName() == "MyTest");
}


void Schema_Test::testGetTags() {
    CPPUNIT_ASSERT(m_schema.getTags("exampleKey1")[0] == "hardware");
    CPPUNIT_ASSERT(m_schema.getTags("exampleKey1")[1] == "poll");
    CPPUNIT_ASSERT(m_schema.getTags("exampleKey2")[0] == "hardware");
    CPPUNIT_ASSERT(m_schema.getTags("exampleKey2")[1] == "poll");
    CPPUNIT_ASSERT(m_schema.getTags("exampleKey3")[0] == "hardware");
    CPPUNIT_ASSERT(m_schema.getTags("exampleKey3")[1] == "set");
    CPPUNIT_ASSERT(m_schema.getTags("exampleKey4")[0] == "software");
    CPPUNIT_ASSERT(m_schema.getTags("exampleKey5")[0] == "h/w");
    CPPUNIT_ASSERT(m_schema.getTags("exampleKey5")[1] == "d.m.y");
}


void Schema_Test::testGetNodeType() {

    int nodeType = m_schema.getNodeType("exampleKey1");
    CPPUNIT_ASSERT(nodeType == Schema::LEAF);

    CPPUNIT_ASSERT(m_schema.getNodeType("exampleKey5") == Schema::LEAF);
}


void Schema_Test::testGetValueType() {
    Types::ReferenceType valueType = m_schema.getValueType("exampleKey1");
    CPPUNIT_ASSERT(valueType == Types::STRING);

    CPPUNIT_ASSERT(m_schema.getValueType("exampleKey2") == Types::INT32);
    CPPUNIT_ASSERT(m_schema.getValueType("exampleKey3") == Types::UINT32);
    CPPUNIT_ASSERT(m_schema.getValueType("exampleKey4") == Types::FLOAT);
    CPPUNIT_ASSERT(m_schema.getValueType("exampleKey5") == Types::INT64);
}


void Schema_Test::testKeyHasAlias() {
    CPPUNIT_ASSERT(m_schema.keyHasAlias("exampleKey1") == false);
    CPPUNIT_ASSERT(m_schema.keyHasAlias("exampleKey2") == true);
    CPPUNIT_ASSERT(m_schema.keyHasAlias("exampleKey3") == true);
    CPPUNIT_ASSERT(m_schema.keyHasAlias("exampleKey4") == true);
    CPPUNIT_ASSERT(m_schema.keyHasAlias("exampleKey5") == true);
}


void Schema_Test::testAliasHasKey() {
    CPPUNIT_ASSERT(m_schema.aliasHasKey(10) == true);
    CPPUNIT_ASSERT(m_schema.aliasHasKey(5.5) == true);
    CPPUNIT_ASSERT(m_schema.aliasHasKey("exampleAlias4") == true);

    vector<int> vecIntAlias;
    vecIntAlias.push_back(10);
    vecIntAlias.push_back(20);
    vecIntAlias.push_back(30);
    CPPUNIT_ASSERT(m_schema.aliasHasKey(vecIntAlias) == true);

    CPPUNIT_ASSERT(m_schema.aliasHasKey(7) == false);
}


void Schema_Test::testGetAliasFromKey() {
    CPPUNIT_ASSERT(m_schema.getAliasFromKey<int>("exampleKey2") == 10);
    CPPUNIT_ASSERT(m_schema.getAliasFromKey<double>("exampleKey3") == 5.5);
    CPPUNIT_ASSERT(m_schema.getAliasFromKey<string > ("exampleKey4") == "exampleAlias4");

    vector<int> aliasVec = m_schema.getAliasFromKey<vector<int> > ("exampleKey5");
    CPPUNIT_ASSERT(aliasVec[0] == 10);
    CPPUNIT_ASSERT(aliasVec[1] == 20);
    CPPUNIT_ASSERT(aliasVec[2] == 30);

}


void Schema_Test::testGetKeyFromAlias() {
    CPPUNIT_ASSERT(m_schema.getKeyFromAlias(10) == "exampleKey2");
    CPPUNIT_ASSERT(m_schema.getKeyFromAlias(5.5) == "exampleKey3");
    CPPUNIT_ASSERT(m_schema.getKeyFromAlias("exampleAlias4") == "exampleKey4");

    vector<int> vecIntAlias;
    vecIntAlias.push_back(10);
    vecIntAlias.push_back(20);
    vecIntAlias.push_back(30);
    CPPUNIT_ASSERT(m_schema.getKeyFromAlias(vecIntAlias) == "exampleKey5");
}


void Schema_Test::testGetAliasAsString() {
    CPPUNIT_ASSERT(m_schema.getAliasAsString("exampleKey2") == "10");
    CPPUNIT_ASSERT(m_schema.getAliasAsString("exampleKey3") == "5.5");
    CPPUNIT_ASSERT(m_schema.getAliasAsString("exampleKey4") == "exampleAlias4");

    string aliasStr = m_schema.getAliasAsString("exampleKey5");
    CPPUNIT_ASSERT(aliasStr == "10,20,30");
}


void Schema_Test::testGetAccessMode() {
    int accessModeKey1 = m_schema.getAccessMode("exampleKey1");
    CPPUNIT_ASSERT(accessModeKey1 == WRITE);

    CPPUNIT_ASSERT(m_schema.getAccessMode("exampleKey2") == INIT);
    CPPUNIT_ASSERT(m_schema.getAccessMode("exampleKey3") == WRITE);
    CPPUNIT_ASSERT(m_schema.getAccessMode("exampleKey4") == INIT);
    CPPUNIT_ASSERT(m_schema.getAccessMode("exampleKey5") == READ);
}


void Schema_Test::testGetAssignment() {
    int assignment = m_schema.getAssignment("exampleKey1");
    CPPUNIT_ASSERT(assignment == Schema::OPTIONAL_PARAM);

    CPPUNIT_ASSERT(m_schema.getAssignment("exampleKey2") == Schema::OPTIONAL_PARAM);
    CPPUNIT_ASSERT(m_schema.getAssignment("exampleKey3") == Schema::MANDATORY_PARAM);
    CPPUNIT_ASSERT(m_schema.getAssignment("exampleKey4") == Schema::INTERNAL_PARAM);
    CPPUNIT_ASSERT(m_schema.getAssignment("exampleKey5") == Schema::OPTIONAL_PARAM);
}


void Schema_Test::testGetOptions() {
    vector<std::string> options = m_schema.getOptions<std::string>("exampleKey1");
    CPPUNIT_ASSERT(options[0] == "Radio");
    CPPUNIT_ASSERT(options[1] == "Air Condition");
    CPPUNIT_ASSERT(options[2] == "Navigation");

    CPPUNIT_ASSERT(m_schema.getOptions<int>("exampleKey2")[0] == 5);
    CPPUNIT_ASSERT(m_schema.getOptions<int>("exampleKey2")[1] == 25);
    CPPUNIT_ASSERT(m_schema.getOptions<int>("exampleKey2")[2] == 10);

    CPPUNIT_ASSERT(m_schema.getOptions<float>("exampleKey4")[0] == 1.11f);
    CPPUNIT_ASSERT(m_schema.getOptions<float>("exampleKey4")[1] == -2.22f);
    CPPUNIT_ASSERT(m_schema.getOptions<float>("exampleKey4")[2] == 5.55f);
    CPPUNIT_ASSERT(m_schema.getOptions<unsigned char>("exampleKey8")[2] == 3);

    // Hijack test to check exception for invalid (empty) options.
    Schema schema;
    CPPUNIT_ASSERT_THROW(DOUBLE_ELEMENT(schema).key("some")
                         .options(std::vector<double>()),
                         karabo::util::ParameterException);

    CPPUNIT_ASSERT_THROW(INT32_ELEMENT(schema).key("someOther")
                         .options(""),
                         karabo::util::ParameterException);
}


void Schema_Test::testGetDefaultValue() {
    string defaultValueKey1 = m_schema.getDefaultValue<string > ("exampleKey1");
    CPPUNIT_ASSERT(defaultValueKey1 == "Navigation");

    int defaultValueKey2 = m_schema.getDefaultValue<int>("exampleKey2");
    CPPUNIT_ASSERT(defaultValueKey2 == 10);
    string defaultValueAsString2 = m_schema.getDefaultValueAs<string > ("exampleKey2");
    CPPUNIT_ASSERT(defaultValueAsString2 == "10");

    long long defaultValue = m_schema.getDefaultValue<long long>("exampleKey5");
    CPPUNIT_ASSERT(defaultValue == 1442244);
    string defaultValueAsString5 = m_schema.getDefaultValueAs<string > ("exampleKey5");
    CPPUNIT_ASSERT(defaultValueAsString5 == "1442244");

    CPPUNIT_ASSERT(m_schema.getDefaultValue<int>("sampleKey") == 10); // Was set from string, but maintains correct data typing

    CPPUNIT_ASSERT(m_schema.getDefaultValueAs<string>("sampleKey") == "10");
    CPPUNIT_ASSERT(m_schema.getDefaultValueAs<int>("sampleKey") == 10);

    CPPUNIT_ASSERT(m_schema.hasDefaultValue("sampleKey2") == true);
    CPPUNIT_ASSERT(m_schema.getDefaultValue<int>("sampleKey2") == 0);
}


void Schema_Test::testGetAllowedStates() {

    vector<State> allowedStates = m_schema.getAllowedStates("exampleKey3");
    CPPUNIT_ASSERT(allowedStates[0] == State::COOLED);
    CPPUNIT_ASSERT(allowedStates[1] == State::ACTIVE);
    CPPUNIT_ASSERT(m_schema.getAllowedStates("exampleKey3")[2] == State::DISABLED);
    CPPUNIT_ASSERT(m_schema.getAllowedStates("exampleKey3")[3] == State::KNOWN);
}


void Schema_Test::testGetUnit() {
    CPPUNIT_ASSERT(m_schema.getUnit("exampleKey2") == Unit::METER);
    CPPUNIT_ASSERT(m_schema.getUnitName("exampleKey2") == "meter");
    CPPUNIT_ASSERT(m_schema.getUnitSymbol("exampleKey2") == "m");

    CPPUNIT_ASSERT(m_schema.getUnit("exampleKey10.data") == Unit::DEGREE_CELSIUS);
    CPPUNIT_ASSERT(m_schema.getUnitName("exampleKey10.data") == "degree_celsius");
    CPPUNIT_ASSERT(m_schema.getUnitSymbol("exampleKey10.data") == "degC");
}


void Schema_Test::testGetMetricPrefix() {
    CPPUNIT_ASSERT(m_schema.getMetricPrefix("exampleKey2") == MetricPrefix::MILLI);
    CPPUNIT_ASSERT(m_schema.getMetricPrefixName("exampleKey2") == "milli");
    CPPUNIT_ASSERT(m_schema.getMetricPrefixSymbol("exampleKey2") == "m");

    CPPUNIT_ASSERT(m_schema.getMetricPrefix("exampleKey10.data") == MetricPrefix::CENTI);
    CPPUNIT_ASSERT(m_schema.getMetricPrefixName("exampleKey10.data") == "centi");
    CPPUNIT_ASSERT(m_schema.getMetricPrefixSymbol("exampleKey10.data") == "c");
}


void Schema_Test::testGetMinIncMaxInc() {

    int minInc = m_schema.getMinInc<int>("exampleKey2");
    string minIncStr = m_schema.getMinIncAs<string>("exampleKey2");
    CPPUNIT_ASSERT(minInc == 5);
    CPPUNIT_ASSERT(minIncStr == "5");


    int maxInc = m_schema.getMaxInc<int>("exampleKey2");
    string maxIncStr = m_schema.getMaxIncAs<string>("exampleKey2");
    CPPUNIT_ASSERT(maxInc == 25);
    CPPUNIT_ASSERT(maxIncStr == "25");
}


void Schema_Test::testGetMinExcMaxExc() {

    unsigned int minExc = m_schema.getMinExc<unsigned int>("exampleKey3");
    string minExcStr = m_schema.getMinExcAs<string>("exampleKey3");
    CPPUNIT_ASSERT(minExc == 10);
    CPPUNIT_ASSERT(minExcStr == "10");

    unsigned int maxExc = m_schema.getMaxExc<unsigned int>("exampleKey3");
    string maxExcStr = m_schema.getMaxExcAs<string>("exampleKey3");
    CPPUNIT_ASSERT(maxExc == 20);
    CPPUNIT_ASSERT(maxExcStr == "20");

}


void Schema_Test::testGetAlarmLowAlarmHigh() {
    CPPUNIT_ASSERT(m_schema.getAlarmLow<long long>("exampleKey5") == -20);
    CPPUNIT_ASSERT(m_schema.getAlarmHigh<long long>("exampleKey5") == 20);
}


void Schema_Test::testGetWarnLowWarnHigh() {
    CPPUNIT_ASSERT(m_schema.getWarnLow<long long>("exampleKey5") == -10);
    CPPUNIT_ASSERT(m_schema.getWarnHigh<long long>("exampleKey5") == 10);
}


void Schema_Test::testHasAlarmWarn() {
    CPPUNIT_ASSERT(m_schema.hasWarnLow("exampleKey5") == true);
    CPPUNIT_ASSERT(m_schema.hasWarnHigh("exampleKey5") == true);
    CPPUNIT_ASSERT(m_schema.hasAlarmLow("exampleKey5") == true);
    CPPUNIT_ASSERT(m_schema.hasAlarmHigh("exampleKey5") == true);
}


void Schema_Test::testArchivePolicy() {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    CPPUNIT_ASSERT(sch.hasArchivePolicy("testfile") == true);
    CPPUNIT_ASSERT(sch.hasArchivePolicy("vecInt") == true);
    CPPUNIT_ASSERT(sch.hasArchivePolicy("vecDouble") == true);

    CPPUNIT_ASSERT(sch.getArchivePolicy("testfile") == Schema::EVERY_10MIN);
    CPPUNIT_ASSERT(sch.getArchivePolicy("vecInt") == Schema::EVERY_EVENT);
    CPPUNIT_ASSERT(sch.getArchivePolicy("vecDouble") == Schema::NO_ARCHIVING);

}


void Schema_Test::testPerKeyFunctionality() {

    std::vector<std::string> keys = m_schema.getKeys();

    for (size_t i = 0; i < keys.size(); ++i) {

        if (keys[i] == "exampleKey1") {
            bool hasAssignment = m_schema.hasAssignment(keys[i]);
            CPPUNIT_ASSERT(hasAssignment == true);
            CPPUNIT_ASSERT(m_schema.isAssignmentOptional(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasDefaultValue(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasAccessMode(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAccessReconfigurable(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasOptions(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.hasTags(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasUnit(keys[i]) == false);
            CPPUNIT_ASSERT(m_schema.hasMetricPrefix(keys[i]) == false);
        }

        if (keys[i] == "exampleKey2") {
            CPPUNIT_ASSERT(m_schema.hasDefaultValue(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasAccessMode(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAccessInitOnly(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasOptions(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.hasTags(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasAllowedStates(keys[i]) == false);

            CPPUNIT_ASSERT(m_schema.hasUnit(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.hasMetricPrefix(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasMinInc(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.hasMaxInc(keys[i]) == true);
        }

        if (keys[i] == "exampleKey3") {

            CPPUNIT_ASSERT(m_schema.hasAssignment(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAssignmentMandatory(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasDefaultValue(keys[i]) == false);

            CPPUNIT_ASSERT(m_schema.hasOptions(keys[i]) == false);

            CPPUNIT_ASSERT(m_schema.hasAllowedStates(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasMinExc(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.hasMaxExc(keys[i]) == true);
        }

        if (keys[i] == "exampleKey4") {
            CPPUNIT_ASSERT(m_schema.hasDefaultValue(keys[i]) == false);

            CPPUNIT_ASSERT(m_schema.isAssignmentInternal(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasAccessMode(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAccessInitOnly(keys[i]) == true);

        }

        if (keys[i] == "exampleKey5") {
            CPPUNIT_ASSERT(m_schema.hasDefaultValue(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasAssignment(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAssignmentOptional(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasAccessMode(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAccessReadOnly(keys[i]) == true);
        }

        if (keys[i] == "exampleKey6") {
            CPPUNIT_ASSERT(m_schema.hasDefaultValue(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasAssignment(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAssignmentOptional(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasAccessMode(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAccessReadOnly(keys[i]) == false);
            CPPUNIT_ASSERT(m_schema.getDisplayType(keys[i]) == "hex");
        }

        if (keys[i] == "exampleKey7") {
            CPPUNIT_ASSERT(m_schema.hasDefaultValue(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasAssignment(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAssignmentOptional(keys[i]) == true);

            CPPUNIT_ASSERT(m_schema.hasAccessMode(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAccessReadOnly(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.getDisplayType(keys[i]) == "bin|0:isError,1:isMoving,2:isBusy,15:isOn");
        }

        if (keys[i] == "exampleKey9") {
            CPPUNIT_ASSERT(m_schema.hasDefaultValue(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.hasAccessMode(keys[i]) == true);
            CPPUNIT_ASSERT(m_schema.isAccessReadOnly(keys[i]) == true);
            CPPUNIT_ASSERT_EQUAL(std::string("TestDisplayType"), m_schema.getDisplayType(keys[i]));
        }
    }

}


void Schema_Test::testSlotElement() {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    CPPUNIT_ASSERT(sch.getDescription("slotTest") == "Test slot element");
    CPPUNIT_ASSERT(sch.getAllowedStates("slotTest")[0] == State::STARTED);
    CPPUNIT_ASSERT(sch.getAllowedStates("slotTest")[2] == State::ERROR);
    CPPUNIT_ASSERT(sch.isCommand("slotTest") == true);
    CPPUNIT_ASSERT(sch.isProperty("slotTest") == false);
}


void Schema_Test::testVectorElements() {

    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    vector<int> vecDef;
    vecDef.push_back(10);
    vecDef.push_back(20);
    vecDef.push_back(30);
    CPPUNIT_ASSERT(sch.getDefaultValue<vector<int> >("vecInt") == vecDef);
    CPPUNIT_ASSERT(sch.getValueType("vecInt") == Types::VECTOR_INT32);
    CPPUNIT_ASSERT(sch.getWarnLow<vector<int> >("vecInt") == vector<int>(3, 50));
    CPPUNIT_ASSERT(sch.getWarnHigh<vector<int> >("vecInt") == vector<int>(3, 100));
    CPPUNIT_ASSERT(sch.isAccessReadOnly("vecInt") == true);
    CPPUNIT_ASSERT(sch.isAssignmentOptional("vecInt") == true);
    CPPUNIT_ASSERT(sch.hasDefaultValue("vecInt") == true);
    CPPUNIT_ASSERT_EQUAL(std::string("Curve"), sch.getDisplayType("vecInt"));

    CPPUNIT_ASSERT(sch.getValueType("vecDouble") == Types::VECTOR_DOUBLE);
    CPPUNIT_ASSERT(sch.getAlarmLow<vector<double> >("vecDouble") == vector<double>(3, -5.5));
    CPPUNIT_ASSERT(sch.getAlarmHigh<vector<double> >("vecDouble") == vector<double>(3, 7.7));
    CPPUNIT_ASSERT(sch.isAccessReadOnly("vecDouble") == true);
    CPPUNIT_ASSERT(sch.isAssignmentOptional("vecDouble") == true);
    CPPUNIT_ASSERT_EQUAL(std::string("Curve"), sch.getDisplayType("vecDouble"));

    //readOnly element has default value (even if initialValue not specified) and default value is vector<double>() -- empty vector
    CPPUNIT_ASSERT(sch.hasDefaultValue("vecDouble") == true);
    CPPUNIT_ASSERT(sch.getDefaultValue<vector<double> >("vecDouble") == vector<double>());

    CPPUNIT_ASSERT(sch.hasAlarmLow("vecDouble") == true);
    CPPUNIT_ASSERT(sch.hasAlarmHigh("vecDouble") == true);
    CPPUNIT_ASSERT(sch.hasWarnLow("vecDouble") == false);
    CPPUNIT_ASSERT(sch.hasWarnHigh("vecDouble") == false);

    CPPUNIT_ASSERT(sch.isAccessReconfigurable("vecIntReconfig") == true);
    CPPUNIT_ASSERT(sch.isAssignmentOptional("vecIntReconfig") == true);
    CPPUNIT_ASSERT(sch.hasDefaultValue("vecIntReconfig") == true);
    CPPUNIT_ASSERT(sch.getDefaultValue<vector<int> >("vecIntReconfig") == vecDef);
    CPPUNIT_ASSERT_EQUAL(std::string("Curve"), sch.getDisplayType("vecIntReconfig"));

    CPPUNIT_ASSERT(sch.hasDefaultValue("vecIntReconfigStr") == true);

    vector<int> compare;
    compare.push_back(11);
    compare.push_back(22);
    compare.push_back(33);

    vector<int> defVecNew = sch.getDefaultValue<vector<int> >("vecIntReconfigStr");
    CPPUNIT_ASSERT(defVecNew == compare);

    vector<double> comp;
    comp.push_back(1.1);
    comp.push_back(2.2);
    comp.push_back(3.3);

    vector<double> defDVecNew = sch.getDefaultValue<vector<double> >("vecDoubleReconfigStr");
    CPPUNIT_ASSERT(defDVecNew == comp);

    CPPUNIT_ASSERT(sch.isAccessInitOnly("vecBool") == true);
    CPPUNIT_ASSERT(sch.isAssignmentOptional("vecBool") == false);
    CPPUNIT_ASSERT(sch.isAssignmentMandatory("vecBool") == true);
    CPPUNIT_ASSERT(sch.hasMinSize("vecBool") == true);
    CPPUNIT_ASSERT(sch.hasMaxSize("vecBool") == true);
    CPPUNIT_ASSERT(sch.getMinSize("vecBool") == 2);
    CPPUNIT_ASSERT(sch.getMaxSize("vecBool") == 7);

    CPPUNIT_ASSERT(sch.getTags("vecBool")[0] == "h/w");
    CPPUNIT_ASSERT(sch.getTags("vecBool")[1] == "d.m.y");

    vector<State> allowedStates = sch.getAllowedStates("vecBool");
    CPPUNIT_ASSERT(allowedStates[0] == State::STARTED);
    CPPUNIT_ASSERT(allowedStates[1] == State::STOPPED);

    CPPUNIT_ASSERT_EQUAL(std::string("Curve"), sch.getDisplayType("vecBool"));

}


void Schema_Test::testArrayElements() {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    std::vector<unsigned long long> shapeVec;
    shapeVec.push_back(3);
    shapeVec.push_back(2);
    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<unsigned long long > >("arrBool.shape") == shapeVec);
    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<unsigned long long > >("arrInt8.shape") == shapeVec);
    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<unsigned long long > >("arrUInt16.shape") == shapeVec);
    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<unsigned long long > >("arrFloat.shape") == shapeVec);
    shapeVec.push_back(-1);
    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<unsigned long long > >("arrDouble.shape") == shapeVec);

    std::vector<unsigned long long> undefShapeVec;
    undefShapeVec.push_back(0);
    undefShapeVec.push_back(3);
    undefShapeVec.push_back(0);
    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<unsigned long long > >("arrUndefined.shape") == undefShapeVec);

    CPPUNIT_ASSERT(sch.isAccessReadOnly("arrBool") == true);
    CPPUNIT_ASSERT(sch.isAccessReadOnly("arrInt8") == true);
    CPPUNIT_ASSERT(sch.isAccessReadOnly("arrUInt16") == true);
    CPPUNIT_ASSERT(sch.isAccessReadOnly("arrFloat") == true);
    CPPUNIT_ASSERT(sch.isAccessReadOnly("arrDouble") == true);

    // Just choose one array to test NDArray discovery:
    CPPUNIT_ASSERT(sch.isNode("arrUInt16"));
    CPPUNIT_ASSERT(sch.isCustomNode("arrUInt16"));
    CPPUNIT_ASSERT_EQUAL(sch.getCustomNodeClass("arrUInt16"), std::string("NDArray"));
}


void Schema_Test::testPathElement() {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    CPPUNIT_ASSERT(sch.getValueType("filename") == Types::STRING);
    CPPUNIT_ASSERT(sch.getAliasAsString("filename") == "5");
    CPPUNIT_ASSERT(sch.hasOptions("filename") == true);
    CPPUNIT_ASSERT(sch.getOptions<string>("filename")[0] == "file1");
    CPPUNIT_ASSERT(sch.getOptions<string>("filename")[1] == "file2");
    CPPUNIT_ASSERT(sch.hasDefaultValue("filename") == true);
    CPPUNIT_ASSERT(sch.getDefaultValue<string>("filename") == "karabo.log");
    CPPUNIT_ASSERT(sch.isAccessReconfigurable("filename") == true);
    CPPUNIT_ASSERT(sch.getAssignment("filename") == Schema::OPTIONAL_PARAM);


    CPPUNIT_ASSERT(sch.isAccessReadOnly("testfile") == true);
    CPPUNIT_ASSERT(sch.hasDefaultValue("testfile") == true);
    CPPUNIT_ASSERT(sch.getDefaultValue<string>("testfile") == "initFile");
    CPPUNIT_ASSERT(sch.hasAlarmHigh("testfile") == true);
    CPPUNIT_ASSERT(sch.getAlarmLow<string>("testfile") == "b");

    CPPUNIT_ASSERT(sch.isProperty("testfile") == true);
}


void Schema_Test::testImageElement() {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);
    CPPUNIT_ASSERT(sch.isNode("image") == true);
    CPPUNIT_ASSERT(sch.has("image.encoding") == true);

    CPPUNIT_ASSERT(sch.isCustomNode("image"));
    CPPUNIT_ASSERT_EQUAL(sch.getCustomNodeClass("image"), std::string("ImageData"));

    // Hijack this test to test also !isCustomNode(path) for almost all kind of elements:

    // A slot element: has KARABO_SCHEMA_CLASS_ID attribute, but not KARABO_HASH_CLASS_ID
    CPPUNIT_ASSERT(!sch.isCustomNode("slotTest"));
    // A TableElement
    CPPUNIT_ASSERT(!sch.isCustomNode("testTable"));
    // A ListElement
    CPPUNIT_ASSERT(!sch.isCustomNode("shapeList"));
    // A PathElement
    CPPUNIT_ASSERT(!sch.isCustomNode("filename"));
    // A vector element
    CPPUNIT_ASSERT(!sch.isCustomNode("vecInt"));

    Schema schemaWithChoice("test");
    GraphicsRenderer1::expectedParameters(schemaWithChoice);
    // A ChoiceOfNodes
    CPPUNIT_ASSERT(!schemaWithChoice.isCustomNode("shapes"));
    // An ordinary node
    CPPUNIT_ASSERT(!schemaWithChoice.isCustomNode("triangle"));
    // A StringElement
    CPPUNIT_ASSERT(!schemaWithChoice.isCustomNode("color"));
    // SimpleElement of POD
    CPPUNIT_ASSERT(!schemaWithChoice.isCustomNode("bold"));

    // NDArray element tested in testArrayElements()
    // Miss testing ByteArray...

}


void Schema_Test::testHelpFunction() {

    //===== uncomment to see 'help()' functionality =====:
    /*
    Schema schema("GraphicsRenderer1", Schema::AssemblyRules(READ | WRITE | INIT));
    GraphicsRenderer1::expectedParameters(schema);
  
    schema.help();
    schema.help("shapes");
    schema.help("shapes.circle");
    schema.help("shapes.circle.radius");
    schema.help("shapes.rectangle");
    schema.help("shapes.rectangle.b");
    schema.help("triangle");
     */
}


void Schema_Test::testOverwriteElement() {
    Schema schema = Configurator<TestStruct1>::getSchema("TestStruct2");

    CPPUNIT_ASSERT(schema.getAliasFromKey<int>("exampleKey2") == 20);
    CPPUNIT_ASSERT(schema.getAliasFromKey<int>("exampleKey3") == 30);
}


void Schema_Test::testMerge() {
    Schema schema = Configurator<SchemaNodeElements>::getSchema("SchemaNodeElements", Schema::AssemblyRules(READ | WRITE | INIT));
    CPPUNIT_ASSERT(schema.getDefaultValue<unsigned int>("monitor.count") == 777);

    Schema schema2("SchemaNodeElements", Schema::AssemblyRules(READ | WRITE | INIT));
    SchemaNodeInjected::expectedParameters(schema2);
    CPPUNIT_ASSERT(schema2.getDefaultValue<float>("monitor.stats.d1") == 3.1415f);

    schema.merge(schema2);
    CPPUNIT_ASSERT(schema.getDefaultValue<unsigned int>("monitor.count") == 777);
    CPPUNIT_ASSERT(schema.getDefaultValue<float>("monitor.stats.d1") == 3.1415f);
}


void Schema_Test::testTable() {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);
    CPPUNIT_ASSERT(sch.isLeaf("testTable") == true);
    CPPUNIT_ASSERT(sch.getParameterHash().hasAttribute("testTable", "rowSchema") == true);
    const std::vector<Hash> shouldBeDefault(2, Hash("a", 3, "b", "foo"));
    const std::vector<Hash>& theDefault = sch.getDefaultValue<std::vector<Hash> >("testTable");
    CPPUNIT_ASSERT_EQUAL(shouldBeDefault.size(), theDefault.size());
    CPPUNIT_ASSERT_EQUAL(shouldBeDefault[0].size(), theDefault[1].size());
    CPPUNIT_ASSERT_EQUAL(shouldBeDefault[1].get<int>("a"), theDefault[0].get<int>("a"));
    CPPUNIT_ASSERT_EQUAL(shouldBeDefault[1].get<std::string>("b"), theDefault[1].get<std::string>("b"));

    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<Hash> >("testTableEmptyDefault").empty());
}


void Schema_Test::testList() {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);
    CPPUNIT_ASSERT(sch.has("shapeList") == true);
    CPPUNIT_ASSERT(sch.isListOfNodes("shapeList") == true);
    CPPUNIT_ASSERT(sch.isNode("shapeList") == false);
    const char* classes[] = {"Circle", "Rectangle"};
    const std::vector<std::string> defaults(classes, classes + sizeof (classes) / sizeof (classes[0]));
    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<std::string> >("shapeList") == defaults);

    CPPUNIT_ASSERT(sch.has("shapeList.Circle"));
    CPPUNIT_ASSERT(sch.isNode("shapeList.Circle"));
    CPPUNIT_ASSERT(sch.has("shapeList.EditableCircle"));
    CPPUNIT_ASSERT(sch.isNode("shapeList.EditableCircle"));
    CPPUNIT_ASSERT(sch.has("shapeList.EditableCircle.radius"));
    CPPUNIT_ASSERT(sch.has("shapeList.BizarreForm"));
    CPPUNIT_ASSERT(sch.isNode("shapeList.BizarreForm"));
    CPPUNIT_ASSERT(sch.has("shapeList.BizarreForm.length"));
}


void Schema_Test::testInvalidNodes() {
    Schema schema("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(schema);

    // Placing an element under a leaf is not allowed
    CPPUNIT_ASSERT(schema.has("vecDouble"));
    CPPUNIT_ASSERT(!schema.has("vecDouble.uint16"));
    CPPUNIT_ASSERT_THROW(UINT16_ELEMENT(schema)
                         .key("vecDouble.uint16")
                         .description("This element's key refers to a parent that is not a node and thus triggers an exception")
                         .readOnly()
                         .commit(),
                         karabo::util::LogicException);

    // A node should not be created automatically
    CPPUNIT_ASSERT(!schema.has("nonExistingNode"));
    CPPUNIT_ASSERT_THROW(INT16_ELEMENT(schema)
                         .key("nonExistingNode.int16")
                         .description("This element refers to a non-existing node and thus triggers an exception")
                         .readOnly()
                         .commit(),
                         karabo::util::LogicException);

    // No leaves directly under LIST_ELEMENT:
    CPPUNIT_ASSERT(schema.has("shapeList"));
    CPPUNIT_ASSERT_THROW(DOUBLE_ELEMENT(schema)
                         .key("shapeList.orphanedLength")
                         .description("Only nodes can be placed under a list element, so this triggers an exception.")
                         .assignmentOptional().defaultValue(5.)
                         .commit(),
                         karabo::util::LogicException);

    // Empty strings are forbidden as keys:
    CPPUNIT_ASSERT_THROW(INT32_ELEMENT(schema)
                         .key("")
                         .description("Empty key is forbidden"),
                         karabo::util::ParameterException);

    CPPUNIT_ASSERT_THROW(FLOAT_ELEMENT(schema)
                         .key("shapeList.BizarreForm.")
                         .description("Also an empty key at the end of a longer path is invalid"),
                         karabo::util::ParameterException);

}


void Schema_Test::testOverwriteRestrictions() {
    Schema schema;
    PATH_ELEMENT(schema).key("path")
            .readOnly()
            .commit();

    CPPUNIT_ASSERT_THROW(
                         OVERWRITE_ELEMENT(schema).key("path")
                         .setNewMinInc(100)
                         .commit(), karabo::util::LogicException);


}


void Schema_Test::testOverwriteRestrictionsForOptions() {
    {
        Schema schema;
        INT32_ELEMENT(schema).key("range")
                .displayedName("Range")
                .options("0,1")
                .assignmentOptional().defaultValue(0)
                .reconfigurable()
                .commit();

        const auto& range1 = schema.getOptions<int>("range");
        CPPUNIT_ASSERT_EQUAL(range1.size(), 2ul);
        CPPUNIT_ASSERT_EQUAL(range1[0], 0);
        CPPUNIT_ASSERT_EQUAL(range1[1], 1);

        CPPUNIT_ASSERT_NO_THROW(
            OVERWRITE_ELEMENT(schema).key("range")
                .setNewOptions("0,1,2")
                .commit());

        const auto& range2 = schema.getOptions<int>("range");
        CPPUNIT_ASSERT_EQUAL(range2.size(), 3ul);
        CPPUNIT_ASSERT_EQUAL(range2[0], 0);
        CPPUNIT_ASSERT_EQUAL(range2[1], 1);
        CPPUNIT_ASSERT_EQUAL(range2[2], 2);
    }
    {
        Schema schema;
        NODE_ELEMENT(schema).key("node")
                .commit();
        INT32_ELEMENT(schema).key("node.range")
                .displayedName("Range")
                .options("0,1")
                .assignmentOptional().defaultValue(0)
                .reconfigurable()
                .commit();

        const auto& range1 = schema.getOptions<int>("node.range");
        CPPUNIT_ASSERT_EQUAL(range1.size(), 2ul);
        CPPUNIT_ASSERT_EQUAL(range1[0], 0);
        CPPUNIT_ASSERT_EQUAL(range1[1], 1);

        CPPUNIT_ASSERT_NO_THROW(
            OVERWRITE_ELEMENT(schema).key("node.range")
                .setNewOptions("0,1,2")
                .commit());

        const auto& range2 = schema.getOptions<int>("node.range");
        CPPUNIT_ASSERT_EQUAL(range2.size(), 3ul);
        CPPUNIT_ASSERT_EQUAL(range2[0], 0);
        CPPUNIT_ASSERT_EQUAL(range2[1], 1);
        CPPUNIT_ASSERT_EQUAL(range2[2], 2);
    }
}


void Schema_Test::testRuntimeAttributes() {
    Schema schema;
    FLOAT_ELEMENT(schema).key("floatProperty")
            .minInc(-10.0)
            .maxInc(10.0)
            .assignmentOptional().noDefaultValue()
            .reconfigurable()
            .commit();

    CPPUNIT_ASSERT_EQUAL(-10.0f, schema.getMinInc<float>("floatProperty"));
    CPPUNIT_ASSERT_EQUAL(10.0f, schema.getMaxInc<float>("floatProperty"));

    std::vector<Hash> v;
    v.push_back(Hash("path", "floatProperty", "attribute", KARABO_SCHEMA_MIN_INC, "value", -20.0));
    v.push_back(Hash("path", "floatProperty", "attribute", KARABO_SCHEMA_MAX_INC, "value", 100.0));
    CPPUNIT_ASSERT(schema.applyRuntimeUpdates(v) == true);
    CPPUNIT_ASSERT_EQUAL(-20.0f, schema.getMinInc<float>("floatProperty"));
    CPPUNIT_ASSERT_EQUAL(100.0f, schema.getMaxInc<float>("floatProperty"));

    // set integer to float property, still we can set it
    v.clear();
    v.push_back(Hash("path", "floatProperty", "attribute", KARABO_SCHEMA_MIN_INC, "value", -20.0));
    v.push_back(Hash("path", "floatProperty", "attribute", KARABO_SCHEMA_MAX_INC, "value", 1));
    CPPUNIT_ASSERT(schema.applyRuntimeUpdates(v) == true);

    // we set a maxInc that is way smaller than minInc
    v.clear();
    v.push_back(Hash("path", "floatProperty", "attribute", KARABO_SCHEMA_MIN_INC, "value", 100.0));
    v.push_back(Hash("path", "floatProperty", "attribute", KARABO_SCHEMA_MAX_INC, "value", -20.0));
    // test is supposed to fail
    CPPUNIT_ASSERT(schema.applyRuntimeUpdates(v) == true);
}


void Schema_Test::testStateAndAlarmSets() {
    Schema schema;

    STRING_ELEMENT(schema).key("string")
            .readOnly()
            .commit();

    STATE_ELEMENT(schema).key("state")
            .commit();

    ALARM_ELEMENT(schema).key("alarm")
            .commit();

    Hash h("string", "abc");
    Validator val;
    Hash h_out;
    std::pair<bool, std::string> r = val.validate(schema, h, h_out);
    CPPUNIT_ASSERT(r.first == true); //should validate
    h.set("state", "abc");
    r = val.validate(schema, h, h_out);
    CPPUNIT_ASSERT(r.first == false); //should not validate as we are setting string to state
    Hash h2("alarm", "abc");
    r = val.validate(schema, h2, h_out);
    CPPUNIT_ASSERT(r.first == false); //should not validate as we are setting string to alarm
    Hash::Node& n = h.set("state", "UNKNOWN");
    n.setAttribute(KARABO_INDICATE_STATE_SET, true);
    r = val.validate(schema, h, h_out);
    CPPUNIT_ASSERT(r.first == true); //should validate as we faked updateState
    Hash::Node& n2 = h2.set("alarm", "None");
    n2.setAttribute(KARABO_INDICATE_ALARM_SET, true);
    r = val.validate(schema, h2, h_out);
    CPPUNIT_ASSERT(r.first == true); //should validate as we faked setAlarmCondition
}


void Schema_Test::testSubSchema() {
    Schema schema("test");
    GraphicsRenderer1::expectedParameters(schema);
    {
        Schema sub = schema.subSchema("shapes.rectangle");
        CPPUNIT_ASSERT(sub.has("b"));
        CPPUNIT_ASSERT(sub.has("c"));
    }
    {
        Schema sub = schema.subSchema("shapes.rectangle", "b");
        CPPUNIT_ASSERT(sub.has("b"));
        CPPUNIT_ASSERT(!sub.has("c"));
    }

    // Now testing 'by rules':
    {
        Schema::AssemblyRules rules(READ | WRITE | INIT); // i.e. everything
        const Schema sub = schema.subSchemaByRules(rules);
        // Everything is in:
        std::vector<std::string> finalPaths;
        sub.getParameterHash().getPaths(finalPaths);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (7), finalPaths.size());
    }

    {
        Schema::AssemblyRules rules(READ | WRITE | INIT, "ON"); // i.e. required sate ON or non-defined
        const Schema sub = schema.subSchemaByRules(rules);
        CPPUNIT_ASSERT(!sub.has("color"));

        // But all else since only "color" is reconfigurable for state OFF
        std::vector<std::string> finalPaths;
        sub.getParameterHash().getPaths(finalPaths);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (6), finalPaths.size());
    }

    {
        Schema::AssemblyRules rules(READ | WRITE | INIT, "", Schema::OPERATOR);
        const Schema sub = schema.subSchemaByRules(rules);
        CPPUNIT_ASSERT(!sub.has("antiAlias"));

        // But all else is left since "antiAlias" is the only expert access level (defaults are user or observer)
        std::vector<std::string> finalPaths;
        sub.getParameterHash().getPaths(finalPaths);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (6), finalPaths.size());
    }

    {
        Schema::AssemblyRules rules(READ);
        const Schema sub = schema.subSchemaByRules(rules);
        // Nothing is readOnly...
        CPPUNIT_ASSERT(sub.empty());
    }

    {
        Schema::AssemblyRules rules(INIT | READ);
        const Schema sub = schema.subSchemaByRules(rules);
        CPPUNIT_ASSERT(sub.has("antiAlias"));
        CPPUNIT_ASSERT(sub.has("shapes.rectangle.b"));
        CPPUNIT_ASSERT(sub.has("shapes.rectangle.c"));

        // All else is WRITE (i.e. reconfigurable))
        std::vector<std::string> finalPaths;
        sub.getParameterHash().getPaths(finalPaths);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (3), finalPaths.size());
    }
}


void Schema_Test::testDaqDataType() {
    Schema schema("test");
    GraphicsRenderer1::expectedParameters(schema);
    {
        schema.setDaqDataType("triangle", DaqDataType::PULSE);
        CPPUNIT_ASSERT(schema.getDaqDataType("triangle") == DaqDataType::PULSE);
    }
    {
        schema.setDaqDataType("triangle", DaqDataType::TRAIN);
        CPPUNIT_ASSERT(schema.getDaqDataType("triangle") == DaqDataType::TRAIN);
    }
    {
        schema.setDaqDataType("triangle", DaqDataType::PULSEMASTER);
        CPPUNIT_ASSERT(schema.getDaqDataType("triangle") == DaqDataType::PULSEMASTER);
    }
    {
        schema.setDaqDataType("triangle", DaqDataType::TRAINMASTER);
        CPPUNIT_ASSERT(schema.getDaqDataType("triangle") == DaqDataType::TRAINMASTER);
    }
    {
        CPPUNIT_ASSERT_THROW(schema.setDaqDataType("antiAlias", DaqDataType::TRAINMASTER), karabo::util::ParameterException);
    }
}


void Schema_Test::testDaqPolicy() {
    // legacy behavior: save everything if not specified otherwise
    {
        Schema schema;
        STRING_ELEMENT(schema).key("string1")
                .daqPolicy(DAQPolicy::SAVE)
                .readOnly()
                .commit();

        STRING_ELEMENT(schema).key("string2")
                .daqPolicy(DAQPolicy::OMIT)
                .readOnly()
                .commit();

        STRING_ELEMENT(schema).key("string3")
                .readOnly()
                .commit();

        CPPUNIT_ASSERT(schema.getDAQPolicy("string1") == DAQPolicy::SAVE);
        CPPUNIT_ASSERT(schema.getDAQPolicy("string2") == DAQPolicy::OMIT);
        CPPUNIT_ASSERT(schema.getDAQPolicy("string3") == DAQPolicy::UNSPECIFIED);
    }

    // according to specified default policy
    {
        Schema schema;
        schema.setDefaultDAQPolicy(DAQPolicy::OMIT);
        STRING_ELEMENT(schema).key("string1")
                .daqPolicy(DAQPolicy::SAVE)
                .readOnly()
                .commit();

        STRING_ELEMENT(schema).key("string2")
                .daqPolicy(DAQPolicy::OMIT)
                .readOnly()
                .commit();

        STRING_ELEMENT(schema).key("string3")
                .readOnly()
                .commit();

        CPPUNIT_ASSERT(schema.getDAQPolicy("string1") == DAQPolicy::SAVE);
        CPPUNIT_ASSERT(schema.getDAQPolicy("string2") == DAQPolicy::OMIT);
        CPPUNIT_ASSERT(schema.getDAQPolicy("string3") == DAQPolicy::OMIT);
    }


}


void Schema_Test::testNodeDisplayType() {
    {
        Schema schema;
        NODE_ELEMENT(schema).key("node")
                .setSpecialDisplayType("WidgetNode")
                .commit();

        STRING_ELEMENT(schema).key("node.string")
                .readOnly()
                .commit();

        BOOL_ELEMENT(schema).key("node.bool")
                .readOnly()
                .commit();

        CPPUNIT_ASSERT(schema.getDisplayType("node") == "WidgetNode");
    }
}


void Schema_Test::testGetLeaves() {
    Schema schema("test");
    TestStruct1::expectedParameters(schema);
    OtherSchemaElements::expectedParameters(schema);
    Hash h;
    h.set("shapeList.BizarreForm.length", 20.f);
    h.set("slotTest", Hash());
    h.set("filename", string("here"));
    h.set("testTable", vector<Hash>(3, Hash("a", -1, "b", "this_is a va|id string")));
    vector<string> leaves;
    getLeaves(h, schema, leaves, '.');
    CPPUNIT_ASSERT_EQUAL(std::string("shapeList.BizarreForm.length"), leaves[0]);
    CPPUNIT_ASSERT_EQUAL(std::string("filename"), leaves[1]);
    CPPUNIT_ASSERT_EQUAL(std::string("testTable"), leaves[2]);
    CPPUNIT_ASSERT_EQUAL(3ul, leaves.size());
}
