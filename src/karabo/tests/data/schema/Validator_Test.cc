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
 * File:   Validator_Test.cc
 * Author: flucke
 *
 * Created on September 9, 2016, 12:18 PM
 */

#include "Validator_Test.hh"

#include <string>
#include <utility>
#include <vector>

#include "ConfigurationTestClasses.hh"
#include "karabo/core/Device.hh"
#include "karabo/data/schema/AlarmConditionElement.hh"
#include "karabo/data/schema/NodeElement.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/StateElement.hh"
#include "karabo/data/schema/TableElement.hh"
#include "karabo/data/schema/Validator.hh"
#include "karabo/data/schema/VectorElement.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/Schema.hh"
#include "karabo/data/types/StringTools.hh"
#include "karabo/devices/PropertyTest.hh"
#include "karabo/xms/OutputChannel.hh"
#include "karabo/xms/SlotElement.hh"

using namespace karabo;
using data::ALARM_ELEMENT;
using data::INT32_ELEMENT;
using data::STATE_ELEMENT;
using data::STRING_ELEMENT;
using data::TABLE_ELEMENT;
using data::VECTOR_CHAR_ELEMENT;
using data::VECTOR_UINT32_ELEMENT;
using data::VECTOR_UINT8_ELEMENT;
using xms::SLOT_ELEMENT;

CPPUNIT_TEST_SUITE_REGISTRATION(Validator_Test);

Validator_Test::Validator_Test() {}

Validator_Test::~Validator_Test() {}

void Validator_Test::setUp() {}

void Validator_Test::tearDown() {}


void Validator_Test::testTableMandatoryColumn() {
    data::Validator validator;
    data::Hash validated;

    data::Schema mandatoryRowSchema;
    INT32_ELEMENT(mandatoryRowSchema).key("reqInt").assignmentMandatory().commit();
    INT32_ELEMENT(mandatoryRowSchema).key("int").assignmentOptional().defaultValue(2).commit();

    data::Schema mandTblSchema;
    TABLE_ELEMENT(mandTblSchema).key("mandRowTable").setColumns(mandatoryRowSchema).assignmentMandatory().commit();

    // Tests that table schema with mandatory column missing in its default value will throw exception.
    data::Schema corruptedTblSchema;
    CPPUNIT_ASSERT_THROW(TABLE_ELEMENT(corruptedTblSchema)
                               .key("corruptedTable")
                               .setColumns(mandatoryRowSchema)
                               .assignmentOptional()
                               .defaultValue({data::Hash("int", 128)})
                               .commit(),
                         karabo::data::ParameterException);

    // Test to reject a table with a missing mandatory column.
    std::pair<bool, std::string> res = validator.validate(
          mandTblSchema, data::Hash("mandRowTable", std::vector<data::Hash>(1, data::Hash("int", -2))), validated);
    CPPUNIT_ASSERT(!res.first);

    validated.clear();
}


void Validator_Test::testTableOptionalColumn() {
    data::Schema rowSchema;
    INT32_ELEMENT(rowSchema).key("int").assignmentOptional().defaultValue(1).commit();
    STRING_ELEMENT(rowSchema).key("str").assignmentOptional().defaultValue("a string").commit();

    data::Schema schema;
    TABLE_ELEMENT(schema)
          .key("table")
          .setColumns(rowSchema)
          .assignmentOptional()
          .defaultValue(std::vector<data::Hash>())
          .commit();

    data::Schema nonEmptySchema;
    TABLE_ELEMENT(nonEmptySchema)
          .key("nonEmptyTable")
          .setColumns(rowSchema)
          .assignmentOptional()
          .defaultValue({data::Hash("int", 128, "str", "first row")})
          .commit();

    data::Validator validator;
    data::Hash validated;

    // Test to get default if nothing provided
    std::pair<bool, std::string> res = validator.validate(schema, data::Hash(), validated);

    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<data::Hash>>("table"));
    CPPUNIT_ASSERT(validated.get<std::vector<data::Hash>>("table").empty());

    validated.clear();

    // Test to get non-empty default if nothing is provided.
    res = validator.validate(nonEmptySchema, data::Hash(), validated);

    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("nonEmptyTable"));
    CPPUNIT_ASSERT(validated.is<std::vector<data::Hash>>("nonEmptyTable"));
    CPPUNIT_ASSERT(validated.get<std::vector<data::Hash>>("nonEmptyTable").size() == 1);
    CPPUNIT_ASSERT_EQUAL(128, validated.get<std::vector<data::Hash>>("nonEmptyTable")[0].get<int>("int"));
    CPPUNIT_ASSERT_EQUAL(std::string("first row"),
                         validated.get<std::vector<data::Hash>>("nonEmptyTable")[0].get<std::string>("str"));

    validated.clear();

    // Test to accept even special case of empty vector<string>:
    // That is what would be received if bound API (try to) send empty vector<Hash>.
    res = validator.validate(schema, data::Hash("table", std::vector<std::string>()), validated);

    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<data::Hash>>("table"));
    CPPUNIT_ASSERT(validated.get<std::vector<data::Hash>>("table").empty());

    validated.clear();

    // Test to accept a table with fitting row.
    res = validator.validate(
          schema, data::Hash("table", std::vector<data::Hash>(1, data::Hash("int", -2, "str", "testing"))), validated);

    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<data::Hash>>("table"));
    CPPUNIT_ASSERT_EQUAL(1ul, validated.get<std::vector<data::Hash>>("table").size());
    CPPUNIT_ASSERT_EQUAL(-2, validated.get<std::vector<data::Hash>>("table")[0].get<int>("int"));
    CPPUNIT_ASSERT_EQUAL(std::string("testing"),
                         validated.get<std::vector<data::Hash>>("table")[0].get<std::string>("str"));

    validated.clear();

    // Test to accept a table with an "initially bad" row - there's a missing column, but the table validation
    // attributes allow injection of missing columns.
    res = validator.validate(schema, data::Hash("table", std::vector<data::Hash>(1, data::Hash("int", 2))), validated);
    CPPUNIT_ASSERT(res.first);
    // Checks that the missing column has been injected by the validator - see tableValidationAttributes.
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<data::Hash>>("table"));
    CPPUNIT_ASSERT_EQUAL(1ul, validated.get<std::vector<data::Hash>>("table").size());
    CPPUNIT_ASSERT(validated.get<std::vector<data::Hash>>("table")[0].has("str"));
    CPPUNIT_ASSERT_EQUAL(std::string("a string"),
                         validated.get<std::vector<data::Hash>>("table")[0].get<std::string>("str"));

    validated.clear();

    // Test to reject a table with a bad row - unknown column name.
    res = validator.validate(
          schema, data::Hash("table", std::vector<data::Hash>(1, data::Hash("unknownKey", 123, "str", "testing"))),
          validated);
    CPPUNIT_ASSERT(!res.first);

    validated.clear();

    // Test to accept a table with an "initially bad" value - the value of the int column is the string form of an int.
    // The validator should do the conversion.
    // Caveat: The validator currently enforces a "best effort" policy to convert really "bad" values into a "good"
    //         values that match the value type stated in the schema for the column. Examples of those edge cases that
    //         could lead to unacceptable behavior (bugs) depending on the use cases: a value "2a" would be converted
    //         to int 2; a float 4.6 would be truncated to 4; a value "abc" would become int 0 (the initial default
    //         value for an int).
    res = validator.validate(
          schema, data::Hash("table", std::vector<data::Hash>(1, data::Hash("int", "2", "str", "testing"))), validated);
    CPPUNIT_ASSERT(res.first);
    // Checks that the string value has been properly converted to an int by the validator.
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<data::Hash>>("table"));
    CPPUNIT_ASSERT_EQUAL(1ul, validated.get<std::vector<data::Hash>>("table").size());
    CPPUNIT_ASSERT_EQUAL(2, validated.get<std::vector<data::Hash>>("table")[0].get<int>("int"));

    validated.clear();
}


