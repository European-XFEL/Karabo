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
 * File:   DataLogUtils_Test.cc
 * Author: costar
 *
 * Created on February 4, 2019, 9:58 AM
 */
#include "DataLogUtils_Test.hh"

#include <karabo/util/DataLogUtils.hh>

#include "karabo/util/Exception.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(DataLogUtils_Test);

namespace nl = nlohmann;

DataLogUtils_Test::DataLogUtils_Test()
    : m_indexRegex(karabo::util::DATALOG_INDEX_LINE_REGEX), m_indexTailRegex(karabo::util::DATALOG_INDEX_TAIL_REGEX) {}


void DataLogUtils_Test::setUp() {}


void DataLogUtils_Test::tearDown() {}


void DataLogUtils_Test::testValidIndexLines() {
    std::vector<std::pair<std::string, std::vector<std::string>>> resultTable{
          std::make_pair(
                "+LOG 20190204T094210.961209Z 1549273330.961209 0 0 . 0",
                std::vector<std::string>{"+LOG", "20190204T094210.961209Z", "1549273330.961209", "0", "0", ".", "0"}),

          std::make_pair("+LOG 20190204T094210.961209Z 1549273330.961209 12345677 0 . 0", // tranId > 0
                         std::vector<std::string>{"+LOG", "20190204T094210.961209Z", "1549273330.961209", "12345677",
                                                  "0", ".", "0"}),

          std::make_pair("+LOG 20190204T094210.961209Z 1549273330.961209 0 0 real_user 0", // defined user name
                         std::vector<std::string>{"+LOG", "20190204T094210.961209Z", "1549273330.961209", "0", "0",
                                                  "real_user", "0"}),

          std::make_pair("+LOG 20190204T094210.961209Z 1549273330.961209 0 0 . 1230", // non-0 file index
                         std::vector<std::string>{"+LOG", "20190204T094210.961209Z", "1549273330.961209", "0", "0", ".",
                                                  "1230"}),

          std::make_pair("+LOG 20190204T094210.961209Z 1549273330 0 0 . 0", // full second, no microseconds
                         std::vector<std::string>{"+LOG", "20190204T094210.961209Z", "1549273330", "0", "0", ".", "0"}),

          std::make_pair(
                "-LOG 20190204T094210.961209Z 1549273330.961209 0 0 . 0", // -LOG
                std::vector<std::string>{"-LOG", "20190204T094210.961209Z", "1549273330.961209", "0", "0", ".", "0"}),

          std::make_pair(
                "=NEW 20190204T094210.961209Z 1549273330.961209 0 0 . 0", // =NEW
                std::vector<std::string>{"=NEW", "20190204T094210.961209Z", "1549273330.961209", "0", "0", ".", "0"}),

          std::make_pair(
                "+LOG 20190204T094210.961209Z 1549273330.961209 0 987654 . 0", // non-zero position in index file
                std::vector<std::string>{"+LOG", "20190204T094210.961209Z", "1549273330.961209", "0", "987654", ".",
                                         "0"}),
    };

    for (auto& aPair : resultTable) {
        std::smatch indexFields;
        CPPUNIT_ASSERT(std::regex_search(aPair.first, indexFields, m_indexRegex));
        CPPUNIT_ASSERT_EQUAL(aPair.second[0], std::string(indexFields[1])); // indexFields[1] is not a string - LOG
        CPPUNIT_ASSERT_EQUAL(aPair.second[1], std::string(indexFields[2])); // iso timestamp
        CPPUNIT_ASSERT_EQUAL(aPair.second[2], std::string(indexFields[3])); // double timestamp
        // Now the tail:
        std::smatch tailFields;
        std::string tail = std::string(indexFields[4]);
        CPPUNIT_ASSERT(std::regex_search(tail, tailFields, m_indexTailRegex));
        CPPUNIT_ASSERT_EQUAL(aPair.second[3], std::string(tailFields[1])); // train id
        CPPUNIT_ASSERT_EQUAL(aPair.second[4], std::string(tailFields[2])); // index file position
        CPPUNIT_ASSERT_EQUAL(aPair.second[5], std::string(tailFields[3])); // user name
        CPPUNIT_ASSERT_EQUAL(aPair.second[6], std::string(tailFields[4])); // file number
    }
}


