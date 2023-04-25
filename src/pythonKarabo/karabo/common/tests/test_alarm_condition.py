# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
import unittest

from karabo.common.api import AlarmCondition


class AlarmCondition_TestCase(unittest.TestCase):
    def test_alarm_condition_roundtrip(self):
        condition = AlarmCondition.ALARM_HIGH
        self.assertEqual(condition.asString(), "alarmHigh")
        condition = AlarmCondition.fromString(condition.asString())
        self.assertEqual(condition.asString(), "alarmHigh")
        self.assertEqual(condition, AlarmCondition.ALARM_HIGH)

    def test_alarm_condition_no_other_conditions(self):
        self.assertRaises(ValueError, AlarmCondition, "SOME_NEW_NAME")

    def test_alarm_condition_significance_evaluation(self):
        condition_list = [
            AlarmCondition.ALARM_LOW,
            AlarmCondition.WARN,
            AlarmCondition.INTERLOCK,
        ]
        most_significant = AlarmCondition.returnMostSignificant(condition_list)
        self.assertEqual(most_significant, AlarmCondition.INTERLOCK)
        condition_list = condition_list[:-1]
        most_significant = AlarmCondition.returnMostSignificant(condition_list)
        self.assertEqual(most_significant, AlarmCondition.ALARM)

    def test_alarm_condition_comparision(self):
        condition = AlarmCondition.ALARM_HIGH
        self.assertTrue(condition.isSameCriticality(AlarmCondition.ALARM))
        self.assertTrue(condition.isSameCriticality(AlarmCondition.ALARM_LOW))
        self.assertFalse(condition.isSameCriticality(AlarmCondition.WARN))


if __name__ == "__main__":
    unittest.main()