void Validator_Test::testTableMinMaxRows() {
    data::Schema rowSchema;
    INT32_ELEMENT(rowSchema).key("int").assignmentOptional().defaultValue(1).commit();
    STRING_ELEMENT(rowSchema).key("str").assignmentOptional().defaultValue("a string").commit();

    data::Schema tblWithMinMaxSchema;
    TABLE_ELEMENT(tblWithMinMaxSchema)
          .key("tblWithMinMax")
          .setColumns(rowSchema)
          .minSize(1)
          .maxSize(1)
          .assignmentOptional()
          .defaultValue({data::Hash("int", 1, "str", "First Row")})
          .commit();

    data::Validator validator;
    data::Hash validated;

    // Checks that a table with 1 row is valid. The validator ensures
    // that the table is valid by adding the row of the default table
    // value.
    std::pair<bool, std::string> res = validator.validate(tblWithMinMaxSchema, data::Hash(), validated);
    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("tblWithMinMax"));
    CPPUNIT_ASSERT(validated.is<std::vector<data::Hash>>("tblWithMinMax"));
    CPPUNIT_ASSERT_EQUAL(1ul, validated.get<std::vector<data::Hash>>("tblWithMinMax").size());
    validated.clear();

    // Checks that a table with more than maxSize rows is invalid.
    std::vector<data::Hash> tblTwoRows{data::Hash("int", 1, "str", "First Row"),
                                       data::Hash("int", 2, "str", "Second Row")};
    res = validator.validate(tblWithMinMaxSchema, data::Hash("tblWithMinMax", tblTwoRows), validated);
    CPPUNIT_ASSERT(!res.first);
    CPPUNIT_ASSERT_MESSAGE("Expected error with 'must have no more than' substring.\nGot: " + res.second,
                           res.second.find("must have no more than") != std::string::npos);
    validated.clear();

    // Checks that a table with less than minSize rows is invalid.
    tblWithMinMaxSchema.setMinSize("tblWithMinMax", 2u);
    tblWithMinMaxSchema.setMaxSize("tblWithMinMax", 2u);
    res = validator.validate(tblWithMinMaxSchema, data::Hash(), validated);
    CPPUNIT_ASSERT(!res.first);
    CPPUNIT_ASSERT_MESSAGE("Expected error with 'must have at least' substring.\nGot:" + res.second,
                           res.second.find("must have at least") != std::string::npos);
    validated.clear();
}


