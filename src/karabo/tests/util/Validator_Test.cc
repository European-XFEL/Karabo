/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
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

#include "karabo/util/AlarmConditionElement.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/Schema.hh"
#include "karabo/util/SimpleElement.hh"
#include "karabo/util/StateElement.hh"
#include "karabo/util/StringTools.hh"
#include "karabo/util/TableElement.hh"
#include "karabo/util/Validator.hh"
#include "karabo/util/VectorElement.hh"
#include "karabo/xms/SlotElement.hh"

using namespace karabo;
using util::ALARM_ELEMENT;
using util::INT32_ELEMENT;
using util::STATE_ELEMENT;
using util::STRING_ELEMENT;
using util::TABLE_ELEMENT;
using util::VECTOR_CHAR_ELEMENT;
using util::VECTOR_UINT8_ELEMENT;
using xms::SLOT_ELEMENT;

CPPUNIT_TEST_SUITE_REGISTRATION(Validator_Test);

Validator_Test::Validator_Test() {}

Validator_Test::~Validator_Test() {}

void Validator_Test::setUp() {}

void Validator_Test::tearDown() {}


void Validator_Test::testTableMandatoryColumn() {
    util::Validator validator;
    util::Hash validated;

    util::Schema mandatoryRowSchema;
    INT32_ELEMENT(mandatoryRowSchema).key("reqInt").assignmentMandatory().commit();
    INT32_ELEMENT(mandatoryRowSchema).key("int").assignmentOptional().defaultValue(2).commit();

    util::Schema mandTblSchema;
    TABLE_ELEMENT(mandTblSchema).key("mandRowTable").setColumns(mandatoryRowSchema).assignmentMandatory().commit();

    // Tests that table schema with mandatory column missing in its default value will throw exception.
    util::Schema corruptedTblSchema;
    CPPUNIT_ASSERT_THROW(TABLE_ELEMENT(corruptedTblSchema)
                               .key("corruptedTable")
                               .setColumns(mandatoryRowSchema)
                               .assignmentOptional()
                               .defaultValue({util::Hash("int", 128)})
                               .commit(),
                         karabo::util::ParameterException);

    // Test to reject a table with a missing mandatory column.
    std::pair<bool, std::string> res = validator.validate(
          mandTblSchema, util::Hash("mandRowTable", std::vector<util::Hash>(1, util::Hash("int", -2))), validated);
    CPPUNIT_ASSERT(!res.first);

    validated.clear();
}


