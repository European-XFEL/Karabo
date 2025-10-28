/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   Schema_Test.cc
 * Author: irinak
 *
 * Created on September 28, 2012, 1:14 PM
 */

#include <gtest/gtest.h>

#include <karabo/data/types/Schema.hh>
#include <karabo/log/Logger.hh>
#include <karabo/util/DataLogUtils.hh>

#include "ConfigurationTestClasses.hh"
#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/schema/Validator.hh"

static void testOverwriteElementScalarDefault();
static void testOverwriteElementVectorDefault();
static void testOverwriteElementMinMax();
static void testOverwriteElementMinMaxVector();

class TestSchema : public ::testing::Test {
   protected:
    TestSchema() {}
    ~TestSchema() {}
    static void SetUpTestSuite();
    static void TearDownTestSuite() {}

   public:
    static karabo::data::Schema m_schema;
};

karabo::data::Schema TestSchema::m_schema;

using namespace std;
using namespace karabo::data;
using namespace karabo::util;
using namespace configurationTest;


void TestSchema::SetUpTestSuite() {
    try {
        m_schema = Schema("MyTest", Schema::AssemblyRules(READ | WRITE | INIT));
        TestStruct1::expectedParameters(m_schema);
    } catch (const karabo::data::Exception& e) {
        std::clog << "Error (Schema_Test::setUp): " << e << std::endl;
    }
}


TEST_F(TestSchema, testBuildUp) {
    try {
        {
            Schema schema = Configurator<Shape>::getSchema("Circle");
            EXPECT_TRUE(schema.isAccessInitOnly("shadowEnabled"));
            EXPECT_TRUE(schema.isAccessInitOnly("radius"));
            EXPECT_TRUE(schema.isLeaf("radius"));
        }
        {
            Schema schema("test");
            GraphicsRenderer1::expectedParameters(schema);
            EXPECT_TRUE(schema.isAccessInitOnly("circle.radius"));
            EXPECT_TRUE(schema.isLeaf("circle.radius"));
        }
        GraphicsRenderer::Pointer p = GraphicsRenderer::create(
              "GraphicsRenderer", Hash("Circle.radius", 0.5, "color", "red", "antiAlias", "true"));

    } catch (const karabo::data::Exception& e) {
        KARABO_LOG_FRAMEWORK_DEBUG_C("TestSchema") << e;
        FAIL() << e.detailedMsg();
    }
}


TEST_F(TestSchema, testPaths) {
    Schema schema("test");
    GraphicsRenderer::expectedParameters(schema);

    vector<string> paths = schema.getPaths();
    EXPECT_TRUE(paths[0] == "antiAlias");
    EXPECT_TRUE(paths[1] == "color");
    EXPECT_TRUE(paths[2] == "bold");
    EXPECT_TRUE(paths[3] == "shapes");
    EXPECT_TRUE(paths[4] == "Circle.shadowEnabled");
    EXPECT_TRUE(paths[5] == "Circle.radius");
    EXPECT_TRUE(paths[6] == "EditableCircle.shadowEnabled");
    EXPECT_TRUE(paths[7] == "EditableCircle.radius");
    EXPECT_TRUE(paths[8] == "Rectangle.shadowEnabled");
    EXPECT_TRUE(paths[9] == "Rectangle.a");
    EXPECT_TRUE(paths[10] == "Rectangle.b");
    EXPECT_TRUE(paths[11] == "version");
}


TEST_F(TestSchema, testGetRequiredAccessLevel) {
    Schema schema = GraphicsRenderer::getSchema("GraphicsRenderer");
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("shapes")) == Schema::EXPERT);
    // all sub-elements of Node-element 'shapes' will have EXPERT level:
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("Circle.shadowEnabled")) == Schema::EXPERT);
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("Circle")) == Schema::EXPERT);
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("Rectangle.b")) == Schema::EXPERT);

    // but sub-element 'Rectangle.a' with higher level will keep its EXPERT level
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("Rectangle.a")) == Schema::EXPERT);

    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("antiAlias")) == Schema::EXPERT);
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("color")) == Schema::OPERATOR);

    // check requiredAccesLevel set on leaves-elements in expectedParameters
    EXPECT_TRUE(Schema::AccessLevel(m_schema.getRequiredAccessLevel("exampleKey1")) == Schema::OPERATOR);
    EXPECT_TRUE(Schema::AccessLevel(m_schema.getRequiredAccessLevel("exampleKey2")) == Schema::OPERATOR);
    EXPECT_TRUE(Schema::AccessLevel(m_schema.getRequiredAccessLevel("exampleKey3")) == Schema::EXPERT);
    EXPECT_TRUE(Schema::AccessLevel(m_schema.getRequiredAccessLevel("exampleKey4")) == Schema::EXPERT);

    // default for readOnly element - OBSERVER
    EXPECT_TRUE(Schema::AccessLevel(m_schema.getRequiredAccessLevel("exampleKey5")) == Schema::OBSERVER);

    // default for reconfigurable element - OPERATOR
    EXPECT_TRUE(Schema::AccessLevel(m_schema.getRequiredAccessLevel("sampleKey")) == Schema::OPERATOR);

    Schema ose("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(ose);

    // check default requiredAccessLevel by elements : slot, vector, image
    EXPECT_TRUE(Schema::AccessLevel(ose.getRequiredAccessLevel("slotTest")) == Schema::OPERATOR); // SLOT
    EXPECT_TRUE(Schema::AccessLevel(ose.getRequiredAccessLevel("filename")) == Schema::OPERATOR);
    EXPECT_TRUE(Schema::AccessLevel(ose.getRequiredAccessLevel("testfile")) == Schema::OBSERVER); // readOnly STRING
    EXPECT_TRUE(Schema::AccessLevel(ose.getRequiredAccessLevel("vecIntReconfig")) == Schema::OPERATOR);
    EXPECT_TRUE(Schema::AccessLevel(ose.getRequiredAccessLevel("vecInt")) == Schema::OBSERVER);
    EXPECT_TRUE(Schema::AccessLevel(ose.getRequiredAccessLevel("vecBool")) == Schema::OPERATOR); // init VECTOR_BOOL
    EXPECT_TRUE(Schema::AccessLevel(ose.getRequiredAccessLevel("image")) == Schema::OBSERVER);   // IMAGE
}


TEST_F(TestSchema, testSetRequiredAccessLevel) {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    EXPECT_TRUE(Schema::AccessLevel(sch.getRequiredAccessLevel("image")) == Schema::OBSERVER);
    sch.setRequiredAccessLevel("image", Schema::EXPERT);
    EXPECT_TRUE(Schema::AccessLevel(sch.getRequiredAccessLevel("image")) == Schema::EXPERT);

    Schema schema = GraphicsRenderer::getSchema("GraphicsRenderer");
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("shapes")) == Schema::EXPERT);
    // all sub-elements of Node-element 'shapes' will have EXPERT level:
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("Circle.shadowEnabled")) == Schema::EXPERT);
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("Circle")) == Schema::EXPERT);
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("Rectangle.a")) == Schema::EXPERT);
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("Rectangle.b")) == Schema::EXPERT);


    // set top-Node to lower level 'Observer' and check that sub-elements keep previous higher level
    schema.setRequiredAccessLevel("shapes", Schema::OBSERVER);
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("shapes")) == Schema::OBSERVER);
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("Circle.shadowEnabled")) == Schema::EXPERT);
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("Circle")) == Schema::EXPERT);
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("Rectangle.a")) == Schema::EXPERT);
    EXPECT_TRUE(Schema::AccessLevel(schema.getRequiredAccessLevel("Rectangle.b")) == Schema::EXPERT);
}


TEST_F(TestSchema, testGetRootName) {
    EXPECT_TRUE(m_schema.getRootName() == "MyTest");
}


TEST_F(TestSchema, testGetTags) {
    EXPECT_TRUE(m_schema.getTags("exampleKey1")[0] == "hardware");
    EXPECT_TRUE(m_schema.getTags("exampleKey1")[1] == "poll");
    EXPECT_TRUE(m_schema.getTags("exampleKey2")[0] == "hardware");
    EXPECT_TRUE(m_schema.getTags("exampleKey2")[1] == "poll");
    EXPECT_TRUE(m_schema.getTags("exampleKey3")[0] == "hardware");
    EXPECT_TRUE(m_schema.getTags("exampleKey3")[1] == "set");
    EXPECT_TRUE(m_schema.getTags("exampleKey4")[0] == "software");
    EXPECT_TRUE(m_schema.getTags("exampleKey5")[0] == "h/w");
    EXPECT_TRUE(m_schema.getTags("exampleKey5")[1] == "d.m.y");
    EXPECT_TRUE(m_schema.getTags("warray")[0] == "software");
    EXPECT_TRUE(m_schema.getTags("rarray")[0] == "software");
}


TEST_F(TestSchema, testGetNodeType) {
    Schema::NodeType nodeType = m_schema.getNodeType("exampleKey1");
    EXPECT_TRUE(nodeType == Schema::LEAF);

    EXPECT_TRUE(m_schema.getNodeType("exampleKey5") == Schema::LEAF);
    EXPECT_TRUE(m_schema.getNodeType("warray") == Schema::LEAF);
    EXPECT_TRUE(m_schema.getNodeType("rarray") == Schema::LEAF);
}


TEST_F(TestSchema, testGetValueType) {
    Types::ReferenceType valueType = m_schema.getValueType("exampleKey1");
    EXPECT_TRUE(valueType == Types::STRING);

    EXPECT_TRUE(m_schema.getValueType("exampleKey2") == Types::INT32);
    EXPECT_TRUE(m_schema.getValueType("exampleKey3") == Types::UINT32);
    EXPECT_TRUE(m_schema.getValueType("exampleKey4") == Types::FLOAT);
    EXPECT_TRUE(m_schema.getValueType("exampleKey5") == Types::INT64);
    EXPECT_TRUE(m_schema.getValueType("warray") == Types::BYTE_ARRAY);
    EXPECT_TRUE(m_schema.getValueType("rarray") == Types::BYTE_ARRAY);
}


TEST_F(TestSchema, testKeyHasAlias) {
    EXPECT_TRUE(m_schema.keyHasAlias("exampleKey1") == false);
    EXPECT_TRUE(m_schema.keyHasAlias("exampleKey2") == true);
    EXPECT_TRUE(m_schema.keyHasAlias("exampleKey3") == true);
    EXPECT_TRUE(m_schema.keyHasAlias("exampleKey4") == true);
    EXPECT_TRUE(m_schema.keyHasAlias("exampleKey5") == true);
    EXPECT_TRUE(m_schema.keyHasAlias("warray") == true);
}


TEST_F(TestSchema, testAliasHasKey) {
    EXPECT_TRUE(m_schema.aliasHasKey(10) == true);
    EXPECT_TRUE(m_schema.aliasHasKey(5.5) == true);
    EXPECT_TRUE(m_schema.aliasHasKey("exampleAlias4") == true);
    EXPECT_TRUE(m_schema.aliasHasKey("aliasWriteArray") == true);

    vector<int> vecIntAlias;
    vecIntAlias.push_back(10);
    vecIntAlias.push_back(20);
    vecIntAlias.push_back(30);
    EXPECT_TRUE(m_schema.aliasHasKey(vecIntAlias) == true);

    EXPECT_TRUE(m_schema.aliasHasKey(7) == false);
}


TEST_F(TestSchema, testGetAliasFromKey) {
    EXPECT_TRUE(m_schema.getAliasFromKey<int>("exampleKey2") == 10);
    EXPECT_TRUE(m_schema.getAliasFromKey<double>("exampleKey3") == 5.5);
    EXPECT_TRUE(m_schema.getAliasFromKey<string>("exampleKey4") == "exampleAlias4");
    EXPECT_TRUE(m_schema.getAliasFromKey<string>("rarray") == "aliasReadArray");

    vector<int> aliasVec = m_schema.getAliasFromKey<vector<int>>("exampleKey5");
    EXPECT_TRUE(aliasVec[0] == 10);
    EXPECT_TRUE(aliasVec[1] == 20);
    EXPECT_TRUE(aliasVec[2] == 30);
}


TEST_F(TestSchema, testGetKeyFromAlias) {
    EXPECT_TRUE(m_schema.getKeyFromAlias(10) == "exampleKey2");
    EXPECT_TRUE(m_schema.getKeyFromAlias(5.5) == "exampleKey3");
    EXPECT_TRUE(m_schema.getKeyFromAlias("exampleAlias4") == "exampleKey4");
    EXPECT_TRUE(m_schema.getKeyFromAlias("aliasWriteArray") == "warray");

    vector<int> vecIntAlias;
    vecIntAlias.push_back(10);
    vecIntAlias.push_back(20);
    vecIntAlias.push_back(30);
    EXPECT_TRUE(m_schema.getKeyFromAlias(vecIntAlias) == "exampleKey5");
}


TEST_F(TestSchema, testGetAliasAsString) {
    EXPECT_TRUE(m_schema.getAliasAsString("exampleKey2") == "10");
    EXPECT_TRUE(m_schema.getAliasAsString("exampleKey3") == "5.5");
    EXPECT_TRUE(m_schema.getAliasAsString("exampleKey4") == "exampleAlias4");
    EXPECT_TRUE(m_schema.getAliasAsString("rarray") == "aliasReadArray");

    string aliasStr = m_schema.getAliasAsString("exampleKey5");
    EXPECT_TRUE(aliasStr == "10,20,30");
}


