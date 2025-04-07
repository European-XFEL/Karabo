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

#include "Schema_Test.hh"

#include <cppunit/TestAssert.h>

#include "karabo/data/schema/Validator.hh"
using namespace std;
using namespace karabo::data;
using namespace karabo::util;
using namespace configurationTest;

CPPUNIT_TEST_SUITE_REGISTRATION(Schema_Test);

// Enable CPPUNIT_ASSERT_EQUAL for vectors
namespace CppUnit {
    template <typename T>
    struct assertion_traits<std::vector<T>> {
        static bool equal(const std::vector<T>& a, const std::vector<T>& b) {
            return a == b;
        }

        static std::string toString(const std::vector<T>& p) {
            return karabo::data::toString(p);
        }
    };
} // namespace CppUnit


Schema_Test::Schema_Test() {}


Schema_Test::~Schema_Test() {}


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
        GraphicsRenderer::Pointer p = GraphicsRenderer::create(
              "GraphicsRenderer", Hash("shapes.Circle.radius", 0.5, "color", "red", "antiAlias", "true"));

    } catch (const karabo::data::Exception& e) {
        KARABO_LOG_FRAMEWORK_DEBUG << e;
        CPPUNIT_FAIL(e.detailedMsg());
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
    // all sub-elements of Node-element 'shapes' will have EXPERT level:
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Circle.shadowEnabled") == Schema::EXPERT);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Circle") == Schema::EXPERT);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Rectangle.b") == Schema::EXPERT);

    // but sub-element 'shapes.Rectangle.a' with higher level will keep its ADMIN level
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Rectangle.a") == Schema::ADMIN);

    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("antiAlias") == Schema::EXPERT);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("color") == Schema::USER);

    // check requiredAccesLevel set on leaves-elements in expectedParameters
    CPPUNIT_ASSERT(m_schema.getRequiredAccessLevel("exampleKey1") == Schema::USER);
    CPPUNIT_ASSERT(m_schema.getRequiredAccessLevel("exampleKey2") == Schema::OPERATOR);
    CPPUNIT_ASSERT(m_schema.getRequiredAccessLevel("exampleKey3") == Schema::EXPERT);
    CPPUNIT_ASSERT(m_schema.getRequiredAccessLevel("exampleKey4") == Schema::ADMIN);

    // default for readOnly element - OBSERVER
    CPPUNIT_ASSERT(m_schema.getRequiredAccessLevel("exampleKey5") == Schema::OBSERVER);

    // default for reconfigurable element - USER
    CPPUNIT_ASSERT(m_schema.getRequiredAccessLevel("sampleKey") == Schema::USER);

    Schema ose("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(ose);

    // check default requiredAccessLevel by elements : slot, vector, image
    CPPUNIT_ASSERT(ose.getRequiredAccessLevel("slotTest") == Schema::USER);       // SLOT
    CPPUNIT_ASSERT(ose.getRequiredAccessLevel("filename") == Schema::USER);       // reconfigurable PATH
    CPPUNIT_ASSERT(ose.getRequiredAccessLevel("testfile") == Schema::OBSERVER);   // readOnly STRING
    CPPUNIT_ASSERT(ose.getRequiredAccessLevel("vecIntReconfig") == Schema::USER); // reconfigurable VECTOR_INT32
    CPPUNIT_ASSERT(ose.getRequiredAccessLevel("vecInt") == Schema::OBSERVER);     // readOnly VECTOR_INT32
    CPPUNIT_ASSERT(ose.getRequiredAccessLevel("vecBool") == Schema::USER);        // init VECTOR_BOOL
    CPPUNIT_ASSERT(ose.getRequiredAccessLevel("image") == Schema::OBSERVER);      // IMAGE
}


void Schema_Test::testSetRequiredAccessLevel() {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    CPPUNIT_ASSERT(sch.getRequiredAccessLevel("image") == Schema::OBSERVER); // IMAGE (default level OBSERVER)
    sch.setRequiredAccessLevel("image", Schema::ADMIN);
    CPPUNIT_ASSERT(sch.getRequiredAccessLevel("image") == Schema::ADMIN); // IMAGE (changed by 'set' to ADMIN)

    Schema schema = GraphicsRenderer::getSchema("GraphicsRenderer");
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes") == Schema::EXPERT);
    // all sub-elements of Node-element 'shapes' will have EXPERT level:
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Circle.shadowEnabled") == Schema::EXPERT);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Circle") == Schema::EXPERT);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Rectangle.a") == Schema::ADMIN);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Rectangle.b") == Schema::EXPERT);


    // set top-Node to lower level 'Observer' and check that sub-elements keep previous higher level
    schema.setRequiredAccessLevel("shapes", Schema::OBSERVER);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes") == Schema::OBSERVER);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Circle.shadowEnabled") ==
                   Schema::USER); // default level for init-elem is 'user'
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Circle") == Schema::OBSERVER);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Rectangle.a") == Schema::ADMIN);
    CPPUNIT_ASSERT(schema.getRequiredAccessLevel("shapes.Rectangle.b") ==
                   Schema::USER); // default level for init-elem is 'user'
}


