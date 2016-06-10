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
    karabo::util::alarmConditions::Pointer condition = karabo::util::alarmConditions::fromString("WARN");
    std::string conditionString = *condition;
    CPPUNIT_ASSERT(conditionString == "WARN");
    CPPUNIT_ASSERT(condition->asString() == "WARN");
}

void AlarmCondition_Test::testSignificanceEvaluation() {
    std::vector<karabo::util::alarmConditions::Pointer> v;
    karabo::util::alarmConditions::Pointer ms = karabo::util::alarmConditions::returnMostSignificant(v);
    CPPUNIT_ASSERT(ms->isSimilar(karabo::util::alarmConditions::NONE));
    
    v.push_back(karabo::util::alarmConditions::WARN);
    v.push_back(karabo::util::alarmConditions::ALARM_HIGH);
    v.push_back(karabo::util::alarmConditions::INTERLOCK);
    ms = karabo::util::alarmConditions::returnMostSignificant(v);
    CPPUNIT_ASSERT(ms->isSimilar(karabo::util::alarmConditions::INTERLOCK));
    
    
    v.pop_back();
    v.push_back(karabo::util::alarmConditions::WARN);
    ms = karabo::util::alarmConditions::returnMostSignificant(v);
    CPPUNIT_ASSERT(ms->isSimilar(karabo::util::alarmConditions::ALARM));
    
}

