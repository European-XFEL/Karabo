/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
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


void AlarmCondition_Test::testValidation() {
    using namespace karabo::util;

    TimeProfiler profiler("TestProfiler");
    profiler.open();


    Validator val;
    Schema schema;

    for (int i = 0; i < 50; i++) {
        std::ostringstream key_s;
        key_s << i;
        INT8_ELEMENT(schema).key("i8_" + key_s.str()).readOnly().initialValue(0).commit();
        UINT16_ELEMENT(schema)
              .key("ui16_" + key_s.str())
              .readOnly()
              .initialValue(5 + i)
              .alarmLow(5 + i)
              .needsAcknowledging(true)
              .alarmHigh(50 + i)
              .needsAcknowledging(true)
              .commit();
        FLOAT_ELEMENT(schema)
              .key("f_" + key_s.str())
              .readOnly()
              .initialValue(5 + i)
              .warnLow(5 + i)
              .needsAcknowledging(true)
              .warnHigh(50 + i)
              .needsAcknowledging(true)
              .commit();
        DOUBLE_ELEMENT(schema)
              .key("d_" + key_s.str())
              .readOnly()
              .initialValue(5 + i)
              .alarmLow(5 + i)
              .needsAcknowledging(true)
              .alarmHigh(50 + i)
              .needsAcknowledging(true)
              .commit();
        UINT64_ELEMENT(schema)
              .key("ui64_" + key_s.str())
              .readOnly()
              .initialValue(15 + i)
              .warnLow(15 + i)
              .needsAcknowledging(true)
              .warnHigh(50 + i)
              .needsAcknowledging(true)
              .alarmLow(i)
              .needsAcknowledging(true)
              .alarmHigh(75 + i)
              .needsAcknowledging(true)
              .commit();
    }


    profiler.startPeriod("validator");

    Hash h_out;

    for (size_t t = 0; t < 10; t++) {
        for (size_t i = 0; i < 50; i++) {
            std::ostringstream key_s;
            key_s << i;
            Hash h;
            h.set("i8_" + key_s.str(), t);
            h.set("ui16_" + key_s.str(), t);
            h.set("f_" + key_s.str(), t);
            h.set("d_" + key_s.str(), t);
            h.set("ui64_" + key_s.str(), t);
            std::pair<bool, std::string> r = val.validate(schema, h, h_out);

            const karabo::util::Hash& alarmParms = val.getParametersInWarnOrAlarm();
            for (karabo::util::Hash::const_iterator it = alarmParms.begin(); it != alarmParms.end(); ++it) {
                const std::string& scope = it->getKey();
                const std::string& type = it->getValue<Hash>().get<std::string>("type");

                if (scope.find("i8_") != std::string::npos) {
                    CPPUNIT_ASSERT(false); // should not occur
                    continue;
                }

                if (scope.find("ui16_") != std::string::npos) {
                    if (t < (5 + i)) {
                        CPPUNIT_ASSERT(type == AlarmCondition::ALARM_LOW.asString());
                    } else if (t > (50 + i)) {
                        CPPUNIT_ASSERT(type == AlarmCondition::ALARM_HIGH.asString());
                    } else {
                        KARABO_LOG_FRAMEWORK_DEBUG << t << " " << i;
                        KARABO_LOG_FRAMEWORK_DEBUG << scope << it->getValue<Hash>();
                        CPPUNIT_ASSERT(false); // alarm should have been raised
                    }
                }

                if (scope.find("f_") != std::string::npos) {
                    if (t < (5 + i)) {
                        CPPUNIT_ASSERT(type == AlarmCondition::WARN_LOW.asString());
                    } else if (t > (50 + i)) {
                        CPPUNIT_ASSERT(type == AlarmCondition::WARN_HIGH.asString());
                    } else {
                        CPPUNIT_ASSERT(false); // alarm should have been raised
                    }
                }

                if (scope.find("d_") != std::string::npos) {
                    if (t < (5 + i)) {
                        CPPUNIT_ASSERT(type == AlarmCondition::ALARM_LOW.asString());
                    } else if (t > (50 + i)) {
                        CPPUNIT_ASSERT(type == AlarmCondition::ALARM_HIGH.asString());
                    } else {
                        CPPUNIT_ASSERT(false); // alarm should have been raised
                    }
                }

                if (scope.find("ui64_") != std::string::npos) {
                    if (t < i) {
                        CPPUNIT_ASSERT(type == AlarmCondition::ALARM_LOW.asString());
                    } else if (t > (75 + i)) {
                        CPPUNIT_ASSERT(type == AlarmCondition::ALARM_HIGH.asString());
                    }
                    if (t < (15 + i)) {
                        CPPUNIT_ASSERT(type == AlarmCondition::WARN_LOW.asString());
                    } else if (t > (50 + i)) {
                        CPPUNIT_ASSERT(type == AlarmCondition::WARN_HIGH.asString());
                    } else {
                        CPPUNIT_ASSERT(false); // alarm should have been raised
                    }
                }
            }
        }
    }
    profiler.stopPeriod("validator");


    profiler.close();

    KARABO_LOG_FRAMEWORK_DEBUG << "Validation time 250 properties: "
                               << profiler.getPeriod("validator").getDuration() / 10 << " [s/per validation]";
}


