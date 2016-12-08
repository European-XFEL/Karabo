/* 
 * File:   Exception_Test.cc
 * Author: heisenb
 * 
 * Created on September 29, 2016, 5:28 PM
 */

#include "Exception_Test.hh"
#include "karabo/util/Exception.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(Exception_Test);


Exception_Test::Exception_Test() {
}


Exception_Test::~Exception_Test() {
}


void Exception_Test::testMethod() {

    CPPUNIT_ASSERT_THROW(throw KARABO_LOGIC_EXCEPTION("Some message"), karabo::util::LogicException);
    CPPUNIT_ASSERT_THROW(throw KARABO_LOGIC_EXCEPTION("Some message"), karabo::util::Exception);
    CPPUNIT_ASSERT_THROW(throw KARABO_LOGIC_EXCEPTION("Some message"), std::exception);
    try {
        throw KARABO_LOGIC_EXCEPTION("error");
    } catch (const std::exception& e) {
        std::string expected("1. Exception =====>  {");
        CPPUNIT_ASSERT(std::string(e.what(), expected.size()) == expected);
    }
}