void Validator_Test::testTableOptionalColumn() {
    util::Schema rowSchema;
    INT32_ELEMENT(rowSchema).key("int").assignmentOptional().defaultValue(1).commit();
    STRING_ELEMENT(rowSchema).key("str").assignmentOptional().defaultValue("a string").commit();

    util::Schema schema;
    TABLE_ELEMENT(schema)
          .key("table")
          .setColumns(rowSchema)
          .assignmentOptional()
          .defaultValue(std::vector<util::Hash>())
          .commit();

    util::Schema nonEmptySchema;
    TABLE_ELEMENT(nonEmptySchema)
          .key("nonEmptyTable")
          .setColumns(rowSchema)
          .assignmentOptional()
          .defaultValue({util::Hash("int", 128, "str", "first row")})
          .commit();

    util::Validator validator;
    util::Hash validated;

    // Test to get default if nothing provided
    std::pair<bool, std::string> res = validator.validate(schema, util::Hash(), validated);

    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash>>("table"));
    CPPUNIT_ASSERT(validated.get<std::vector<util::Hash>>("table").empty());

    validated.clear();

    // Test to get non-empty default if nothing is provided.
    res = validator.validate(nonEmptySchema, util::Hash(), validated);

    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("nonEmptyTable"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash>>("nonEmptyTable"));
    CPPUNIT_ASSERT(validated.get<std::vector<util::Hash>>("nonEmptyTable").size() == 1);
    CPPUNIT_ASSERT_EQUAL(128, validated.get<std::vector<util::Hash>>("nonEmptyTable")[0].get<int>("int"));
    CPPUNIT_ASSERT_EQUAL(std::string("first row"),
                         validated.get<std::vector<util::Hash>>("nonEmptyTable")[0].get<std::string>("str"));

    validated.clear();

    // Test to accept even special case of empty vector<string>:
    // That is what would be received if bound API (try to) send empty vector<Hash>.
    res = validator.validate(schema, util::Hash("table", std::vector<std::string>()), validated);

    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash>>("table"));
    CPPUNIT_ASSERT(validated.get<std::vector<util::Hash>>("table").empty());

    validated.clear();

    // Test to accept a table with fitting row.
    res = validator.validate(
          schema, util::Hash("table", std::vector<util::Hash>(1, util::Hash("int", -2, "str", "testing"))), validated);

    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash>>("table"));
    CPPUNIT_ASSERT_EQUAL(1ul, validated.get<std::vector<util::Hash>>("table").size());
    CPPUNIT_ASSERT_EQUAL(-2, validated.get<std::vector<util::Hash>>("table")[0].get<int>("int"));
    CPPUNIT_ASSERT_EQUAL(std::string("testing"),
                         validated.get<std::vector<util::Hash>>("table")[0].get<std::string>("str"));

    validated.clear();

    // Test to accept a table with an "initially bad" row - there's a missing column, but the table validation
    // attributes allow injection of missing columns.
    res = validator.validate(schema, util::Hash("table", std::vector<util::Hash>(1, util::Hash("int", 2))), validated);
    CPPUNIT_ASSERT(res.first);
    // Checks that the missing column has been injected by the validator - see tableValidationAttributes.
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash>>("table"));
    CPPUNIT_ASSERT_EQUAL(1ul, validated.get<std::vector<util::Hash>>("table").size());
    CPPUNIT_ASSERT(validated.get<std::vector<util::Hash>>("table")[0].has("str"));
    CPPUNIT_ASSERT_EQUAL(std::string("a string"),
                         validated.get<std::vector<util::Hash>>("table")[0].get<std::string>("str"));

    validated.clear();

    // Test to reject a table with a bad row - unknown column name.
    res = validator.validate(
          schema, util::Hash("table", std::vector<util::Hash>(1, util::Hash("unknownKey", 123, "str", "testing"))),
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
          schema, util::Hash("table", std::vector<util::Hash>(1, util::Hash("int", "2", "str", "testing"))), validated);
    CPPUNIT_ASSERT(res.first);
    // Checks that the string value has been properly converted to an int by the validator.
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash>>("table"));
    CPPUNIT_ASSERT_EQUAL(1ul, validated.get<std::vector<util::Hash>>("table").size());
    CPPUNIT_ASSERT_EQUAL(2, validated.get<std::vector<util::Hash>>("table")[0].get<int>("int"));

    validated.clear();
}


void Validator_Test::testTableMinMaxRows() {
    util::Schema rowSchema;
    INT32_ELEMENT(rowSchema).key("int").assignmentOptional().defaultValue(1).commit();
    STRING_ELEMENT(rowSchema).key("str").assignmentOptional().defaultValue("a string").commit();

    util::Schema tblWithMinMaxSchema;
    TABLE_ELEMENT(tblWithMinMaxSchema)
          .key("tblWithMinMax")
          .setColumns(rowSchema)
          .minSize(1)
          .maxSize(1)
          .assignmentOptional()
          .defaultValue({util::Hash("int", 1, "str", "First Row")})
          .commit();

    util::Validator validator;
    util::Hash validated;

    // Checks that a table with 1 row is valid. The validator ensures
    // that the table is valid by adding the row of the default table
    // value.
    std::pair<bool, std::string> res = validator.validate(tblWithMinMaxSchema, util::Hash(), validated);
    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("tblWithMinMax"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash>>("tblWithMinMax"));
    CPPUNIT_ASSERT_EQUAL(1ul, validated.get<std::vector<util::Hash>>("tblWithMinMax").size());
    validated.clear();

    // Checks that a table with more than maxSize rows is invalid.
    std::vector<util::Hash> tblTwoRows{util::Hash("int", 1, "str", "First Row"),
                                       util::Hash("int", 2, "str", "Second Row")};
    res = validator.validate(tblWithMinMaxSchema, util::Hash("tblWithMinMax", tblTwoRows), validated);
    CPPUNIT_ASSERT(!res.first);
    CPPUNIT_ASSERT_MESSAGE("Expected error with 'must have no more than' substring.\nGot: " + res.second,
                           res.second.find("must have no more than") != std::string::npos);
    validated.clear();

    // Checks that a table with less than minSize rows is invalid.
    tblWithMinMaxSchema.setMinSize("tblWithMinMax", 2u);
    tblWithMinMaxSchema.setMaxSize("tblWithMinMax", 2u);
    res = validator.validate(tblWithMinMaxSchema, util::Hash(), validated);
    CPPUNIT_ASSERT(!res.first);
    CPPUNIT_ASSERT_MESSAGE("Expected error with 'must have at least' substring.\nGot:" + res.second,
                           res.second.find("must have at least") != std::string::npos);
    validated.clear();
}


