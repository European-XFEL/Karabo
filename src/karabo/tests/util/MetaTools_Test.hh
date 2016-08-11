/* 
 * File:   MetaTools_Test.hh
 * Author: heisenb
 *
 * Created on August 11, 2016, 11:18 AM
 */

#ifndef METATOOLS_TEST_HH
#define	METATOOLS_TEST_HH

#include <karabo/util/Hash.hh>
#include <cppunit/extensions/HelperMacros.h>

struct MyPublicHash : public karabo::util::Hash {

};

struct MyProtectedHash : protected karabo::util::Hash {

};

struct MyPrivateHash : private karabo::util::Hash {

};

class MetaTools_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(MetaTools_Test);
    CPPUNIT_TEST(testMethod);
    CPPUNIT_TEST_SUITE_END();

public:
    MetaTools_Test();
    virtual ~MetaTools_Test();

private:

    void testMethod();

};

#endif	/* METATOOLS_TEST_HH */

