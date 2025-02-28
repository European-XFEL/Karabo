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
 * $Id: AlarmTester.cc 7755 2016-06-24 14:10:56Z haufs $
 *
 * Author: <steffen.hauf@xfel.eu>
 *
 * Created on June, 2016, 03:03 PM
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "AlarmTesterDevice.hh"

#include <cassert>

using namespace std;

USING_KARABO_NAMESPACES

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device, AlarmTester)

    void AlarmTester::expectedParameters(Schema& expected) {
        NODE_ELEMENT(expected).key("nodeA").commit();

        FLOAT_ELEMENT(expected)
              .key("nodeA.floatPropNoAck2")
              .displayedName("Float Property2 NO needsAcknowledging")
              .readOnly()
              .initialValue(0)
              .warnLow(-2.0)
              .info("A floatPropNoAck2 warnLow")
              .needsAcknowledging(false)
              .warnHigh(2.0)
              .info("A floatPropNoAck2 warnHigh")
              .needsAcknowledging(false)
              .alarmLow(-3.0)
              .info("A floatPropNoAck2 alarmLow")
              .needsAcknowledging(false)
              .alarmHigh(3.0)
              .info("A floatPropNoAck2 alarmHigh")
              .needsAcknowledging(false)
              //                .enableRollingStats().warnVarianceHigh(2.5).needsAcknowledging(true).evaluationInterval(100)
              .commit();

        FLOAT_ELEMENT(expected)
              .key("nodeA.floatPropNeedsAck2")
              .displayedName("Float Property2 needsAcknowledging")
              .readOnly()
              .initialValue(0)
              .warnLow(-2.0)
              .info("A floatPropNeedsAck2 warnLow")
              .needsAcknowledging(true)
              .warnHigh(2.0)
              .info("A floatPropNeedsAck2 warnHigh")
              .needsAcknowledging(true)
              .alarmLow(-3.0)
              .info("A floatPropNeedsAck2 alarmLow")
              .needsAcknowledging(true)
              .alarmHigh(3.0)
              .info("A floatPropNeedsAck2 alarmHigh")
              .needsAcknowledging(true)
              .commit();

        INT32_ELEMENT(expected)
              .key("intPropNeedsAck")
              .displayedName("Int Property needsAcknowledging")
              .readOnly()
              .initialValue(0)
              .warnLow(-30)
              .info("A intPropNeedsAck warnLow")
              .needsAcknowledging(true)
              .warnHigh(30)
              .info("A intPropNeedsAck warnHigh")
              .needsAcknowledging(true)
              .alarmLow(-40)
              .info("A intPropNeedsAck alarmLow")
              .needsAcknowledging(true)
              .alarmHigh(40)
              .info("A intPropNeedsAck alarmHigh")
              .needsAcknowledging(true)
              //                .enableRollingStats().warnVarianceHigh(3).needsAcknowledging(false).evaluationInterval(100)
              .commit();

        INT32_ELEMENT(expected)
              .key("intPropNoAck")
              .displayedName("Int Property NO needsAcknowledging")
              .readOnly()
              .initialValue(0)
              .warnLow(-30)
              .info("A intPropNoAck warnLow")
              .needsAcknowledging(false)
              .warnHigh(30)
              .info("A intPropNoAck warnHigh")
              .needsAcknowledging(false)
              .alarmLow(-40)
              .info("A intPropNoAck alarmLow")
              .needsAcknowledging(false)
              .alarmHigh(40)
              .info("A intPropNoAck alarmHigh")
              .needsAcknowledging(false)
              .commit();

        STRING_ELEMENT(expected).key("result").displayedName("Result").readOnly().initialValue("").commit();

        SLOT_ELEMENT(expected).key("triggerWarnLowAck").displayedName("Trigger warnLow needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerWarnHighAck").displayedName("Trigger warnHigh needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerAlarmLowAck").displayedName("Trigger alarmLow needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerAlarmHighAck").displayedName("Trigger alarmHigh needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerWarnLowNoAck").displayedName("Trigger warnLow NO needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerWarnHighNoAck").displayedName("Trigger warnHigh NO needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerAlarmLowNoAck").displayedName("Trigger alarmLow NO needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerAlarmHighNoAck").displayedName("Trigger alarmHigh NO needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerWarnLowAckNode").displayedName("Trigger nodewarnLow needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerWarnHighAckNode").displayedName("Trigger node warnHigh needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerAlarmLowAckNode").displayedName("Trigger node alarmLow needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerAlarmHighAckNode").displayedName("Trigger node alarmHigh needsAck").commit();

        SLOT_ELEMENT(expected)
              .key("triggerWarnLowNoAckNode")
              .displayedName("Trigger node warnLow NO needsAck")
              .commit();

        SLOT_ELEMENT(expected)
              .key("triggerWarnHighNoAckNode")
              .displayedName("Trigger node warnHigh NO needsAck")
              .commit();

        SLOT_ELEMENT(expected)
              .key("triggerAlarmLowNoAckNode")
              .displayedName("Trigger node alarmLow NO needsAck")
              .commit();

        SLOT_ELEMENT(expected)
              .key("triggerAlarmHighNoAckNode")
              .displayedName("Trigger node alarmHigh NO needsAck")
              .commit();

        SLOT_ELEMENT(expected).key("triggerGlobalWarnAck").displayedName("Trigger Global Warn needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerGlobalAlarmAck").displayedName("Trigger Global Alarm needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerInterlockAck").displayedName("Trigger INTERLOCK needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerGlobalWarn").displayedName("Trigger Global Warn").commit();

        SLOT_ELEMENT(expected).key("triggerGlobalAlarm").displayedName("Trigger Global Alarm").commit();

        SLOT_ELEMENT(expected).key("triggerInterlock").displayedName("Trigger INTERLOCK").commit();

        SLOT_ELEMENT(expected).key("triggerNormalAck").displayedName("Back to normal needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerNormalNoAck").displayedName("Back to normal NO needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerNormalAckNode").displayedName("Back to node normal needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerNormalNoAckNode").displayedName("Back to node normal NO needsAck").commit();

        SLOT_ELEMENT(expected).key("triggerGlobalNormal").displayedName("Back to global normal").commit();

        SLOT_ELEMENT(expected)
              .key("alarmConditionToResult")
              .displayedName("Put the alarm condition in the result string")
              .commit();
    }

    AlarmTester::AlarmTester(const karabo::util::Hash& config) : Device(config) {
        KARABO_SLOT(triggerWarnLowAck);
        KARABO_SLOT(triggerWarnHighAck);
        KARABO_SLOT(triggerAlarmLowAck);
        KARABO_SLOT(triggerAlarmHighAck);

        KARABO_SLOT(triggerWarnLowNoAck);
        KARABO_SLOT(triggerWarnHighNoAck);
        KARABO_SLOT(triggerAlarmLowNoAck);
        KARABO_SLOT(triggerAlarmHighNoAck);

        KARABO_SLOT(triggerWarnLowAckNode);
        KARABO_SLOT(triggerWarnHighAckNode);
        KARABO_SLOT(triggerAlarmLowAckNode);
        KARABO_SLOT(triggerAlarmHighAckNode);

        KARABO_SLOT(triggerWarnLowNoAckNode);
        KARABO_SLOT(triggerWarnHighNoAckNode);
        KARABO_SLOT(triggerAlarmLowNoAckNode);
        KARABO_SLOT(triggerAlarmHighNoAckNode);

        KARABO_SLOT(triggerGlobalWarnAck);
        KARABO_SLOT(triggerGlobalAlarmAck);
        KARABO_SLOT(triggerInterlockAck);

        KARABO_SLOT(triggerGlobalWarn);
        KARABO_SLOT(triggerGlobalAlarm);
        KARABO_SLOT(triggerInterlock);

        KARABO_SLOT(triggerNormalAck);
        KARABO_SLOT(triggerNormalNoAck);

        KARABO_SLOT(triggerNormalAckNode);
        KARABO_SLOT(triggerNormalNoAckNode);

        KARABO_SLOT(triggerGlobalNormal);
        KARABO_SLOT(alarmConditionToResult);

        KARABO_INITIAL_FUNCTION(initialize);
    }

    AlarmTester::~AlarmTester() {}

    void AlarmTester::initialize() {
        Schema schema = getFullSchema();

        schema.setWarnLow<int>("intPropNeedsAck", -3);
        schema.setWarnHigh<int>("intPropNeedsAck", 3);
        schema.setAlarmLow<int>("intPropNeedsAck", -4);
        schema.setAlarmHigh<int>("intPropNeedsAck", 4);

        schema.setWarnLow<int>("intPropNoAck", -3);
        schema.setWarnHigh<int>("intPropNoAck", 3);
        schema.setAlarmLow<int>("intPropNoAck", -4);
        schema.setAlarmHigh<int>("intPropNoAck", 4);

        appendSchema(schema, true);
    }

    void AlarmTester::preReconfigure(karabo::util::Hash& incomingReconfiguration) {}


    void AlarmTester::postReconfigure() {}

    void AlarmTester::triggerWarnLowAck() {
        set("intPropNeedsAck", -4);
        set("result", "triggerWarnLowAck");
    }

    void AlarmTester::triggerWarnHighAck() {
        set("intPropNeedsAck", 4);
        set("result", "triggerWarnHighAck");
    }

    void AlarmTester::triggerAlarmLowAck() {
        set("intPropNeedsAck", -5);

        set("result", "triggerAlarmLowAck");
    }

    void AlarmTester::triggerAlarmHighAck() {
        set("intPropNeedsAck", 5);
        set("result", "triggerAlarmHighAck");
    }

    void AlarmTester::triggerWarnLowNoAck() {
        set("intPropNoAck", -4);
        set("result", "triggerWarnLowNoAck");
    }

    void AlarmTester::triggerWarnHighNoAck() {
        set("intPropNoAck", 4);
        set("result", "triggerWarnHighNoAck");
    }

    void AlarmTester::triggerAlarmLowNoAck() {
        set("intPropNoAck", -5);

        set("result", "triggerAlarmLowNoAck");
    }

    void AlarmTester::triggerAlarmHighNoAck() {
        set("intPropNoAck", 5);

        set("result", "triggerAlarmHighNoAck");
    }

    void AlarmTester::triggerWarnLowAckNode() {
        set("nodeA.floatPropNeedsAck2", -2.2);

        set("result", "triggerWarnLowAckNode");
    }

    void AlarmTester::triggerWarnHighAckNode() {
        set("nodeA.floatPropNeedsAck2", 2.2);

        set("result", "triggerWarnHighAckNode");
    }

    void AlarmTester::triggerAlarmLowAckNode() {
        set("nodeA.floatPropNeedsAck2", -3.2);

        set("result", "triggerAlarmLowAckNode");
    }

    void AlarmTester::triggerAlarmHighAckNode() {
        set("nodeA.floatPropNeedsAck2", 3.2);

        set("result", "triggerAlarmHighAckNode");
    }

    void AlarmTester::triggerWarnLowNoAckNode() {
        set("nodeA.floatPropNoAck2", -2.2);

        set("result", "triggerWarnLowNoAckNode");
    }

    void AlarmTester::triggerWarnHighNoAckNode() {
        set("nodeA.floatPropNoAck2", 2.2);

        set("result", "triggerWarnHighNoAckNode");
    }

    void AlarmTester::triggerAlarmLowNoAckNode() {
        set("nodeA.floatPropNoAck2", -3.2);

        set("result", "triggerAlarmLowNoAckNode");
    }

    void AlarmTester::triggerAlarmHighNoAckNode() {
        set("nodeA.floatPropNoAck2", 3.2);

        set("result", "triggerAlarmHighNoAckNode");
    }

    void AlarmTester::triggerInterlockAck() {
        setAlarmCondition(AlarmCondition::INTERLOCK, true);
        set("result", "triggerInterlockAck");
    }

    void AlarmTester::triggerGlobalWarnAck() {
        setAlarmCondition(AlarmCondition::WARN, true);
        set("result", "triggerGlobalWarnAck");
    }

    void AlarmTester::triggerGlobalAlarmAck() {
        setAlarmCondition(AlarmCondition::ALARM, true);
        set("result", "triggerGlobalAlarmAck");
    }

    void AlarmTester::triggerInterlock() {
        setAlarmCondition(AlarmCondition::INTERLOCK);
        set("result", "triggerInterlock");
    }

    void AlarmTester::triggerGlobalWarn() {
        setAlarmCondition(AlarmCondition::WARN);
        set("result", "triggerGlobalWarn");
    }

    void AlarmTester::triggerGlobalAlarm() {
        setAlarmCondition(AlarmCondition::ALARM);
        set("result", "triggerGlobalAlarm");
    }

    void AlarmTester::triggerNormalAck() {
        set("intPropNeedsAck", 0);
        set("result", "triggerNormalAck");
    }

    void AlarmTester::triggerNormalNoAck() {
        set("intPropNoAck", 0);
        set("result", "triggerNormalNoAck");
    }

    void AlarmTester::triggerNormalAckNode() {
        set("nodeA.floatPropNeedsAck2", 0);
        set("result", "triggerNormalAckNode");
    }

    void AlarmTester::triggerNormalNoAckNode() {
        set("nodeA.floatPropNoAck2", 0);
        set("result", "triggerNormalNoAckNode");
    }

    void AlarmTester::triggerGlobalNormal() {
        setAlarmCondition(AlarmCondition::NONE, true);
        set("result", "triggerGlobalNormal");
    }


    void AlarmTester::alarmConditionToResult() {
        const karabo::util::AlarmCondition alarmCondition = getAlarmCondition();
        set("result", alarmCondition.asString());
    }
} // namespace karabo
