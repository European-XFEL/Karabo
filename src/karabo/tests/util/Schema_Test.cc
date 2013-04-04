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
    //cout << endl << endl;
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
    string valueType = m_schema.getValueType("exampleKey1");
    CPPUNIT_ASSERT(valueType == "STRING");

    CPPUNIT_ASSERT(m_schema.getValueType("exampleKey2") == "INT32");
    CPPUNIT_ASSERT(m_schema.getValueType("exampleKey3") == "UINT32");
    CPPUNIT_ASSERT(m_schema.getValueType("exampleKey4") == "FLOAT");
    CPPUNIT_ASSERT(m_schema.getValueType("exampleKey5") == "INT64");
}


void Schema_Test::testGetAlias() {
    CPPUNIT_ASSERT(m_schema.hasAlias("exampleKey1") == false);
    CPPUNIT_ASSERT(m_schema.hasAlias("exampleKey2") == true);
    CPPUNIT_ASSERT(m_schema.getAlias<int>("exampleKey2") == 10);
    CPPUNIT_ASSERT(m_schema.getAlias<double>("exampleKey3") == 5.5);
    CPPUNIT_ASSERT(m_schema.getAlias<string > ("exampleKey4") == "exampleAlias4");
    CPPUNIT_ASSERT(m_schema.getAlias<string > ("exampleKey5") == "exampleAlias5");
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
    CPPUNIT_ASSERT(units == Units::METER);

    string unitName = m_schema.getUnitName("exampleKey2");
    CPPUNIT_ASSERT(unitName == "meter");

    string unitSymbol = m_schema.getUnitSymbol("exampleKey2");
    CPPUNIT_ASSERT(unitSymbol == "m");
}


void Schema_Test::testGetMetricPrefix() {
    CPPUNIT_ASSERT(m_schema.getMetricPrefix("exampleKey2") == Units::MILLI);
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


void Schema_Test::testPerKeyFunctionality() {

    std::vector<std::string> keys = m_schema.getParameters();

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
    }

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