void DataLogUtils_Test::testInvalidIndexLines() {
    std::vector<std::pair<std::string, int>> resultsTable{
          // The int which is the second element of the pair indicates whether the regex that didn't match was the
          // one for the whole line (value 1) or was the one for the regex for the line tail (value 2).
          std::make_pair("+LOG 20190204+LOG 20190204T094210.961209Z 1549273330.961209 0 0 . 0",
                         1), // intermingled contents

          std::make_pair("+LOG 20190204T094210.961209Z 1549273330.961209 -1 0 . 0", 2), // negative tranId

          std::make_pair("+LOG 20190204T094210.961209Z 1549273330.961209 0 0 0", 2), // missing user name

          std::make_pair("+LOG 20190204T094210.961209Z 1549273330.961209 0 0 . -1", 2), // negative file index

          std::make_pair("+LOG 2AD0190204T094210.961209Z 1549273330 0 0 . 0", 1), // invalid Iso8061 timestamp

          std::make_pair("*LOG 20190204T094210.961209Z 1549273330.961209 0 0 . 0", 1), // invalid event specifier

          std::make_pair("=NEW 20190204T094210.961209Z 1549273330.961209 0 -21 . -1", 2), // invalid file position

          std::make_pair("+LOG 20190204T094210.961209Z 15492AB73330.961209 0 987654 . 0",
                         1), // invalid numeric timestamp
    };

    int invalidTestIdx = 0;
    for (auto& aPair : resultsTable) {
        std::smatch indexFields;
        int faillingRegEx = 0;
        bool matchIndex = std::regex_search(aPair.first, indexFields, m_indexRegex);
        if (!matchIndex) {
            faillingRegEx = 1;
        } else {
            std::smatch tailFields;
            std::string tail = indexFields[4];
            bool matchTail = std::regex_search(tail, tailFields, m_indexTailRegex);
            if (!matchTail) {
                faillingRegEx = 2;
            }
        }
        CPPUNIT_ASSERT_MESSAGE("For invalid test #" + karabo::util::toString(invalidTestIdx) +
                                     " a faillingRegEx value of " + karabo::util::toString(aPair.second) +
                                     " was expected, but got " + karabo::util::toString(faillingRegEx),
                               aPair.second == faillingRegEx);
        invalidTestIdx++;
    }
}


void DataLogUtils_Test::testValueFromJSON() {
    boost::optional<std::string> value;
    nl::json j = nl::json::parse(std::string("null"));
    value = karabo::util::jsonValueAsString(j);
    CPPUNIT_ASSERT_MESSAGE("optional value set on null input", !value);
    j = nl::json::parse(std::string("\"anything\""));
    value = karabo::util::jsonValueAsString(j);
    CPPUNIT_ASSERT_MESSAGE("optional value not set on string input", value);
    j = nl::json::parse(std::string("true"));
    value = karabo::util::jsonValueAsString(j);
    CPPUNIT_ASSERT_MESSAGE("optional value not set on bool input", value);
    j = nl::json::parse(std::string("0.1"));
    value = karabo::util::jsonValueAsString(j);
    CPPUNIT_ASSERT_MESSAGE("optional value not set on float input", value);
    j = nl::json::parse(std::string("42"));
    value = karabo::util::jsonValueAsString(j);
    CPPUNIT_ASSERT_MESSAGE("optional value not set on integer input", value);
}