void Validator_Test::testColumnMinMaxAttrs() {
    data::Schema rowSchema;
    INT32_ELEMENT(rowSchema).key("int_1").assignmentOptional().defaultValue(1).minInc(1).maxInc(20).commit();
    INT32_ELEMENT(rowSchema).key("int_2").assignmentOptional().defaultValue(2).minExc(1).maxExc(20).commit();

    data::Schema tblSchema;
    TABLE_ELEMENT(tblSchema)
          .key("tbl")
          .setColumns(rowSchema)
          .assignmentOptional()
          .defaultValue(std::vector<data::Hash>())
          .commit();

    data::Validator validator;
    data::Hash validated;

    // Checks that the empty table doesn't violate columns min, max attributes.
    std::pair<int, std::string> res = validator.validate(tblSchema, data::Hash(), validated);
    CPPUNIT_ASSERT(res.first);
    validated.clear();

    // Checks that minInc and maxInc are enforced.
    std::vector<data::Hash> tblRows{
          data::Hash("int_1", 1, "int_2", 19), // Valid line.
          data::Hash("int_1", 1, "int_2", 20), // int_2 MaxExc(20) violation.
          data::Hash("int_1", 1, "int_2", 1),  // int_2 MinExc(1) violation.
          data::Hash("int_1", 0, "int_2", 19), // int_1 MinInc(1) violation.
          data::Hash("int_1", 21, "int_2", 19) // int_1 MaxInc(20) violation.
    };
    res = validator.validate(tblSchema, data::Hash("tbl", tblRows), validated);
    CPPUNIT_ASSERT(!res.first);
    CPPUNIT_ASSERT_MESSAGE("Expected error with 'Value 20 for parameter' substring.\nGot: " + res.second,
                           res.second.find("Value 20 for parameter") != std::string::npos);
    validated.clear();
    tblRows.erase(tblRows.begin() + 1); // Advances to next invalid line
    res = validator.validate(tblSchema, data::Hash("tbl", tblRows), validated);
    CPPUNIT_ASSERT(!res.first);
    CPPUNIT_ASSERT_MESSAGE("Expected error with 'Value 1 for parameter' substring.\nGot: " + res.second,
                           res.second.find("Value 1 for parameter") != std::string::npos);
    validated.clear();
    tblRows.erase(tblRows.begin() + 1); // Advances to next invalid line
    res = validator.validate(tblSchema, data::Hash("tbl", tblRows), validated);
    CPPUNIT_ASSERT(!res.first);
    CPPUNIT_ASSERT_MESSAGE("Expected error with 'Value 0 for parameter' substring.\nGot: " + res.second,
                           res.second.find("Value 0 for parameter") != std::string::npos);
    validated.clear();
    tblRows.erase(tblRows.begin() + 1); // Advances to next invalid line
    res = validator.validate(tblSchema, data::Hash("tbl", tblRows), validated);
    CPPUNIT_ASSERT(!res.first);
    CPPUNIT_ASSERT_MESSAGE("Expected error with 'Value 21 for parameter' substring.\nGot: " + res.second,
                           res.second.find("Value 21 for parameter") != std::string::npos);
    validated.clear();
}


void Validator_Test::testVectorCharVectorByteSize() {
    using data::toString;
    using std::vector;

    data::Validator validator;
    data::Hash validated;

    data::Schema vecSchValid;
    VECTOR_CHAR_ELEMENT(vecSchValid)
          .key("MinSizeVectChar")
          .description("VectorChar that respects MinSize")
          .minSize(2)
          .readOnly()
          .initialValue({'A', 'B'})
          .commit();
    std::pair<bool, std::string> res = validator.validate(vecSchValid, data::Hash(), validated);
    // vecSchValid should be valid, res.first == true,  with no error message.
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Value of 'MinSizeVectChar' should be equal to provided 'initialValue'.",
                                 toString(vector<char>({'A', 'B'})),
                                 toString(validated.get<vector<char>>("MinSizeVectChar")));

    data::Schema vecSchInvalid;
    CPPUNIT_ASSERT_THROW(VECTOR_CHAR_ELEMENT(vecSchInvalid)
                               .key("MinSizeVectChar")
                               .description("VectorChar with less than MinSize elements")
                               .minSize(1)
                               .readOnly()
                               .initialValue({})
                               .commit(),
                         karabo::data::ParameterException);

    data::Schema vecByteSchValid;
    VECTOR_UINT8_ELEMENT(vecByteSchValid)
          .key("MinSizeVectByte")
          .description("VectorByte that respects MinSize")
          .minSize(2)
          .readOnly()
          .initialValue({0xFF, 0xA2})
          .commit();
    res = validator.validate(vecByteSchValid, data::Hash(), validated);
    // vecByteSchValid must be valid, res.first == true, with no error message.
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Value of 'MinSizeVectByte' should be equal to provided 'initialValue'.",
                                 toString(vector<unsigned char>({0xFF, 0xA2})),
                                 toString(validated.get<vector<unsigned char>>("MinSizeVectByte")));

    data::Schema vecByteSchInvalid;
    CPPUNIT_ASSERT_THROW(VECTOR_UINT8_ELEMENT(vecByteSchValid)
                               .key("MaxSizeVectByte")
                               .description("VectorByte that doesn't respect MaxSize")
                               .minSize(1)
                               .maxSize(2)
                               .readOnly()
                               .initialValue({0xFF, 0xA2, 0x16})
                               .commit(),
                         karabo::data::ParameterException);
}

void Validator_Test::testState() {
    data::Validator validator;
    data::Hash validated;

    data::Schema schema;
    STATE_ELEMENT(schema).key("goofyState").initialValue(data::State::UNKNOWN).commit();

    // Test to reject a state that is set with a bad state.
    std::pair<bool, std::string> res = validator.validate(schema, data::Hash("goofyState", "NotAState"), validated);
    CPPUNIT_ASSERT_MESSAGE(std::string("Validation succeeded unexpectedly :") + data::toString(validated), !res.first);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.second.find("is not a valid state string") != std::string::npos);
    validated.clear();

    // Test to allow a state that is set with a good state.
    res = validator.validate(schema, data::Hash("goofyState", "ERROR"), validated);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    // Test the validated hash should have the attribute set.
    CPPUNIT_ASSERT(validated.getAttributes("goofyState").has(KARABO_INDICATE_STATE_SET));

    validated.clear();

    STATE_ELEMENT(schema).key("defaultValue").defaultValue(data::State::CHANGING).commit();
    CPPUNIT_ASSERT_EQUAL(karabo::data::State::CHANGING.name(), schema.getDefaultValue<std::string>("defaultValue"));
}