void Schema_Test::setUp() {
    try {
        m_schema = Schema("MyTest", Schema::AssemblyRules(READ | WRITE | INIT));
        TestStruct1::expectedParameters(m_schema);
    } catch (const karabo::data::Exception& e) {
        std::clog << "Error (Schema_Test::setUp): " << e << std::endl;
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
    CPPUNIT_ASSERT(m_schema.getTags("warray")[0] == "software");
    CPPUNIT_ASSERT(m_schema.getTags("rarray")[0] == "software");
}


void Schema_Test::testGetNodeType() {
    int nodeType = m_schema.getNodeType("exampleKey1");
    CPPUNIT_ASSERT(nodeType == Schema::LEAF);

    CPPUNIT_ASSERT(m_schema.getNodeType("exampleKey5") == Schema::LEAF);
    CPPUNIT_ASSERT(m_schema.getNodeType("warray") == Schema::LEAF);
    CPPUNIT_ASSERT(m_schema.getNodeType("rarray") == Schema::LEAF);
}


void Schema_Test::testGetValueType() {
    Types::ReferenceType valueType = m_schema.getValueType("exampleKey1");
    CPPUNIT_ASSERT(valueType == Types::STRING);

    CPPUNIT_ASSERT(m_schema.getValueType("exampleKey2") == Types::INT32);
    CPPUNIT_ASSERT(m_schema.getValueType("exampleKey3") == Types::UINT32);
    CPPUNIT_ASSERT(m_schema.getValueType("exampleKey4") == Types::FLOAT);
    CPPUNIT_ASSERT(m_schema.getValueType("exampleKey5") == Types::INT64);
    CPPUNIT_ASSERT(m_schema.getValueType("warray") == Types::BYTE_ARRAY);
    CPPUNIT_ASSERT(m_schema.getValueType("rarray") == Types::BYTE_ARRAY);
}


void Schema_Test::testKeyHasAlias() {
    CPPUNIT_ASSERT(m_schema.keyHasAlias("exampleKey1") == false);
    CPPUNIT_ASSERT(m_schema.keyHasAlias("exampleKey2") == true);
    CPPUNIT_ASSERT(m_schema.keyHasAlias("exampleKey3") == true);
    CPPUNIT_ASSERT(m_schema.keyHasAlias("exampleKey4") == true);
    CPPUNIT_ASSERT(m_schema.keyHasAlias("exampleKey5") == true);
    CPPUNIT_ASSERT(m_schema.keyHasAlias("warray") == true);
}


void Schema_Test::testAliasHasKey() {
    CPPUNIT_ASSERT(m_schema.aliasHasKey(10) == true);
    CPPUNIT_ASSERT(m_schema.aliasHasKey(5.5) == true);
    CPPUNIT_ASSERT(m_schema.aliasHasKey("exampleAlias4") == true);
    CPPUNIT_ASSERT(m_schema.aliasHasKey("aliasWriteArray") == true);

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
    CPPUNIT_ASSERT(m_schema.getAliasFromKey<string>("exampleKey4") == "exampleAlias4");
    CPPUNIT_ASSERT(m_schema.getAliasFromKey<string>("rarray") == "aliasReadArray");

    vector<int> aliasVec = m_schema.getAliasFromKey<vector<int>>("exampleKey5");
    CPPUNIT_ASSERT(aliasVec[0] == 10);
    CPPUNIT_ASSERT(aliasVec[1] == 20);
    CPPUNIT_ASSERT(aliasVec[2] == 30);
}


void Schema_Test::testGetKeyFromAlias() {
    CPPUNIT_ASSERT(m_schema.getKeyFromAlias(10) == "exampleKey2");
    CPPUNIT_ASSERT(m_schema.getKeyFromAlias(5.5) == "exampleKey3");
    CPPUNIT_ASSERT(m_schema.getKeyFromAlias("exampleAlias4") == "exampleKey4");
    CPPUNIT_ASSERT(m_schema.getKeyFromAlias("aliasWriteArray") == "warray");

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
    CPPUNIT_ASSERT(m_schema.getAliasAsString("rarray") == "aliasReadArray");

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
    // CPPUNIT_ASSERT(m_schema.getAccessMode("warray") == INIT);
    CPPUNIT_ASSERT(m_schema.getAccessMode("rarray") == READ);
}


void Schema_Test::testGetAssignment() {
    int assignment = m_schema.getAssignment("exampleKey1");
    CPPUNIT_ASSERT(assignment == Schema::OPTIONAL_PARAM);

    CPPUNIT_ASSERT(m_schema.getAssignment("exampleKey2") == Schema::OPTIONAL_PARAM);
    CPPUNIT_ASSERT(m_schema.getAssignment("exampleKey3") == Schema::MANDATORY_PARAM);
    CPPUNIT_ASSERT(m_schema.getAssignment("exampleKey4") == Schema::INTERNAL_PARAM);
    CPPUNIT_ASSERT(m_schema.getAssignment("exampleKey5") == Schema::OPTIONAL_PARAM);
    CPPUNIT_ASSERT(m_schema.getAssignment("warray") == Schema::OPTIONAL_PARAM);
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
    CPPUNIT_ASSERT_THROW(DOUBLE_ELEMENT(schema).key("some").options(std::vector<double>()),
                         karabo::data::ParameterException);

    CPPUNIT_ASSERT_THROW(INT32_ELEMENT(schema).key("someOther").options(""), karabo::data::ParameterException);
}


void Schema_Test::testGetDefaultValue() {
    string defaultValueKey1 = m_schema.getDefaultValue<string>("exampleKey1");
    CPPUNIT_ASSERT(defaultValueKey1 == "Navigation");

    int defaultValueKey2 = m_schema.getDefaultValue<int>("exampleKey2");
    CPPUNIT_ASSERT(defaultValueKey2 == 10);
    string defaultValueAsString2 = m_schema.getDefaultValueAs<string>("exampleKey2");
    CPPUNIT_ASSERT(defaultValueAsString2 == "10");

    long long defaultValue = m_schema.getDefaultValue<long long>("exampleKey5");
    CPPUNIT_ASSERT(defaultValue == 1442244);
    string defaultValueAsString5 = m_schema.getDefaultValueAs<string>("exampleKey5");
    CPPUNIT_ASSERT(defaultValueAsString5 == "1442244");

    CPPUNIT_ASSERT(m_schema.getDefaultValue<int>("sampleKey") ==
                   10); // Was set from string, but maintains correct data typing

    CPPUNIT_ASSERT(m_schema.getDefaultValueAs<string>("sampleKey") == "10");
    CPPUNIT_ASSERT(m_schema.getDefaultValueAs<int>("sampleKey") == 10);

    CPPUNIT_ASSERT(m_schema.hasDefaultValue("sampleKey2") == true);
    CPPUNIT_ASSERT(m_schema.getDefaultValue<int>("sampleKey2") == 0);

    CPPUNIT_ASSERT(m_schema.hasDefaultValue("warray") == true);
}


void Schema_Test::testInvalidDefaultsThrow() {
    karabo::data::Schema schInvalidDefault;
    CPPUNIT_ASSERT_THROW_MESSAGE("Expected exception adding int element with default smaller than minimum.",
                                 INT32_ELEMENT(schInvalidDefault)
                                       .key("int")
                                       .assignmentOptional()
                                       .defaultValue(1)
                                       .minExc(1)
                                       .reconfigurable()
                                       .commit(),
                                 karabo::data::ParameterException);
    CPPUNIT_ASSERT_THROW_MESSAGE("Expected exception adding float element with default smaller than minimum.",
                                 FLOAT_ELEMENT(schInvalidDefault)
                                       .key("float")
                                       .assignmentOptional()
                                       .defaultValue(0.9999f)
                                       .minInc(1.0f)
                                       .reconfigurable()
                                       .commit(),
                                 karabo::data::ParameterException);
    CPPUNIT_ASSERT_THROW_MESSAGE("Expected exception adding double element with default greater than maximum.",
                                 DOUBLE_ELEMENT(schInvalidDefault)
                                       .key("double")
                                       .assignmentOptional()
                                       .defaultValue(1.0)
                                       .maxExc(1.0)
                                       .reconfigurable()
                                       .commit(),
                                 karabo::data::ParameterException);
    CPPUNIT_ASSERT_THROW_MESSAGE("Expected exception adding uint32 element with default greater than maximum.",
                                 UINT32_ELEMENT(schInvalidDefault)
                                       .key("uint32")
                                       .assignmentOptional()
                                       .defaultValue(121u)
                                       .maxInc(120u)
                                       .reconfigurable()
                                       .commit(),
                                 karabo::data::ParameterException);
    CPPUNIT_ASSERT_THROW_MESSAGE("Expected exception adding string element with default not among options.",
                                 STRING_ELEMENT(schInvalidDefault)
                                       .key("string")
                                       .assignmentOptional()
                                       .defaultValue("NotAnOption")
                                       .options("OneOption AnotherOption")
                                       .reconfigurable()
                                       .commit(),
                                 karabo::data::ParameterException);
    CPPUNIT_ASSERT_THROW_MESSAGE("Expected exception adding int vector with size smaller than minSize.",
                                 VECTOR_INT32_ELEMENT(schInvalidDefault)
                                       .key("vectorInt")
                                       .assignmentOptional()
                                       .defaultValue(std::vector<int>{1, 2})
                                       .minSize(4)
                                       .reconfigurable()
                                       .commit(),
                                 karabo::data::ParameterException);
    CPPUNIT_ASSERT_THROW_MESSAGE("Expected exception adding bool vector with size greater than maxSize.",
                                 VECTOR_BOOL_ELEMENT(schInvalidDefault)
                                       .key("vectorBool")
                                       .assignmentOptional()
                                       .defaultValue(std::vector<bool>{true, false, true})
                                       .maxSize(2)
                                       .reconfigurable()
                                       .commit(),
                                 karabo::data::ParameterException);
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
            CPPUNIT_ASSERT_EQUAL(std::string("Int64DisplayType"), m_schema.getDisplayType(keys[i]));
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
    CPPUNIT_ASSERT(sch.hasClassId("slotTest") == true);
    CPPUNIT_ASSERT(sch.getClassId("slotTest") == "Slot");

    // An underscores should not be allowed in slot keys since they interfere with slots under a node.
    // But currently we cannot exclude them for backward compatibility of some Beckhoff devices.
    //    CPPUNIT_ASSERT_THROW(SLOT_ELEMENT(sch).key("slot_withunderscore"),
    //                         karabo::data::ParameterException);
    // But this one is forbidden due to interference with GUI client:
    CPPUNIT_ASSERT_THROW(SLOT_ELEMENT(sch).key("clear_namespace"), karabo::data::ParameterException);
}


void Schema_Test::testVectorElements() {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);

    vector<int> vecDef;
    vecDef.push_back(10);
    vecDef.push_back(20);
    vecDef.push_back(30);
    CPPUNIT_ASSERT(sch.getDefaultValue<vector<int>>("vecInt") == vecDef);
    CPPUNIT_ASSERT(sch.getValueType("vecInt") == Types::VECTOR_INT32);
    CPPUNIT_ASSERT(sch.isAccessReadOnly("vecInt") == true);
    CPPUNIT_ASSERT(sch.isAssignmentOptional("vecInt") == true);
    CPPUNIT_ASSERT(sch.hasDefaultValue("vecInt") == true);
    CPPUNIT_ASSERT_EQUAL(std::string("Curve"), sch.getDisplayType("vecInt"));

    CPPUNIT_ASSERT(sch.getValueType("vecDouble") == Types::VECTOR_DOUBLE);
    CPPUNIT_ASSERT(sch.isAccessReadOnly("vecDouble") == true);
    CPPUNIT_ASSERT(sch.isAssignmentOptional("vecDouble") == true);
    CPPUNIT_ASSERT_EQUAL(std::string("Curve"), sch.getDisplayType("vecDouble"));

    // readOnly element has default value (even if initialValue not specified) and default value is vector<double>() --
    // empty vector
    CPPUNIT_ASSERT(sch.hasDefaultValue("vecDouble") == true);
    CPPUNIT_ASSERT(sch.getDefaultValue<vector<double>>("vecDouble") == vector<double>());

    CPPUNIT_ASSERT(sch.isAccessReconfigurable("vecIntReconfig") == true);
    CPPUNIT_ASSERT(sch.isAssignmentOptional("vecIntReconfig") == true);
    CPPUNIT_ASSERT(sch.hasDefaultValue("vecIntReconfig") == true);
    CPPUNIT_ASSERT(sch.getDefaultValue<vector<int>>("vecIntReconfig") == vecDef);
    CPPUNIT_ASSERT_EQUAL(std::string("Curve"), sch.getDisplayType("vecIntReconfig"));

    CPPUNIT_ASSERT(sch.hasDefaultValue("vecIntReconfigStr") == true);

    vector<int> compare;
    compare.push_back(11);
    compare.push_back(22);
    compare.push_back(33);

    vector<int> defVecNew = sch.getDefaultValue<vector<int>>("vecIntReconfigStr");
    CPPUNIT_ASSERT(defVecNew == compare);

    vector<double> comp;
    comp.push_back(1.1);
    comp.push_back(2.2);
    comp.push_back(3.3);

    vector<double> defDVecNew = sch.getDefaultValue<vector<double>>("vecDoubleReconfigStr");
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
    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<unsigned long long>>("arrBool.shape") == shapeVec);
    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<unsigned long long>>("arrInt8.shape") == shapeVec);
    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<unsigned long long>>("arrUInt16.shape") == shapeVec);
    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<unsigned long long>>("arrFloat.shape") == shapeVec);
    shapeVec.push_back(-1);
    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<unsigned long long>>("arrDouble.shape") == shapeVec);

    std::vector<unsigned long long> undefShapeVec;
    undefShapeVec.push_back(0);
    undefShapeVec.push_back(3);
    undefShapeVec.push_back(0);
    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<unsigned long long>>("arrUndefined.shape") == undefShapeVec);

    CPPUNIT_ASSERT_EQUAL(static_cast<int>(karabo::data::Types::BOOL), sch.getDefaultValue<int>("arrBool.type"));
    CPPUNIT_ASSERT_EQUAL(static_cast<int>(karabo::data::Types::INT8), sch.getDefaultValue<int>("arrInt8.type"));
    CPPUNIT_ASSERT_EQUAL(static_cast<int>(karabo::data::Types::UINT16), sch.getDefaultValue<int>("arrUInt16.type"));
    CPPUNIT_ASSERT_EQUAL(static_cast<int>(karabo::data::Types::FLOAT), sch.getDefaultValue<int>("arrFloat.type"));
    CPPUNIT_ASSERT_EQUAL(static_cast<int>(karabo::data::Types::UNKNOWN), sch.getDefaultValue<int>("arrUndefined.type"));

    CPPUNIT_ASSERT(sch.isAccessReadOnly("arrBool") == true);
    CPPUNIT_ASSERT(sch.isAccessReadOnly("arrInt8") == true);
    CPPUNIT_ASSERT(sch.isAccessReadOnly("arrUInt16") == true);
    CPPUNIT_ASSERT(sch.isAccessReadOnly("arrFloat") == true);
    CPPUNIT_ASSERT(sch.isAccessReadOnly("arrDouble") == true);

    // Just choose one array to test NDArray discovery:
    CPPUNIT_ASSERT(sch.isNode("arrUInt16"));
    CPPUNIT_ASSERT(sch.isCustomNode("arrUInt16"));
    CPPUNIT_ASSERT_EQUAL(sch.getCustomNodeClass("arrUInt16"), std::string("NDArray"));
    // Before 2.7.2, NDARRAY_ELEMENTs (as custom nodes) erroneously created this attribute in the Schema:
    CPPUNIT_ASSERT(!sch.getParameterHash().hasAttribute("arrUInt16", "__classId"));
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

    // A slot element
    CPPUNIT_ASSERT(!sch.isCustomNode("slotTest"));
    // A TableElement
    CPPUNIT_ASSERT(!sch.isCustomNode("testTable"));
    // A String Element
    CPPUNIT_ASSERT(!sch.isCustomNode("filename"));
    // A vector element
    CPPUNIT_ASSERT(!sch.isCustomNode("vecInt"));

    Schema schemaWithChoice("test");
    GraphicsRenderer1::expectedParameters(schemaWithChoice);
    // A ChoiceOfNodes
    CPPUNIT_ASSERT(!schemaWithChoice.isCustomNode("shapes"));
    // ... and its choices
    CPPUNIT_ASSERT(!schemaWithChoice.isCustomNode("shapes.circle"));
    CPPUNIT_ASSERT(!schemaWithChoice.isCustomNode("shapes.rectangle"));
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
    {
        Schema schema = Configurator<TestStruct1>::getSchema("TestStruct2");

        CPPUNIT_ASSERT(schema.getAliasFromKey<int>("exampleKey2") == 20);
        CPPUNIT_ASSERT(schema.getAliasFromKey<int>("exampleKey3") == 30);
    }

    // Check that overwrite element does not accept non-existing paths
    {
        Schema schema;
        CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(schema).key("non_existing_path"), karabo::data::ParameterException);
    }

    // Check that overwrite element complains if key(...) is not called first (too lazy to test all cases...)
    {
        Schema schema;
        CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(schema).commit(), karabo::data::LogicException);
    }

    testOverwriteElementScalarDefault();
    testOverwriteElementVectorDefault();
    testOverwriteElementMinMax();
    testOverwriteElementMinMaxVector();
}