void Validator_Test::testColumnMinMaxAttrs() {
    util::Schema rowSchema;
    INT32_ELEMENT(rowSchema).key("int_1").assignmentOptional().defaultValue(1).minInc(1).maxInc(20).commit();
    INT32_ELEMENT(rowSchema).key("int_2").assignmentOptional().defaultValue(2).minExc(1).maxExc(20).commit();

    util::Schema tblSchema;
    TABLE_ELEMENT(tblSchema)
          .key("tbl")
          .setColumns(rowSchema)
          .assignmentOptional()
          .defaultValue(std::vector<util::Hash>())
          .commit();

    util::Validator validator;
    util::Hash validated;

    // Checks that the empty table doesn't violate columns min, max attributes.
    std::pair<int, std::string> res = validator.validate(tblSchema, util::Hash(), validated);
    CPPUNIT_ASSERT(res.first);
    validated.clear();

    // Checks that minInc and maxInc are enforced.
    std::vector<util::Hash> tblRows{
          util::Hash("int_1", 1, "int_2", 19), // Valid line.
          util::Hash("int_1", 1, "int_2", 20), // int_2 MaxExc(20) violation.
          util::Hash("int_1", 1, "int_2", 1),  // int_2 MinExc(1) violation.
          util::Hash("int_1", 0, "int_2", 19), // int_1 MinInc(1) violation.
          util::Hash("int_1", 21, "int_2", 19) // int_1 MaxInc(20) violation.
    };
    res = validator.validate(tblSchema, util::Hash("tbl", tblRows), validated);
    CPPUNIT_ASSERT(!res.first);
    CPPUNIT_ASSERT_MESSAGE("Expected error with 'Value 20 for parameter' substring.\nGot: " + res.second,
                           res.second.find("Value 20 for parameter") != std::string::npos);
    validated.clear();
    tblRows.erase(tblRows.begin() + 1); // Advances to next invalid line
    res = validator.validate(tblSchema, util::Hash("tbl", tblRows), validated);
    CPPUNIT_ASSERT(!res.first);
    CPPUNIT_ASSERT_MESSAGE("Expected error with 'Value 1 for parameter' substring.\nGot: " + res.second,
                           res.second.find("Value 1 for parameter") != std::string::npos);
    validated.clear();
    tblRows.erase(tblRows.begin() + 1); // Advances to next invalid line
    res = validator.validate(tblSchema, util::Hash("tbl", tblRows), validated);
    CPPUNIT_ASSERT(!res.first);
    CPPUNIT_ASSERT_MESSAGE("Expected error with 'Value 0 for parameter' substring.\nGot: " + res.second,
                           res.second.find("Value 0 for parameter") != std::string::npos);
    validated.clear();
    tblRows.erase(tblRows.begin() + 1); // Advances to next invalid line
    res = validator.validate(tblSchema, util::Hash("tbl", tblRows), validated);
    CPPUNIT_ASSERT(!res.first);
    CPPUNIT_ASSERT_MESSAGE("Expected error with 'Value 21 for parameter' substring.\nGot: " + res.second,
                           res.second.find("Value 21 for parameter") != std::string::npos);
    validated.clear();
}


void Validator_Test::testVectorCharVectorByteSize() {
    using std::vector;
    using util::toString;

    util::Validator validator;
    util::Hash validated;

    util::Schema vecSchValid;
    VECTOR_CHAR_ELEMENT(vecSchValid)
          .key("MinSizeVectChar")
          .description("VectorChar that respects MinSize")
          .minSize(2)
          .readOnly()
          .initialValue({'A', 'B'})
          .commit();
    std::pair<bool, std::string> res = validator.validate(vecSchValid, util::Hash(), validated);
    // vecSchValid should be valid, res.first == true,  with no error message.
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Value of 'MinSizeVectChar' should be equal to provided 'initialValue'.",
                                 toString(vector<char>({'A', 'B'})),
                                 toString(validated.get<vector<char>>("MinSizeVectChar")));

    util::Schema vecSchInvalid;
    CPPUNIT_ASSERT_THROW(VECTOR_CHAR_ELEMENT(vecSchInvalid)
                               .key("MinSizeVectChar")
                               .description("VectorChar with less than MinSize elements")
                               .minSize(1)
                               .readOnly()
                               .initialValue({})
                               .commit(),
                         karabo::util::ParameterException);

    util::Schema vecByteSchValid;
    VECTOR_UINT8_ELEMENT(vecByteSchValid)
          .key("MinSizeVectByte")
          .description("VectorByte that respects MinSize")
          .minSize(2)
          .readOnly()
          .initialValue({0xFF, 0xA2})
          .commit();
    res = validator.validate(vecByteSchValid, util::Hash(), validated);
    // vecByteSchValid must be valid, res.first == true, with no error message.
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Value of 'MinSizeVectByte' should be equal to provided 'initialValue'.",
                                 toString(vector<unsigned char>({0xFF, 0xA2})),
                                 toString(validated.get<vector<unsigned char>>("MinSizeVectByte")));

    util::Schema vecByteSchInvalid;
    CPPUNIT_ASSERT_THROW(VECTOR_UINT8_ELEMENT(vecByteSchValid)
                               .key("MaxSizeVectByte")
                               .description("VectorByte that doesn't respect MaxSize")
                               .minSize(1)
                               .maxSize(2)
                               .readOnly()
                               .initialValue({0xFF, 0xA2, 0x16})
                               .commit(),
                         karabo::util::ParameterException);
}

