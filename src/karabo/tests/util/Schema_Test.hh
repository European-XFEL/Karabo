/* 
 * File:   Schema_Test.hh
 * Author: irinak
 *
 * Created on September 28, 2012, 1:14 PM
 */

#ifndef SCHEMA_TEST_HH
#define	SCHEMA_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/util/Schema.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/VectorElement.hh>
#include <karabo/util/ImageElement.hh>

class Schema_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Schema_Test);
    CPPUNIT_TEST(testHasKey);
    CPPUNIT_TEST_SUITE_END();

public:
    Schema_Test();
    virtual ~Schema_Test();

private:
    void settingExpectedParameters(karabo::util::Schema&);
    void testHasKey();
};

#endif	/* SCHEMA_TEST_HH */