TEST_F(TestSchema, testGetAccessMode) {
    int accessModeKey1 = m_schema.getAccessMode("exampleKey1");
    EXPECT_TRUE(accessModeKey1 == WRITE);

    EXPECT_TRUE(m_schema.getAccessMode("exampleKey2") == INIT);
    EXPECT_TRUE(m_schema.getAccessMode("exampleKey3") == WRITE);
    EXPECT_TRUE(m_schema.getAccessMode("exampleKey4") == INIT);
    EXPECT_TRUE(m_schema.getAccessMode("exampleKey5") == READ);
    // EXPECT_TRUE(m_schema.getAccessMode("warray") == INIT);
    EXPECT_TRUE(m_schema.getAccessMode("rarray") == READ);
}


TEST_F(TestSchema, testGetAssignment) {
    Schema::AssignmentType assignment = m_schema.getAssignment("exampleKey1");
    EXPECT_TRUE(assignment == Schema::OPTIONAL_PARAM);

    EXPECT_TRUE(m_schema.getAssignment("exampleKey2") == Schema::OPTIONAL_PARAM);
    EXPECT_TRUE(m_schema.getAssignment("exampleKey3") == Schema::MANDATORY_PARAM);
    EXPECT_TRUE(m_schema.getAssignment("exampleKey4") == Schema::INTERNAL_PARAM);
    EXPECT_TRUE(m_schema.getAssignment("exampleKey5") == Schema::OPTIONAL_PARAM);
    EXPECT_TRUE(m_schema.getAssignment("warray") == Schema::OPTIONAL_PARAM);
}


TEST_F(TestSchema, testGetOptions) {
    vector<std::string> options = m_schema.getOptions<std::string>("exampleKey1");
    EXPECT_TRUE(options[0] == "Radio");
    EXPECT_TRUE(options[1] == "Air Condition");
    EXPECT_TRUE(options[2] == "Navigation");

    EXPECT_TRUE(m_schema.getOptions<int>("exampleKey2")[0] == 5);
    EXPECT_TRUE(m_schema.getOptions<int>("exampleKey2")[1] == 25);
    EXPECT_TRUE(m_schema.getOptions<int>("exampleKey2")[2] == 10);

    EXPECT_TRUE(m_schema.getOptions<float>("exampleKey4")[0] == 1.11f);
    EXPECT_TRUE(m_schema.getOptions<float>("exampleKey4")[1] == -2.22f);
    EXPECT_TRUE(m_schema.getOptions<float>("exampleKey4")[2] == 5.55f);
    EXPECT_TRUE(m_schema.getOptions<unsigned char>("exampleKey8")[2] == 3);

    // Hijack test to check exception for invalid (empty) options.
    Schema schema;
    EXPECT_THROW(DOUBLE_ELEMENT(schema).key("some").options(std::vector<double>()), karabo::data::ParameterException);

    EXPECT_THROW(INT32_ELEMENT(schema).key("someOther").options(""), karabo::data::ParameterException);
}


TEST_F(TestSchema, testGetDefaultValue) {
    string defaultValueKey1 = m_schema.getDefaultValue<string>("exampleKey1");
    EXPECT_TRUE(defaultValueKey1 == "Navigation");

    int defaultValueKey2 = m_schema.getDefaultValue<int>("exampleKey2");
    EXPECT_TRUE(defaultValueKey2 == 10);
    string defaultValueAsString2 = m_schema.getDefaultValueAs<string>("exampleKey2");
    EXPECT_TRUE(defaultValueAsString2 == "10");

    long long defaultValue = m_schema.getDefaultValue<long long>("exampleKey5");
    EXPECT_TRUE(defaultValue == 1442244);
    string defaultValueAsString5 = m_schema.getDefaultValueAs<string>("exampleKey5");
    EXPECT_TRUE(defaultValueAsString5 == "1442244");

    EXPECT_TRUE(m_schema.getDefaultValue<int>("sampleKey") ==
                10); // Was set from string, but maintains correct data typing

    EXPECT_TRUE(m_schema.getDefaultValueAs<string>("sampleKey") == "10");
    EXPECT_TRUE(m_schema.getDefaultValueAs<int>("sampleKey") == 10);

    EXPECT_TRUE(m_schema.hasDefaultValue("sampleKey2") == true);
    EXPECT_TRUE(m_schema.getDefaultValue<int>("sampleKey2") == 0);

    EXPECT_TRUE(m_schema.hasDefaultValue("warray") == true);
}


TEST_F(TestSchema, testInvalidDefaultsThrow) {
    karabo::data::Schema schInvalidDefault;
    EXPECT_THROW(INT32_ELEMENT(schInvalidDefault)
                       .key("int")
                       .assignmentOptional()
                       .defaultValue(1)
                       .minExc(1)
                       .reconfigurable()
                       .commit(),
                 karabo::data::ParameterException)
          << "Expected exception adding int element with default smaller than minimum.";
    EXPECT_THROW(FLOAT_ELEMENT(schInvalidDefault)
                       .key("float")
                       .assignmentOptional()
                       .defaultValue(0.9999f)
                       .minInc(1.0f)
                       .reconfigurable()
                       .commit(),
                 karabo::data::ParameterException)
          << "Expected exception adding float element with default smaller than minimum.";
    EXPECT_THROW(DOUBLE_ELEMENT(schInvalidDefault)
                       .key("double")
                       .assignmentOptional()
                       .defaultValue(1.0)
                       .maxExc(1.0)
                       .reconfigurable()
                       .commit(),
                 karabo::data::ParameterException)
          << "Expected exception adding double element with default greater than maximum.";
    EXPECT_THROW(UINT32_ELEMENT(schInvalidDefault)
                       .key("uint32")
                       .assignmentOptional()
                       .defaultValue(121u)
                       .maxInc(120u)
                       .reconfigurable()
                       .commit(),
                 karabo::data::ParameterException)
          << "Expected exception adding uint32 element with default greater than maximum.";
    EXPECT_THROW(STRING_ELEMENT(schInvalidDefault)
                       .key("string")
                       .assignmentOptional()
                       .defaultValue("NotAnOption")
                       .options("OneOption AnotherOption")
                       .reconfigurable()
                       .commit(),
                 karabo::data::ParameterException)
          << "Expected exception adding string element with default not among options.";
    EXPECT_THROW(VECTOR_INT32_ELEMENT(schInvalidDefault)
                       .key("vectorInt")
                       .assignmentOptional()
                       .defaultValue(std::vector<int>{1, 2})
                       .minSize(4)
                       .reconfigurable()
                       .commit(),
                 karabo::data::ParameterException)
          << "Expected exception adding int vector with size smaller than minSize.";
    EXPECT_THROW(VECTOR_BOOL_ELEMENT(schInvalidDefault)
                       .key("vectorBool")
                       .assignmentOptional()
                       .defaultValue(std::vector<bool>{true, false, true})
                       .maxSize(2)
                       .reconfigurable()
                       .commit(),
                 karabo::data::ParameterException)
          << "Expected exception adding bool vector with size greater than maxSize.";
}


TEST_F(TestSchema, testGetAllowedStates) {
    vector<State> allowedStates = m_schema.getAllowedStates("exampleKey3");
    EXPECT_TRUE(allowedStates[0] == State::COOLED);
    EXPECT_TRUE(allowedStates[1] == State::ACTIVE);
    EXPECT_TRUE(m_schema.getAllowedStates("exampleKey3")[2] == State::DISABLED);
    EXPECT_TRUE(m_schema.getAllowedStates("exampleKey3")[3] == State::KNOWN);
}


TEST_F(TestSchema, testGetUnit) {
    EXPECT_TRUE(m_schema.getUnit("exampleKey2") == Unit::METER);
    EXPECT_TRUE(m_schema.getUnitName("exampleKey2") == "meter");
    EXPECT_TRUE(m_schema.getUnitSymbol("exampleKey2") == "m");

    EXPECT_TRUE(m_schema.getUnit("exampleKey10.data") == Unit::DEGREE_CELSIUS);
    EXPECT_TRUE(m_schema.getUnitName("exampleKey10.data") == "degree_celsius");
    EXPECT_TRUE(m_schema.getUnitSymbol("exampleKey10.data") == "degC");
}


TEST_F(TestSchema, testGetMetricPrefix) {
    EXPECT_TRUE(m_schema.getMetricPrefix("exampleKey2") == MetricPrefix::MILLI);
    EXPECT_TRUE(m_schema.getMetricPrefixName("exampleKey2") == "milli");
    EXPECT_TRUE(m_schema.getMetricPrefixSymbol("exampleKey2") == "m");

    EXPECT_TRUE(m_schema.getMetricPrefix("exampleKey10.data") == MetricPrefix::CENTI);
    EXPECT_TRUE(m_schema.getMetricPrefixName("exampleKey10.data") == "centi");
    EXPECT_TRUE(m_schema.getMetricPrefixSymbol("exampleKey10.data") == "c");
}


TEST_F(TestSchema, testGetMinIncMaxInc) {
    int minInc = m_schema.getMinInc<int>("exampleKey2");
    string minIncStr = m_schema.getMinIncAs<string>("exampleKey2");
    EXPECT_TRUE(minInc == 5);
    EXPECT_TRUE(minIncStr == "5");


    int maxInc = m_schema.getMaxInc<int>("exampleKey2");
    string maxIncStr = m_schema.getMaxIncAs<string>("exampleKey2");
    EXPECT_TRUE(maxInc == 25);
    EXPECT_TRUE(maxIncStr == "25");
}


TEST_F(TestSchema, testGetMinExcMaxExc) {
    unsigned int minExc = m_schema.getMinExc<unsigned int>("exampleKey3");
    string minExcStr = m_schema.getMinExcAs<string>("exampleKey3");
    EXPECT_TRUE(minExc == 10);
    EXPECT_TRUE(minExcStr == "10");

    unsigned int maxExc = m_schema.getMaxExc<unsigned int>("exampleKey3");
    string maxExcStr = m_schema.getMaxExcAs<string>("exampleKey3");
    EXPECT_TRUE(maxExc == 20);
    EXPECT_TRUE(maxExcStr == "20");
}


TEST_F(TestSchema, testArchivePolicy) {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    EXPECT_TRUE(sch.hasArchivePolicy("testfile") == false);
    EXPECT_TRUE(sch.hasArchivePolicy("vecInt") == false);
    EXPECT_TRUE(sch.hasArchivePolicy("vecDouble") == true);
    EXPECT_TRUE(sch.getArchivePolicy("vecDouble") == Schema::NO_ARCHIVING);
}