void DataLogUtils_Test::testMultipleJSONObjects() {
    std::string simple =
          ""
          "{\"results\":"
          "[{"
          "\"statement_id\":0,"
          "\"series\":[{"
          "\"name\":\"prop_name\","
          "\"columns\":[\"time\",\"value\"],"
          "\"values\":["
          "[1597043525897755,40],"
          "[1597043525897855,null]"
          "]}]"
          "}]"
          "}";

    karabo::util::InfluxResultSet simpleInfluxResult;
    karabo::util::jsonResultsToInfluxResultSet(simple, simpleInfluxResult, "");

    CPPUNIT_ASSERT_EQUAL(2ul, simpleInfluxResult.first.size());              // check the number of columns
    CPPUNIT_ASSERT_EQUAL(std::string("time"), simpleInfluxResult.first[0]);  // check the column title
    CPPUNIT_ASSERT_EQUAL(std::string("value"), simpleInfluxResult.first[1]); // check the column title
    CPPUNIT_ASSERT_EQUAL(2ul, simpleInfluxResult.second.size());             // check the number of rows
    CPPUNIT_ASSERT(simpleInfluxResult.second[0][0]);                         // 1st row, 1st column is not null
    CPPUNIT_ASSERT_EQUAL(std::string("1597043525897755"),
                         *simpleInfluxResult.second[0][0]);                    // 1st row, 1st column: check value
    CPPUNIT_ASSERT(simpleInfluxResult.second[0][1]);                           // 1st row, 2nd column is not null
    CPPUNIT_ASSERT_EQUAL(std::string("40"), *simpleInfluxResult.second[0][1]); // 1st row, 2nd column: check value
    CPPUNIT_ASSERT(simpleInfluxResult.second[1][0]);                           // 2nd row, 1st column is not null
    CPPUNIT_ASSERT_EQUAL(std::string("1597043525897855"),
                         *simpleInfluxResult.second[1][0]); // 2nd row, 1st column: check value
    CPPUNIT_ASSERT(!simpleInfluxResult.second[1][1]);       // 2nd row, 2nd column **is** null
    std::string complex =
          ""
          "{\"results\":"
          "[{"
          "\"statement_id\":0,"
          "\"series\":[{"
          "\"name\":\"prop_name\","
          "\"columns\":[\"time\",\"value\"],"
          "\"values\":[[1597043525897755,40],[1597043525897855,42]],"
          "\"partial\":true"
          "}],"
          "\"partial\":true"
          "}]"
          "}\n{\"results\":"
          "[{"
          "\"statement_id\":0,"
          "\"series\":[{"
          "\"name\":\"prop_name\","
          "\"columns\":[\"time\",\"value\"],"
          "\"values\":[[1597043525897955,44],[1597043525898055,46]]"
          "}]"
          "}]}";

    karabo::util::InfluxResultSet complexInfluxResult;

    karabo::util::jsonResultsToInfluxResultSet(complex, complexInfluxResult, "");
    CPPUNIT_ASSERT_EQUAL(2ul, complexInfluxResult.first.size());              // check the number of columns
    CPPUNIT_ASSERT_EQUAL(std::string("time"), complexInfluxResult.first[0]);  // check the column title
    CPPUNIT_ASSERT_EQUAL(std::string("value"), complexInfluxResult.first[1]); // check the column title
    CPPUNIT_ASSERT_EQUAL(4ul, complexInfluxResult.second.size());             // check the number of rows
    CPPUNIT_ASSERT_EQUAL(std::string("1597043525897755"), *complexInfluxResult.second[0][0]);
    CPPUNIT_ASSERT_EQUAL(std::string("40"), *complexInfluxResult.second[0][1]);
    CPPUNIT_ASSERT_EQUAL(std::string("1597043525897855"), *complexInfluxResult.second[1][0]);
    CPPUNIT_ASSERT_EQUAL(std::string("42"), *complexInfluxResult.second[1][1]);
    CPPUNIT_ASSERT_EQUAL(std::string("1597043525897955"), *complexInfluxResult.second[2][0]);
    CPPUNIT_ASSERT_EQUAL(std::string("44"), *complexInfluxResult.second[2][1]);
    CPPUNIT_ASSERT_EQUAL(std::string("1597043525898055"), *complexInfluxResult.second[3][0]);
    CPPUNIT_ASSERT_EQUAL(std::string("46"), *complexInfluxResult.second[3][1]);


    std::string mixed =
          ""
          "{\"results\":"
          "[{"
          "\"statement_id\":0,"
          "\"series\":[{"
          "\"name\":\"prop_name\","
          "\"columns\":[\"time\",\"value\"],"
          "\"values\":[[1597043525897755,40],[1597043525897855,42]],"
          "\"partial\":true"
          "}],"
          "\"partial\":true"
          "}]"
          "}\n{\"results\":"
          "[{"
          "\"statement_id\":0,"
          "\"series\":[{"
          "\"name\":\"prop_name\","
          "\"columns\":[\"time\",\"ANOTHER_ONE!\"],"
          "\"values\":[[1597043525897955,44],[1597043525898055,46]]"
          "}]"
          "}]}";

    CPPUNIT_ASSERT_THROW(karabo::util::jsonResultsToInfluxResultSet(mixed, complexInfluxResult, ""),
                         karabo::util::NotSupportedException);
}