void Validator_Test::testState() {
    util::Validator validator;
    util::Hash validated;

    util::Schema schema;
    STATE_ELEMENT(schema).key("goofyState").initialValue(util::State::UNKNOWN).commit();

    // Test to reject a state that is set with a bad state.
    std::pair<bool, std::string> res = validator.validate(schema, util::Hash("goofyState", "NotAState"), validated);
    CPPUNIT_ASSERT_MESSAGE(std::string("Validation succeeded unexpectedly :") + util::toString(validated), !res.first);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.second.find("is not a valid state string") != std::string::npos);
    validated.clear();

    // Test to allow a state that is set with a good state.
    res = validator.validate(schema, util::Hash("goofyState", "ERROR"), validated);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    // Test the validated hash should have the attribute set.
    CPPUNIT_ASSERT(validated.getAttributes("goofyState").has(KARABO_INDICATE_STATE_SET));

    validated.clear();
}


void Validator_Test::testAlarms() {
    util::Validator validator;
    util::Hash validated;

    util::Schema schema;
    ALARM_ELEMENT(schema).key("goofyAlarm").commit();

    // Test to reject a state that is set with a bad alarm string
    std::pair<bool, std::string> res =
          validator.validate(schema, util::Hash("goofyAlarm", "LondonIsBurningCallTheEngines"), validated);
    CPPUNIT_ASSERT_MESSAGE(std::string("Validation succeeded unexpectedly :") + util::toString(validated), !res.first);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.second.find("is not a valid alarm string") != std::string::npos);
    validated.clear();

    // Test to allow a state that is set with a good alarm string
    res = validator.validate(schema, util::Hash("goofyAlarm", "alarm"), validated);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    // Test the validated hash should have the attribute set.
    CPPUNIT_ASSERT(validated.getAttributes("goofyAlarm").has(KARABO_INDICATE_ALARM_SET));

    validated.clear();
}


void Validator_Test::testSlots() {
    util::Schema s;
    SLOT_ELEMENT(s).key("slot").commit();

    // Slot does not appear in validated config.
    util::Hash in, out;
    util::Validator val;
    std::pair<bool, std::string> res = val.validate(s, in, out);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    CPPUNIT_ASSERT_MESSAGE(util::toString(out), out.empty());

    // Empty node allowed for slot (for backward compatibility).
    in.set("slot", util::Hash());
    res = val.validate(s, in, out);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    CPPUNIT_ASSERT_MESSAGE(util::toString(out), out.empty());

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
    util::Schema schema;
    util::BOOL_ELEMENT(schema)
          .key("boolProperty")
          .displayedName("Bool property")
          .description("A bool property")
          .init()
          .assignmentInternal()
          .defaultValue(false)
          .commit();

    util::Validator validator;
    util::Hash validated;
    // Test default without setting
    std::pair<bool, std::string> res;
    res = validator.validate(schema, util::Hash(), validated);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    CPPUNIT_ASSERT(validated.has("boolProperty"));
    CPPUNIT_ASSERT(!validated.get<bool>("boolProperty"));
    // Test to set a parameter with assignment internal
    validated.clear();
    res = validator.validate(schema, util::Hash("boolProperty", true), validated);
    CPPUNIT_ASSERT_MESSAGE(res.second, res.first);
    CPPUNIT_ASSERT(validated.has("boolProperty"));
    CPPUNIT_ASSERT(validated.get<bool>("boolProperty"));
}
