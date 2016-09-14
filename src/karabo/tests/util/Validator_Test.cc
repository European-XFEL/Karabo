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

CPPUNIT_TEST_SUITE_REGISTRATION(Validator_Test);


Validator_Test::Validator_Test() {
}


Validator_Test::~Validator_Test() {
}


void Validator_Test::setUp() {

}


void Validator_Test::tearDown() {

}


void Validator_Test::testTableDefault() {

    util::Schema rowSchema;
    INT32_ELEMENT(rowSchema).key("int")
            .assignmentOptional().defaultValue(1)
            .readOnly()
            .commit();

    util::Schema schema;
    TABLE_ELEMENT(schema).key("table")
            .setNodeSchema(rowSchema)
            .assignmentOptional().defaultValue(std::vector<util::Hash>())
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

    // Test to accept even special case of empty vector<string>:
    // That is what would be received if bound API (try to) send empty vector<Hash>.
    res = validator.validate(schema, util::Hash("table", std::vector<std::string>()), validated);

    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash> >("table"));
    CPPUNIT_ASSERT(validated.get<std::vector<util::Hash> >("table").empty());

    validated.clear();

    // Test to accept a table with fitting row.
    res = validator.validate(schema, util::Hash("table", std::vector<util::Hash>(1, util::Hash("int", -2))), validated);

    CPPUNIT_ASSERT(res.first);
    CPPUNIT_ASSERT(validated.has("table"));
    CPPUNIT_ASSERT(validated.is<std::vector<util::Hash> >("table"));
    CPPUNIT_ASSERT(1u == validated.get<std::vector<util::Hash> >("table").size());
    CPPUNIT_ASSERT_EQUAL(-2, validated.get<std::vector<util::Hash> >("table")[0].get<int>("int"));
}