void Validator_Test::testAlarms() {
    data::Validator validator;
    data::Hash validated;

    data::Schema schema;
    ALARM_ELEMENT(schema).key("goofyAlarm").defaultValue(karabo::data::AlarmCondition::ALARM).commit();
    CPPUNIT_ASSERT_EQUAL(karabo::data::AlarmCondition::ALARM.asString(),
                         schema.getDefaultValue<std::string>("goofyAlarm"));
    // Test to reject a state that is set with a bad alarm string
    std::pair<bool, std::string> res =
          validator.validate(schema, data::Hash("goofyAlarm", "LondonIsBurningCallTheEngines"), validated);
    CPPUNIT_ASSERT_MESSAGE(std::string("Validation succeeded unexpectedly :") + data::toString(validated), !res.first);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.second.find("is not a valid alarm string") != std::string::npos);
    validated.clear();

    // Test to allow a state that is set with a good alarm string
    res = validator.validate(schema, data::Hash("goofyAlarm", "alarm"), validated);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    // Test the validated hash should have the attribute set.
    CPPUNIT_ASSERT(validated.getAttributes("goofyAlarm").has(KARABO_INDICATE_ALARM_SET));

    validated.clear();
}


void Validator_Test::testSlots() {
    data::Schema s;
    SLOT_ELEMENT(s).key("slot").commit();

    // Slot does not appear in validated config.
    data::Hash in, out;
    data::Validator val;
    std::pair<bool, std::string> res = val.validate(s, in, out);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    CPPUNIT_ASSERT_MESSAGE(data::toString(out), out.empty());

    // Empty node allowed for slot (for backward compatibility).
    in.set("slot", data::Hash());
    res = val.validate(s, in, out);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    CPPUNIT_ASSERT_MESSAGE(data::toString(out), out.empty());

    // Non-empty node not allowed for slot.
    in.set("slot.a", 1);
    res = val.validate(s, in, out);
    CPPUNIT_ASSERT(!res.first);
    CPPUNIT_ASSERT_EQUAL(std::string("There is configuration provided for Slot 'slot'"), res.second);

    // Other things than node not allowed for slot, either.
    out.clear(); // just in case...
    in.set("slot", "buh");
    res = val.validate(s, in, out);
    CPPUNIT_ASSERT(!res.first);
    CPPUNIT_ASSERT_EQUAL(std::string("There is configuration provided for Slot 'slot'"), res.second);
}


void Validator_Test::testLeafAssignmentInternal() {
    data::Schema schema;
    data::BOOL_ELEMENT(schema)
          .key("boolProperty")
          .displayedName("Bool property")
          .description("A bool property")
          .init()
          .assignmentInternal()
          .defaultValue(false)
          .commit();

    data::Validator validator;
    data::Hash validated;
    // Test default without setting
    std::pair<bool, std::string> res;
    res = validator.validate(schema, data::Hash(), validated);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    CPPUNIT_ASSERT(validated.has("boolProperty"));
    CPPUNIT_ASSERT(!validated.get<bool>("boolProperty"));
    // Test to set a parameter with assignment internal
    validated.clear();
    res = validator.validate(schema, data::Hash("boolProperty", true), validated);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    CPPUNIT_ASSERT(validated.has("boolProperty"));
    CPPUNIT_ASSERT(validated.get<bool>("boolProperty"));
}


void Validator_Test::testOutputChannelSchemaRemoval() {
    data::Schema schema;
    data::Schema channelSchema;

    data::BOOL_ELEMENT(channelSchema)
          .key("boolProperty")
          .displayedName("Bool property")
          .description("A bool property")
          .init()
          .assignmentInternal()
          .defaultValue(false)
          .commit();

    data::NODE_ELEMENT(channelSchema).key("node").commit();

    data::BOOL_ELEMENT(channelSchema)
          .key("node.boolProperty")
          .displayedName("Bool property")
          .description("A bool property")
          .init()
          .assignmentInternal()
          .defaultValue(false)
          .commit();

    data::NODE_ELEMENT(channelSchema).key("emptyNode").commit();

    xms::OUTPUT_CHANNEL(schema)
          .key("outputChannel")
          .displayedName("tcp pipe")
          .description("a test output channel")
          .dataSchema(channelSchema)
          .commit();

    data::Validator validator;
    data::Hash validated;

    auto res = validator.validate(schema, data::Hash(), validated);

    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    CPPUNIT_ASSERT(validated.has("outputChannel.schema"));
    CPPUNIT_ASSERT(validated.get<data::Hash>("outputChannel.schema").empty());

    // Test to set a parameter with assignment internal
    validated.clear();
    res = validator.validate(schema, data::Hash("outputChannel.schema.boolProperty", true), validated);

    CPPUNIT_ASSERT_MESSAGE(res.second, !res.first); // fails because configuring schema of
                                                    // outputChannel is not allowed

    validated.clear();
    res = validator.validate(schema, data::Hash("outputChannel.schema.node.boolProperty", true), validated);

    CPPUNIT_ASSERT_MESSAGE(res.second, !res.first);

    // This accomodates case where user configuration has an empty
    // hash for outputChannel.schema
    validated.clear();
    res = validator.validate(schema, data::Hash("outputChannel.schema", data::Hash()), validated);

    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    CPPUNIT_ASSERT(validated.has("outputChannel.schema"));
    CPPUNIT_ASSERT(validated.get<data::Hash>("outputChannel.schema").empty());

    // This accomodates special case: presence of outputChannel.schema.A.B.C in
    // configuration schema. FIXME: we should not have to support this case once
    // this behavior is corrected in code later on.
    validated.clear();
    res = validator.validate(
          schema,
          data::Hash("outputChannel.schema.node", data::Hash(), "outputChannel.schema.nonexistent", data::Hash()),
          validated);

    CPPUNIT_ASSERT_MESSAGE(res.second, res.first); // "special" case exception
    CPPUNIT_ASSERT(validated.has("outputChannel.schema"));
    CPPUNIT_ASSERT(validated.get<data::Hash>("outputChannel.schema").empty());

    validated.clear();
    res = validator.validate(
          schema, data::Hash("outputChannel.schema.node", data::Hash(), "outputChannel.schema.boolProperty", true),
          validated);

    CPPUNIT_ASSERT_MESSAGE(res.second, !res.first);
    CPPUNIT_ASSERT(validated.has("outputChannel.schema"));
    CPPUNIT_ASSERT(validated.get<data::Hash>("outputChannel.schema").empty());
}


