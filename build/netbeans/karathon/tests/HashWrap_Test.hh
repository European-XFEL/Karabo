/*
 * File:   HashWrap_Test.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on Mar 8, 2013, 10:47:43 AM
 */

#ifndef NEWTESTCLASS_HH
#define	NEWTESTCLASS_HH

#include <boost/python.hpp>
#include <cppunit/extensions/HelperMacros.h>


class HashWrap_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(HashWrap_Test);

    CPPUNIT_TEST(testConstructors);
    CPPUNIT_TEST(testGetSet);
    CPPUNIT_TEST(testGetAs);
    CPPUNIT_TEST(testFind);
    CPPUNIT_TEST(testAttributes);
    CPPUNIT_TEST(testIteration);
    CPPUNIT_TEST(testMerge);

    CPPUNIT_TEST_SUITE_END();

public:
    HashWrap_Test();
    virtual ~HashWrap_Test();
    void setUp();
    void tearDown();

private:
    void testConstructors();
    void testGetSet();
    void testGetAs();
    void testFind();
    void testAttributes();
    void testIteration();
    void testMerge();
    
private:
    boost::python::object o_main;
    boost::python::object o_global;
};

#endif	/* NEWTESTCLASS_HH */

