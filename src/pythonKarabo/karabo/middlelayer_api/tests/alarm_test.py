# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
from unittest import TestCase, main

from karabo.middlelayer import (
    AccessLevel, AlarmCondition, Configurable, Hash, Int32, MetricPrefix, Node,
    Unit, unit)
from karabo.middlelayer_api.alarm import AlarmMixin


class Tests(TestCase):
    def test_classId(self):
        schema_hash = AlarmMixin.getClassSchema().hash
        classId = schema_hash.getAttribute("alarmCondition", "classId")
        self.assertEqual(classId, "AlarmCondition")
        classId = schema_hash.getAttribute("globalAlarmCondition", "classId")
        self.assertEqual(classId, "AlarmCondition")

    def test_simple(self):
        class A(AlarmMixin):
            amin = Int32(alarmLow=3)
            amax = Int32(alarmHigh=22)
            wmin = Int32(warnLow=2)
            wmax = Int32(warnHigh=6)

        a = A({})

        self.assertEqual(a.alarmCondition, AlarmCondition.NONE)
        a.amin = 5
        self.assertEqual(a.alarmCondition, AlarmCondition.NONE)
        a.amin = 1
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)
        a.amax = 30
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)
        a.amax = 3
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)
        a.wmax = 22
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)
        a.wmin = 1
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)
        a.amin = 55
        self.assertEqual(a.alarmCondition, AlarmCondition.WARN)

    def test_multiple(self):
        class A(AlarmMixin):
            aminwmax = Int32(alarmLow=3, warnHigh=7)
            amaxwmax = Int32(alarmHigh=7, warnHigh=9)

        a = A({})
        self.assertEqual(a.alarmCondition, AlarmCondition.NONE)
        a.aminwmax = 2
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)
        a.aminwmax = 22
        self.assertEqual(a.alarmCondition, AlarmCondition.WARN)
        a.aminwmax = 4
        self.assertEqual(a.alarmCondition, AlarmCondition.NONE)
        a.amaxwmax = 10
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)
        a.amaxwmax = 8
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)
        a.amaxwmax = 5
        self.assertEqual(a.alarmCondition, AlarmCondition.NONE)

    def test_nested(self):
        class N(Configurable):
            amin = Int32(alarmLow=3)
            wmax = Int32(warnHigh=3)

        class A(AlarmMixin):
            left = Node(N)
            right = Node(N)
            wmin = Int32(warnLow=22)

        a = A({})
        self.assertEqual(a.alarmCondition, AlarmCondition.NONE)
        a.left.amin = 1
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)
        a.right.wmax = 22
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)
        a.left.amin = 5
        self.assertEqual(a.alarmCondition, AlarmCondition.WARN)
        a.wmin = 3
        self.assertEqual(a.alarmCondition, AlarmCondition.WARN)
        a.right.wmax = 1
        self.assertEqual(a.alarmCondition, AlarmCondition.WARN)

    def test_units(self):
        class N(Configurable):
            amin = Int32(
                unitSymbol=Unit.METER,
                metricPrefixSymbol=MetricPrefix.MILLI,
                alarmLow=30)
            wmax = Int32(
                unitSymbol=Unit.METER,
                warnHigh=3)

        class A(AlarmMixin):
            left = Node(N)
            right = Node(N)
            wmin = Int32(warnLow=22)

        a = A({})
        self.assertEqual(a.alarmCondition, AlarmCondition.NONE)
        a.left.amin = 1
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)
        # 40 is larger than 30 all ok
        a.left.amin = 40
        self.assertEqual(a.alarmCondition, AlarmCondition.NONE)
        # 4000 mm are larger than 30 all ok
        a.left.amin = 4 * unit.meter
        self.assertEqual(a.alarmCondition, AlarmCondition.NONE)
        # 100 um are smaller than 30 mm ALARM!!!
        a.left.amin = 100 * unit.micrometer
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)
        # 22 is larger than 3 but alarm trumps warning
        a.right.wmax = 22
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)
        # alarm goes away, but warning stays
        a.left.amin = 5e3
        self.assertEqual(a.alarmCondition, AlarmCondition.WARN)
        a.wmin = 23
        self.assertEqual(a.alarmCondition, AlarmCondition.WARN)
        a.right.wmax = 1
        self.assertEqual(a.alarmCondition, AlarmCondition.NONE)
        a.wmin = 3
        self.assertEqual(a.alarmCondition, AlarmCondition.WARN)

    def test_init(self):
        class A(AlarmMixin):
            amin = Int32(alarmLow=5)

        a = A({"amin": 2})
        self.assertEqual(a.amin, 2)
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)

    def test_default(self):
        class A(AlarmMixin):
            amin = Int32(alarmLow=5, defaultValue=2)

        a = A({})
        self.assertEqual(a.amin, 2)
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM)
        self.assertEqual(a.alarmCondition.descriptor.displayType,
                         'AlarmCondition')
        self.assertEqual(a.globalAlarmCondition.descriptor.displayType,
                         'AlarmCondition')
        self.assertEqual(a.globalAlarmCondition.descriptor.requiredAccessLevel,
                         AccessLevel.ADMIN)

    def test_resubmit(self):
        """Test the slotReSubmitAlarms function"""

        class A(AlarmMixin):
            # DeviceId is required as no device is present!
            deviceId = 'remoteAlarm'
            amin = Int32(
                alarmLow=5,
                defaultValue=7)
            amax = Int32(
                alarmHigh=10,
                defaultValue=5)

        a = A(configuration={})

        # First return value is the deviceId
        deviceId, als1 = a.slotReSubmitAlarms(Hash())
        self.assertEqual(deviceId, 'remoteAlarm')
        self.assertEqual(len(als1['toAdd']), 0)
        self.assertEqual(len(als1['toClear']), 0)

        a.amin = 3
        # We have one alarmLow!
        _, als2 = a.slotReSubmitAlarms(Hash())
        self.assertEqual(len(als2['toAdd']), 1)
        self.assertEqual(len(als2['toClear']), 0)

        a.amax = 12
        # We add an alarmHigh!
        _, als3 = a.slotReSubmitAlarms(Hash())
        self.assertEqual(len(als3['toAdd']), 2)
        self.assertEqual(len(als3['toClear']), 0)

        a.amin = 7
        # We remove the alarmLow and add use previous as existing!
        _, als4 = a.slotReSubmitAlarms(als3['toAdd'])
        self.assertTrue("amax.alarmHigh" in als4['toAdd'])
        self.assertEqual(len(als4['toAdd']), 1)
        self.assertEqual(als4['toClear.amin'], ["alarmLow"])
        self.assertEqual(len(als4['toClear']), 1)

        a.amax = 5
        # We remove the alarmHigh!
        _, als5 = a.slotReSubmitAlarms(als4['toAdd'])
        self.assertEqual(len(als5['toAdd']), 0)
        self.assertEqual(len(als5['toClear']), 1)


if __name__ == "__main__":
    main()
