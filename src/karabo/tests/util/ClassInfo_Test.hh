/* 
 * $Id$
 * 
 * Author: <irina.kozlova@xfel.eu>
 * 
 * Created on October 29, 2012, 12:38 PM
 */

#ifndef CLASSINFO_TEST_HH
#define	CLASSINFO_TEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include <karabo/util/Schema.hh>
#include "resources/BobbyCar.hh"
#include "resources/Vehicle.hh"

class ClassInfo_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(ClassInfo_Test);

    CPPUNIT_TEST(testGetClassId);
    CPPUNIT_TEST(testGetClassName);
    CPPUNIT_TEST(testGetNamespace);
    CPPUNIT_TEST(testAnotherUsage);
    
    CPPUNIT_TEST_SUITE_END();

public:
    ClassInfo_Test();
    virtual ~ClassInfo_Test();
    void setUp();
    karabo::util::Vehicle::Pointer vp;
private:
    void testGetClassId();
    void testGetClassName();
    void testGetNamespace();
    void testAnotherUsage();
};

#endif	/* CLASSINFO_TEST_HH */