TEST_F(TestSchema, testPerKeyFunctionality) {
    std::vector<std::string> keys = m_schema.getKeys();

    for (size_t i = 0; i < keys.size(); ++i) {
        if (keys[i] == "exampleKey1") {
            bool hasAssignment = m_schema.hasAssignment(keys[i]);
            EXPECT_TRUE(hasAssignment == true);
            EXPECT_TRUE(m_schema.isAssignmentOptional(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasDefaultValue(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasAccessMode(keys[i]) == true);
            EXPECT_TRUE(m_schema.isAccessReconfigurable(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasOptions(keys[i]) == true);
            EXPECT_TRUE(m_schema.hasTags(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasUnit(keys[i]) == false);
            EXPECT_TRUE(m_schema.hasMetricPrefix(keys[i]) == false);
        }

        if (keys[i] == "exampleKey2") {
            EXPECT_TRUE(m_schema.hasDefaultValue(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasAccessMode(keys[i]) == true);
            EXPECT_TRUE(m_schema.isAccessInitOnly(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasOptions(keys[i]) == true);
            EXPECT_TRUE(m_schema.hasTags(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasAllowedStates(keys[i]) == false);

            EXPECT_TRUE(m_schema.hasUnit(keys[i]) == true);
            EXPECT_TRUE(m_schema.hasMetricPrefix(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasMinInc(keys[i]) == true);
            EXPECT_TRUE(m_schema.hasMaxInc(keys[i]) == true);
        }

        if (keys[i] == "exampleKey3") {
            EXPECT_TRUE(m_schema.hasAssignment(keys[i]) == true);
            EXPECT_TRUE(m_schema.isAssignmentMandatory(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasDefaultValue(keys[i]) == false);

            EXPECT_TRUE(m_schema.hasOptions(keys[i]) == false);

            EXPECT_TRUE(m_schema.hasAllowedStates(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasMinExc(keys[i]) == true);
            EXPECT_TRUE(m_schema.hasMaxExc(keys[i]) == true);
        }

        if (keys[i] == "exampleKey4") {
            EXPECT_TRUE(m_schema.hasDefaultValue(keys[i]) == false);

            EXPECT_TRUE(m_schema.isAssignmentInternal(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasAccessMode(keys[i]) == true);
            EXPECT_TRUE(m_schema.isAccessInitOnly(keys[i]) == true);
        }

        if (keys[i] == "exampleKey5") {
            EXPECT_TRUE(m_schema.hasDefaultValue(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasAssignment(keys[i]) == true);
            EXPECT_TRUE(m_schema.isAssignmentOptional(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasAccessMode(keys[i]) == true);
            EXPECT_TRUE(m_schema.isAccessReadOnly(keys[i]) == true);
            EXPECT_STREQ("Int64DisplayType", m_schema.getDisplayType(keys[i]).c_str());
        }

        if (keys[i] == "exampleKey6") {
            EXPECT_TRUE(m_schema.hasDefaultValue(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasAssignment(keys[i]) == true);
            EXPECT_TRUE(m_schema.isAssignmentOptional(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasAccessMode(keys[i]) == true);
            EXPECT_TRUE(m_schema.isAccessReadOnly(keys[i]) == false);
            EXPECT_TRUE(m_schema.getDisplayType(keys[i]) == "hex");
        }

        if (keys[i] == "exampleKey7") {
            EXPECT_TRUE(m_schema.hasDefaultValue(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasAssignment(keys[i]) == true);
            EXPECT_TRUE(m_schema.isAssignmentOptional(keys[i]) == true);

            EXPECT_TRUE(m_schema.hasAccessMode(keys[i]) == true);
            EXPECT_TRUE(m_schema.isAccessReadOnly(keys[i]) == true);
            EXPECT_TRUE(m_schema.getDisplayType(keys[i]) == "bin|0:isError,1:isMoving,2:isBusy,15:isOn");
        }

        if (keys[i] == "exampleKey9") {
            EXPECT_TRUE(m_schema.hasDefaultValue(keys[i]) == true);
            EXPECT_TRUE(m_schema.hasAccessMode(keys[i]) == true);
            EXPECT_TRUE(m_schema.isAccessReadOnly(keys[i]) == true);
            EXPECT_STREQ("TestDisplayType", m_schema.getDisplayType(keys[i]).c_str());
        }
    }
}


TEST_F(TestSchema, testSlotElement) {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    EXPECT_TRUE(sch.getDescription("slotTest") == "Test slot element");
    EXPECT_TRUE(sch.getAllowedStates("slotTest")[0] == State::STARTED);
    EXPECT_TRUE(sch.getAllowedStates("slotTest")[2] == State::ERROR);
    EXPECT_TRUE(sch.isCommand("slotTest") == true);
    EXPECT_TRUE(sch.isProperty("slotTest") == false);
    EXPECT_TRUE(sch.hasClassId("slotTest") == true);
    EXPECT_TRUE(sch.getClassId("slotTest") == "Slot");

    // An underscores should not be allowed in slot keys since they interfere with slots under a node.
    // But currently we cannot exclude them for backward compatibility of some Beckhoff devices.
    //    EXPECT_THROW(SLOT_ELEMENT(sch).key("slot_withunderscore"),
    //                         karabo::data::ParameterException);
    // But this one is forbidden due to interference with GUI client:
    EXPECT_THROW(SLOT_ELEMENT(sch).key("clear_namespace"), karabo::data::ParameterException);

    // With the 'strict' flag set to false one can widen a bit the set of allowed characters, e.g. '-'
    EXPECT_THROW(SLOT_ELEMENT(sch).key("slot-some", true), karabo::data::ParameterException);
    EXPECT_NO_THROW(SLOT_ELEMENT(sch).key("slot-some", false).commit());
}


TEST_F(TestSchema, testVectorElements) {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    vector<int> vecDef;
    vecDef.push_back(10);
    vecDef.push_back(20);
    vecDef.push_back(30);
    EXPECT_TRUE(sch.getDefaultValue<vector<int>>("vecInt") == vecDef);
    EXPECT_TRUE(sch.getValueType("vecInt") == Types::VECTOR_INT32);
    EXPECT_TRUE(sch.isAccessReadOnly("vecInt") == true);
    EXPECT_TRUE(sch.isAssignmentOptional("vecInt") == true);
    EXPECT_TRUE(sch.hasDefaultValue("vecInt") == true);
    EXPECT_STREQ("Curve", sch.getDisplayType("vecInt").c_str());

    EXPECT_TRUE(sch.getValueType("vecDouble") == Types::VECTOR_DOUBLE);
    EXPECT_TRUE(sch.isAccessReadOnly("vecDouble") == true);
    EXPECT_TRUE(sch.isAssignmentOptional("vecDouble") == true);
    EXPECT_STREQ("Curve", sch.getDisplayType("vecDouble").c_str());

    // readOnly element has default value (even if initialValue not specified) and default value is vector<double>() --
    // empty vector
    EXPECT_TRUE(sch.hasDefaultValue("vecDouble") == true);
    EXPECT_TRUE(sch.getDefaultValue<vector<double>>("vecDouble") == vector<double>());

    EXPECT_TRUE(sch.isAccessReconfigurable("vecIntReconfig") == true);
    EXPECT_TRUE(sch.isAssignmentOptional("vecIntReconfig") == true);
    EXPECT_TRUE(sch.hasDefaultValue("vecIntReconfig") == true);
    EXPECT_TRUE(sch.getDefaultValue<vector<int>>("vecIntReconfig") == vecDef);
    EXPECT_STREQ("Curve", sch.getDisplayType("vecIntReconfig").c_str());

    EXPECT_TRUE(sch.hasDefaultValue("vecIntReconfigStr") == true);

    vector<int> compare;
    compare.push_back(11);
    compare.push_back(22);
    compare.push_back(33);

    vector<int> defVecNew = sch.getDefaultValue<vector<int>>("vecIntReconfigStr");
    EXPECT_TRUE(defVecNew == compare);

    vector<double> comp;
    comp.push_back(1.1);
    comp.push_back(2.2);
    comp.push_back(3.3);

    vector<double> defDVecNew = sch.getDefaultValue<vector<double>>("vecDoubleReconfigStr");
    EXPECT_TRUE(defDVecNew == comp);

    EXPECT_TRUE(sch.isAccessInitOnly("vecBool") == true);
    EXPECT_TRUE(sch.isAssignmentOptional("vecBool") == false);
    EXPECT_TRUE(sch.isAssignmentMandatory("vecBool") == true);
    EXPECT_TRUE(sch.hasMinSize("vecBool") == true);
    EXPECT_TRUE(sch.hasMaxSize("vecBool") == true);
    EXPECT_TRUE(sch.getMinSize("vecBool") == 2);
    EXPECT_TRUE(sch.getMaxSize("vecBool") == 7);

    EXPECT_TRUE(sch.getTags("vecBool")[0] == "h/w");
    EXPECT_TRUE(sch.getTags("vecBool")[1] == "d.m.y");

    vector<State> allowedStates = sch.getAllowedStates("vecBool");
    EXPECT_TRUE(allowedStates[0] == State::STARTED);
    EXPECT_TRUE(allowedStates[1] == State::STOPPED);

    EXPECT_STREQ("Curve", sch.getDisplayType("vecBool").c_str());
}


TEST_F(TestSchema, testArrayElements) {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    std::vector<unsigned long long> shapeVec;
    shapeVec.push_back(3);
    shapeVec.push_back(2);
    EXPECT_TRUE(sch.getDefaultValue<std::vector<unsigned long long>>("arrBool.shape") == shapeVec);
    EXPECT_TRUE(sch.getDefaultValue<std::vector<unsigned long long>>("arrInt8.shape") == shapeVec);
    EXPECT_TRUE(sch.getDefaultValue<std::vector<unsigned long long>>("arrUInt16.shape") == shapeVec);
    EXPECT_TRUE(sch.getDefaultValue<std::vector<unsigned long long>>("arrFloat.shape") == shapeVec);
    shapeVec.push_back(-1);
    EXPECT_TRUE(sch.getDefaultValue<std::vector<unsigned long long>>("arrDouble.shape") == shapeVec);

    std::vector<unsigned long long> undefShapeVec;
    undefShapeVec.push_back(0);
    undefShapeVec.push_back(3);
    undefShapeVec.push_back(0);
    EXPECT_TRUE(sch.getDefaultValue<std::vector<unsigned long long>>("arrUndefined.shape") == undefShapeVec);

    EXPECT_EQ(static_cast<int>(karabo::data::Types::BOOL), sch.getDefaultValue<int>("arrBool.type"));
    EXPECT_EQ(static_cast<int>(karabo::data::Types::INT8), sch.getDefaultValue<int>("arrInt8.type"));
    EXPECT_EQ(static_cast<int>(karabo::data::Types::UINT16), sch.getDefaultValue<int>("arrUInt16.type"));
    EXPECT_EQ(static_cast<int>(karabo::data::Types::FLOAT), sch.getDefaultValue<int>("arrFloat.type"));
    EXPECT_EQ(static_cast<int>(karabo::data::Types::UNKNOWN), sch.getDefaultValue<int>("arrUndefined.type"));

    EXPECT_TRUE(sch.isAccessReadOnly("arrBool") == true);
    EXPECT_TRUE(sch.isAccessReadOnly("arrInt8") == true);
    EXPECT_TRUE(sch.isAccessReadOnly("arrUInt16") == true);
    EXPECT_TRUE(sch.isAccessReadOnly("arrFloat") == true);
    EXPECT_TRUE(sch.isAccessReadOnly("arrDouble") == true);

    // Just choose one array to test NDArray discovery:
    EXPECT_TRUE(sch.isNode("arrUInt16"));
    EXPECT_TRUE(sch.isCustomNode("arrUInt16"));
    EXPECT_STREQ(sch.getCustomNodeClass("arrUInt16").c_str(), "NDArray");
    // Before 2.7.2, NDARRAY_ELEMENTs (as custom nodes) erroneously created this attribute in the Schema:
    EXPECT_TRUE(!sch.getParameterHash().hasAttribute("arrUInt16", "__classId"));
}


TEST_F(TestSchema, testPathElement) {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    EXPECT_TRUE(sch.getValueType("filename") == Types::STRING);
    EXPECT_TRUE(sch.getAliasAsString("filename") == "5");
    EXPECT_TRUE(sch.hasOptions("filename") == true);
    EXPECT_TRUE(sch.getOptions<string>("filename")[0] == "file1");
    EXPECT_TRUE(sch.getOptions<string>("filename")[1] == "file2");
    EXPECT_TRUE(sch.hasDefaultValue("filename") == true);
    EXPECT_TRUE(sch.getDefaultValue<string>("filename") == "karabo.log");
    EXPECT_TRUE(sch.isAccessReconfigurable("filename") == true);
    EXPECT_TRUE(sch.getAssignment("filename") == Schema::AssignmentType::OPTIONAL_PARAM);


    EXPECT_TRUE(sch.isAccessReadOnly("testfile") == true);
    EXPECT_TRUE(sch.hasDefaultValue("testfile") == true);
    EXPECT_TRUE(sch.getDefaultValue<string>("testfile") == "initFile");

    EXPECT_TRUE(sch.isProperty("testfile") == true);
}


TEST_F(TestSchema, testImageElement) {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);
    EXPECT_TRUE(sch.isNode("image") == true);
    EXPECT_TRUE(sch.has("image.encoding") == true);

    EXPECT_TRUE(sch.isCustomNode("image"));
    EXPECT_STREQ(sch.getCustomNodeClass("image").c_str(), "ImageData");

    // Hijack this test to test also !isCustomNode(path) for almost all kind of elements:

    // A slot element
    EXPECT_TRUE(!sch.isCustomNode("slotTest"));
    // A TableElement
    EXPECT_TRUE(!sch.isCustomNode("testTable"));
    // A String Element
    EXPECT_TRUE(!sch.isCustomNode("filename"));
    // A vector element
    EXPECT_TRUE(!sch.isCustomNode("vecInt"));

    Schema schemaWithChoice("test");
    GraphicsRenderer1::expectedParameters(schemaWithChoice);
    // A ChoiceOfNodes
    EXPECT_TRUE(!schemaWithChoice.isCustomNode("shapes"));
    // ... and its choices
    EXPECT_TRUE(schemaWithChoice.isCustomNode("circle"));
    EXPECT_TRUE(!schemaWithChoice.isCustomNode("rectangle"));
    // An ordinary node
    EXPECT_TRUE(!schemaWithChoice.isCustomNode("triangle"));
    // A StringElement
    EXPECT_TRUE(!schemaWithChoice.isCustomNode("color"));
    // SimpleElement of POD
    EXPECT_TRUE(!schemaWithChoice.isCustomNode("bold"));

    // NDArray element tested in testArrayElements()
    // Miss testing ByteArray...
}


TEST_F(TestSchema, testHelpFunction) {
    //===== uncomment to see 'help()' functionality =====:
    /*
    Schema schema("GraphicsRenderer1", Schema::AssemblyRules(READ | WRITE | INIT));
    GraphicsRenderer1::expectedParameters(schema);

    schema.help();
    schema.help("shapes");
    schema.help("circle");
    schema.help("circle.radius");
    schema.help("rectangle");
    schema.help("rectangle.b");
    schema.help("triangle");
     */
}


TEST_F(TestSchema, testOverwriteElement) {
    {
        Schema schema = Configurator<TestStruct1>::getSchema("TestStruct2");

        EXPECT_TRUE(schema.getAliasFromKey<int>("exampleKey2") == 20);
        EXPECT_TRUE(schema.getAliasFromKey<int>("exampleKey3") == 30);
    }

    // Check that overwrite element does not accept non-existing paths
    {
        Schema schema;
        EXPECT_THROW(OVERWRITE_ELEMENT(schema).key("non_existing_path"), karabo::data::ParameterException);
    }

    // Check that overwrite element complains if key(...) is not called first (too lazy to test all cases...)
    {
        Schema schema;
        EXPECT_THROW(OVERWRITE_ELEMENT(schema).commit(), karabo::data::LogicException);
    }

    testOverwriteElementScalarDefault();
    testOverwriteElementVectorDefault();
    testOverwriteElementMinMax();
    testOverwriteElementMinMaxVector();
}


void testOverwriteElementScalarDefault() {
    Schema schema;
    UINT16_ELEMENT(schema)
          .key("uint16")
          .assignmentOptional()
          .defaultValue(5u)
          .options(std::vector<unsigned short>{1u, 5u})
          .commit();
    INT32_ELEMENT(schema)
          .key("int32") //
          .assignmentOptional()
          .defaultValue(-5)
          .options(std::vector<int>{3, -5})
          .commit();
    DOUBLE_ELEMENT(schema)
          .key("double")
          .assignmentOptional()
          .defaultValue(0.)
          .options(std::vector<double>{2.2, -3.3, 0.})
          .commit();
    STRING_ELEMENT(schema)
          .key("string")
          .assignmentOptional()
          .defaultValue("default")
          .options(std::vector<std::string>{"default", "other"})
          .commit();
    STATE_ELEMENT(schema)
          .key("state")
          .initialValue(State::INIT)
          .options(State::INIT, State::ON, State::CHANGING)
          .commit();
    INT64_ELEMENT(schema) //
          .key("int64Inc")
          .assignmentOptional()
          .defaultValue(0)
          .minInc(-5)
          .maxInc(5)
          .commit();
    INT64_ELEMENT(schema) //
          .key("int64Exc")
          .assignmentOptional()
          .defaultValue(0)
          .minExc(-5)
          .maxExc(5)
          .commit();

    Schema workSchema(schema);
    // unit16
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema)
                       .key("uint16")
                       .setNewDefaultValue(static_cast<unsigned short>(2u))
                       .commit(), // options are 1 and 5
                 karabo::data::LogicException);
    workSchema = schema; // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key("uint16").setNewOptions("1, 2").commit(),
                 karabo::data::LogicException); // default is 5

    // int32
    workSchema = schema; // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema)
                       .key("int32")
                       .setNewDefaultValue(2) // options are 3 and -5
                       .commit(),
                 karabo::data::LogicException);
    workSchema = schema; // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key("int32").setNewOptions("1, 2").commit(),
                 karabo::data::LogicException); // default is -5

    // double
    workSchema = schema; // start clean
    EXPECT_THROW(
          OVERWRITE_ELEMENT(workSchema).key("double").setNewDefaultValue(2.1).commit(), // options are 2.2, -3.3 and 0.
          karabo::data::LogicException);
    workSchema = schema; // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key("double").setNewOptions("1.1, 2.2").commit(),
                 karabo::data::LogicException); // default is 0.

    // string
    workSchema = schema; // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema)
                       .key("string")
                       .setNewDefaultValue("further")
                       .commit(), // options are "default" and "other"
                 karabo::data::LogicException);
    workSchema = schema; // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key("string").setNewOptions("one, another").commit(),
                 karabo::data::LogicException); // default is "default"

    // State
    workSchema = schema; // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema)
                       .key("state")
                       .setNewDefaultValue(State::UNKNOWN)
                       .commit(), // options are INIT, ON, CHANGING
                 karabo::data::LogicException);
    workSchema = schema; // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema)
                       .key("state")
                       .setNewOptions(std::vector<State>{State::ON, State::ACQUIRING})
                       .commit(),
                 karabo::data::LogicException); // default is INIT

    // Check inclusive minimum and maximum: -5 <= x <= 5
    workSchema = schema;
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema) //
                          .key("int64Inc")
                          .setNewDefaultValue(-5LL)
                          .commit());

    workSchema = schema;                          // start clean
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema) //
                          .key("int64Inc")
                          .setNewDefaultValue(5LL)
                          .commit());

    workSchema = schema;                       // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema) //
                       .key("int64Inc")
                       .setNewDefaultValue(-6LL)
                       .commit(),
                 karabo::data::ParameterException);

    workSchema = schema;                       // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema) //
                       .key("int64Inc")
                       .setNewDefaultValue(6LL)
                       .commit(),
                 karabo::data::ParameterException);

    // Check exclusive minimum and maximum: -5 < x < 5
    workSchema = schema;                          // start clean
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema) //
                          .key("int64Exc")
                          .setNewDefaultValue(-4LL)
                          .commit());

    workSchema = schema;                          // start clean
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema) //
                          .key("int64Exc")
                          .setNewDefaultValue(4LL)
                          .commit());
    workSchema = schema;                       // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema) //
                       .key("int64Exc")
                       .setNewDefaultValue(-5LL)
                       .commit(),
                 karabo::data::ParameterException);

    workSchema = schema;                       // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema) //
                       .key("int64Exc")
                       .setNewDefaultValue(5LL)
                       .commit(),
                 karabo::data::ParameterException);

    // We skip explicit testing of BOOL, CHAR, [U]INT8, INT16, UINT32, [U]INT64, FLOAT
}


