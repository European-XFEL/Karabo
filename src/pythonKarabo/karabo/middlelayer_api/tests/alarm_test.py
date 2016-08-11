from unittest import main, TestCase

from karabo.middlelayer_api.alarm import AlarmMixin
from karabo.middlelayer import AlarmCondition, Configurable, Int32, Node


class Tests(TestCase):
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
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM_LOW)
        a.amax = 30
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM_HIGH)
        a.amax = 3
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM_LOW)
        a.wmax = 22
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM_LOW)
        a.wmin = 1
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM_LOW)
        a.amin = 55
        self.assertEqual(a.alarmCondition, AlarmCondition.WARN_HIGH)

    def test_multiple(self):
        class A(AlarmMixin):
            aminwmax = Int32(alarmLow=3, warnHigh=7)
            amaxwmax = Int32(alarmHigh=7, warnHigh=9)


        a = A({})
        self.assertEqual(a.alarmCondition, AlarmCondition.NONE)
        a.aminwmax = 2
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM_LOW)
        a.aminwmax = 22
        self.assertEqual(a.alarmCondition, AlarmCondition.WARN_HIGH)
        a.aminwmax = 4
        self.assertEqual(a.alarmCondition, AlarmCondition.NONE)
        a.amaxwmax = 10
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM_HIGH)
        a.amaxwmax = 8
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM_HIGH)
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
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM_LOW)
        a.right.wmax = 22
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM_LOW)
        a.left.amin = 5
        self.assertEqual(a.alarmCondition, AlarmCondition.WARN_HIGH)
        a.wmin = 3
        self.assertEqual(a.alarmCondition, AlarmCondition.WARN_HIGH)
        a.right.wmax = 1
        self.assertEqual(a.alarmCondition, AlarmCondition.WARN_LOW)

    def test_init(self):
        class A(AlarmMixin):
            amin = Int32(alarmLow=5)

        a = A({"amin": 2})
        self.assertEqual(a.amin, 2)
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM_LOW)

    def test_default(self):
        class A(AlarmMixin):
            amin = Int32(alarmLow=5, defaultValue=2)

        a = A({})
        self.assertEqual(a.amin, 2)
        self.assertEqual(a.alarmCondition, AlarmCondition.ALARM_LOW)


if __name__ == "__main__":
    main()
