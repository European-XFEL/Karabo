/* 
 * File:   DataLogUtils_Test.hh
 * Author: costar
 *
 * Created on February 4, 2019, 9:58 AM
 */

#ifndef DATALOGUTILS_TEST_HH
#define	DATALOGUTILS_TEST_HH

#include <boost/regex.hpp>
#include <cppunit/extensions/HelperMacros.h>

class DataLogUtils_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(DataLogUtils_Test);

    CPPUNIT_TEST(testValidIndexString);
    CPPUNIT_TEST(testInvalidEventField);
    CPPUNIT_TEST(testInvalidIsoTimestampField);
    CPPUNIT_TEST(testValidTailString);
    CPPUNIT_TEST(testInvalidTailPositionField);
    CPPUNIT_TEST(testInvalidTailFileIndexField);

    CPPUNIT_TEST_SUITE_END();

public:

    DataLogUtils_Test();
    virtual ~DataLogUtils_Test() = default;
    void setUp();
    void tearDown();

private:

    /**
     * Tests a valid data logger index line. For a detailed description
     * of the index line format see the declaration of
     * karabo::util::DataLogUtils::DATALOG_INDEX_LINE_REGEX.
     */
    void testValidIndexString();

    /**
     * Tests a valid data logger index line tail. The index line tail is
     * composed of all the fields in the index line that come after the
     * event field and the two timestamp fields that follow it.
     * Detailed description of the tail can be found at
     * karabo::util::DataLogUtils::DATALOG_INDEX_TAIL_REGEX.
     */
    void testValidTailString();


    /**
     * An index line with an invalid event field.
     */
    void testInvalidEventField();

    /**
     * An index line with an invalid Iso8601 invalid timestamp field.
     */
    void testInvalidIsoTimestampField();

    /**
     * An index line tail with an invalid position field.
     */
    void testInvalidTailPositionField();

    /**
     * An index line tail with an invalid file index field.
     */
    void testInvalidTailFileIndexField();

    boost::regex m_indexRegex;
    boost::regex m_indexTailRegex;

};

#endif	/* DATALOGUTILS_TEST_HH */