// Helper
template <typename TYPE>
void testArrayBoundaries(const Schema& schema, const std::string& name, uint32_t vmin, uint32_t vmax, uint32_t vdef) {
    Schema workSchema = schema;
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key(name).setNewDefaultValue(std::vector<TYPE>(vmin - 1)).commit(),
                 ParameterException);
    workSchema = schema;
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key(name).setNewDefaultValue(std::vector<TYPE>(vmax + 1)).commit(),
                 ParameterException);
    workSchema = schema;
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(name).setNewDefaultValue(std::vector<TYPE>(vmin)).commit());
    workSchema = schema;
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(name).setNewDefaultValue(std::vector<TYPE>(vmax)).commit());
    workSchema = schema;
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key(name).setNewMinSize(vdef + 1).commit(), ParameterException);
    workSchema = schema;
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key(name).setNewMaxSize(vdef - 1).commit(), ParameterException);
}


void testOverwriteElementVectorDefault() {
    constexpr unsigned int vector_min_size{10};
    constexpr unsigned int vector_max_size{15};
    constexpr unsigned int vector_default_size{12};

    Schema schema;

    VECTOR_BOOL_ELEMENT(schema)
          .key("boolVector")
          .assignmentOptional()
          .defaultValue(std::vector<bool>(vector_default_size))
          .minSize(vector_min_size)
          .maxSize(vector_max_size)
          .commit();
    VECTOR_CHAR_ELEMENT(schema)
          .key("charVector")
          .assignmentOptional()
          .defaultValue(std::vector<char>(vector_default_size))
          .minSize(vector_min_size)
          .maxSize(vector_max_size)
          .commit();
    VECTOR_INT8_ELEMENT(schema)
          .key("int8Vector")
          .assignmentOptional()
          .defaultValue(std::vector<signed char>(vector_default_size))
          .minSize(vector_min_size)
          .maxSize(vector_max_size)
          .commit();
    VECTOR_UINT8_ELEMENT(schema)
          .key("uint8Vector")
          .assignmentOptional()
          .defaultValue(std::vector<unsigned char>(vector_default_size))
          .minSize(vector_min_size)
          .maxSize(vector_max_size)
          .commit();
    VECTOR_INT16_ELEMENT(schema)
          .key("int16Vector")
          .assignmentOptional()
          .defaultValue(std::vector<signed short int>(vector_default_size))
          .minSize(vector_min_size)
          .maxSize(vector_max_size)
          .commit();
    VECTOR_UINT16_ELEMENT(schema)
          .key("uint16Vector")
          .assignmentOptional()
          .defaultValue(std::vector<unsigned short int>(vector_default_size))
          .minSize(vector_min_size)
          .maxSize(vector_max_size)
          .commit();
    VECTOR_INT32_ELEMENT(schema)
          .key("int32Vector")
          .assignmentOptional()
          .defaultValue(std::vector<signed int>(vector_default_size))
          .minSize(vector_min_size)
          .maxSize(vector_max_size)
          .commit();
    VECTOR_UINT32_ELEMENT(schema)
          .key("uint32Vector")
          .assignmentOptional()
          .defaultValue(std::vector<unsigned int>(vector_default_size))
          .minSize(vector_min_size)
          .maxSize(vector_max_size)
          .commit();
    VECTOR_INT64_ELEMENT(schema)
          .key("int64Vector")
          .assignmentOptional()
          .defaultValue(std::vector<signed long long int>(vector_default_size))
          .minSize(vector_min_size)
          .maxSize(vector_max_size)
          .commit();
    VECTOR_UINT64_ELEMENT(schema)
          .key("uint64Vector")
          .assignmentOptional()
          .defaultValue(std::vector<unsigned long long int>(vector_default_size))
          .minSize(vector_min_size)
          .maxSize(vector_max_size)
          .commit();
    VECTOR_FLOAT_ELEMENT(schema)
          .key("floatVector")
          .assignmentOptional()
          .defaultValue(std::vector<float>(vector_default_size))
          .minSize(vector_min_size)
          .maxSize(vector_max_size)
          .commit();
    VECTOR_DOUBLE_ELEMENT(schema)
          .key("doubleVector")
          .assignmentOptional()
          .defaultValue(std::vector<double>(vector_default_size))
          .minSize(vector_min_size)
          .maxSize(vector_max_size)
          .commit();
    VECTOR_STRING_ELEMENT(schema)
          .key("stringVector")
          .assignmentOptional()
          .defaultValue(std::vector<std::string>(vector_default_size))
          .minSize(vector_min_size)
          .maxSize(vector_max_size)
          .commit();

    Schema rowSchema;
    STRING_ELEMENT(rowSchema).key("string").assignmentOptional().noDefaultValue().commit();
    INT64_ELEMENT(rowSchema).key("int").assignmentOptional().noDefaultValue().commit();

    Hash row{"string", "Hello", "int", 1ll};

    constexpr unsigned int table_min_size{3};
    constexpr unsigned int table_max_size{10};
    constexpr unsigned int table_default_size{5};

    TABLE_ELEMENT(schema)
          .key("tableElement")
          .setColumns(rowSchema)
          .assignmentOptional()
          .defaultValue(std::vector<Hash>{table_default_size, row})
          .minSize(table_min_size)
          .maxSize(table_max_size)
          .commit();

    // See macro definition above. We check boundary cases, one case less
    // than minimum and one greater than maximum. Also, one test that
    // changes the minimum value, and another that changes the maximum value.

    testArrayBoundaries<bool>(schema, "boolVector", vector_min_size, vector_max_size, vector_default_size);
    testArrayBoundaries<char>(schema, "charVector", vector_min_size, vector_max_size, vector_default_size);
    testArrayBoundaries<int8_t>(schema, "int8Vector", vector_min_size, vector_max_size, vector_default_size);
    testArrayBoundaries<uint8_t>(schema, "uint8Vector", vector_min_size, vector_max_size, vector_default_size);
    testArrayBoundaries<int16_t>(schema, "int16Vector", vector_min_size, vector_max_size, vector_default_size);
    testArrayBoundaries<uint16_t>(schema, "uint16Vector", vector_min_size, vector_max_size, vector_default_size);
    testArrayBoundaries<int>(schema, "int32Vector", vector_min_size, vector_max_size, vector_default_size);
    testArrayBoundaries<uint32_t>(schema, "uint32Vector", vector_min_size, vector_max_size, vector_default_size);
    testArrayBoundaries<long long>(schema, "int64Vector", vector_min_size, vector_max_size, vector_default_size);
    testArrayBoundaries<unsigned long long>(schema, "uint64Vector", vector_min_size, vector_max_size,
                                            vector_default_size);
    testArrayBoundaries<float>(schema, "floatVector", vector_min_size, vector_max_size, vector_default_size);
    testArrayBoundaries<double>(schema, "doubleVector", vector_min_size, vector_max_size, vector_default_size);
    testArrayBoundaries<std::string>(schema, "stringVector", vector_min_size, vector_max_size, vector_default_size);

    // We check boundary cases, one case less than minimum and one greater
    // than maximum. Also, one test that changes the minimum value, and
    // another that changes the maximum value.
    //
    // 'row' is defined with 'tableElement' above, to make code easier to
    // read
    Schema workSchema = schema;                // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema) //
                       .key("tableElement")
                       .setNewDefaultValue(std::vector<Hash>{table_min_size - 1, row})
                       .commit(),
                 karabo::data::ParameterException);
    workSchema = schema;                       // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema) //
                       .key("tableElement")
                       .setNewDefaultValue(std::vector<Hash>{table_max_size + 1, row})
                       .commit(),
                 karabo::data::ParameterException);
    workSchema = schema;                          // start clean
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema) //
                          .key("tableElement")
                          .setNewDefaultValue(std::vector<Hash>{table_min_size, row})
                          .commit());
    workSchema = schema;                          // start clean
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema) //
                          .key("tableElement")
                          .setNewDefaultValue(std::vector<Hash>{table_max_size, row})
                          .commit());
    workSchema = schema;                       // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema) //
                       .key("tableElement")
                       .setNewMinSize(table_default_size + 1)
                       .commit(),
                 karabo::data::ParameterException);
    workSchema = schema;                       // start clean
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema) //
                       .key("tableElement")
                       .setNewMaxSize(table_default_size - 1)
                       .commit(),
                 karabo::data::ParameterException);
}


// Helper
static void checkBoundariesMinMax(const Schema& schema, const std::string& name, int minimum, int maximum) {
    Schema workSchema = schema;
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key(name + "Exc").setNewMaxExc(minimum).commit(), ParameterException);
    workSchema = schema;
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key(name + "Exc").setNewMinExc(maximum).commit(), ParameterException);
    workSchema = schema;
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(name + "Exc").setNewMaxExc(minimum + 1).commit());
    workSchema = schema;
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(name + "Exc").setNewMinExc(maximum - 1).commit());

    workSchema = schema;
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key(name + "Inc").setNewMaxInc(minimum - 1).commit(),
                 ParameterException);
    workSchema = schema;
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key(name + "Inc").setNewMinInc(maximum + 1).commit(),
                 ParameterException);
    workSchema = schema;
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(name + "Inc").setNewMaxInc(minimum).commit());
    workSchema = schema;
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(name + "Inc").setNewMinInc(maximum).commit());

    workSchema = schema;
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key(name + "IncExc").setNewMaxExc(minimum).commit(), ParameterException);
    workSchema = schema;
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key(name + "ExcInc").setNewMinExc(maximum).commit(), ParameterException);
    workSchema = schema;
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(name + "IncExc").setNewMaxExc(minimum + 1).commit());
    workSchema = schema;
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(name + "ExcInc").setNewMinExc(maximum - 1).commit());
}


