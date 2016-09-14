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

// Forward
class Aggregated;

class Base {

public:

    KARABO_CLASSINFO(Base, "Base", "");

    static void expectedParameters(karabo::util::Schema& s);

    Base(const karabo::util::Hash& hash);

    const boost::shared_ptr<Aggregated>& getAggregated() const;

private:

    boost::shared_ptr<Aggregated> m_aggregated;

};

class Aggregated {

public:

    KARABO_CLASSINFO(Aggregated, "Aggregated", "");

    static void expectedParameters(karabo::util::Schema& s);

    Aggregated(const karabo::util::Hash& hash);

    Aggregated(const int answer);

    int foo() const;

private:

    int m_answer;

};

class Configurator_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(Configurator_Test);
    CPPUNIT_TEST(test);
    CPPUNIT_TEST_SUITE_END();

    void test();

};

#endif	/* CONFIGURATOR_TEST_HH */

