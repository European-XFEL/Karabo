/*
 * File:   AlarmCondition_Test.cc
 * Author: haufs
 *
 * Created on Jun 9, 2016, 9:42:54 AM
 */

#include "AlarmCondition_Test.hh"
#include <karabo/util/AlarmConditions.hh>
#include <vector>
#include <string>
#include <boost/aligned_storage.hpp>


CPPUNIT_TEST_SUITE_REGISTRATION(AlarmCondition_Test);

AlarmCondition_Test::AlarmCondition_Test() {
}

AlarmCondition_Test::~AlarmCondition_Test() {
}

void AlarmCondition_Test::setUp() {
}

void AlarmCondition_Test::tearDown() {
}

void AlarmCondition_Test::testStringAssignmentRoundTrip() {
    karabo::util::AlarmCondition condition = karabo::util::AlarmCondition::fromString("WARN");
    std::string conditionString = condition;
    CPPUNIT_ASSERT(conditionString == "WARN");
    CPPUNIT_ASSERT(condition.asString() == "WARN");
}

void AlarmCondition_Test::testSignificanceEvaluation() {
    std::vector<karabo::util::AlarmCondition> v;
    karabo::util::AlarmCondition ms = karabo::util::AlarmCondition::returnMostSignificant(v);
    CPPUNIT_ASSERT(ms.isSameCriticality(karabo::util::AlarmCondition::NONE));
    
    v.push_back(karabo::util::AlarmCondition::WARN);
    v.push_back(karabo::util::AlarmCondition::ALARM_HIGH);
    v.push_back(karabo::util::AlarmCondition::INTERLOCK);
    ms = karabo::util::AlarmCondition::returnMostSignificant(v);
    CPPUNIT_ASSERT(ms.isSameCriticality(karabo::util::AlarmCondition::INTERLOCK));
    
    
    v.pop_back();
    v.push_back(karabo::util::AlarmCondition::WARN);
    ms = karabo::util::AlarmCondition::returnMostSignificant(v);
    CPPUNIT_ASSERT(ms.isSameCriticality(karabo::util::AlarmCondition::ALARM));
    
}