void Validator_Test::testPropertyTestValidation() {
    data::Validator::ValidationRules rules;
    // Set validation rules used during reconfiguration...
    rules.allowAdditionalKeys = false;
    rules.allowMissingKeys = true;
    rules.allowUnrootedConfiguration = true;
    rules.injectDefaults = false;
    data::Validator validator(rules);
    data::Hash validated;

    using namespace karabo::devices;

    int64_t elapsedTimeIn_microseconds = 0ll;

    // No schema was provided for validator ...
    CPPUNIT_ASSERT(!validator.hasReconfigurableParameter());

    auto schema = core::Device::getSchema("PropertyTest");
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("boolProperty", true), validated);
        // Schema for PropertyTest contains reconfigurable parameters...
        CPPUNIT_ASSERT_MESSAGE("Reconfigurable parameters are expected in PropertyTest schema",
                               validator.hasReconfigurableParameter());

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("boolProperty"));
    CPPUNIT_ASSERT(validated.get<bool>("boolProperty") == true);

    validated.clear();

    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("charProperty", 'B'), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("charProperty"));
    CPPUNIT_ASSERT(validated.get<char>("charProperty") == 'B');

    validated.clear();

    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("int8Property", 34), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("int8Property"));
    CPPUNIT_ASSERT(validated.get<signed char>("int8Property") == 34);


    validated.clear();

    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("uint8Property", 113), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("uint8Property"));
    CPPUNIT_ASSERT(validated.get<unsigned char>("uint8Property") == 113);


    validated.clear();

    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("int16Property", 2300), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("int16Property"));
    CPPUNIT_ASSERT(validated.get<short>("int16Property") == 2300);


    validated.clear();

    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("uint16Property", 55555), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("uint16Property"));
    CPPUNIT_ASSERT(validated.get<unsigned short>("uint16Property") == 55555);

    validated.clear();

    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("int32Property", 23000000), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("int32Property"));
    CPPUNIT_ASSERT(validated.get<int>("int32Property") == 23000000);


    validated.clear();

    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("int64Property", 3200000000LL), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("int64Property"));
    CPPUNIT_ASSERT(validated.get<long long>("int64Property") == 3200000000LL);

    validated.clear();

    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("uint64Property", 3200000000ULL), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("uint64Property"));
    CPPUNIT_ASSERT(validated.get<unsigned long long>("uint64Property") == 3200000000ULL);

    validated.clear();

    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("floatProperty", 3.45678F), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("floatProperty"));
    float fvalue = validated.get<float>("floatProperty");
    CPPUNIT_ASSERT(fvalue == 3.45678F);

    validated.clear();

    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("doubleProperty", 5.678901234), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("doubleProperty"));
    CPPUNIT_ASSERT(validated.get<double>("doubleProperty") == 5.678901234);

    validated.clear();

    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("stringProperty", "Some text"), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("stringProperty"));
    CPPUNIT_ASSERT(validated.get<std::string>("stringProperty") == "Some text");

    validated.clear();

    const std::vector<bool> vbool{true, false, false, true, true};
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("vectors.boolProperty", vbool), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("vectors.boolProperty"));
    {
        const std::vector<bool> v = validated.get<std::vector<bool>>("vectors.boolProperty");
        const size_t size = v.size();
        CPPUNIT_ASSERT(size == 5);
        for (size_t i = 0; i < size; ++i) CPPUNIT_ASSERT(v[i] == vbool[i]);
    }

    validated.clear();

    const std::vector<char> vchar{'A', 'B', 'C', 'X', 'Y', 'Z'};
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("vectors.charProperty", vchar), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("vectors.charProperty"));
    {
        const std::vector<char>& v = validated.get<std::vector<char>>("vectors.charProperty");
        const size_t size = v.size();
        CPPUNIT_ASSERT(size == 6);
        for (size_t i = 0; i < size; ++i) CPPUNIT_ASSERT(v[i] == vchar[i]);
    }

    validated.clear();

    const std::vector<signed char> vschar{41, 42, 43, 44, 45, 46};
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("vectors.int8Property", vschar), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("vectors.int8Property"));
    {
        const std::vector<signed char>& v = validated.get<std::vector<signed char>>("vectors.int8Property");
        const size_t size = v.size();
        CPPUNIT_ASSERT(size == 6);
        for (size_t i = 0; i < size; ++i) CPPUNIT_ASSERT(v[i] == vschar[i]);
    }

    validated.clear();

    const std::vector<unsigned char> vuchar{41, 42, 43, 44, 45, 46};
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("vectors.uint8Property", vuchar), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("vectors.uint8Property"));
    {
        const std::vector<unsigned char>& v = validated.get<std::vector<unsigned char>>("vectors.uint8Property");
        const size_t size = v.size();
        CPPUNIT_ASSERT(size == 6);
        for (size_t i = 0; i < size; ++i) CPPUNIT_ASSERT(v[i] == vuchar[i]);
    }

    validated.clear();

    const std::vector<short> vshort{41, 42, 43, 44, 45, 46};
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("vectors.int16Property", vshort), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("vectors.int16Property"));
    {
        const std::vector<short>& v = validated.get<std::vector<short>>("vectors.int16Property");
        const size_t size = v.size();
        CPPUNIT_ASSERT(size == 6);
        for (size_t i = 0; i < size; ++i) CPPUNIT_ASSERT(v[i] == vshort[i]);
    }

    validated.clear();

    const std::vector<unsigned short> vushort{41, 42, 43, 44, 45, 46};
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("vectors.uint16Property", vushort), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("vectors.uint16Property"));
    {
        const std::vector<unsigned short>& v = validated.get<std::vector<unsigned short>>("vectors.uint16Property");
        const size_t size = v.size();
        CPPUNIT_ASSERT(size == 6);
        for (size_t i = 0; i < size; ++i) CPPUNIT_ASSERT(v[i] == vushort[i]);
    }

    validated.clear();

    const std::vector<int> vint{41, 42, 43, 44, 45, 46};
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("vectors.int32Property", vint), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("vectors.int32Property"));
    {
        const std::vector<int>& v = validated.get<std::vector<int>>("vectors.int32Property");
        const size_t size = v.size();
        CPPUNIT_ASSERT(size == 6);
        for (size_t i = 0; i < size; ++i) CPPUNIT_ASSERT(v[i] == vint[i]);
    }

    validated.clear();

    const std::vector<unsigned int> vuint{90000041, 90000042, 90000043, 90000044, 90000045, 90000046};
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("vectors.uint32Property", vuint), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("vectors.uint32Property"));
    {
        const std::vector<unsigned int>& v = validated.get<std::vector<unsigned int>>("vectors.uint32Property");
        const size_t size = v.size();
        CPPUNIT_ASSERT(size == 6);
        for (size_t i = 0; i < size; ++i) CPPUNIT_ASSERT(v[i] == vuint[i]);
    }


    VECTOR_UINT32_ELEMENT(schema)
          .key("vectors.uint32PropertyRestrict")
          .assignmentOptional()
          .noDefaultValue()
          .minSize(2)
          .maxSize(4)
          .reconfigurable()
          .commit();

    validated.clear();

    const std::vector<unsigned int> vuintAboveMaxSize{90000041, 90000042, 90000043, 90000044, 90000045, 90000046};
    {
        auto res =
              validator.validate(schema, data::Hash("vectors.uint32PropertyRestrict", vuintAboveMaxSize), validated);

        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'is greater than upper bound' substring.\nGot:" + res.second,
                               res.second.find("is greater than upper bound") != std::string::npos);
    }

    validated.clear();

    const std::vector<unsigned int> vuintBelowMinSize{
          90000041,
    };
    {
        auto res =
              validator.validate(schema, data::Hash("vectors.uint32PropertyRestrict", vuintBelowMinSize), validated);

        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'is smaller than lower bound' substring.\nGot:" + res.second,
                               res.second.find("is smaller than lower bound") != std::string::npos);
    }


    validated.clear();

    const std::vector<long long> vlonglong{20000000041LL, 20000000042LL, 20000000043LL,
                                           20000000044LL, 20000000045LL, 20000000046LL};
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("vectors.int64Property", vlonglong), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("vectors.int64Property"));
    {
        const std::vector<long long>& v = validated.get<std::vector<long long>>("vectors.int64Property");
        const size_t size = v.size();
        CPPUNIT_ASSERT(size == 6);
        for (size_t i = 0; i < size; ++i) CPPUNIT_ASSERT(v[i] == vlonglong[i]);
    }

    validated.clear();

    const std::vector<unsigned long long> vulonglong{90000000041LL, 90000000042LL, 90000000043LL,
                                                     90000000044LL, 90000000045LL, 90000000046LL};
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("vectors.uint64Property", vulonglong), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("vectors.uint64Property"));
    {
        const std::vector<unsigned long long>& v =
              validated.get<std::vector<unsigned long long>>("vectors.uint64Property");
        const size_t size = v.size();
        CPPUNIT_ASSERT(size == 6);
        for (size_t i = 0; i < size; ++i) CPPUNIT_ASSERT(v[i] == vulonglong[i]);
    }

    validated.clear();

    const std::vector<float> vfloat{1.23456, 2.34567, 3.45678, 4.56789, 5.67891, 6.78912};
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("vectors.floatProperty", vfloat), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("vectors.floatProperty"));
    {
        const std::vector<float>& v = validated.get<std::vector<float>>("vectors.floatProperty");
        const size_t size = v.size();
        CPPUNIT_ASSERT(size == 6);
        for (size_t i = 0; i < size; ++i) CPPUNIT_ASSERT(v[i] == vfloat[i]);
    }

    validated.clear();

    const std::vector<double> vdouble{1.234567891, 2.345678912, 3.456789123, 4.567891234, 5.678901234, 6.123456789};
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("vectors.doubleProperty", vdouble), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("vectors.doubleProperty"));
    {
        const std::vector<double>& v = validated.get<std::vector<double>>("vectors.doubleProperty");
        const size_t size = v.size();
        CPPUNIT_ASSERT(size == 6);
        for (size_t i = 0; i < size; ++i) CPPUNIT_ASSERT(v[i] == vdouble[i]);
    }

    validated.clear();

    const std::vector<std::string> vstring{"1111111", "2222222", "3333333", "4444444", "5555555", "6666666"};
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("vectors.stringProperty", vstring), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("vectors.stringProperty"));
    {
        const std::vector<std::string>& v = validated.get<std::vector<std::string>>("vectors.stringProperty");
        const size_t size = v.size();
        CPPUNIT_ASSERT(size == 6);
        for (size_t i = 0; i < size; ++i) CPPUNIT_ASSERT_EQUAL(v[i], vstring[i]);
    }

    validated.clear();

    const std::vector<data::Hash> vtable{{data::Hash("e1", "abc", "e2", true, "e3", 12, "e4", 0.9837F, "e5", 1.23456),
                                          data::Hash("e1", "def", "e2", true, "e3", 13, "e4", 0.3456F, "e5", 2.23456),
                                          data::Hash("e1", "ghi", "e2", false, "e3", 14, "e4", 0.7891F, "e5", 3.2345),
                                          data::Hash("e1", "jkl", "e2", false, "e3", 15, "e4", 0.2222F, "e5", 4.2345)}};
    {
        const auto startTimepoint = std::chrono::high_resolution_clock::now();

        auto res = validator.validate(schema, data::Hash("table", vtable), validated);

        const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
        elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("table"));
    {
        const std::vector<data::Hash>& v = validated.get<std::vector<data::Hash>>("table");
        const size_t size = v.size();
        CPPUNIT_ASSERT(size == 4);
        for (size_t i = 0; i < size; ++i) {
            CPPUNIT_ASSERT(v[i].fullyEquals(vtable[i], true));
        }
    }

    data::Schema rowSchemaRestrict;
    INT32_ELEMENT(rowSchemaRestrict)
          .key("par1")
          .assignmentOptional()
          .defaultValue(5)
          .minInc(3)
          .maxInc(10)
          .reconfigurable()
          .commit();
    INT32_ELEMENT(rowSchemaRestrict)
          .key("par2")
          .assignmentOptional()
          .defaultValue(6)
          .minExc(1)
          .maxExc(10)
          .reconfigurable()
          .commit();
    STRING_ELEMENT(rowSchemaRestrict)
          .key("par3")
          .options("word1, word2, word3")
          .assignmentOptional()
          .defaultValue("word2")
          .reconfigurable()
          .commit();

    TABLE_ELEMENT(schema)
          .key("tableRestrict")
          .setColumns(rowSchemaRestrict)
          .assignmentOptional()
          .defaultValue(std::vector<data::Hash>())
          .minSize(2)
          .maxSize(3)
          .reconfigurable()
          .commit();

    // Test when the table size above maximum
    validated.clear();

    const std::vector<data::Hash> vAboveMaxSize{
          {data::Hash("par1", 7, "par2", 10, "par3", "word1"), data::Hash("par1", 8, "par2", 9, "par3", "word1"),
           data::Hash("par1", 9, "par2", 8, "par3", "word3"), data::Hash("par1", 10, "par2", 7, "par3", "word3")}};

    {
        auto res = validator.validate(schema, data::Hash("tableRestrict", vAboveMaxSize), validated);
        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'must have no more than' substring.\nGot:" + res.second,
                               res.second.find("must have no more than") != std::string::npos);
    }

    // Test when the table size below minimum
    validated.clear();

    const std::vector<data::Hash> vBelowMinSize({data::Hash("par1", 7, "par2", 10, "par3", "word1")});
    {
        auto res = validator.validate(schema, data::Hash("tableRestrict", vBelowMinSize), validated);
        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'must have at least' substring.\nGot:" + res.second,
                               res.second.find("must have at least") != std::string::npos);
    }

    // Test if unknown column
    validated.clear();

    const std::vector<data::Hash> vUnknownPar(
          {data::Hash("par1", 7, "par2", 8, "par3", "word1"), data::Hash("par1", 8, "par2", 7, "par4", "word1")});
    {
        auto res = validator.validate(schema, data::Hash("tableRestrict", vUnknownPar), validated);
        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'unexpected configuration parameter' substring.\nGot:" + res.second,
                               res.second.find("unexpected configuration parameter") != std::string::npos);
    }

    // Test when the value above maximum
    validated.clear();

    const std::vector<data::Hash> vAboveMaxValue(
          {data::Hash("par1", 7, "par2", 11, "par3", "word1"), data::Hash("par1", 8, "par2", 7, "par3", "word1")});
    {
        auto res = validator.validate(schema, data::Hash("tableRestrict", vAboveMaxValue), validated);

        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'out of upper bound' substring.\nGot:" + res.second,
                               res.second.find("out of upper bound") != std::string::npos);
    }

    // Test when the value below minimum
    validated.clear();

    const std::vector<data::Hash> vBelowMinValue(
          {data::Hash("par1", 1, "par2", 7, "par3", "word1"), data::Hash("par1", 8, "par2", 7, "par3", "word1")});
    {
        auto res = validator.validate(schema, data::Hash("tableRestrict", vBelowMinValue), validated);

        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'out of lower bound' substring.\nGot:" + res.second,
                               res.second.find("out of lower bound") != std::string::npos);
    }

    // Test for wrong option
    validated.clear();

    const std::vector<data::Hash> vUnknownOption(
          {data::Hash("par1", 4, "par2", 7, "par3", "word5"), data::Hash("par1", 8, "par2", 7, "par3", "word1")});
    {
        auto res = validator.validate(schema, data::Hash("tableRestrict", vUnknownOption), validated);

        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'is not one of the valid options' substring.\nGot:" + res.second,
                               res.second.find("is not one of the valid options") != std::string::npos);
    }

    // State test:  success
    validated.clear();

    {
        auto res = validator.validate(schema, data::Hash("state", "STARTED"), validated);

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("state"));
    CPPUNIT_ASSERT(validated.get<std::string>("state") == "STARTED");
    CPPUNIT_ASSERT(validated.getAttributes("state").has(KARABO_INDICATE_STATE_SET));

    // State test: "is not a valid state string"
    validated.clear();

    {
        auto res = validator.validate(schema, data::Hash("state", "NOTSTARTED"), validated);

        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'is not a valid state string' substring.\nGot:" + res.second,
                               res.second.find("is not a valid state string") != std::string::npos);
    }

    // State test: "is not on e of the valid options"
    validated.clear();

    {
        auto res = validator.validate(schema, data::Hash("state", "RUNNING"), validated);

        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'is not one of the valid options' substring.\nGot:" + res.second,
                               res.second.find("is not one of the valid options") != std::string::npos);
    }

    // Add ALARM_ELEMENT to the schema
    ALARM_ELEMENT(schema).key("alarmCond").initialValue(karabo::data::AlarmCondition::WARN).commit();
    CPPUNIT_ASSERT_EQUAL(karabo::data::AlarmCondition::WARN.asString(),
                         schema.getDefaultValue<std::string>("alarmCond"));
    // Alarm test: success
    validated.clear();

    {
        auto res = validator.validate(schema, data::Hash("alarmCond", "alarm"), validated);

        CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    }
    CPPUNIT_ASSERT(validated.size() == 1);
    CPPUNIT_ASSERT(validated.has("alarmCond"));
    CPPUNIT_ASSERT(validated.get<std::string>("alarmCond") == "alarm");
    CPPUNIT_ASSERT(validated.getAttributes("alarmCond").has(KARABO_INDICATE_ALARM_SET));

    // Alarm test: "is not a valid alarm string"
    validated.clear();

    {
        auto res = validator.validate(schema, data::Hash("alarmCond", "SomeCrazyReason"), validated);
        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'is not a valid alarm string' substring.\nGot:" + res.second,
                               res.second.find("is not a valid alarm string") != std::string::npos);
    }

    // Check minInc, maxInc: out of ... bound

    data::OVERWRITE_ELEMENT(schema)
          .key("int16Property")
          .setNewDefaultValue(7)
          .setNewMinInc(5)
          .setNewMaxInc(10)
          .commit();

    validated.clear();

    {
        auto res = validator.validate(schema, data::Hash("int16Property", 25), validated);

        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'out of upper bound' substring.\nGot:" + res.second,
                               res.second.find("out of upper bound") != std::string::npos);
    }

    validated.clear();

    {
        auto res = validator.validate(schema, data::Hash("int16Property", 2), validated);

        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'out of lower bound' substring.\nGot:" + res.second,
                               res.second.find("out of lower bound") != std::string::npos);
    }

    // Check minExc, maxExc: out of ... bound
    data::OVERWRITE_ELEMENT(schema)
          .key("int16Property")
          .setNewDefaultValue(7)
          .setNewMinExc(5)
          .setNewMaxExc(10)
          .commit();

    validated.clear();

    {
        auto res = validator.validate(schema, data::Hash("int16Property", 10), validated);

        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'out of upper bound' substring.\nGot:" + res.second,
                               res.second.find("out of upper bound") != std::string::npos);
    }

    validated.clear();

    {
        auto res = validator.validate(schema, data::Hash("int16Property", 5), validated);

        CPPUNIT_ASSERT(!res.first);
        CPPUNIT_ASSERT_MESSAGE("Expected error with 'out of lower bound' substring.\nGot:" + res.second,
                               res.second.find("out of lower bound") != std::string::npos);
    }

    using namespace configurationTest;

    validated.clear();

    {
        auto gr = GraphicsRenderer::create("GraphicsRenderer", Hash("color", "orange"));
        const Schema& schema = gr->getSchema("GraphicsRenderer");

        for (int i = 1; i <= 99; ++i) {
            const auto startTimepoint = std::chrono::high_resolution_clock::now();

            // Check top NODE_ELEMENT and BOOL_ELEMENT processing ...
            auto res = validator.validate(schema, Hash("Circle.radius", float(i), "bold", true), validated);

            const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
            elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

            CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
            CPPUNIT_ASSERT(validated.size() == 2);
            CPPUNIT_ASSERT(validated.has("Circle.radius"));
            CPPUNIT_ASSERT(validated.get<float>("Circle.radius") == float(i));
            CPPUNIT_ASSERT(validated.has("bold"));
            CPPUNIT_ASSERT(validated.get<bool>("bold"));
        }
    }

    validated.clear();

    {
        Schema schema;
        OtherSchemaElements::expectedParameters(schema);

        for (int i = 1; i <= 99; ++i) {
            const auto startTimepoint = std::chrono::high_resolution_clock::now();

            // Check VECTOR_DOUBLE_ELEMENT processing...
            auto conf = Hash();
            std::vector<double> vdouble{5.55, 4.44, 3.33};
            conf.set("vecDoubleReconfigStr", vdouble);
            auto res = validator.validate(schema, conf, validated);

            const auto dur = std::chrono::high_resolution_clock::now() - startTimepoint;
            elapsedTimeIn_microseconds += std::chrono::duration_cast<std::chrono::microseconds>(dur).count();

            CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
            CPPUNIT_ASSERT(validated.size() == 1);
            CPPUNIT_ASSERT(validated.has("vecDoubleReconfigStr"));
            const std::vector<double>& vd = validated.get<std::vector<double>>("vecDoubleReconfigStr");
            CPPUNIT_ASSERT(vd[0] == 5.55);
            CPPUNIT_ASSERT(vd[1] == 4.44);
            CPPUNIT_ASSERT(vd[2] == 3.33);
        }
    }
    std::cerr << "\nTest elapsed time (microsecs) : " << elapsedTimeIn_microseconds << std::endl;
}