void testOverwriteElementMinMax() {
    Schema schema;

    constexpr int minimum{5};
    constexpr int maximum{15};

#define INSERT_ELEMENT(ELEMENT, NAME)                                                                   \
    ELEMENT(schema).key(#NAME "Exc").assignmentMandatory().minExc(minimum).maxExc(maximum).commit();    \
    ELEMENT(schema).key(#NAME "Inc").assignmentMandatory().minInc(minimum).maxInc(maximum).commit();    \
    ELEMENT(schema).key(#NAME "IncExc").assignmentMandatory().minInc(minimum).maxExc(maximum).commit(); \
    ELEMENT(schema).key(#NAME "ExcInc").assignmentMandatory().minExc(minimum).maxInc(maximum).commit()

    INSERT_ELEMENT(INT8_ELEMENT, int8);
    INSERT_ELEMENT(UINT8_ELEMENT, uint8);
    INSERT_ELEMENT(INT16_ELEMENT, int16);
    INSERT_ELEMENT(UINT16_ELEMENT, uint16);
    INSERT_ELEMENT(INT32_ELEMENT, int32);
    INSERT_ELEMENT(UINT32_ELEMENT, uint32);
    INSERT_ELEMENT(INT64_ELEMENT, int64);
    INSERT_ELEMENT(UINT64_ELEMENT, uint64);
    INSERT_ELEMENT(FLOAT_ELEMENT, _float);
    INSERT_ELEMENT(DOUBLE_ELEMENT, _double);

#undef INSERT_ELEMENT

    checkBoundariesMinMax(schema, "int8", minimum, maximum);
    checkBoundariesMinMax(schema, "uint8", minimum, maximum);
    checkBoundariesMinMax(schema, "int16", minimum, maximum);
    checkBoundariesMinMax(schema, "uint16", minimum, maximum);
    checkBoundariesMinMax(schema, "int32", minimum, maximum);
    checkBoundariesMinMax(schema, "uint32", minimum, maximum);
    checkBoundariesMinMax(schema, "int64", minimum, maximum);
    checkBoundariesMinMax(schema, "uint64", minimum, maximum);
    checkBoundariesMinMax(schema, "_float", minimum, maximum);
    checkBoundariesMinMax(schema, "_double", minimum, maximum);
}


// Helper
static void checkBoundariesMinMaxVector(const Schema& schema, const std::string& name, uint32_t minimum,
                                        uint32_t maximum) {
    Schema workSchema;
    workSchema = schema;
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key(name).setNewMinSize(maximum + 1).commit(), ParameterException);
    workSchema = schema;
    EXPECT_THROW(OVERWRITE_ELEMENT(workSchema).key(name).setNewMaxSize(minimum - 1).commit(), ParameterException);
    workSchema = schema;
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(name).setNewMinSize(maximum).commit());
    workSchema = schema;
    EXPECT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(name).setNewMaxSize(minimum).commit());
}

void testOverwriteElementMinMaxVector() {
    Schema schema;

    constexpr unsigned int minimum{5};
    constexpr unsigned int maximum{15};

#define INSERT_ELEMENT(ELEMENT, NAME) \
    ELEMENT(schema).key(#NAME).assignmentMandatory().minSize(minimum).maxSize(maximum).commit();

    INSERT_ELEMENT(VECTOR_INT8_ELEMENT, int8);
    INSERT_ELEMENT(VECTOR_UINT8_ELEMENT, uint8);
    INSERT_ELEMENT(VECTOR_INT16_ELEMENT, int16);
    INSERT_ELEMENT(VECTOR_UINT16_ELEMENT, uint16);
    INSERT_ELEMENT(VECTOR_INT32_ELEMENT, int32);
    INSERT_ELEMENT(VECTOR_UINT32_ELEMENT, uint32);
    INSERT_ELEMENT(VECTOR_INT64_ELEMENT, int64);
    INSERT_ELEMENT(VECTOR_UINT64_ELEMENT, uint64);
    INSERT_ELEMENT(VECTOR_FLOAT_ELEMENT, _float);
    INSERT_ELEMENT(VECTOR_DOUBLE_ELEMENT, _double);

#undef INSERT_ELEMENT


    checkBoundariesMinMaxVector(schema, "int8", minimum, maximum);
    checkBoundariesMinMaxVector(schema, "uint8", minimum, maximum);
    checkBoundariesMinMaxVector(schema, "int16", minimum, maximum);
    checkBoundariesMinMaxVector(schema, "uint16", minimum, maximum);
    checkBoundariesMinMaxVector(schema, "int32", minimum, maximum);
    checkBoundariesMinMaxVector(schema, "uint32", minimum, maximum);
    checkBoundariesMinMaxVector(schema, "int64", minimum, maximum);
    checkBoundariesMinMaxVector(schema, "uint64", minimum, maximum);
    checkBoundariesMinMaxVector(schema, "_float", minimum, maximum);
    checkBoundariesMinMaxVector(schema, "_double", minimum, maximum);
}


TEST_F(TestSchema, testMerge) {
    Schema schema = Configurator<SchemaNodeElements>::getSchema(
          "SchemaNodeElements", Schema::AssemblyRules(AccessType::READ | AccessType::WRITE | AccessType::INIT));
    EXPECT_TRUE(schema.getDefaultValue<unsigned int>("monitor.count") == 777);

    Schema schema2("SchemaNodeElements",
                   Schema::AssemblyRules(AccessType::READ | AccessType::WRITE | AccessType::INIT));
    SchemaNodeInjected::expectedParameters(schema2);
    EXPECT_TRUE(schema2.getDefaultValue<float>("monitor.stats.d1") == 3.1415f);

    schema.merge(schema2);
    EXPECT_TRUE(schema.getDefaultValue<unsigned int>("monitor.count") == 777);
    EXPECT_TRUE(schema.getDefaultValue<float>("monitor.stats.d1") == 3.1415f);
}


TEST_F(TestSchema, testInvalidNodes) {
    Schema schema("OtherSchemaElements",
                  Schema::AssemblyRules(AccessType::READ | AccessType::WRITE | AccessType::INIT));
    OtherSchemaElements::expectedParameters(schema);

    // Placing an element under a leaf is not allowed
    EXPECT_TRUE(schema.has("vecDouble"));
    EXPECT_TRUE(!schema.has("vecDouble.uint16"));
    EXPECT_THROW(
          UINT16_ELEMENT(schema)
                .key("vecDouble.uint16")
                .description("This element's key refers to a parent that is not a node and thus triggers an exception")
                .readOnly()
                .commit(),
          karabo::data::LogicException);

    // A node should not be created automatically
    EXPECT_TRUE(!schema.has("nonExistingNode"));
    EXPECT_THROW(INT16_ELEMENT(schema)
                       .key("nonExistingNode.int16")
                       .description("This element refers to a non-existing node and thus triggers an exception")
                       .readOnly()
                       .commit(),
                 karabo::data::LogicException);

    // Empty strings are forbidden as keys:
    EXPECT_THROW(INT32_ELEMENT(schema).key("").description("Empty key is forbidden"), karabo::data::ParameterException);

    // Also under a node:
    NODE_ELEMENT(schema).key("node").commit();
    EXPECT_THROW(INT32_ELEMENT(schema).key("node.").description("Empty key is forbidden"),
                 karabo::data::ParameterException);

    // Various characters are forbidden:
    constexpr char forbidden[] = "()[]<>`~!#$%^&*-+=|\\:;'\",? ";
    const std::string valid("valid");
    std::string invalid(valid + " ");
    for (size_t i = 0; i < sizeof(forbidden) / sizeof(forbidden[0]); ++i) {
        invalid.back() = forbidden[i];
        EXPECT_THROW(INT8_ELEMENT(schema).key(invalid), karabo::data::ParameterException) << invalid;
        EXPECT_THROW(INT8_ELEMENT(schema).key("node." + invalid), karabo::data::ParameterException) << invalid;
    }
    // Digits as first character are also forbidden
    invalid = valid;
    for (char i = 0; i < 10; ++i) {
        invalid[0] = '0' + i; // test '0', '1', '2', ... one after another
        EXPECT_THROW(INT8_ELEMENT(schema).key(invalid), karabo::data::ParameterException) << invalid;
        EXPECT_THROW(INT8_ELEMENT(schema).key("node." + invalid), karabo::data::ParameterException) << invalid;
    }
    constexpr char validCharacters[] =
          "abcdefghijklmnopqrstuvwxyz"
          "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
          "0123456789_";
    EXPECT_NO_THROW(INT8_ELEMENT(schema).key(validCharacters).readOnly().commit());
    EXPECT_NO_THROW(INT8_ELEMENT(schema).key(std::string("node.") += validCharacters).readOnly().commit());

    // Check tolerance if requested to be not strict:
    std::string tolerated(valid);
    tolerated[1] = '@';
    EXPECT_NO_THROW(INT8_ELEMENT(schema).key(tolerated, false).readOnly().commit());
    EXPECT_THROW(INT8_ELEMENT(schema).key(tolerated), karabo::data::ParameterException);

    // Now with two tolerated characters, one at the end
    tolerated.back() = '-';
    EXPECT_NO_THROW(INT8_ELEMENT(schema).key(tolerated, false).readOnly().commit());
    EXPECT_THROW(INT8_ELEMENT(schema).key(tolerated), karabo::data::ParameterException);

    // '/' is tolerated as well - but not as first
    tolerated.back() = '/';
    EXPECT_NO_THROW(INT8_ELEMENT(schema).key(tolerated, false).readOnly().commit());
    EXPECT_THROW(INT8_ELEMENT(schema).key(tolerated), karabo::data::ParameterException);
    tolerated.front() = '/';
    EXPECT_THROW(INT8_ELEMENT(schema).key(tolerated, false), karabo::data::ParameterException);
}


TEST_F(TestSchema, testOverwriteRestrictions) {
    Schema schema;
    STATE_ELEMENT(schema).key("state").commit();

    EXPECT_THROW(OVERWRITE_ELEMENT(schema).key("state").setNewMinInc(100).commit(), karabo::data::LogicException);
}


TEST_F(TestSchema, testOverwriteTags) {
    Schema schema;
    INT32_ELEMENT(schema).key("taggedProp").tags("greet tip").readOnly().commit();

    EXPECT_NO_THROW(OVERWRITE_ELEMENT(schema).key("taggedProp").setNewTags({"doff"}).commit());

    std::vector<std::string> tags = schema.getTags("taggedProp");
    EXPECT_EQ(1ul, tags.size());
    EXPECT_STREQ("doff", tags[0].c_str());

    EXPECT_NO_THROW(OVERWRITE_ELEMENT(schema).key("taggedProp").setNewTags({"left", "right"}).commit());

    tags = schema.getTags("taggedProp");
    EXPECT_EQ(2ul, tags.size());
    EXPECT_STREQ("left", tags[0].c_str());
    EXPECT_STREQ("right", tags[1].c_str());
}


TEST_F(TestSchema, testTagsFromVector) {
    Schema schema;
    INT32_ELEMENT(schema).key("taggedProp").tags(std::vector<std::string>({"greet", "doff"})).readOnly().commit();

    const std::vector<std::string>& tags = schema.getTags("taggedProp");
    EXPECT_EQ(2ul, tags.size());
    EXPECT_STREQ("greet", tags[0].c_str());
    EXPECT_STREQ("doff", tags[1].c_str());
}


TEST_F(TestSchema, testOverwriteRestrictionsForOptions) {
    {
        Schema schema;
        INT32_ELEMENT(schema)
              .key("range")
              .displayedName("Range")
              .options("0,1")
              .assignmentOptional()
              .defaultValue(0)
              .reconfigurable()
              .commit();

        const auto& range1 = schema.getOptions<int>("range");
        EXPECT_EQ(range1.size(), 2ul);
        EXPECT_EQ(range1[0], 0);
        EXPECT_EQ(range1[1], 1);

        EXPECT_NO_THROW(OVERWRITE_ELEMENT(schema).key("range").setNewOptions("0,1,2").commit());

        const auto& range2 = schema.getOptions<int>("range");
        EXPECT_EQ(range2.size(), 3ul);
        EXPECT_EQ(range2[0], 0);
        EXPECT_EQ(range2[1], 1);
        EXPECT_EQ(range2[2], 2);
    }
    {
        Schema schema;
        NODE_ELEMENT(schema).key("node").commit();
        INT32_ELEMENT(schema)
              .key("node.range")
              .displayedName("Range")
              .options("0,1")
              .assignmentOptional()
              .defaultValue(0)
              .reconfigurable()
              .commit();

        const auto& range1 = schema.getOptions<int>("node.range");
        EXPECT_EQ(range1.size(), 2ul);
        EXPECT_EQ(range1[0], 0);
        EXPECT_EQ(range1[1], 1);

        EXPECT_NO_THROW(OVERWRITE_ELEMENT(schema).key("node.range").setNewOptions("0,1,2").commit());

        const auto& range2 = schema.getOptions<int>("node.range");
        EXPECT_EQ(range2.size(), 3ul);
        EXPECT_EQ(range2[0], 0);
        EXPECT_EQ(range2[1], 1);
        EXPECT_EQ(range2[2], 2);
    }
}


TEST_F(TestSchema, testStateAndAlarmSets) {
    Schema schema;

    STRING_ELEMENT(schema).key("string").readOnly().commit();

    STATE_ELEMENT(schema).key("state").commit();

    ALARM_ELEMENT(schema).key("alarm").commit();

    Hash h("string", "abc");
    Validator val;
    Hash h_out;
    std::pair<bool, std::string> r = val.validate(schema, h, h_out);
    EXPECT_TRUE(r.first == true); // should validate
    h.set("state", "abc");
    r = val.validate(schema, h, h_out);
    EXPECT_TRUE(r.first == false); // should not validate as we are setting string to state
    Hash h2("alarm", "abc");
    r = val.validate(schema, h2, h_out);
    EXPECT_TRUE(r.first == false); // should not validate as we are setting string to alarm
    Hash::Node& n = h.set("state", "UNKNOWN");
    n.setAttribute(KARABO_INDICATE_STATE_SET, true);
    r = val.validate(schema, h, h_out);
    EXPECT_TRUE(r.first == true); // should validate as we faked updateState
    Hash::Node& n2 = h2.set("alarm", "none");
    n2.setAttribute(KARABO_INDICATE_ALARM_SET, true);
    r = val.validate(schema, h2, h_out);
    EXPECT_TRUE(r.first == true); // should validate as we faked setAlarmCondition
}


TEST_F(TestSchema, testSubSchema) {
    Schema schema("test");
    GraphicsRenderer1::expectedParameters(schema);
    const int alias = 1;
    OVERWRITE_ELEMENT(schema).key("rectangle.c").setNewAlias(alias).commit();
    {
        Schema sub = schema.subSchema("rectangle");
        EXPECT_TRUE(sub.has("b"));
        EXPECT_TRUE(sub.has("c"));
        EXPECT_TRUE(sub.keyHasAlias("c"));
        EXPECT_TRUE(sub.aliasHasKey(alias));
        EXPECT_STREQ("c", sub.getKeyFromAlias(alias).c_str());
        EXPECT_EQ(alias, sub.getAliasFromKey<int>("c"));
        EXPECT_STREQ("", sub.getRootName().c_str()); // we have another hierarchy level, not match anymore
    }
    {
        Schema sub = schema.subSchema("rectangle", "b"); // filter for tag "b"
        EXPECT_TRUE(sub.has("b"));
        EXPECT_TRUE(!sub.has("c"));
        EXPECT_TRUE(!sub.aliasHasKey(alias));
    }

    // Now testing 'by rules':
    {
        Schema::AssemblyRules rules(AccessType::READ | AccessType::WRITE | AccessType::INIT); // i.e. everything
        const Schema sub = schema.subSchemaByRules(rules);
        // Everything is in:
        std::vector<std::string> finalPaths;
        sub.getParameterHash().getPaths(finalPaths);
        EXPECT_EQ(static_cast<size_t>(8), finalPaths.size());
    }

    {
        Schema::AssemblyRules rules(AccessType::READ | AccessType::WRITE | AccessType::INIT,
                                    "ON"); // i.e. required state ON or non-defined
        const Schema sub = schema.subSchemaByRules(rules);
        EXPECT_TRUE(!sub.has("color"));

        // But all else since only "color" is reconfigurable for state OFF
        std::vector<std::string> finalPaths;
        sub.getParameterHash().getPaths(finalPaths);
        EXPECT_EQ(static_cast<size_t>(7), finalPaths.size());

        // Check rules are preserved
        const Schema::AssemblyRules subRules = sub.getAssemblyRules();
        EXPECT_EQ(rules.m_accessLevel, subRules.m_accessLevel);
        EXPECT_EQ(rules.m_accessMode, subRules.m_accessMode);
        EXPECT_EQ(rules.m_state, subRules.m_state);
        // ...and alias as well
        EXPECT_TRUE(sub.keyHasAlias("rectangle.c"));
        EXPECT_TRUE(sub.aliasHasKey(alias));
        EXPECT_STREQ("rectangle.c", sub.getKeyFromAlias(alias).c_str());
        EXPECT_EQ(alias, sub.getAliasFromKey<int>("rectangle.c"));
        EXPECT_STREQ(schema.getRootName().c_str(), sub.getRootName().c_str());
    }

    {
        Schema::AssemblyRules rules(AccessType::READ | AccessType::WRITE | AccessType::INIT, "", int(Schema::OPERATOR));
        const Schema sub = schema.subSchemaByRules(rules);
        EXPECT_TRUE(!sub.has("antiAlias"));

        // But all else is left since "antiAlias" is the only expert access level (defaults are user or observer)
        std::vector<std::string> finalPaths;
        sub.getParameterHash().getPaths(finalPaths);
        EXPECT_EQ(static_cast<size_t>(7), finalPaths.size());
    }

    {
        Schema::AssemblyRules rules(AccessType::READ);
        const Schema sub = schema.subSchemaByRules(rules);
        // Nothing is readOnly...
        EXPECT_TRUE(sub.empty());
    }

    {
        Schema::AssemblyRules rules(AccessType::INIT | AccessType::READ);
        const Schema sub = schema.subSchemaByRules(rules);
        EXPECT_TRUE(sub.has("antiAlias"));
        EXPECT_TRUE(sub.has("rectangle.b"));
        EXPECT_TRUE(sub.has("rectangle.c"));
        EXPECT_TRUE(sub.has("circle.radius"));

        // All else is WRITE (i.e. reconfigurable))
        std::vector<std::string> finalPaths;
        sub.getParameterHash().getPaths(finalPaths);
        EXPECT_EQ(static_cast<size_t>(5), finalPaths.size()) << toString(sub);
    }

    // Test subSchemaByPaths
    {
        // Extend the schema to test options
        OVERWRITE_ELEMENT(schema).key("color").setNewOptions({"red", "yellow", "blue"}).commit();
        const Schema sub = schema.subSchemaByPaths({"color",         // first level, but endpoint
                                                    "circle",        // a node among choices
                                                    "rectangle.c"}); // end point within a choice
        // Check that all the paths (and no more) are there
        EXPECT_TRUE(sub.has("color"));
        EXPECT_TRUE(sub.has("circle"));
        EXPECT_TRUE(sub.has("circle.radius"));
        EXPECT_TRUE(sub.has("rectangle"));
        EXPECT_TRUE(!sub.has("rectangle.b"));
        EXPECT_TRUE(sub.has("rectangle.c"));
        EXPECT_EQ(3ul, sub.getPaths().size());
        EXPECT_LT(3ul, schema.getPaths().size());

        // Check whether attributes are there
        EXPECT_STREQ("red", sub.getDefaultValue<std::string>("color").c_str());
        EXPECT_EQ(std::vector<std::string>(1, "prop").size(), sub.getTags("color").size());
        EXPECT_STREQ("Color", sub.getDisplayedName("color").c_str());
        EXPECT_TRUE(std::vector<std::string>({"red", "yellow", "blue"}) == sub.getOptions<std::string>("color"));
        EXPECT_TRUE(sub.isAssignmentOptional("color"));
        EXPECT_TRUE(sub.isAccessReconfigurable("color"));

        EXPECT_EQ(Unit::METER, sub.getUnit("circle.radius"));
        EXPECT_STREQ("m", sub.getUnitSymbol("circle.radius").c_str());
        EXPECT_EQ(MetricPrefix::MILLI, sub.getMetricPrefix("circle.radius"));
        EXPECT_STREQ("m", sub.getMetricPrefixSymbol("circle.radius").c_str());
        EXPECT_FLOAT_EQ(0.f, sub.getMinExc<float>("circle.radius"));
        EXPECT_FLOAT_EQ(100.f, sub.getMaxExc<float>("circle.radius"));

        // Test alias and root name
        EXPECT_TRUE(sub.keyHasAlias("rectangle.c"));
        EXPECT_TRUE(sub.aliasHasKey(alias));
        EXPECT_STREQ("rectangle.c", sub.getKeyFromAlias(alias).c_str());
        EXPECT_EQ(alias, sub.getAliasFromKey<int>("rectangle.c"));
        EXPECT_STREQ(schema.getRootName().c_str(), sub.getRootName().c_str());
    }
}


TEST_F(TestSchema, testDaqDataType) {
    Schema schema("test");
    GraphicsRenderer1::expectedParameters(schema);
    {
        schema.setDaqDataType("triangle", DaqDataType::PULSE);
        EXPECT_TRUE(schema.getDaqDataType("triangle") == DaqDataType::PULSE);
    }
    {
        schema.setDaqDataType("triangle", DaqDataType::TRAIN);
        EXPECT_TRUE(schema.getDaqDataType("triangle") == DaqDataType::TRAIN);
    }
}


TEST_F(TestSchema, testNodeDisplayType) {
    {
        Schema schema;
        NODE_ELEMENT(schema).key("node").setSpecialDisplayType("WidgetNode").commit();

        STRING_ELEMENT(schema).key("node.string").readOnly().commit();

        BOOL_ELEMENT(schema).key("node.bool").readOnly().commit();

        EXPECT_TRUE(schema.getDisplayType("node") == "WidgetNode");
    }
}


TEST_F(TestSchema, testGetLeaves) {
    Schema schema("test");
    TestStruct1::expectedParameters(schema);
    OtherSchemaElements::expectedParameters(schema);
    Hash h;
    h.set("slotTest", Hash());
    h.set("filename", string("here"));
    h.set("testTable", vector<Hash>(3, Hash("a", -1, "b", "this_is a va|id string")));
    vector<string> leaves;
    getLeaves(h, schema, leaves, '.'); // from DataLogUtils
    EXPECT_STREQ("filename", leaves[0].c_str());
    EXPECT_STREQ("testTable", leaves[1].c_str());
    EXPECT_EQ(2ul, leaves.size());
}


TEST_F(TestSchema, testAlarmStateElement) {
    Schema schema("test");
    OtherSchemaElements::expectedParameters(schema);
    EXPECT_TRUE(schema.isProperty("alarmCondition"));
    EXPECT_TRUE(schema.isProperty("state"));

    // And the classId for both elements
    EXPECT_TRUE(schema.hasClassId("alarmCondition"));
    EXPECT_TRUE(schema.hasClassId("state"));
    EXPECT_TRUE(schema.getClassId("alarmCondition") == "AlarmCondition");
    EXPECT_TRUE(schema.getClassId("state") == "State");
}


TEST_F(TestSchema, testInvalidReadOnlyThrows) {
    karabo::data::Schema invalidSchema;

    // assignmentMandatory() and readOnly() contradict each other
    EXPECT_THROW(FLOAT_ELEMENT(invalidSchema).key("float").assignmentMandatory().readOnly().commit(),
                 karabo::data::LogicException);

    // The assignmentOptional().defaultValue(1).readOnly() sequence below,
    // if accepted, would reset the element value to 0, overriding the
    // defaultValue setting.
    EXPECT_THROW(INT32_ELEMENT(invalidSchema).key("int").assignmentOptional().defaultValue(1).readOnly().commit(),
                 karabo::data::LogicException);

    karabo::data::Schema validReadOnlySchema;

    EXPECT_NO_THROW(INT32_ELEMENT(validReadOnlySchema).key("int").readOnly().initialValue(1).commit());
}


TEST_F(TestSchema, testTable) {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(AccessType::READ | AccessType::WRITE | AccessType::INIT));
    OtherSchemaElements::expectedParameters(sch);
    EXPECT_TRUE(sch.isLeaf("testTable") == true);
    EXPECT_TRUE(sch.getParameterHash().hasAttribute("testTable", "rowSchema") == true);
    const std::vector<Hash> shouldBeDefault(2, Hash("a", 3, "b", "foo"));
    const std::vector<Hash>& theDefault = sch.getDefaultValue<std::vector<Hash>>("testTable");
    EXPECT_EQ(shouldBeDefault.size(), theDefault.size());
    EXPECT_EQ(shouldBeDefault[0].size(), theDefault[1].size());
    EXPECT_EQ(shouldBeDefault[1].get<int>("a"), theDefault[0].get<int>("a"));
    EXPECT_STREQ(shouldBeDefault[1].get<std::string>("b").c_str(), theDefault[1].get<std::string>("b").c_str());

    EXPECT_TRUE(sch.getDefaultValue<std::vector<Hash>>("testTableEmptyDefault").empty());
}


TEST_F(TestSchema, testTableReadOnly) {
    karabo::data::Schema rowSchema;

    STRING_ELEMENT(rowSchema).key("s").assignmentOptional().noDefaultValue().commit();

    BOOL_ELEMENT(rowSchema).key("b").assignmentOptional().noDefaultValue().commit();

    karabo::data::Schema invalidReadOnlySchema;

    // assignmentMandatory() and readOnly() contradict each other
    EXPECT_THROW(TABLE_ELEMENT(invalidReadOnlySchema)
                       .key("InvalidTable")
                       .setColumns(rowSchema)
                       .assignmentMandatory()
                       .readOnly()
                       .commit(),
                 karabo::data::LogicException);

    // The assignmentOptional().defaultValue(...).readOnly() sequence below,
    // if accepted, would reset the element value to empty vector of hashes,
    // overriding the defaultValue setting.
    EXPECT_THROW(TABLE_ELEMENT(invalidReadOnlySchema)
                       .key("InvalidTable")
                       .setColumns(rowSchema)
                       .assignmentOptional()
                       .defaultValue(std::vector<Hash>(1, Hash("s", "foo", "b", false)))
                       .readOnly()
                       .commit(),
                 karabo::data::LogicException);

    karabo::data::Schema validReadOnlySchema;

    EXPECT_NO_THROW(TABLE_ELEMENT(validReadOnlySchema)
                          .key("ValidTable")
                          .setColumns(rowSchema)
                          .readOnly()
                          .initialValue(std::vector<Hash>(1, Hash("s", "bar", "b", true)))
                          .archivePolicy(Schema::NO_ARCHIVING)
                          .commit());
    // Verify default and archive policy in schema:
    const std::vector<Hash>& specifiedDefault = validReadOnlySchema.getDefaultValue<std::vector<Hash>>("ValidTable");
    EXPECT_EQ(1ul, specifiedDefault.size());
    EXPECT_TRUE(specifiedDefault[0].fullyEquals(Hash("s", "bar", "b", true))) << toString(specifiedDefault[0]);
    EXPECT_TRUE(validReadOnlySchema.hasArchivePolicy("ValidTable"));
    EXPECT_EQ(Schema::NO_ARCHIVING, validReadOnlySchema.getArchivePolicy("ValidTable"));

    // Verify implicit default for readOnly (empty table) in schema:
    EXPECT_NO_THROW(TABLE_ELEMENT(validReadOnlySchema).key("ValidTable2").setColumns(rowSchema).readOnly().commit());
    const std::vector<Hash>& specifiedDefault2 = validReadOnlySchema.getDefaultValue<std::vector<Hash>>("ValidTable2");
    EXPECT_EQ(0ul, specifiedDefault2.size());
    EXPECT_TRUE(!validReadOnlySchema.hasArchivePolicy("ValidTable2"));

    // Verifies that a config built from a Schema with read-only TABLE_ELEMENT is valid
    // and has the specified initial value.
    Hash configWithTable;
    Validator validator;
    auto res = validator.validate(validReadOnlySchema, Hash(), configWithTable);
    EXPECT_EQ(true, res.first);
    const std::vector<Hash>& tableRows = configWithTable.get<std::vector<Hash>>("ValidTable");
    EXPECT_EQ(1UL, tableRows.size());
    EXPECT_STREQ("bar", tableRows[0].get<std::string>("s").c_str());
    EXPECT_EQ(true, tableRows[0].get<bool>("b"));

    // Tables need their row schema:
    karabo::data::Schema noColumnsSchema;
    EXPECT_THROW(TABLE_ELEMENT(noColumnsSchema).key("tableLackingColumnsSchema").readOnly().commit(),
                 karabo::data::LogicException);
}


TEST_F(TestSchema, testTableColNoDefaultValue) {
    karabo::data::Schema rowSchema;
    // All the supported column types with no default value.
    BOOL_ELEMENT(rowSchema).key("bool").assignmentOptional().noDefaultValue().reconfigurable().commit();
    DOUBLE_ELEMENT(rowSchema).key("double").assignmentOptional().noDefaultValue().reconfigurable().commit();
    FLOAT_ELEMENT(rowSchema).key("float").assignmentOptional().noDefaultValue().reconfigurable().commit();
    STRING_ELEMENT(rowSchema).key("string").assignmentOptional().noDefaultValue().reconfigurable().commit();
    INT8_ELEMENT(rowSchema).key("int8").assignmentOptional().noDefaultValue().reconfigurable().commit();
    INT16_ELEMENT(rowSchema).key("int16").assignmentOptional().noDefaultValue().reconfigurable().commit();
    INT32_ELEMENT(rowSchema).key("int32").assignmentOptional().noDefaultValue().reconfigurable().commit();
    INT64_ELEMENT(rowSchema).key("int64").assignmentOptional().noDefaultValue().reconfigurable().commit();
    UINT8_ELEMENT(rowSchema).key("uint8").assignmentOptional().noDefaultValue().reconfigurable().commit();
    UINT16_ELEMENT(rowSchema).key("uint16").assignmentOptional().noDefaultValue().reconfigurable().commit();
    UINT32_ELEMENT(rowSchema).key("uint32").assignmentOptional().noDefaultValue().reconfigurable().commit();
    UINT64_ELEMENT(rowSchema).key("uint64").assignmentOptional().noDefaultValue().reconfigurable().commit();
    VECTOR_BOOL_ELEMENT(rowSchema).key("vectorBool").assignmentOptional().noDefaultValue().reconfigurable().commit();
    VECTOR_DOUBLE_ELEMENT(rowSchema)
          .key("vectorDouble")
          .assignmentOptional()
          .noDefaultValue()
          .reconfigurable()
          .commit();
    VECTOR_FLOAT_ELEMENT(rowSchema).key("vectorFloat").assignmentOptional().noDefaultValue().reconfigurable().commit();
    VECTOR_STRING_ELEMENT(rowSchema)
          .key("vectorString")
          .assignmentOptional()
          .noDefaultValue()
          .reconfigurable()
          .commit();
    VECTOR_INT8_ELEMENT(rowSchema).key("vectorInt8").assignmentOptional().noDefaultValue().reconfigurable().commit();
    VECTOR_INT16_ELEMENT(rowSchema).key("vectorInt16").assignmentOptional().noDefaultValue().reconfigurable().commit();
    VECTOR_INT32_ELEMENT(rowSchema).key("vectorInt32").assignmentOptional().noDefaultValue().reconfigurable().commit();
    VECTOR_INT64_ELEMENT(rowSchema).key("vectorInt64").assignmentOptional().noDefaultValue().reconfigurable().commit();
    VECTOR_UINT8_ELEMENT(rowSchema).key("vectorUint8").assignmentOptional().noDefaultValue().reconfigurable().commit();
    VECTOR_UINT16_ELEMENT(rowSchema)
          .key("vectorUint16")
          .assignmentOptional()
          .noDefaultValue()
          .reconfigurable()
          .commit();
    VECTOR_UINT32_ELEMENT(rowSchema)
          .key("vectorUint32")
          .assignmentOptional()
          .noDefaultValue()
          .reconfigurable()
          .commit();
    VECTOR_UINT64_ELEMENT(rowSchema)
          .key("vectorUint64")
          .assignmentOptional()
          .noDefaultValue()
          .reconfigurable()
          .commit();


    // Checks that a table with the sanitizable row schema above can
    // be instantiated.
    karabo::data::Schema tblSchema;
    EXPECT_NO_THROW(TABLE_ELEMENT(tblSchema)
                          .key("tbl")
                          .setColumns(rowSchema)
                          .assignmentOptional()
                          .defaultValue(vector<karabo::data::Hash>())
                          .reconfigurable()
                          .commit(););

    // Checks the synthesized defaults for each of the supported column types.
    const Schema& sanitRowSchema = tblSchema.getParameterHash().getAttribute<Schema>("tbl", KARABO_SCHEMA_ROW_SCHEMA);

    // For simple elements.
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("bool"));
    EXPECT_EQ(false, sanitRowSchema.getDefaultValue<bool>("bool"));
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("int8"));
    EXPECT_EQ(static_cast<signed char>(0), sanitRowSchema.getDefaultValue<signed char>("int8"));
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("int16"));
    EXPECT_EQ(static_cast<signed short>(0), sanitRowSchema.getDefaultValue<signed short>("int16"));
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("int32"));
    EXPECT_EQ(0, sanitRowSchema.getDefaultValue<int>("int32"));
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("int64"));
    EXPECT_EQ(0ll, sanitRowSchema.getDefaultValue<long long>("int64"));
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("uint8"));
    EXPECT_EQ(static_cast<unsigned char>(0), sanitRowSchema.getDefaultValue<unsigned char>("uint8"));
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("uint16"));
    EXPECT_EQ(static_cast<unsigned short>(0), sanitRowSchema.getDefaultValue<unsigned short>("uint16"));
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("uint32"));
    EXPECT_EQ(0u, sanitRowSchema.getDefaultValue<unsigned int>("uint32"));
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("uint64"));
    EXPECT_EQ(0ull, sanitRowSchema.getDefaultValue<unsigned long long>("uint64"));
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("float"));
    EXPECT_EQ(0.0f, sanitRowSchema.getDefaultValue<float>("float"));
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("double"));
    EXPECT_EQ(0.0, sanitRowSchema.getDefaultValue<double>("double"));
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("string"));
    EXPECT_TRUE(sanitRowSchema.getDefaultValue<string>("string").empty());
    // For vector elements.
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("vectorBool"));
    EXPECT_EQ(0ul, sanitRowSchema.getDefaultValue<vector<bool>>("vectorBool").size());
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("vectorInt8"));
    EXPECT_EQ(0ul, sanitRowSchema.getDefaultValue<vector<signed char>>("vectorInt8").size());
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("vectorInt16"));
    EXPECT_EQ(0ul, sanitRowSchema.getDefaultValue<vector<signed short>>("vectorInt16").size());
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("vectorInt32"));
    EXPECT_EQ(0ul, sanitRowSchema.getDefaultValue<vector<int>>("vectorInt32").size());
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("vectorInt64"));
    EXPECT_EQ(0ul, sanitRowSchema.getDefaultValue<vector<long long>>("vectorInt64").size());
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("vectorUint8"));
    EXPECT_EQ(0ul, sanitRowSchema.getDefaultValue<vector<unsigned char>>("vectorUint8").size());
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("vectorUint16"));
    EXPECT_EQ(0ul, sanitRowSchema.getDefaultValue<vector<unsigned short>>("vectorUint16").size());
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("vectorUint32"));
    EXPECT_EQ(0ul, sanitRowSchema.getDefaultValue<vector<unsigned int>>("vectorUint32").size());
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("vectorUint64"));
    EXPECT_EQ(0ul, sanitRowSchema.getDefaultValue<vector<unsigned long long>>("vectorUint64").size());
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("vectorFloat"));
    EXPECT_EQ(0ul, sanitRowSchema.getDefaultValue<vector<float>>("vectorFloat").size());
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("vectorDouble"));
    EXPECT_EQ(0ul, sanitRowSchema.getDefaultValue<vector<double>>("vectorDouble").size());
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("vectorString"));
    EXPECT_EQ(0ul, sanitRowSchema.getDefaultValue<vector<string>>("vectorString").size());

    // Checks that a read-only table with no default values for its columns
    // can be instantiated and that the missing default column values are
    // generated by the sanitization process.
    karabo::data::Schema roTblRowSchema;
    INT32_ELEMENT(roTblRowSchema).key("int").assignmentOptional().noDefaultValue().commit();
    STRING_ELEMENT(roTblRowSchema).key("str").assignmentOptional().noDefaultValue().commit();
    INT32_ELEMENT(roTblRowSchema)
          .key("intWithOptions")
          .assignmentOptional()
          .noDefaultValue()
          .options("0 1 2 3 4 5 6 7 8 9")
          .minInc(0)
          .minExc(-1)
          .maxInc(0)
          .maxExc(1)
          .commit();
    karabo::data::Schema roTblSchema;
    EXPECT_NO_THROW(TABLE_ELEMENT(roTblSchema)
                          .key("tbl")
                          .setColumns(roTblRowSchema)
                          .readOnly()
                          .initialValue(vector<Hash>())
                          .commit(););
    // Checks the synthesized defaults for the columns.
    const Schema& sanitRoRowSchema =
          roTblSchema.getParameterHash().getAttribute<Schema>("tbl", KARABO_SCHEMA_ROW_SCHEMA);
    EXPECT_TRUE(sanitRoRowSchema.hasDefaultValue("int"));
    EXPECT_EQ(0, sanitRoRowSchema.getDefaultValue<int>("int"));
    EXPECT_TRUE(sanitRoRowSchema.hasDefaultValue("str"));
    EXPECT_TRUE(sanitRoRowSchema.getDefaultValue<string>("str").empty());
    EXPECT_TRUE(sanitRoRowSchema.hasDefaultValue("intWithOptions"));
    EXPECT_EQ(0, sanitRoRowSchema.getDefaultValue<int>("intWithOptions"));

    // Checks that tables with unsanitizable row schemas regarding default
    // values throw - the default value that would be synthesized is either
    // outside the default range (single elements) or outside the allowed
    // cardinalities (vector elements).
    karabo::data::Schema invalidRowSchema;
    INT32_ELEMENT(invalidRowSchema)
          .key("intNoDefault")
          .assignmentOptional()
          .noDefaultValue()
          .minInc(1)
          .reconfigurable()
          .commit();
    karabo::data::Schema tblInvalidSchema;
    EXPECT_THROW(TABLE_ELEMENT(tblInvalidSchema)
                       .key("invalidSchema")
                       .setColumns(invalidRowSchema)
                       .assignmentOptional()
                       .defaultValue(vector<Hash>())
                       .reconfigurable()
                       .commit(),
                 karabo::data::ParameterException)
          << "Expected exception while creating table with column missing default value and 'minInc' greater than "
             "synthezisable default.";
    karabo::data::Schema invalidRowSchema2;
    INT32_ELEMENT(invalidRowSchema2)
          .key("intNoDefault")
          .assignmentOptional()
          .noDefaultValue()
          .maxInc(-1)
          .reconfigurable()
          .commit();
    karabo::data::Schema tblInvalidSchema2;
    EXPECT_THROW(TABLE_ELEMENT(tblInvalidSchema2)
                       .key("invalidSchema2")
                       .setColumns(invalidRowSchema2)
                       .assignmentOptional()
                       .defaultValue(vector<Hash>())
                       .reconfigurable()
                       .commit(),
                 karabo::data::ParameterException)
          << "Expected exception while creating table with column missing default value and 'maxInc' lower than "
             "synthezisable default.";
    karabo::data::Schema invalidRowSchema3;
    INT32_ELEMENT(invalidRowSchema3)
          .key("intNoDefault")
          .assignmentOptional()
          .noDefaultValue()
          .minExc(0)
          .reconfigurable()
          .commit();
    karabo::data::Schema tblInvalidSchema3;
    EXPECT_THROW(TABLE_ELEMENT(tblInvalidSchema3)
                       .key("invalidSchema3")
                       .setColumns(invalidRowSchema3)
                       .assignmentOptional()
                       .defaultValue(vector<Hash>())
                       .reconfigurable()
                       .commit(),
                 karabo::data::ParameterException)
          << "Expected exception while creating table with column missing default value and 'minExc' greater than "
             "synthezisable default.";
    karabo::data::Schema invalidRowSchema4;
    INT32_ELEMENT(invalidRowSchema4)
          .key("intNoDefault")
          .assignmentOptional()
          .noDefaultValue()
          .maxExc(0)
          .reconfigurable()
          .commit();
    karabo::data::Schema tblInvalidSchema4;
    EXPECT_THROW(TABLE_ELEMENT(tblInvalidSchema4)
                       .key("invalidSchema4")
                       .setColumns(invalidRowSchema4)
                       .assignmentOptional()
                       .defaultValue(vector<Hash>())
                       .reconfigurable()
                       .commit(),
                 karabo::data::ParameterException)
          << "Expected exception while creating table with column missing default value and 'maxExc' lower than "
             "synthezisable default.";
    karabo::data::Schema invalidRowSchema5;
    VECTOR_INT32_ELEMENT(invalidRowSchema5)
          .key("vectorIntNoDefault")
          .assignmentOptional()
          .noDefaultValue()
          .minSize(1)
          .reconfigurable()
          .commit();
    karabo::data::Schema tblInvalidSchema5;
    EXPECT_THROW(TABLE_ELEMENT(tblInvalidSchema5)
                       .key("invalidSchema5")
                       .setColumns(invalidRowSchema5)
                       .assignmentOptional()
                       .defaultValue(vector<Hash>())
                       .reconfigurable()
                       .commit(),
                 karabo::data::ParameterException)
          << "Expected exception while creating table with column missing default value and 'minSize' greater than "
             "size of synthezisable default.";
    karabo::data::Schema invalidRowSchema6;
    INT32_ELEMENT(invalidRowSchema6)
          .key("invalidInt")
          .options("1 2 3 4 5 6 7 8 9") // 0 not in options.
          .assignmentOptional()
          .noDefaultValue()
          .reconfigurable()
          .commit();
    karabo::data::Schema tblInvalidSchema6;
    EXPECT_THROW(TABLE_ELEMENT(tblInvalidSchema6)
                       .key("invalidSchema6")
                       .setColumns(invalidRowSchema6)
                       .assignmentOptional()
                       .defaultValue(vector<Hash>())
                       .reconfigurable()
                       .commit(),
                 karabo::data::ParameterException)
          << "Expected exception while creating table with column missing default value and sinthazisable default not "
             "in "
             "the 'options' set.";
}