void Schema_Test::testOverwriteElementScalarDefault() {
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
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema)
                               .key("uint16")
                               .setNewDefaultValue(static_cast<unsigned short>(2u))
                               .commit(), // options are 1 and 5
                         karabo::data::LogicException);
    workSchema = schema; // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema).key("uint16").setNewOptions("1, 2").commit(),
                         karabo::data::LogicException); // default is 5

    // int32
    workSchema = schema; // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema)
                               .key("int32")
                               .setNewDefaultValue(2) // options are 3 and -5
                               .commit(),
                         karabo::data::LogicException);
    workSchema = schema; // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema).key("int32").setNewOptions("1, 2").commit(),
                         karabo::data::LogicException); // default is -5

    // double
    workSchema = schema; // start clean
    CPPUNIT_ASSERT_THROW(
          OVERWRITE_ELEMENT(workSchema).key("double").setNewDefaultValue(2.1).commit(), // options are 2.2, -3.3 and 0.
          karabo::data::LogicException);
    workSchema = schema; // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema).key("double").setNewOptions("1.1, 2.2").commit(),
                         karabo::data::LogicException); // default is 0.

    // string
    workSchema = schema; // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema)
                               .key("string")
                               .setNewDefaultValue("further")
                               .commit(), // options are "default" and "other"
                         karabo::data::LogicException);
    workSchema = schema; // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema).key("string").setNewOptions("one, another").commit(),
                         karabo::data::LogicException); // default is "default"

    // State
    workSchema = schema; // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema)
                               .key("state")
                               .setNewDefaultValue(State::UNKNOWN)
                               .commit(), // options are INIT, ON, CHANGING
                         karabo::data::LogicException);
    workSchema = schema; // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema)
                               .key("state")
                               .setNewOptions(std::vector<State>{State::ON, State::ACQUIRING})
                               .commit(),
                         karabo::data::LogicException); // default is INIT

    // Check inclusive minimum and maximum: -5 <= x <= 5
    workSchema = schema;
    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(workSchema) //
                                  .key("int64Inc")
                                  .setNewDefaultValue(-5LL)
                                  .commit());

    workSchema = schema;                                  // start clean
    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(workSchema) //
                                  .key("int64Inc")
                                  .setNewDefaultValue(5LL)
                                  .commit());

    workSchema = schema;                               // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema) //
                               .key("int64Inc")
                               .setNewDefaultValue(-6LL)
                               .commit(),
                         karabo::data::ParameterException);

    workSchema = schema;                               // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema) //
                               .key("int64Inc")
                               .setNewDefaultValue(6LL)
                               .commit(),
                         karabo::data::ParameterException);

    // Check exclusive minimum and maximum: -5 < x < 5
    workSchema = schema;                                  // start clean
    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(workSchema) //
                                  .key("int64Exc")
                                  .setNewDefaultValue(-4LL)
                                  .commit());

    workSchema = schema;                                  // start clean
    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(workSchema) //
                                  .key("int64Exc")
                                  .setNewDefaultValue(4LL)
                                  .commit());
    workSchema = schema;                               // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema) //
                               .key("int64Exc")
                               .setNewDefaultValue(-5LL)
                               .commit(),
                         karabo::data::ParameterException);

    workSchema = schema;                               // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema) //
                               .key("int64Exc")
                               .setNewDefaultValue(5LL)
                               .commit(),
                         karabo::data::ParameterException);

    // We skip explicit testing of BOOL, CHAR, [U]INT8, INT16, UINT32, [U]INT64, FLOAT
}


