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
 * File:   AlarmCondition_Test.hh
 * Author: haufs
 *
 * Created on Jun 9, 2016, 9:42:54 AM
 */

#include <gtest/gtest.h>

#include <boost/aligned_storage.hpp>
#include <karabo/log/Logger.hh>
#include <karabo/util/TimeProfiler.hh>
#include <string>
#include <vector>

#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/schema/SimpleElement.hh"
#include "karabo/data/schema/Validator.hh"
#include "karabo/data/time/TimeDuration.hh"
#include "karabo/data/time/TimePeriod.hh"
#include "karabo/data/types/AlarmCondition.hh"
#include "karabo/data/types/Schema.hh"


TEST(TestAlarmCondition, testOperators) {
    EXPECT_TRUE(karabo::data::AlarmCondition::WARN == karabo::data::AlarmCondition::WARN);
    EXPECT_TRUE(!(karabo::data::AlarmCondition::WARN != karabo::data::AlarmCondition::WARN));
}

TEST(TestAlarmCondition, testStringAssignmentRoundTrip) {
    karabo::data::AlarmCondition condition = karabo::data::AlarmCondition::fromString("warn");
    std::string conditionString = condition;
    EXPECT_TRUE(conditionString == "warn");
    EXPECT_TRUE(condition.asString() == "warn");
}


TEST(TestAlarmCondition, testSignificanceEvaluation) {
    std::vector<karabo::data::AlarmCondition> v;
    karabo::data::AlarmCondition ms = karabo::data::AlarmCondition::returnMostSignificant(v);
    EXPECT_TRUE(ms.isSameCriticality(karabo::data::AlarmCondition::NONE));

    v.push_back(karabo::data::AlarmCondition::WARN);
    v.push_back(karabo::data::AlarmCondition::ALARM);
    v.push_back(karabo::data::AlarmCondition::INTERLOCK);
    ms = karabo::data::AlarmCondition::returnMostSignificant(v);
    EXPECT_TRUE(ms.isSameCriticality(karabo::data::AlarmCondition::INTERLOCK));


    v.pop_back();
    v.push_back(karabo::data::AlarmCondition::WARN);
    ms = karabo::data::AlarmCondition::returnMostSignificant(v);
    EXPECT_TRUE(ms.isSameCriticality(karabo::data::AlarmCondition::ALARM));
}