TEST_F(TestSchema, testTableColUnsupportedType) {
    // Checks that a table with all supported column types
    // is accepted.
    Schema rowSchema;
    BOOL_ELEMENT(rowSchema).key("bool").assignmentOptional().defaultValue(false).reconfigurable().commit();
    DOUBLE_ELEMENT(rowSchema).key("double").assignmentOptional().defaultValue(0.0).reconfigurable().commit();
    FLOAT_ELEMENT(rowSchema).key("float").assignmentOptional().defaultValue(0.0f).reconfigurable().commit();
    STRING_ELEMENT(rowSchema).key("string").assignmentOptional().defaultValue("").reconfigurable().commit();
    INT8_ELEMENT(rowSchema).key("int8").assignmentOptional().defaultValue(0).reconfigurable().commit();
    INT16_ELEMENT(rowSchema).key("int16").assignmentOptional().defaultValue(0).reconfigurable().commit();
    INT32_ELEMENT(rowSchema).key("int32").assignmentOptional().defaultValue(0).reconfigurable().commit();
    INT64_ELEMENT(rowSchema).key("int64").assignmentOptional().defaultValue(0).reconfigurable().commit();
    UINT8_ELEMENT(rowSchema).key("uint8").assignmentOptional().defaultValue(0).reconfigurable().commit();
    UINT16_ELEMENT(rowSchema).key("uint16").assignmentOptional().defaultValue(0).reconfigurable().commit();
    UINT32_ELEMENT(rowSchema).key("uint32").assignmentOptional().defaultValue(0).reconfigurable().commit();
    UINT64_ELEMENT(rowSchema).key("uint64").assignmentOptional().defaultValue(0).reconfigurable().commit();
    VECTOR_BOOL_ELEMENT(rowSchema)
          .key("vectorBool")
          .assignmentOptional()
          .defaultValue(vector<bool>())
          .reconfigurable()
          .commit();
    VECTOR_DOUBLE_ELEMENT(rowSchema)
          .key("vectorDouble")
          .assignmentOptional()
          .defaultValue(vector<double>())
          .reconfigurable()
          .commit();
    VECTOR_FLOAT_ELEMENT(rowSchema)
          .key("vectorFloat")
          .assignmentOptional()
          .defaultValue(vector<float>())
          .reconfigurable()
          .commit();
    VECTOR_STRING_ELEMENT(rowSchema)
          .key("vectorString")
          .assignmentOptional()
          .defaultValue(vector<string>())
          .reconfigurable()
          .commit();
    VECTOR_INT8_ELEMENT(rowSchema)
          .key("vectorInt8")
          .assignmentOptional()
          .defaultValue(vector<signed char>())
          .reconfigurable()
          .commit();
    VECTOR_INT16_ELEMENT(rowSchema)
          .key("vectorInt16")
          .assignmentOptional()
          .defaultValue(vector<signed short>())
          .reconfigurable()
          .commit();
    VECTOR_INT32_ELEMENT(rowSchema)
          .key("vectorInt32")
          .assignmentOptional()
          .defaultValue(vector<int>())
          .reconfigurable()
          .commit();
    VECTOR_INT64_ELEMENT(rowSchema)
          .key("vectorInt64")
          .assignmentOptional()
          .defaultValue(vector<long long>())
          .reconfigurable()
          .commit();
    VECTOR_UINT8_ELEMENT(rowSchema)
          .key("vectorUint8")
          .assignmentOptional()
          .defaultValue(vector<unsigned char>())
          .reconfigurable()
          .commit();
    VECTOR_UINT16_ELEMENT(rowSchema)
          .key("vectorUint16")
          .assignmentOptional()
          .defaultValue(vector<unsigned short>())
          .reconfigurable()
          .commit();
    VECTOR_UINT32_ELEMENT(rowSchema)
          .key("vectorUint32")
          .assignmentOptional()
          .defaultValue(vector<unsigned int>())
          .reconfigurable()
          .commit();
    VECTOR_UINT64_ELEMENT(rowSchema)
          .key("vectorUint64")
          .assignmentOptional()
          .defaultValue(vector<unsigned long long>())
          .reconfigurable()
          .commit();

    Schema tblSchema;
    EXPECT_NO_THROW(TABLE_ELEMENT(tblSchema)
                          .key("tbl")
                          .setColumns(rowSchema)
                          .assignmentOptional()
                          .defaultValue(vector<Hash>())
                          .reconfigurable()
                          .commit(););

    // Checks that non-supported column types in the row schema of a
    // table leads to an exception being thrown.
    Schema invalidRowSchema;
    INT32_ELEMENT(invalidRowSchema).key("int32").assignmentOptional().defaultValue(1).reconfigurable().commit();
    // Non-supported col. type.
    VECTOR_CHAR_ELEMENT(invalidRowSchema)
          .key("vector_char")
          .assignmentOptional()
          .defaultValue(vector<char>())
          .reconfigurable()
          .commit();

    Schema invalidTblSchema;
    EXPECT_THROW(TABLE_ELEMENT(invalidTblSchema)
                       .key("invalidTbl")
                       .setColumns(invalidRowSchema)
                       .assignmentOptional()
                       .defaultValue(vector<Hash>())
                       .reconfigurable()
                       .commit(),
                 karabo::data::ParameterException)
          << "Expected exception trying to create VECTOR_CHAR table column.";
}