void AlarmCondition_Test::testValidationConditionalRoundTrip() {
    using namespace karabo::util;
    karabo::util::Validator::ValidationRules rules; // same rules as the internal validator of device.
    // It will fail if defaults are injected but this is not relevant
    rules.allowAdditionalKeys = true;
    rules.allowMissingKeys = true;
    rules.allowUnrootedConfiguration = true;
    rules.injectDefaults = false;
    rules.injectTimestamps = true;

    Validator val(rules);
    Schema schema;


    FLOAT_ELEMENT(schema)
          .key("f1")
          .readOnly()
          .initialValue(5)
          .warnLow(5)
          .info("This is an optional description")
          .needsAcknowledging(true)
          .warnHigh(50)
          .needsAcknowledging(true)
          .commit();

    FLOAT_ELEMENT(schema)
          .key("f2")
          .readOnly()
          .initialValue(5)
          .warnLow(5)
          .needsAcknowledging(true)
          .warnHigh(50)
          .needsAcknowledging(true)
          .commit();

    Hash h1, h2, h_out;
    h1.set("f1", 3);

    std::pair<bool, std::string> r = val.validate(schema, h1, h_out);

    karabo::util::Hash alarmParms = val.getParametersInWarnOrAlarm();

    CPPUNIT_ASSERT(alarmParms.has("f1"));
    CPPUNIT_ASSERT(alarmParms.get<Hash>("f1").get<std::string>("type") == AlarmCondition::WARN_LOW.asString());

    // f1 should still be in alarm, additionally, f2
    h2.set("f2", 4);
    r = val.validate(schema, h2, h_out);
    alarmParms = val.getParametersInWarnOrAlarm();
    CPPUNIT_ASSERT(alarmParms.has("f1"));
    CPPUNIT_ASSERT(alarmParms.get<Hash>("f1").get<std::string>("type") == AlarmCondition::WARN_LOW.asString());
    CPPUNIT_ASSERT(alarmParms.has("f2"));
    CPPUNIT_ASSERT(alarmParms.get<Hash>("f2").get<std::string>("type") == AlarmCondition::WARN_LOW.asString());

    // now only f2 in alarm
    h1.set("f1", 6);
    r = val.validate(schema, h1, h_out);
    alarmParms = val.getParametersInWarnOrAlarm();

    CPPUNIT_ASSERT(!alarmParms.has("f1"));
    CPPUNIT_ASSERT(alarmParms.has("f2"));
    CPPUNIT_ASSERT(alarmParms.get<Hash>("f2").get<std::string>("type") == AlarmCondition::WARN_LOW.asString());

    CPPUNIT_ASSERT(schema.getInfoForAlarm("f1", AlarmCondition::WARN_LOW) == "This is an optional description");
    CPPUNIT_ASSERT(schema.getInfoForAlarm("f1", AlarmCondition::WARN_HIGH) == "");
}