void Schema_Test::testOverwriteElementVectorDefault() {
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

    Schema workSchema;

#define TEST_ARRAY_BOUNDARIES(TYPE, NAME)                                                                              \
    workSchema = schema;                                                                                               \
    CPPUNIT_ASSERT_THROW(                                                                                              \
          OVERWRITE_ELEMENT(workSchema).key(NAME).setNewDefaultValue(std::vector<TYPE>(vector_min_size - 1)).commit(), \
          karabo::data::ParameterException);                                                                           \
    workSchema = schema;                                                                                               \
    CPPUNIT_ASSERT_THROW(                                                                                              \
          OVERWRITE_ELEMENT(workSchema).key(NAME).setNewDefaultValue(std::vector<TYPE>(vector_max_size + 1)).commit(), \
          karabo::data::ParameterException);                                                                           \
    workSchema = schema;                                                                                               \
    CPPUNIT_ASSERT_NO_THROW(                                                                                           \
          OVERWRITE_ELEMENT(workSchema).key(NAME).setNewDefaultValue(std::vector<TYPE>(vector_min_size)).commit());    \
    workSchema = schema;                                                                                               \
    CPPUNIT_ASSERT_NO_THROW(                                                                                           \
          OVERWRITE_ELEMENT(workSchema).key(NAME).setNewDefaultValue(std::vector<TYPE>(vector_max_size)).commit());    \
    workSchema = schema;                                                                                               \
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema).key(NAME).setNewMinSize(vector_default_size + 1).commit(),      \
                         karabo::data::ParameterException);                                                            \
    workSchema = schema;                                                                                               \
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema).key(NAME).setNewMaxSize(vector_default_size - 1).commit(),      \
                         karabo::data::ParameterException)


    // See macro definition above. We check boundary cases, one case less
    // than minimum and one greater than maximum. Also, one test that
    // changes the minimum value, and another that changes the maximum value.

    TEST_ARRAY_BOUNDARIES(bool, "boolVector");
    TEST_ARRAY_BOUNDARIES(char, "charVector");
    TEST_ARRAY_BOUNDARIES(signed char, "int8Vector");
    TEST_ARRAY_BOUNDARIES(unsigned char, "uint8Vector");
    TEST_ARRAY_BOUNDARIES(short, "int16Vector");
    TEST_ARRAY_BOUNDARIES(unsigned short, "uint16Vector");
    TEST_ARRAY_BOUNDARIES(int, "int32Vector");
    TEST_ARRAY_BOUNDARIES(unsigned int, "uint32Vector");
    TEST_ARRAY_BOUNDARIES(long long, "int64Vector");
    TEST_ARRAY_BOUNDARIES(unsigned long long, "uint64Vector");
    TEST_ARRAY_BOUNDARIES(float, "floatVector");
    TEST_ARRAY_BOUNDARIES(double, "doubleVector");
    TEST_ARRAY_BOUNDARIES(std::string, "stringVector");

#undef TEST_ARRAY_BOUNDARIES

    // We check boundary cases, one case less than minimum and one greater
    // than maximum. Also, one test that changes the minimum value, and
    // another that changes the maximum value.
    //
    // 'row' is defined with 'tableElement' above, to make code easier to
    // read
    workSchema = schema;                               // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema) //
                               .key("tableElement")
                               .setNewDefaultValue(std::vector<Hash>{table_min_size - 1, row})
                               .commit(),
                         karabo::data::ParameterException);
    workSchema = schema;                               // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema) //
                               .key("tableElement")
                               .setNewDefaultValue(std::vector<Hash>{table_max_size + 1, row})
                               .commit(),
                         karabo::data::ParameterException);
    workSchema = schema;                                  // start clean
    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(workSchema) //
                                  .key("tableElement")
                                  .setNewDefaultValue(std::vector<Hash>{table_min_size, row})
                                  .commit());
    workSchema = schema;                                  // start clean
    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(workSchema) //
                                  .key("tableElement")
                                  .setNewDefaultValue(std::vector<Hash>{table_max_size, row})
                                  .commit());
    workSchema = schema;                               // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema) //
                               .key("tableElement")
                               .setNewMinSize(table_default_size + 1)
                               .commit(),
                         karabo::data::ParameterException);
    workSchema = schema;                               // start clean
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema) //
                               .key("tableElement")
                               .setNewMaxSize(table_default_size - 1)
                               .commit(),
                         karabo::data::ParameterException);
}

void Schema_Test::testOverwriteElementMinMax() {
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

    Schema workSchema;

#define CHECK_BOUNDARIES(NAME)                                                                                     \
    workSchema = schema;                                                                                           \
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME "Exc").setNewMaxExc(minimum).commit(),            \
                         karabo::data::ParameterException);                                                        \
    workSchema = schema;                                                                                           \
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME "Exc").setNewMinExc(maximum).commit(),            \
                         karabo::data::ParameterException);                                                        \
    workSchema = schema;                                                                                           \
    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME "Exc").setNewMaxExc(minimum + 1).commit());    \
    workSchema = schema;                                                                                           \
    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME "Exc").setNewMinExc(maximum - 1).commit());    \
                                                                                                                   \
    workSchema = schema;                                                                                           \
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME "Inc").setNewMaxInc(minimum - 1).commit(),        \
                         karabo::data::ParameterException);                                                        \
    workSchema = schema;                                                                                           \
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME "Inc").setNewMinInc(maximum + 1).commit(),        \
                         karabo::data::ParameterException);                                                        \
    workSchema = schema;                                                                                           \
    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME "Inc").setNewMaxInc(minimum).commit());        \
    workSchema = schema;                                                                                           \
    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME "Inc").setNewMinInc(maximum).commit());        \
                                                                                                                   \
    workSchema = schema;                                                                                           \
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME "IncExc").setNewMaxExc(minimum).commit(),         \
                         karabo::data::ParameterException);                                                        \
    workSchema = schema;                                                                                           \
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME "ExcInc").setNewMinExc(maximum).commit(),         \
                         karabo::data::ParameterException);                                                        \
    workSchema = schema;                                                                                           \
    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME "IncExc").setNewMaxExc(minimum + 1).commit()); \
    workSchema = schema;                                                                                           \
    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME "ExcInc").setNewMinExc(maximum - 1).commit())

    CHECK_BOUNDARIES(int8);
    CHECK_BOUNDARIES(uint8);
    CHECK_BOUNDARIES(int16);
    CHECK_BOUNDARIES(uint16);
    CHECK_BOUNDARIES(int32);
    CHECK_BOUNDARIES(uint32);
    CHECK_BOUNDARIES(int64);
    CHECK_BOUNDARIES(uint64);
    CHECK_BOUNDARIES(_float);
    CHECK_BOUNDARIES(_double);

#undef CHECK_BOUNDARIES
}

void Schema_Test::testOverwriteElementMinMaxVector() {
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

    Schema workSchema;

#define CHECK_BOUNDARIES(NAME)                                                                         \
    workSchema = schema;                                                                               \
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME).setNewMinSize(maximum + 1).commit(), \
                         karabo::data::ParameterException);                                            \
    workSchema = schema;                                                                               \
    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME).setNewMaxSize(minimum - 1).commit(), \
                         karabo::data::ParameterException);                                            \
    workSchema = schema;                                                                               \
    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME).setNewMinSize(maximum).commit()); \
    workSchema = schema;                                                                               \
    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(workSchema).key(#NAME).setNewMaxSize(minimum).commit())


    CHECK_BOUNDARIES(int8);
    CHECK_BOUNDARIES(uint8);
    CHECK_BOUNDARIES(int16);
    CHECK_BOUNDARIES(uint16);
    CHECK_BOUNDARIES(int32);
    CHECK_BOUNDARIES(uint32);
    CHECK_BOUNDARIES(int64);
    CHECK_BOUNDARIES(uint64);
    CHECK_BOUNDARIES(_float);
    CHECK_BOUNDARIES(_double);

#undef CHECK_BOUNDARIES
}


void Schema_Test::testMerge() {
    Schema schema =
          Configurator<SchemaNodeElements>::getSchema("SchemaNodeElements", Schema::AssemblyRules(READ | WRITE | INIT));
    CPPUNIT_ASSERT(schema.getDefaultValue<unsigned int>("monitor.count") == 777);

    Schema schema2("SchemaNodeElements", Schema::AssemblyRules(READ | WRITE | INIT));
    SchemaNodeInjected::expectedParameters(schema2);
    CPPUNIT_ASSERT(schema2.getDefaultValue<float>("monitor.stats.d1") == 3.1415f);

    schema.merge(schema2);
    CPPUNIT_ASSERT(schema.getDefaultValue<unsigned int>("monitor.count") == 777);
    CPPUNIT_ASSERT(schema.getDefaultValue<float>("monitor.stats.d1") == 3.1415f);
}


void Schema_Test::testInvalidNodes() {
    Schema schema("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(schema);

    // Placing an element under a leaf is not allowed
    CPPUNIT_ASSERT(schema.has("vecDouble"));
    CPPUNIT_ASSERT(!schema.has("vecDouble.uint16"));
    CPPUNIT_ASSERT_THROW(
          UINT16_ELEMENT(schema)
                .key("vecDouble.uint16")
                .description("This element's key refers to a parent that is not a node and thus triggers an exception")
                .readOnly()
                .commit(),
          karabo::data::LogicException);

    // A node should not be created automatically
    CPPUNIT_ASSERT(!schema.has("nonExistingNode"));
    CPPUNIT_ASSERT_THROW(INT16_ELEMENT(schema)
                               .key("nonExistingNode.int16")
                               .description("This element refers to a non-existing node and thus triggers an exception")
                               .readOnly()
                               .commit(),
                         karabo::data::LogicException);

    // Empty strings are forbidden as keys:
    CPPUNIT_ASSERT_THROW(INT32_ELEMENT(schema).key("").description("Empty key is forbidden"),
                         karabo::data::ParameterException);

    // Spaces in keys are forbidden:
    CPPUNIT_ASSERT_THROW(INT8_ELEMENT(schema).key("constains space").description("Space inside a key is forbidden"),
                         karabo::data::ParameterException);
}


void Schema_Test::testOverwriteRestrictions() {
    Schema schema;
    STATE_ELEMENT(schema).key("state").commit();

    CPPUNIT_ASSERT_THROW(OVERWRITE_ELEMENT(schema).key("state").setNewMinInc(100).commit(),
                         karabo::data::LogicException);
}


void Schema_Test::testOverwriteTags() {
    Schema schema;
    INT32_ELEMENT(schema).key("taggedProp").tags("greet tip").readOnly().commit();

    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(schema).key("taggedProp").setNewTags({"doff"}).commit());

    std::vector<std::string> tags = schema.getTags("taggedProp");
    CPPUNIT_ASSERT_EQUAL(1ul, tags.size());
    CPPUNIT_ASSERT_EQUAL(std::string("doff"), tags[0]);

    CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(schema).key("taggedProp").setNewTags({"left", "right"}).commit());

    tags = schema.getTags("taggedProp");
    CPPUNIT_ASSERT_EQUAL(2ul, tags.size());
    CPPUNIT_ASSERT_EQUAL(std::string("left"), tags[0]);
    CPPUNIT_ASSERT_EQUAL(std::string("right"), tags[1]);
}


