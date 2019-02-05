/*
 * File:   DataLogUtils_Test.cc
 * Author: costar
 *
 * Created on February 4, 2019, 9:58 AM
 */

#include "DataLogUtils_Test.hh"
#include <karabo/util/DataLogUtils.hh>

CPPUNIT_TEST_SUITE_REGISTRATION(DataLogUtils_Test);


DataLogUtils_Test::DataLogUtils_Test() :
    m_indexRegex(karabo::util::DATALOG_INDEX_LINE_REGEX, boost::regex::extended),
    m_indexTailRegex(karabo::util::DATALOG_INDEX_TAIL_REGEX, boost::regex::extended) {

}


void DataLogUtils_Test::setUp() {
}


void DataLogUtils_Test::tearDown() {
}


void DataLogUtils_Test::testValidIndexLines() {

    std::vector<std::pair<std::string, std::vector < std::string>>> resultTable
    {
        std::make_pair("+LOG 20190204T094210.961209Z 1549273330.961209 0 0 . 0",
                       std::vector<std::string>{"+LOG", "20190204T094210.961209Z", "1549273330.961209", "0", "0", ".", "0"}),

        std::make_pair("+LOG 20190204T094210.961209Z 1549273330.961209 12345677 0 . 0", // tranId > 0
                       std::vector<std::string>{"+LOG", "20190204T094210.961209Z", "1549273330.961209", "12345677", "0", ".", "0"}),

        std::make_pair("+LOG 20190204T094210.961209Z 1549273330.961209 0 0 real_user 0", // defined user name
                       std::vector<std::string>{"+LOG", "20190204T094210.961209Z", "1549273330.961209", "0", "0", "real_user", "0"}),

        std::make_pair("+LOG 20190204T094210.961209Z 1549273330.961209 0 0 . 1230", // non-0 file index
                       std::vector<std::string>{"+LOG", "20190204T094210.961209Z", "1549273330.961209", "0", "0", ".", "1230"}),

        std::make_pair("+LOG 20190204T094210.961209Z 1549273330 0 0 . 0", // full second, no microseconds
                       std::vector<std::string>{"+LOG", "20190204T094210.961209Z", "1549273330", "0", "0", ".", "0"}),

        std::make_pair("-LOG 20190204T094210.961209Z 1549273330.961209 0 0 . 0", // -LOG
                       std::vector<std::string>{"-LOG", "20190204T094210.961209Z", "1549273330.961209", "0", "0", ".", "0"}),

        std::make_pair("=NEW 20190204T094210.961209Z 1549273330.961209 0 0 . 0", // =NEW
                       std::vector<std::string>{"=NEW", "20190204T094210.961209Z", "1549273330.961209", "0", "0", ".", "0"}),

        std::make_pair("+LOG 20190204T094210.961209Z 1549273330.961209 0 987654 . 0", // non-zero position in index file
                       std::vector<std::string>{"+LOG", "20190204T094210.961209Z", "1549273330.961209", "0", "987654", ".", "0"}),
    };

    for (auto& aPair : resultTable) {
        boost::smatch indexFields;
        CPPUNIT_ASSERT(boost::regex_search(aPair.first, indexFields, m_indexRegex));
        CPPUNIT_ASSERT_EQUAL(aPair.second[0], std::string(indexFields[1])); // indexFields[1] is not a string - LOG
        CPPUNIT_ASSERT_EQUAL(aPair.second[1], std::string(indexFields[2])); // iso timestamp
        CPPUNIT_ASSERT_EQUAL(aPair.second[2], std::string(indexFields[3])); // double timestamp
        // Now the tail:
        CPPUNIT_ASSERT(boost::regex_search(std::string(indexFields[4]), indexFields, m_indexTailRegex));
        CPPUNIT_ASSERT_EQUAL(aPair.second[3], std::string(indexFields[1])); // train id
        CPPUNIT_ASSERT_EQUAL(aPair.second[4], std::string(indexFields[2])); // index file position
        CPPUNIT_ASSERT_EQUAL(aPair.second[5], std::string(indexFields[3])); // user name
        CPPUNIT_ASSERT_EQUAL(aPair.second[6], std::string(indexFields[4])); // file number
    }
}


void DataLogUtils_Test::testInvalidIndexLines() {

    std::vector<std::pair<std::string, int>> resultsTable
    {
        // The int which is the second element of the pair indicates whether the regex that didn't match was the
        // one for the whole line (value 1) or was the one for the regex for the line tail (value 2).
        std::make_pair("+LOG 20190204+LOG 20190204T094210.961209Z 1549273330.961209 0 0 . 0", 1), // intermingled contents

        std::make_pair("+LOG 20190204T094210.961209Z 1549273330.961209 -1 0 . 0", 2), // negative tranId 

        std::make_pair("+LOG 20190204T094210.961209Z 1549273330.961209 0 0 0", 2), // missing user name

        std::make_pair("+LOG 20190204T094210.961209Z 1549273330.961209 0 0 . -1", 2), // negative file index

        std::make_pair("+LOG 2AD0190204T094210.961209Z 1549273330 0 0 . 0", 1), // invalid Iso8061 timestamp

        std::make_pair("*LOG 20190204T094210.961209Z 1549273330.961209 0 0 . 0", 1), // invalid event specifier

        std::make_pair("=NEW 20190204T094210.961209Z 1549273330.961209 0 -21 . -1", 2), // invalid file position

        std::make_pair("+LOG 20190204T094210.961209Z 15492AB73330.961209 0 987654 . 0", 1), // invalid numeric timestamp
    };

    int invalidTestIdx = 0;
    for (auto& aPair : resultsTable) {
        boost::smatch indexFields;
        int faillingRegEx = 0;
        bool matchIndex = boost::regex_search(aPair.first, indexFields, m_indexRegex);
        if (!matchIndex) {
            faillingRegEx = 1;
        } else {
            boost::smatch tailFields;
            bool matchTail = boost::regex_search(std::string(indexFields[4]), tailFields, m_indexTailRegex);
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

