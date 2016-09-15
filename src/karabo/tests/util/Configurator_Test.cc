/* 
 * File:   Configurator_Test.cc
 * Author: heisenb
 * 
 * Created on January 28, 2013, 2:49 PM
 */

#include <karabo/util/Configurator.hh>

#include "Configurator_Test.hh"
#include "karabo/util/NodeElement.hh"
#include "karabo/util/LeafElement.hh"
#include "karabo/util/SimpleElement.hh"

using namespace karabo::util;

CPPUNIT_TEST_SUITE_REGISTRATION(Configurator_Test);

KARABO_REGISTER_FOR_CONFIGURATION(Base);
KARABO_REGISTER_FOR_CONFIGURATION(Aggregated);


void Base::expectedParameters(karabo::util::Schema& s) {
    NODE_ELEMENT(s).key("node")
            .appendParametersOf<Aggregated>()
            .commit();
}


Base::Base(const karabo::util::Hash& hash) {
    m_aggregated = Configurator<Aggregated>::createNode("node", hash);
}


boost::shared_ptr<Aggregated>& Base::getAggregated() {
    return m_aggregated;
}


void Aggregated::expectedParameters(karabo::util::Schema& s) {
    INT32_ELEMENT(s).key("answer")
            .description("The answer")
            .displayedName("Answer")
            .assignmentOptional().defaultValue(0)
            .commit();
}


Aggregated::Aggregated(const karabo::util::Hash& hash) : m_answer(hash.get<int>("answer")) {

}


Aggregated::Aggregated(const int answer) : m_answer(answer) {

}


int Aggregated::foo() const {
    return m_answer;
}


void Configurator_Test::test() {

    {
        // Test to construct Base from Aggregated params
        Hash config("node.answer", 42);
        Base::Pointer b = Configurator<Base>::create("Base", config);
        CPPUNIT_ASSERT(b->getAggregated()->foo() == 42);
    }

    {
        // Test to construct Base from Aggregated object
        Hash config("node", Aggregated::Pointer(new Aggregated(42)));
        Base::Pointer b = Configurator<Base>::create("Base", config);
        CPPUNIT_ASSERT(b->getAggregated()->foo() == 42);

    }
}