void Schema_Test::testTagsFromVector() {
    Schema schema;
    INT32_ELEMENT(schema).key("taggedProp").tags(std::vector<std::string>({"greet", "doff"})).readOnly().commit();

    const std::vector<std::string>& tags = schema.getTags("taggedProp");
    CPPUNIT_ASSERT_EQUAL(2ul, tags.size());
    CPPUNIT_ASSERT_EQUAL(std::string("greet"), tags[0]);
    CPPUNIT_ASSERT_EQUAL(std::string("doff"), tags[1]);
}


void Schema_Test::testOverwriteRestrictionsForOptions() {
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
        CPPUNIT_ASSERT_EQUAL(range1.size(), 2ul);
        CPPUNIT_ASSERT_EQUAL(range1[0], 0);
        CPPUNIT_ASSERT_EQUAL(range1[1], 1);

        CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(schema).key("range").setNewOptions("0,1,2").commit());

        const auto& range2 = schema.getOptions<int>("range");
        CPPUNIT_ASSERT_EQUAL(range2.size(), 3ul);
        CPPUNIT_ASSERT_EQUAL(range2[0], 0);
        CPPUNIT_ASSERT_EQUAL(range2[1], 1);
        CPPUNIT_ASSERT_EQUAL(range2[2], 2);
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
        CPPUNIT_ASSERT_EQUAL(range1.size(), 2ul);
        CPPUNIT_ASSERT_EQUAL(range1[0], 0);
        CPPUNIT_ASSERT_EQUAL(range1[1], 1);

        CPPUNIT_ASSERT_NO_THROW(OVERWRITE_ELEMENT(schema).key("node.range").setNewOptions("0,1,2").commit());

        const auto& range2 = schema.getOptions<int>("node.range");
        CPPUNIT_ASSERT_EQUAL(range2.size(), 3ul);
        CPPUNIT_ASSERT_EQUAL(range2[0], 0);
        CPPUNIT_ASSERT_EQUAL(range2[1], 1);
        CPPUNIT_ASSERT_EQUAL(range2[2], 2);
    }
}


void Schema_Test::testStateAndAlarmSets() {
    Schema schema;

    STRING_ELEMENT(schema).key("string").readOnly().commit();

    STATE_ELEMENT(schema).key("state").commit();

    ALARM_ELEMENT(schema).key("alarm").commit();

    Hash h("string", "abc");
    Validator val;
    Hash h_out;
    std::pair<bool, std::string> r = val.validate(schema, h, h_out);
    CPPUNIT_ASSERT(r.first == true); // should validate
    h.set("state", "abc");
    r = val.validate(schema, h, h_out);
    CPPUNIT_ASSERT(r.first == false); // should not validate as we are setting string to state
    Hash h2("alarm", "abc");
    r = val.validate(schema, h2, h_out);
    CPPUNIT_ASSERT(r.first == false); // should not validate as we are setting string to alarm
    Hash::Node& n = h.set("state", "UNKNOWN");
    n.setAttribute(KARABO_INDICATE_STATE_SET, true);
    r = val.validate(schema, h, h_out);
    CPPUNIT_ASSERT(r.first == true); // should validate as we faked updateState
    Hash::Node& n2 = h2.set("alarm", "none");
    n2.setAttribute(KARABO_INDICATE_ALARM_SET, true);
    r = val.validate(schema, h2, h_out);
    CPPUNIT_ASSERT(r.first == true); // should validate as we faked setAlarmCondition
}


void Schema_Test::testSubSchema() {
    Schema schema("test");
    GraphicsRenderer1::expectedParameters(schema);
    const int alias = 1;
    OVERWRITE_ELEMENT(schema).key("shapes.rectangle.c").setNewAlias(alias).commit();
    {
        Schema sub = schema.subSchema("shapes.rectangle");
        CPPUNIT_ASSERT(sub.has("b"));
        CPPUNIT_ASSERT(sub.has("c"));
        CPPUNIT_ASSERT(sub.keyHasAlias("c"));
        CPPUNIT_ASSERT(sub.aliasHasKey(alias));
        CPPUNIT_ASSERT_EQUAL(std::string("c"), sub.getKeyFromAlias(alias));
        CPPUNIT_ASSERT_EQUAL(alias, sub.getAliasFromKey<int>("c"));
        CPPUNIT_ASSERT_EQUAL(std::string(), sub.getRootName()); // we have another hierarchy level, not match anymore
    }
    {
        Schema sub = schema.subSchema("shapes.rectangle", "b"); // filter for tag "b"
        CPPUNIT_ASSERT(sub.has("b"));
        CPPUNIT_ASSERT(!sub.has("c"));
        CPPUNIT_ASSERT(!sub.aliasHasKey(alias));
    }

    // Now testing 'by rules':
    {
        Schema::AssemblyRules rules(READ | WRITE | INIT); // i.e. everything
        const Schema sub = schema.subSchemaByRules(rules);
        // Everything is in:
        std::vector<std::string> finalPaths;
        sub.getParameterHash().getPaths(finalPaths);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(7), finalPaths.size());
    }

    {
        Schema::AssemblyRules rules(READ | WRITE | INIT, "ON"); // i.e. required state ON or non-defined
        const Schema sub = schema.subSchemaByRules(rules);
        CPPUNIT_ASSERT(!sub.has("color"));

        // But all else since only "color" is reconfigurable for state OFF
        std::vector<std::string> finalPaths;
        sub.getParameterHash().getPaths(finalPaths);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(6), finalPaths.size());

        // Check rules are preserved
        const Schema::AssemblyRules subRules = sub.getAssemblyRules();
        CPPUNIT_ASSERT_EQUAL(rules.m_accessLevel, subRules.m_accessLevel);
        CPPUNIT_ASSERT_EQUAL(rules.m_accessMode, subRules.m_accessMode);
        CPPUNIT_ASSERT_EQUAL(rules.m_state, subRules.m_state);
        // ...and alias as well
        CPPUNIT_ASSERT(sub.keyHasAlias("shapes.rectangle.c"));
        CPPUNIT_ASSERT(sub.aliasHasKey(alias));
        CPPUNIT_ASSERT_EQUAL(std::string("shapes.rectangle.c"), sub.getKeyFromAlias(alias));
        CPPUNIT_ASSERT_EQUAL(alias, sub.getAliasFromKey<int>("shapes.rectangle.c"));
        CPPUNIT_ASSERT_EQUAL(schema.getRootName(), sub.getRootName());
    }

    {
        Schema::AssemblyRules rules(READ | WRITE | INIT, "", Schema::OPERATOR);
        const Schema sub = schema.subSchemaByRules(rules);
        CPPUNIT_ASSERT(!sub.has("antiAlias"));

        // But all else is left since "antiAlias" is the only expert access level (defaults are user or observer)
        std::vector<std::string> finalPaths;
        sub.getParameterHash().getPaths(finalPaths);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(6), finalPaths.size());
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
        CPPUNIT_ASSERT(sub.has("shapes.circle.radius"));

        // All else is WRITE (i.e. reconfigurable))
        std::vector<std::string> finalPaths;
        sub.getParameterHash().getPaths(finalPaths);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(toString(sub), static_cast<size_t>(4), finalPaths.size());
    }

    // Test subSchemaByPaths
    {
        // Extend the schema to test options
        OVERWRITE_ELEMENT(schema).key("color").setNewOptions({"red", "yellow", "blue"}).commit();
        const Schema sub = schema.subSchemaByPaths({"color",                // first level, but endpoint
                                                    "shapes.circle",        // a node among choices
                                                    "shapes.rectangle.c"}); // end point within a choice
        // Check that all the paths (and no more) are there
        CPPUNIT_ASSERT(sub.has("color"));
        CPPUNIT_ASSERT(sub.has("shapes"));
        CPPUNIT_ASSERT(sub.has("shapes.circle"));
        CPPUNIT_ASSERT(sub.has("shapes.circle.radius"));
        CPPUNIT_ASSERT(sub.has("shapes.rectangle"));
        CPPUNIT_ASSERT(!sub.has("shapes.rectangle.b"));
        CPPUNIT_ASSERT(sub.has("shapes.rectangle.c"));
        CPPUNIT_ASSERT_EQUAL(3ul, sub.getPaths().size());
        CPPUNIT_ASSERT_GREATER(3ul, schema.getPaths().size());

        // Check whether attributes are there
        CPPUNIT_ASSERT_EQUAL(std::string("red"), sub.getDefaultValue<std::string>("color"));
        CPPUNIT_ASSERT_EQUAL(std::vector<std::string>(1, "prop"), sub.getTags("color"));
        CPPUNIT_ASSERT_EQUAL(std::string("Color"), sub.getDisplayedName("color"));
        CPPUNIT_ASSERT_EQUAL(std::vector<std::string>({"red", "yellow", "blue"}), sub.getOptions<std::string>("color"));
        CPPUNIT_ASSERT(sub.isAssignmentOptional("color"));
        CPPUNIT_ASSERT(sub.isAccessReconfigurable("color"));

        CPPUNIT_ASSERT_EQUAL(std::string("circle"), sub.getDefaultValue<std::string>("shapes"));
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(Unit::METER), sub.getUnit("shapes.circle.radius"));
        CPPUNIT_ASSERT_EQUAL(std::string("m"), sub.getUnitSymbol("shapes.circle.radius"));
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(MetricPrefix::MILLI), sub.getMetricPrefix("shapes.circle.radius"));
        CPPUNIT_ASSERT_EQUAL(std::string("m"), sub.getMetricPrefixSymbol("shapes.circle.radius"));
        CPPUNIT_ASSERT_EQUAL(0.f, sub.getMinExc<float>("shapes.circle.radius"));
        CPPUNIT_ASSERT_EQUAL(100.f, sub.getMaxExc<float>("shapes.circle.radius"));

        // Test alias and root name
        CPPUNIT_ASSERT(sub.keyHasAlias("shapes.rectangle.c"));
        CPPUNIT_ASSERT(sub.aliasHasKey(alias));
        CPPUNIT_ASSERT_EQUAL(std::string("shapes.rectangle.c"), sub.getKeyFromAlias(alias));
        CPPUNIT_ASSERT_EQUAL(alias, sub.getAliasFromKey<int>("shapes.rectangle.c"));
        CPPUNIT_ASSERT_EQUAL(schema.getRootName(), sub.getRootName());
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
        CPPUNIT_ASSERT_THROW(schema.setDaqDataType("antiAlias", DaqDataType::TRAINMASTER),
                             karabo::data::ParameterException);
    }
}


