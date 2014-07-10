/* 
 * File:   Schema_Test.cc
 * Author: irinak
 * 
 * Created on September 28, 2012, 1:14 PM
 */

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
        cout << e << endl;
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
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Circle.shadowEnabled") == Schema::USER);//default level for init-elem is 'user'
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Circle") == Schema::OBSERVER);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Rectangle.a") == Schema::ADMIN);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Rectangle.b") == Schema::USER);//default level for init-elem is 'user'
    
    
}


void Schema_Test::setUp() {
    try {
        m_schema = Schema("MyTest", Schema::AssemblyRules(READ | WRITE | INIT));
        TestStruct1::expectedParameters(m_schema);
    } catch (karabo::util::Exception e) {
        cout << e << endl;
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
    CPPUNIT_ASSERT(m_schema.getAliasAsString("exampleKey3") == "5.500000000000000");
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
    vector<std::string> options = m_schema.getOptions("exampleKey1");
    CPPUNIT_ASSERT(options[0] == "Radio");
    CPPUNIT_ASSERT(options[1] == "Air Condition");
    CPPUNIT_ASSERT(options[2] == "Navigation");

    CPPUNIT_ASSERT(m_schema.getOptions("exampleKey2")[0] == "5");
    CPPUNIT_ASSERT(m_schema.getOptions("exampleKey2")[1] == "25");
    CPPUNIT_ASSERT(m_schema.getOptions("exampleKey2")[2] == "10");

    CPPUNIT_ASSERT(m_schema.getOptions("exampleKey4")[0] == "1.11");
    CPPUNIT_ASSERT(m_schema.getOptions("exampleKey4")[1] == "-2.22");
    CPPUNIT_ASSERT(m_schema.getOptions("exampleKey4")[2] == "5.55");
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

    //readOnly element has default value which is equal to string "0"
    //(does not matter what is the type of the element itself)
    CPPUNIT_ASSERT(m_schema.hasDefaultValue("sampleKey2") == true);
    CPPUNIT_ASSERT(m_schema.getDefaultValue<string>("sampleKey2") == "0");
}


void Schema_Test::testGetAllowedStates() {
    vector<string> allowedStates = m_schema.getAllowedStates("exampleKey3");
    CPPUNIT_ASSERT(allowedStates[0] == "AllOk.Started");
    CPPUNIT_ASSERT(allowedStates[1] == "AllOk.Stopped");
    CPPUNIT_ASSERT(m_schema.getAllowedStates("exampleKey3")[2] == "AllOk.Run.On");
    CPPUNIT_ASSERT(m_schema.getAllowedStates("exampleKey3")[3] == "NewState");
}


void Schema_Test::testGetUnit() {
    int units = m_schema.getUnit("exampleKey2");
    CPPUNIT_ASSERT(units == Unit::METER);

    string unitName = m_schema.getUnitName("exampleKey2");
    CPPUNIT_ASSERT(unitName == "meter");

    string unitSymbol = m_schema.getUnitSymbol("exampleKey2");
    CPPUNIT_ASSERT(unitSymbol == "m");
}


void Schema_Test::testGetMetricPrefix() {
    CPPUNIT_ASSERT(m_schema.getMetricPrefix("exampleKey2") == MetricPrefix::MILLI);
    CPPUNIT_ASSERT(m_schema.getMetricPrefixName("exampleKey2") == "milli");
    CPPUNIT_ASSERT(m_schema.getMetricPrefixSymbol("exampleKey2") == "m");
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
    }

}


void Schema_Test::testSlotElement() {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    CPPUNIT_ASSERT(sch.getDescription("slotTest") == "Test slot element");
    CPPUNIT_ASSERT(sch.getAllowedStates("slotTest")[0] == "Started");
    CPPUNIT_ASSERT(sch.getAllowedStates("slotTest")[2] == "Reset");
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

    CPPUNIT_ASSERT(sch.getValueType("vecDouble") == Types::VECTOR_DOUBLE);
    CPPUNIT_ASSERT(sch.getAlarmLow<vector<double> >("vecDouble") == vector<double>(3, -5.5));
    CPPUNIT_ASSERT(sch.getAlarmHigh<vector<double> >("vecDouble") == vector<double>(3, 7.7));
    CPPUNIT_ASSERT(sch.isAccessReadOnly("vecDouble") == true);
    CPPUNIT_ASSERT(sch.isAssignmentOptional("vecDouble") == true);

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

    vector<string> allowedStates = sch.getAllowedStates("vecBool");
    CPPUNIT_ASSERT(allowedStates[0] == "AllOk.Started");
    CPPUNIT_ASSERT(allowedStates[1] == "AllOk.Stopped");

}


void Schema_Test::testPathElement() {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    CPPUNIT_ASSERT(sch.getValueType("filename") == Types::STRING);
    CPPUNIT_ASSERT(sch.getAliasAsString("filename") == "5");
    CPPUNIT_ASSERT(sch.hasOptions("filename") == true);
    CPPUNIT_ASSERT(sch.getOptions("filename")[0] == "file1");
    CPPUNIT_ASSERT(sch.getOptions("filename")[1] == "file2");
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
    CPPUNIT_ASSERT(sch.has("image.data") == true);
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

    schema = Configurator<TestStruct1>::getSchema("TestStruct2", Schema::AssemblyRules(WRITE));
    CPPUNIT_ASSERT(schema.getAliasFromKey<int>("exampleKey3") == 20);
}