TEST_F(TestSchema, testTableColInitOnly) {
    // Checks that the 'initOnlyInt' column becomes a writable column
    // in the sanitized row schema when the table is reconfigurable.
    karabo::data::Schema rowSchema;
    INT32_ELEMENT(rowSchema).key("initOnlyInt").assignmentOptional().defaultValue(2).init().commit();
    STRING_ELEMENT(rowSchema).key("str").assignmentOptional().defaultValue("a string...").reconfigurable().commit();
    karabo::data::Schema tblSchema;
    TABLE_ELEMENT(tblSchema)
          .key("tbl")
          .setColumns(rowSchema)
          .assignmentOptional()
          .defaultValue(vector<Hash>())
          .reconfigurable()
          .commit();
    const Schema& sanitRowSchema = tblSchema.getParameterHash().getAttribute<Schema>("tbl", KARABO_SCHEMA_ROW_SCHEMA);
    EXPECT_TRUE(sanitRowSchema.hasAccessMode("initOnlyInt"));
    EXPECT_EQ(static_cast<int>(karabo::data::AccessType::WRITE), sanitRowSchema.getAccessMode("initOnlyInt"))
          << "Init-only columns of reconfigurable tables should have been converted to reconfigurable columns.";

    // Checks that the 'initOnlyInt' column becomes a read-only column
    // in the sanitized row schema when the table is read-only.
    karabo::data::Schema rowSchema2;
    INT32_ELEMENT(rowSchema2).key("initOnlyInt").assignmentOptional().defaultValue(2).init().commit();
    STRING_ELEMENT(rowSchema2).key("str").assignmentOptional().defaultValue("a string...").reconfigurable().commit();
    karabo::data::Schema ReadOnlyTblSchema;
    TABLE_ELEMENT(ReadOnlyTblSchema).key("tbl").setColumns(rowSchema2).readOnly().initialValue(vector<Hash>()).commit();
    const Schema& sanitRowSchema2 =
          ReadOnlyTblSchema.getParameterHash().getAttribute<Schema>("tbl", KARABO_SCHEMA_ROW_SCHEMA);
    EXPECT_TRUE(sanitRowSchema2.hasAccessMode("initOnlyInt"));
    EXPECT_EQ(static_cast<int>(karabo::data::AccessType::READ), sanitRowSchema2.getAccessMode("initOnlyInt"))
          << "Init-only columns of read-only tables should be converted to read-only columns.";

    // Checks that the 'initOnlyInt' column becomes a writable column
    // in the sanitized row schema when the table is init-only.
    karabo::data::Schema rowSchema3;
    INT32_ELEMENT(rowSchema3).key("initOnlyInt").assignmentOptional().defaultValue(2).init().commit();
    STRING_ELEMENT(rowSchema3).key("str").assignmentOptional().defaultValue("a string...").reconfigurable().commit();
    karabo::data::Schema initOnlyTblSchema;
    TABLE_ELEMENT(initOnlyTblSchema)
          .key("tbl")
          .setColumns(rowSchema)
          .assignmentOptional()
          .defaultValue(vector<Hash>())
          .init()
          .commit();
    const Schema& sanitRowSchema3 =
          initOnlyTblSchema.getParameterHash().getAttribute<Schema>("tbl", KARABO_SCHEMA_ROW_SCHEMA);
    EXPECT_TRUE(sanitRowSchema3.hasAccessMode("initOnlyInt"));
    EXPECT_EQ(static_cast<int>(karabo::data::AccessType::WRITE), sanitRowSchema3.getAccessMode("initOnlyInt"))
          << "Init-only columns of init-only tables should be converted to reconfigurable columns.";
}