void Schema_Test::testDaqPolicy() {
    // legacy behavior: save everything if not specified otherwise
    {
        Schema schema;
        STRING_ELEMENT(schema).key("string1").daqPolicy(DAQPolicy::SAVE).readOnly().commit();

        STRING_ELEMENT(schema).key("string2").daqPolicy(DAQPolicy::OMIT).readOnly().commit();

        STRING_ELEMENT(schema).key("string3").readOnly().commit();

        STATE_ELEMENT(schema).key("state1").commit();

        STATE_ELEMENT(schema).key("state2").daqPolicy(DAQPolicy::SAVE).commit();

        STATE_ELEMENT(schema).key("state3").daqPolicy(DAQPolicy::OMIT).commit();

        CPPUNIT_ASSERT(schema.getDAQPolicy("string1") == DAQPolicy::SAVE);
        CPPUNIT_ASSERT(schema.getDAQPolicy("string2") == DAQPolicy::OMIT);
        CPPUNIT_ASSERT(schema.getDAQPolicy("string3") == DAQPolicy::UNSPECIFIED);
        CPPUNIT_ASSERT(schema.getDAQPolicy("state1") == DAQPolicy::UNSPECIFIED);
        CPPUNIT_ASSERT(schema.getDAQPolicy("state2") == DAQPolicy::SAVE);
        CPPUNIT_ASSERT(schema.getDAQPolicy("state3") == DAQPolicy::OMIT);
    }

    // according to specified default policy
    {
        Schema schema;
        schema.setDefaultDAQPolicy(DAQPolicy::OMIT);
        STRING_ELEMENT(schema).key("string1").daqPolicy(DAQPolicy::SAVE).readOnly().commit();

        STRING_ELEMENT(schema).key("string2").daqPolicy(DAQPolicy::OMIT).readOnly().commit();

        STRING_ELEMENT(schema).key("string3").readOnly().commit();

        CPPUNIT_ASSERT(schema.getDAQPolicy("string1") == DAQPolicy::SAVE);
        CPPUNIT_ASSERT(schema.getDAQPolicy("string2") == DAQPolicy::OMIT);
        CPPUNIT_ASSERT(schema.getDAQPolicy("string3") == DAQPolicy::OMIT);
    }
}


void Schema_Test::testNodeDisplayType() {
    {
        Schema schema;
        NODE_ELEMENT(schema).key("node").setSpecialDisplayType("WidgetNode").commit();

        STRING_ELEMENT(schema).key("node.string").readOnly().commit();

        BOOL_ELEMENT(schema).key("node.bool").readOnly().commit();

        CPPUNIT_ASSERT(schema.getDisplayType("node") == "WidgetNode");
    }
}


void Schema_Test::testGetLeaves() {
    Schema schema("test");
    TestStruct1::expectedParameters(schema);
    OtherSchemaElements::expectedParameters(schema);
    Hash h;
    h.set("slotTest", Hash());
    h.set("filename", string("here"));
    h.set("testTable", vector<Hash>(3, Hash("a", -1, "b", "this_is a va|id string")));
    vector<string> leaves;
    getLeaves(h, schema, leaves, '.'); // from DataLogUtils
    CPPUNIT_ASSERT_EQUAL(std::string("filename"), leaves[0]);
    CPPUNIT_ASSERT_EQUAL(std::string("testTable"), leaves[1]);
    CPPUNIT_ASSERT_EQUAL(2ul, leaves.size());
}


void Schema_Test::testAlarmStateElement() {
    Schema schema("test");
    OtherSchemaElements::expectedParameters(schema);
    CPPUNIT_ASSERT(schema.isProperty("alarmCondition"));
    CPPUNIT_ASSERT(schema.isProperty("state"));

    // And the classId for both elements
    CPPUNIT_ASSERT(schema.hasClassId("alarmCondition"));
    CPPUNIT_ASSERT(schema.hasClassId("state"));
    CPPUNIT_ASSERT(schema.getClassId("alarmCondition") == "AlarmCondition");
    CPPUNIT_ASSERT(schema.getClassId("state") == "State");
}


void Schema_Test::testAllowedActions() {
    Schema s;
    NODE_ELEMENT(s).key("node").setAllowedActions({"action1", "action2"}).commit();
    INT32_ELEMENT(s).key("node.int").assignmentMandatory().commit();
    NDARRAY_ELEMENT(s).key("arr").setAllowedActions({"otherAction"}).commit();

    CPPUNIT_ASSERT(s.hasAllowedActions("node"));
    CPPUNIT_ASSERT(!s.hasAllowedActions("node.int"));
    const auto& actions = s.getAllowedActions("node");
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2u), actions.size());
    CPPUNIT_ASSERT_EQUAL(std::string("action1"), actions[0]);
    CPPUNIT_ASSERT_EQUAL(std::string("action2"), actions[1]);

    CPPUNIT_ASSERT(s.hasAllowedActions("arr"));
    const auto& arrActions = s.getAllowedActions("arr");
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1u), arrActions.size());
    CPPUNIT_ASSERT_EQUAL(std::string("otherAction"), arrActions[0]);

    // Only (custom) nodes can have allowed actions:
    CPPUNIT_ASSERT_THROW(s.setAllowedActions("node.int", {"bla", "blue"}), karabo::data::ParameterException);
}


void Schema_Test::testInvalidReadOnlyThrows() {
    karabo::data::Schema invalidSchema;

    // assignmentMandatory() and readOnly() contradict each other
    CPPUNIT_ASSERT_THROW(FLOAT_ELEMENT(invalidSchema).key("float").assignmentMandatory().readOnly().commit(),
                         karabo::data::LogicException);

    // The assignmentOptional().defaultValue(1).readOnly() sequence below,
    // if accepted, would reset the element value to 0, overriding the
    // defaultValue setting.
    CPPUNIT_ASSERT_THROW(
          INT32_ELEMENT(invalidSchema).key("int").assignmentOptional().defaultValue(1).readOnly().commit(),
          karabo::data::LogicException);

    karabo::data::Schema validReadOnlySchema;

    CPPUNIT_ASSERT_NO_THROW(INT32_ELEMENT(validReadOnlySchema).key("int").readOnly().initialValue(1).commit());
}


