/* 
 * File:   MetaTools_Test.cc
 * Author: heisenb
 * 
 * Created on August 11, 2016, 11:18 AM
 */

#include "MetaTools_Test.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(MetaTools_Test);

using namespace karabo::util;

MetaTools_Test::MetaTools_Test() {
}


MetaTools_Test::~MetaTools_Test() {
}


void MetaTools_Test::testMethod() {

    CPPUNIT_ASSERT(PointerTest::isSharedPointer<boost::shared_ptr<int> >());
    CPPUNIT_ASSERT(!PointerTest::isSharedPointer<int>());

    CPPUNIT_ASSERT((is_base_of<Hash, MyPublicHash>::value));
    CPPUNIT_ASSERT((is_base_of<Hash, MyProtectedHash>::value));
    CPPUNIT_ASSERT((is_base_of<Hash, MyPrivateHash>::value));
    CPPUNIT_ASSERT((!is_base_of<Hash, int>::value));
}
