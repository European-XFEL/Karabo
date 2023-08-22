/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   AlarmCondition_Test.cc
 * Author: haufs
 *
 * Created on Jun 9, 2016, 9:42:54 AM
 */

#include "AlarmCondition_Test.hh"

#include <boost/aligned_storage.hpp>
#include <karabo/util/AlarmConditions.hh>
#include <karabo/util/Schema.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/TimeDuration.hh>
#include <karabo/util/TimePeriod.hh>
#include <karabo/util/TimeProfiler.hh>
#include <karabo/util/Validator.hh>
#include <string>
#include <vector>

CPPUNIT_TEST_SUITE_REGISTRATION(AlarmCondition_Test);


AlarmCondition_Test::AlarmCondition_Test() {}


AlarmCondition_Test::~AlarmCondition_Test() {}


void AlarmCondition_Test::setUp() {}


void AlarmCondition_Test::tearDown() {}


void AlarmCondition_Test::testOperators() {
    CPPUNIT_ASSERT(karabo::util::AlarmCondition::WARN == karabo::util::AlarmCondition::WARN);
    CPPUNIT_ASSERT(!(karabo::util::AlarmCondition::WARN != karabo::util::AlarmCondition::WARN));

    CPPUNIT_ASSERT(!(karabo::util::AlarmCondition::WARN == karabo::util::AlarmCondition::WARN_LOW));
    CPPUNIT_ASSERT(karabo::util::AlarmCondition::WARN != karabo::util::AlarmCondition::WARN_LOW);
}

void AlarmCondition_Test::testStringAssignmentRoundTrip() {
    karabo::util::AlarmCondition condition = karabo::util::AlarmCondition::fromString("warn");
    std::string conditionString = condition;
    CPPUNIT_ASSERT(conditionString == "warn");
    CPPUNIT_ASSERT(condition.asString() == "warn");
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