void Schema_Test::testTable() {
    Schema sch("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
    OtherSchemaElements::expectedParameters(sch);
    CPPUNIT_ASSERT(sch.isLeaf("testTable") == true);
    CPPUNIT_ASSERT(sch.getParameterHash().hasAttribute("testTable", "rowSchema") == true);
    const std::vector<Hash> shouldBeDefault(2, Hash("a", 3, "b", "foo"));
    const std::vector<Hash>& theDefault = sch.getDefaultValue<std::vector<Hash>>("testTable");
    CPPUNIT_ASSERT_EQUAL(shouldBeDefault.size(), theDefault.size());
    CPPUNIT_ASSERT_EQUAL(shouldBeDefault[0].size(), theDefault[1].size());
    CPPUNIT_ASSERT_EQUAL(shouldBeDefault[1].get<int>("a"), theDefault[0].get<int>("a"));
    CPPUNIT_ASSERT_EQUAL(shouldBeDefault[1].get<std::string>("b"), theDefault[1].get<std::string>("b"));

    CPPUNIT_ASSERT(sch.getDefaultValue<std::vector<Hash>>("testTableEmptyDefault").empty());
}


void Schema_Test::testTableReadOnly() {
    karabo::data::Schema rowSchema;

    STRING_ELEMENT(rowSchema).key("s").assignmentOptional().noDefaultValue().commit();

    BOOL_ELEMENT(rowSchema).key("b").assignmentOptional().noDefaultValue().commit();

    karabo::data::Schema invalidReadOnlySchema;

    // assignmentMandatory() and readOnly() contradict each other
    CPPUNIT_ASSERT_THROW(TABLE_ELEMENT(invalidReadOnlySchema)
                               .key("InvalidTable")
                               .setColumns(rowSchema)
                               .assignmentMandatory()
                               .readOnly()
                               .commit(),
                         karabo::data::LogicException);

    // The assignmentOptional().defaultValue(...).readOnly() sequence below,
    // if accepted, would reset the element value to empty vector of hashes,
    // overriding the defaultValue setting.
    CPPUNIT_ASSERT_THROW(TABLE_ELEMENT(invalidReadOnlySchema)
                               .key("InvalidTable")
                               .setColumns(rowSchema)
                               .assignmentOptional()
                               .defaultValue(std::vector<Hash>(1, Hash("s", "foo", "b", false)))
                               .readOnly()
                               .commit(),
                         karabo::data::LogicException);

    karabo::data::Schema validReadOnlySchema;

    CPPUNIT_ASSERT_NO_THROW(TABLE_ELEMENT(validReadOnlySchema)
                                  .key("ValidTable")
                                  .setColumns(rowSchema)
                                  .readOnly()
                                  .initialValue(std::vector<Hash>(1, Hash("s", "bar", "b", true)))
                                  .archivePolicy(Schema::NO_ARCHIVING)
                                  .commit());
    // Verify default and archive policy in schema:
    const std::vector<Hash>& specifiedDefault = validReadOnlySchema.getDefaultValue<std::vector<Hash>>("ValidTable");
    CPPUNIT_ASSERT_EQUAL(1ul, specifiedDefault.size());
    CPPUNIT_ASSERT_MESSAGE(toString(specifiedDefault[0]), specifiedDefault[0].fullyEquals(Hash("s", "bar", "b", true)));
    CPPUNIT_ASSERT(validReadOnlySchema.hasArchivePolicy("ValidTable"));
    CPPUNIT_ASSERT_EQUAL(static_cast<int>(Schema::NO_ARCHIVING), validReadOnlySchema.getArchivePolicy("ValidTable"));

    // Verify implicit default for readOnly (empty table) in schema:
    CPPUNIT_ASSERT_NO_THROW(
          TABLE_ELEMENT(validReadOnlySchema).key("ValidTable2").setColumns(rowSchema).readOnly().commit());
    const std::vector<Hash>& specifiedDefault2 = validReadOnlySchema.getDefaultValue<std::vector<Hash>>("ValidTable2");
    CPPUNIT_ASSERT_EQUAL(0ul, specifiedDefault2.size());
    CPPUNIT_ASSERT(!validReadOnlySchema.hasArchivePolicy("ValidTable2"));

    // Verifies that a config built from a Schema with read-only TABLE_ELEMENT is valid
    // and has the specified initial value.
    Hash configWithTable;
    Validator validator;
    auto res = validator.validate(validReadOnlySchema, Hash(), configWithTable);
    CPPUNIT_ASSERT_EQUAL(true, res.first);
    const std::vector<Hash>& tableRows = configWithTable.get<std::vector<Hash>>("ValidTable");
    CPPUNIT_ASSERT_EQUAL(1UL, tableRows.size());
    CPPUNIT_ASSERT_EQUAL(std::string("bar"), tableRows[0].get<std::string>("s"));
    CPPUNIT_ASSERT_EQUAL(true, tableRows[0].get<bool>("b"));

    // Tables need their row schema:
    karabo::data::Schema noColumnsSchema;
    CPPUNIT_ASSERT_THROW(TABLE_ELEMENT(noColumnsSchema).key("tableLackingColumnsSchema").readOnly().commit(),
                         karabo::data::LogicException);
}


void Schema_Test::testTableColNoDefaultValue() {
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
    CPPUNIT_ASSERT_NO_THROW(TABLE_ELEMENT(tblSchema)
                                  .key("tbl")
                                  .setColumns(rowSchema)
                                  .assignmentOptional()
                                  .defaultValue(vector<karabo::data::Hash>())
                                  .reconfigurable()
                                  .commit(););

    // Checks the synthesized defaults for each of the supported column types.
    const Schema& sanitRowSchema = tblSchema.getParameterHash().getAttribute<Schema>("tbl", KARABO_SCHEMA_ROW_SCHEMA);

    // For simple elements.
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("bool"));
    CPPUNIT_ASSERT_EQUAL(false, sanitRowSchema.getDefaultValue<bool>("bool"));
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("int8"));
    CPPUNIT_ASSERT_EQUAL(static_cast<signed char>(0), sanitRowSchema.getDefaultValue<signed char>("int8"));
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("int16"));
    CPPUNIT_ASSERT_EQUAL(static_cast<signed short>(0), sanitRowSchema.getDefaultValue<signed short>("int16"));
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("int32"));
    CPPUNIT_ASSERT_EQUAL(0, sanitRowSchema.getDefaultValue<int>("int32"));
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("int64"));
    CPPUNIT_ASSERT_EQUAL(0ll, sanitRowSchema.getDefaultValue<long long>("int64"));
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("uint8"));
    CPPUNIT_ASSERT_EQUAL(static_cast<unsigned char>(0), sanitRowSchema.getDefaultValue<unsigned char>("uint8"));
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("uint16"));
    CPPUNIT_ASSERT_EQUAL(static_cast<unsigned short>(0), sanitRowSchema.getDefaultValue<unsigned short>("uint16"));
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("uint32"));
    CPPUNIT_ASSERT_EQUAL(0u, sanitRowSchema.getDefaultValue<unsigned int>("uint32"));
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("uint64"));
    CPPUNIT_ASSERT_EQUAL(0ull, sanitRowSchema.getDefaultValue<unsigned long long>("uint64"));
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("float"));
    CPPUNIT_ASSERT_EQUAL(0.0f, sanitRowSchema.getDefaultValue<float>("float"));
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("double"));
    CPPUNIT_ASSERT_EQUAL(0.0, sanitRowSchema.getDefaultValue<double>("double"));
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("string"));
    CPPUNIT_ASSERT(sanitRowSchema.getDefaultValue<string>("string").empty());
    // For vector elements.
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("vectorBool"));
    CPPUNIT_ASSERT_EQUAL(0ul, sanitRowSchema.getDefaultValue<vector<bool>>("vectorBool").size());
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("vectorInt8"));
    CPPUNIT_ASSERT_EQUAL(0ul, sanitRowSchema.getDefaultValue<vector<signed char>>("vectorInt8").size());
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("vectorInt16"));
    CPPUNIT_ASSERT_EQUAL(0ul, sanitRowSchema.getDefaultValue<vector<signed short>>("vectorInt16").size());
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("vectorInt32"));
    CPPUNIT_ASSERT_EQUAL(0ul, sanitRowSchema.getDefaultValue<vector<int>>("vectorInt32").size());
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("vectorInt64"));
    CPPUNIT_ASSERT_EQUAL(0ul, sanitRowSchema.getDefaultValue<vector<long long>>("vectorInt64").size());
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("vectorUint8"));
    CPPUNIT_ASSERT_EQUAL(0ul, sanitRowSchema.getDefaultValue<vector<unsigned char>>("vectorUint8").size());
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("vectorUint16"));
    CPPUNIT_ASSERT_EQUAL(0ul, sanitRowSchema.getDefaultValue<vector<unsigned short>>("vectorUint16").size());
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("vectorUint32"));
    CPPUNIT_ASSERT_EQUAL(0ul, sanitRowSchema.getDefaultValue<vector<unsigned int>>("vectorUint32").size());
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("vectorUint64"));
    CPPUNIT_ASSERT_EQUAL(0ul, sanitRowSchema.getDefaultValue<vector<unsigned long long>>("vectorUint64").size());
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("vectorFloat"));
    CPPUNIT_ASSERT_EQUAL(0ul, sanitRowSchema.getDefaultValue<vector<float>>("vectorFloat").size());
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("vectorDouble"));
    CPPUNIT_ASSERT_EQUAL(0ul, sanitRowSchema.getDefaultValue<vector<double>>("vectorDouble").size());
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("vectorString"));
    CPPUNIT_ASSERT_EQUAL(0ul, sanitRowSchema.getDefaultValue<vector<string>>("vectorString").size());

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
    CPPUNIT_ASSERT_NO_THROW(TABLE_ELEMENT(roTblSchema)
                                  .key("tbl")
                                  .setColumns(roTblRowSchema)
                                  .readOnly()
                                  .initialValue(vector<Hash>())
                                  .commit(););
    // Checks the synthesized defaults for the columns.
    const Schema& sanitRoRowSchema =
          roTblSchema.getParameterHash().getAttribute<Schema>("tbl", KARABO_SCHEMA_ROW_SCHEMA);
    CPPUNIT_ASSERT(sanitRoRowSchema.hasDefaultValue("int"));
    CPPUNIT_ASSERT_EQUAL(0, sanitRoRowSchema.getDefaultValue<int>("int"));
    CPPUNIT_ASSERT(sanitRoRowSchema.hasDefaultValue("str"));
    CPPUNIT_ASSERT(sanitRoRowSchema.getDefaultValue<string>("str").empty());
    CPPUNIT_ASSERT(sanitRoRowSchema.hasDefaultValue("intWithOptions"));
    CPPUNIT_ASSERT_EQUAL(0, sanitRoRowSchema.getDefaultValue<int>("intWithOptions"));

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
    CPPUNIT_ASSERT_THROW_MESSAGE(
          "Expected exception while creating table with column missing default value and 'minInc' greater than "
          "synthezisable default.",
          TABLE_ELEMENT(tblInvalidSchema)
                .key("invalidSchema")
                .setColumns(invalidRowSchema)
                .assignmentOptional()
                .defaultValue(vector<Hash>())
                .reconfigurable()
                .commit(),
          karabo::data::ParameterException);
    karabo::data::Schema invalidRowSchema2;
    INT32_ELEMENT(invalidRowSchema2)
          .key("intNoDefault")
          .assignmentOptional()
          .noDefaultValue()
          .maxInc(-1)
          .reconfigurable()
          .commit();
    karabo::data::Schema tblInvalidSchema2;
    CPPUNIT_ASSERT_THROW_MESSAGE(
          "Expected exception while creating table with column missing default value and 'maxInc' lower than "
          "synthezisable default.",
          TABLE_ELEMENT(tblInvalidSchema2)
                .key("invalidSchema2")
                .setColumns(invalidRowSchema2)
                .assignmentOptional()
                .defaultValue(vector<Hash>())
                .reconfigurable()
                .commit(),
          karabo::data::ParameterException);
    karabo::data::Schema invalidRowSchema3;
    INT32_ELEMENT(invalidRowSchema3)
          .key("intNoDefault")
          .assignmentOptional()
          .noDefaultValue()
          .minExc(0)
          .reconfigurable()
          .commit();
    karabo::data::Schema tblInvalidSchema3;
    CPPUNIT_ASSERT_THROW_MESSAGE(
          "Expected exception while creating table with column missing default value and 'minExc' greater than "
          "synthezisable default.",
          TABLE_ELEMENT(tblInvalidSchema3)
                .key("invalidSchema3")
                .setColumns(invalidRowSchema3)
                .assignmentOptional()
                .defaultValue(vector<Hash>())
                .reconfigurable()
                .commit(),
          karabo::data::ParameterException);
    karabo::data::Schema invalidRowSchema4;
    INT32_ELEMENT(invalidRowSchema4)
          .key("intNoDefault")
          .assignmentOptional()
          .noDefaultValue()
          .maxExc(0)
          .reconfigurable()
          .commit();
    karabo::data::Schema tblInvalidSchema4;
    CPPUNIT_ASSERT_THROW_MESSAGE(
          "Expected exception while creating table with column missing default value and 'maxExc' lower than "
          "synthezisable default.",
          TABLE_ELEMENT(tblInvalidSchema4)
                .key("invalidSchema4")
                .setColumns(invalidRowSchema4)
                .assignmentOptional()
                .defaultValue(vector<Hash>())
                .reconfigurable()
                .commit(),
          karabo::data::ParameterException);
    karabo::data::Schema invalidRowSchema5;
    VECTOR_INT32_ELEMENT(invalidRowSchema5)
          .key("vectorIntNoDefault")
          .assignmentOptional()
          .noDefaultValue()
          .minSize(1)
          .reconfigurable()
          .commit();
    karabo::data::Schema tblInvalidSchema5;
    CPPUNIT_ASSERT_THROW_MESSAGE(
          "Expected exception while creating table with column missing default value and 'minSize' greater than size "
          "of synthezisable default.",
          TABLE_ELEMENT(tblInvalidSchema5)
                .key("invalidSchema5")
                .setColumns(invalidRowSchema5)
                .assignmentOptional()
                .defaultValue(vector<Hash>())
                .reconfigurable()
                .commit(),
          karabo::data::ParameterException);
    karabo::data::Schema invalidRowSchema6;
    INT32_ELEMENT(invalidRowSchema6)
          .key("invalidInt")
          .options("1 2 3 4 5 6 7 8 9") // 0 not in options.
          .assignmentOptional()
          .noDefaultValue()
          .reconfigurable()
          .commit();
    karabo::data::Schema tblInvalidSchema6;
    CPPUNIT_ASSERT_THROW_MESSAGE(
          "Expected exception while creating table with column missing default value and sinthazisable default not in "
          "the 'options' set.",
          TABLE_ELEMENT(tblInvalidSchema6)
                .key("invalidSchema6")
                .setColumns(invalidRowSchema6)
                .assignmentOptional()
                .defaultValue(vector<Hash>())
                .reconfigurable()
                .commit(),
          karabo::data::ParameterException);
}


