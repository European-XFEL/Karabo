/* 
 * File:   Configurator_Test.hh
 * Author: heisenb
 *
 * Created on January 28, 2013, 2:49 PM
 */

#ifndef KARABO_UTIL_CONFIGURATOR_TEST_HH
#define	KARABO_UTIL_CONFIGURATOR_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/util/ClassInfo.hh>

struct Base_N {


    KARABO_CLASSINFO(Base_N, "Base_N", "1.0");

    karabo::util::Hash m_config;

    virtual karabo::util::Hash getConfig() {
        return karabo::util::Hash();
    }

};

struct Base_Y {


    KARABO_CLASSINFO(Base_Y, "Base_Y", "1.0");

    karabo::util::Hash m_config;

    void configure(const karabo::util::Hash & configuration) {
        m_config.set<karabo::util::Hash > ("base", configuration);
    }

    virtual karabo::util::Hash getConfig() {
        return m_config;
    }
};

template < class Base>
struct Sub1_N : public Base {


    KARABO_CLASSINFO(Sub1_N, Base::classInfo().getClassId() + "-Sub1_N", "1.0");

    virtual karabo::util::Hash getConfig() {
        return Base::m_config;
    }

};

template < class Base>
struct Sub1_Y : public Base {


    KARABO_CLASSINFO(Sub1_Y, Base::classInfo().getClassId() + "-Sub1_Y", "1.0");

    void configure(const karabo::util::Hash & configuration) {
        Base::m_config.template set<karabo::util::Hash > ("sub1", configuration);
    }

    virtual karabo::util::Hash getConfig() {
        return Base::m_config;
    }

};

class Configurator_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(Configurator_Test);
    CPPUNIT_TEST(testSub1);
    CPPUNIT_TEST_SUITE_END();

public:
    Configurator_Test();
    virtual ~Configurator_Test();
    void setUp();
    void tearDown();

private:

    void testSub1();

};

#endif	/* CONFIGURATOR_TEST_HH */

