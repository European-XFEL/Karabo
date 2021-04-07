/*
 * File:   Validator_Test.hh
 * Author: flucke
 *
 * Created on September 9, 2016, 12:18 PM
 */

#ifndef VALIDATOR_TEST_HH
#define	VALIDATOR_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

class Validator_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(Validator_Test);
    CPPUNIT_TEST(testTableOptionalColumn);
    CPPUNIT_TEST(testTableMandatoryColumn);
    CPPUNIT_TEST(testTableMinMaxRows);
    CPPUNIT_TEST(testColumnMinMaxAttrs);
    CPPUNIT_TEST_SUITE_END();

public:
    Validator_Test();
    virtual ~Validator_Test();
    void setUp();
    void tearDown();

private:

    /**
     * @brief Checks that optional columns with default values in the
     * row schema are properly initialized to their default values,
     * even when the default row value for the table element is an
     * empty Hash.
     *
     * The initialization is performed as part of the validation process.
     */
    void testTableOptionalColumn();

    /**
     * @brief Checks that mandatory columns in the row schema are
     * indeed required.
     *
     * An attempt to set the default value for the table rows with
     * a mandatory column missing throws an exception (ParameterException).
     *
     * A table element (actually a vector of Hashes with a defined
     * RowSchema) fails validation if any mandatory column specified
     * in the row schema is missing.
     *
     * @note As mandatory columns will be dropped in the future, this
     * test case will only be kept until the dropping happens. In the
     * meantime, the checkings made in here guarantee the contract of
     * validation for mandatory columns.
     */
    void testTableMandatoryColumn();

    /**
     * @brief Checks that minSize and maxSize attributes for table elements
     * size are enforced.
     */
    void testTableMinMaxRows();

    /**
     * @brief Checks that minInc, maxInc, minExc, and maxExc attributes for
     * table cells (elements in the RowSchema) are enforced.
     */
    void testColumnMinMaxAttrs();
};

#endif	/* VALIDATOR_TEST_HH */
