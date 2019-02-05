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


void DataLogUtils_Test::testValidIndexString() {
    const std::string indexLine{"+LOG 20190204T094210.961209Z 1549273330.961209 0 0 . 0"};

    boost::smatch indexFields;
    bool matches = boost::regex_search(indexLine, indexFields, m_indexRegex);

    CPPUNIT_ASSERT(matches);
    CPPUNIT_ASSERT(indexFields[1] == "+LOG");
    CPPUNIT_ASSERT(indexFields[2] == "20190204T094210.961209Z");
    CPPUNIT_ASSERT(indexFields[3] == "1549273330.961209");
    CPPUNIT_ASSERT(indexFields[4] == "0 0 . 0");
}


void DataLogUtils_Test::testValidTailString() {

    const std::string indexTail{"0 0 . 0"};

    boost::smatch tailFields;
    bool matches = boost::regex_search(indexTail, tailFields, m_indexTailRegex);

    CPPUNIT_ASSERT(matches);
    CPPUNIT_ASSERT(tailFields[1] == "0");
    CPPUNIT_ASSERT(tailFields[2] == "0");
    CPPUNIT_ASSERT(tailFields[3] == ".");
    CPPUNIT_ASSERT(tailFields[4] == "0");
}


void DataLogUtils_Test::testInvalidEventField() {
    const std::string indexLine{"+LOG2019+LOG 20190204T094210.961209Z 1549273330.961209 0 0 . 0"};

    boost::smatch indexFields;
    bool matches = boost::regex_search(indexLine, indexFields, m_indexRegex);

    CPPUNIT_ASSERT(matches == false);
}


void DataLogUtils_Test::testInvalidTailPositionField() {
    const std::string indexTail{"0 P0 . 0"};

    boost::smatch tailFields;
    bool matches = boost::regex_search(indexTail, tailFields, m_indexTailRegex);

    CPPUNIT_ASSERT(matches == false);
}


void DataLogUtils_Test::testInvalidIsoTimestampField() {
    const std::string indexLine{"+LOG 20190204T094210F.961209Z 1549273330.961209 0 0 . 0"};

    boost::smatch indexFields;
    bool matches = boost::regex_search(indexLine, indexFields, m_indexRegex);

    CPPUNIT_ASSERT(matches == false);
}


void DataLogUtils_Test::testInvalidTailFileIndexField() {
    const std::string indexTail{"0 0 . -1"};

    boost::smatch tailFields;
    bool matches = boost::regex_search(indexTail, tailFields, m_indexTailRegex);

    CPPUNIT_ASSERT(matches == false);
}