void Schema_Test::testTableColUnsupportedType() {
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
    CPPUNIT_ASSERT_NO_THROW(TABLE_ELEMENT(tblSchema)
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
    CPPUNIT_ASSERT_THROW_MESSAGE("Expected exception trying to create VECTOR_CHAR table column.",
                                 TABLE_ELEMENT(invalidTblSchema)
                                       .key("invalidTbl")
                                       .setColumns(invalidRowSchema)
                                       .assignmentOptional()
                                       .defaultValue(vector<Hash>())
                                       .reconfigurable()
                                       .commit(),
                                 karabo::data::ParameterException);
}


void Schema_Test::testTableColInitOnly() {
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
    CPPUNIT_ASSERT(sanitRowSchema.hasAccessMode("initOnlyInt"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
          "Init-only columns of reconfigurable tables should have been converted to reconfigurable columns.",
          static_cast<int>(karabo::data::AccessType::WRITE), sanitRowSchema.getAccessMode("initOnlyInt"));

    // Checks that the 'initOnlyInt' column becomes a read-only column
    // in the sanitized row schema when the table is read-only.
    karabo::data::Schema rowSchema2;
    INT32_ELEMENT(rowSchema2).key("initOnlyInt").assignmentOptional().defaultValue(2).init().commit();
    STRING_ELEMENT(rowSchema2).key("str").assignmentOptional().defaultValue("a string...").reconfigurable().commit();
    karabo::data::Schema ReadOnlyTblSchema;
    TABLE_ELEMENT(ReadOnlyTblSchema).key("tbl").setColumns(rowSchema2).readOnly().initialValue(vector<Hash>()).commit();
    const Schema& sanitRowSchema2 =
          ReadOnlyTblSchema.getParameterHash().getAttribute<Schema>("tbl", KARABO_SCHEMA_ROW_SCHEMA);
    CPPUNIT_ASSERT(sanitRowSchema2.hasAccessMode("initOnlyInt"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Init-only columns of read-only tables should be converted to read-only columns.",
                                 static_cast<int>(karabo::data::AccessType::READ),
                                 sanitRowSchema2.getAccessMode("initOnlyInt"));

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
    CPPUNIT_ASSERT(sanitRowSchema3.hasAccessMode("initOnlyInt"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Init-only columns of init-only tables should be converted to reconfigurable columns.",
                                 static_cast<int>(karabo::data::AccessType::WRITE),
                                 sanitRowSchema3.getAccessMode("initOnlyInt"));
}


void Schema_Test::testTableColWrongAccessMode() {
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
    CPPUNIT_ASSERT(sanitRowSchema.hasAccessMode("int"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
          "Reconfigurable column 'int' of read-only table 'tbl' should have been converted to read-only.",
          static_cast<int>(karabo::data::AccessType::READ), sanitRowSchema.getAccessMode("int"));
    CPPUNIT_ASSERT(sanitRowSchema.hasAccessMode("str"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(
          "Reconfigurable column 'str' of read-only table 'tbl' should have been converted to read-only.",
          static_cast<int>(karabo::data::AccessType::READ), sanitRowSchema.getAccessMode("str"));
    // The initialValue method of LeafElement actually sets the "defaultValue"
    // attribute, so we check by asserting that the default value has been
    // preserved by the row schema sanitization process.
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("int"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Default value of column 'int', which was '2', should have been preserved.", 2,
                                 sanitRowSchema.getDefaultValue<int>("int"));
    CPPUNIT_ASSERT(sanitRowSchema.hasDefaultValue("str"));
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Default value of column 'str', which was 'a string', should have been preserved.",
                                 string("a string"), sanitRowSchema.getDefaultValue<string>("str"));
}