TEST_F(TestSchema, testTableColWrongAccessMode) {
    karabo::data::Schema rowSchema;
    INT32_ELEMENT(rowSchema).key("int").assignmentOptional().defaultValue(2).reconfigurable().commit();
    STRING_ELEMENT(rowSchema).key("str").assignmentOptional().defaultValue("a string").reconfigurable().commit();
    karabo::data::Schema ReadOnlyTblSchema;
    TABLE_ELEMENT(ReadOnlyTblSchema).key("tbl").setColumns(rowSchema).readOnly().initialValue(vector<Hash>()).commit();
    const Schema& sanitRowSchema =
          ReadOnlyTblSchema.getParameterHash().getAttribute<Schema>("tbl", KARABO_SCHEMA_ROW_SCHEMA);
    // Checks that due to the table read-only access mode, the reconfigurable
    // columns became read-only columns, with their initial values set to the
    // default value of their previously reconfigurable forms.
    EXPECT_TRUE(sanitRowSchema.hasAccessMode("int"));
    EXPECT_EQ(static_cast<int>(karabo::data::AccessType::READ), sanitRowSchema.getAccessMode("int"))
          << "Reconfigurable column 'int' of read-only table 'tbl' should have been converted to read-only.";
    EXPECT_TRUE(sanitRowSchema.hasAccessMode("str"));
    EXPECT_EQ(static_cast<int>(karabo::data::AccessType::READ), sanitRowSchema.getAccessMode("str"))
          << "Reconfigurable column 'str' of read-only table 'tbl' should have been converted to read-only.";
    // The initialValue method of LeafElement actually sets the "defaultValue"
    // attribute, so we check by asserting that the default value has been
    // preserved by the row schema sanitization process.
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("int"));
    EXPECT_EQ(2, sanitRowSchema.getDefaultValue<int>("int"))
          << "Default value of column 'int', which was '2', should have been preserved.";
    EXPECT_TRUE(sanitRowSchema.hasDefaultValue("str"));
    EXPECT_STREQ("a string", sanitRowSchema.getDefaultValue<string>("str").c_str())
          << "Default value of column 'str', which was 'a string', should have been preserved.";
}
