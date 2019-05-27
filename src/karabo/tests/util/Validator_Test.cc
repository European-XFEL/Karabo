/* 
 * File:   Validator_Test.cc
 * Author: flucke
 * 
 * Created on September 9, 2016, 12:18 PM
 */

#include <vector>
#include <utility>
#include <string>

#include "Validator_Test.hh"

#include "karabo/util/Schema.hh"
#include "karabo/util/Hash.hh"
#include "karabo/util/Validator.hh"
#include "karabo/util/TableElement.hh"
#include "karabo/util/SimpleElement.hh"

using namespace karabo;
using util::TABLE_ELEMENT;
using util::INT32_ELEMENT;
using util::STRING_ELEMENT;

CPPUNIT_TEST_SUITE_REGISTRATION(Validator_Test);


Validator_Test::Validator_Test() {
}


Validator_Test::~Validator_Test() {
}


void Validator_Test::setUp() {

}


void Validator_Test::tearDown() {

}


void Validator_Test::testTableMandatoryColumn() {

    util::Validator validator;
    util::Hash validated;

    util::Schema mandatoryRowSchema;
    INT32_ELEMENT(mandatoryRowSchema).key("reqInt")
            .assignmentMandatory()
            .commit();
    INT32_ELEMENT(mandatoryRowSchema).key("int")
            .assignmentOptional().defaultValue(2)
            .commit();

    util::Schema mandTblSchema;
    TABLE_ELEMENT(mandTblSchema).key("mandRowTable")
            .setColumns(mandatoryRowSchema).assignmentMandatory()
            .commit();

    // Tests that table schema with mandatory column missing in its default value will throw exception.
    util::Schema corruptedTblSchema;
    CPPUNIT_ASSERT_THROW(TABLE_ELEMENT(corruptedTblSchema).key("corruptedTable")
                         .setColumns(mandatoryRowSchema)
                         .assignmentOptional().defaultValue({util::Hash("int", 128)})
                         .commit(),
                         karabo::util::ParameterException);

    // Test to reject a table with a missing mandatory column.
    std::pair<bool, std::string> res = validator.validate(mandTblSchema,
                             util::Hash("mandRowTable", std::vector<util::Hash>(1, util::Hash("int", -2))),
                             validated);
    CPPUNIT_ASSERT(!res.first);

    validated.clear();
}


void Validator_Test::testTableOptionalColumn() {

    util::Schema rowSchema;
    INT32_ELEMENT(rowSchema).key("int")
            .assignmentOptional().defaultValue(1)
            .commit();
    STRING_ELEMENT(rowSchema).key("str")
            .assignmentOptional().defaultValue("a string")
            .commit();

    util::Schema schema;
    TABLE_ELEMENT(schema).key("table")
            .setColumns(rowSchema)
            .assignmentOptional().defaultValue(std::vector<util::Hash>())
            .commit();

    util::Schema nonEmptySchema;
    TABLE_ELEMENT(nonEmptySchema).key("nonEmptyTable")
            .setColumns(rowSchema)
            .assignmentOptional().defaultValue({util::Hash("int", 128, "str", "first row")})
    .commit();

    util::Validator validator;
    util::Hash validated;

    // Test to get default if nothing provided
    std::pair<bool, std::string> res = validator.validate(schema, util::Hash(), validated);

    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash> >("table"));
    CPPUNIT_ASSERT(validated.get<std::vector<util::Hash> >("table").empty());

    validated.clear();

    // Test to get non-empty default if nothing is provided.
    res = validator.validate(nonEmptySchema, util::Hash(), validated);

    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("nonEmptyTable"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash> >("nonEmptyTable"));
    CPPUNIT_ASSERT(validated.get<std::vector<util::Hash> >("nonEmptyTable").size() == 1);
    CPPUNIT_ASSERT_EQUAL(128, validated.get<std::vector<util::Hash> >("nonEmptyTable")[0].get<int>("int"));
    CPPUNIT_ASSERT_EQUAL(std::string("first row"), validated.get<std::vector<util::Hash> >("nonEmptyTable")[0].get<std::string>("str"));

    validated.clear();

    // Test to accept even special case of empty vector<string>:
    // That is what would be received if bound API (try to) send empty vector<Hash>.
    res = validator.validate(schema, util::Hash("table", std::vector<std::string>()), validated);

    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash> >("table"));
    CPPUNIT_ASSERT(validated.get<std::vector<util::Hash> >("table").empty());

    validated.clear();

    // Test to accept a table with fitting row.
    res = validator.validate(schema,
                             util::Hash("table", std::vector<util::Hash>(1, util::Hash("int", -2, "str", "testing"))),
                             validated);

    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash> >("table"));
    CPPUNIT_ASSERT(1u == validated.get<std::vector<util::Hash> >("table").size());
    CPPUNIT_ASSERT_EQUAL(-2, validated.get<std::vector<util::Hash> >("table")[0].get<int>("int"));
    CPPUNIT_ASSERT_EQUAL(std::string("testing"), validated.get<std::vector<util::Hash> >("table")[0].get<std::string>("str"));

    validated.clear();

    // Test to accept a table with an "initially bad" row - there's a missing column, but the table validation
    // attributes allow injection of missing columns.
    res = validator.validate(schema,
                             util::Hash("table", std::vector<util::Hash>(1, util::Hash("int", 2))),
                             validated);
    CPPUNIT_ASSERT(res.first);
    // Checks that the missing column has been injected by the validator - see tableValidationAttributes.
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash> >("table"));
    CPPUNIT_ASSERT(1u == validated.get<std::vector<util::Hash> >("table").size());
    CPPUNIT_ASSERT(validated.get<std::vector<util::Hash> >("table")[0].has("str"));
    CPPUNIT_ASSERT_EQUAL(std::string("a string"),
                         validated.get<std::vector < util::Hash >> ("table")[0].get<std::string>("str"));

    validated.clear();

    // Test to reject a table with a bad row - unknown column name.
    res = validator.validate(schema,
                             util::Hash("table", std::vector<util::Hash>(1, util::Hash("unknownKey", 123, "str", "testing"))),
                             validated);
    CPPUNIT_ASSERT(!res.first);

    validated.clear();

    // Test to accept a table with an "initially bad" value - the value of the int column is the string form of an int.
    // The validator should do the conversion.
    // Caveat: The validator currently enforces a "best effort" policy to convert really "bad" values into a "good"
    //         values that match the value type stated in the schema for the column. Examples of those edge cases that
    //         could lead to unacceptable behavior (bugs) depending on the use cases: a value "2a" would be converted
    //         to int 2; a float 4.6 would be truncated to 4; a value "abc" would become int 0 (the initial default value
    //         for an int).
    res = validator.validate(schema,
                             util::Hash("table", std::vector<util::Hash>(1, util::Hash("int", "2", "str", "testing"))),
                             validated);
    CPPUNIT_ASSERT(res.first);
    // Checks that the string value has been properly converted to an int by the validator.
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash> >("table"));
    CPPUNIT_ASSERT(1u == validated.get<std::vector<util::Hash> >("table").size());
    CPPUNIT_ASSERT_EQUAL(2, validated.get<std::vector<util::Hash> >("table")[0].get<int>("int"));

    validated.clear();
}
