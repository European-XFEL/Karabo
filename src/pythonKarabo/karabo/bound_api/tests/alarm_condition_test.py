# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
import threading
import time
import socket

from karabo.bound import AlarmCondition



class  AlarmCondition_TestCase(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_alarm_condition_roundtrip(self):
        try:
            condition = AlarmCondition.ALARM_HIGH
            self.assertEqual(condition.asString(), "ALARM_HIGH")
            condition = AlarmCondition.fromString(condition.asString())
            self.assertEqual(condition.asString(), "ALARM_HIGH")
            self.assertEqual(condition, AlarmCondition.ALARM_HIGH)
        except Exception as e:
            self.fail("test_alarm_condition_roundtrip exception: " + str(e))

    def test_alarm_condition_no_other_conditions(self):
        try:
            failing_condition = AlarmCondition("SOME_NEW_NAME", rank = 0)
        except Exception as e:
            self.assertEqual("only predeclared conditions are allowed" in str(e), True)

    def test_alarm_condition_significance_evaluation(self):
        try:
            condition_list = [AlarmCondition.ALARM_LOW, AlarmCondition.WARN, AlarmCondition.INTERLOCK]
            most_significant = AlarmCondition.returnMostSignificant(condition_list)
            self.assertEqual(most_significant, AlarmCondition.INTERLOCK)
            condition_list = condition_list[:-1]
            most_significant = AlarmCondition.returnMostSignificant(condition_list)
            self.assertEqual(most_significant, AlarmCondition.ALARM)
        except Exception as e:
            self.fail("test_alarm_condition_significance_evaluation exception: " + str(e))


    def test_alarm_condition_comparision(self):
        try:
            condition = AlarmCondition.ALARM_HIGH
            self.assertEqual(condition.isSameCriticality(AlarmCondition.ALARM), True)
            self.assertEqual(condition.isSameCriticality(AlarmCondition.ALARM_LOW), True)
            self.assertEqual(condition.isSameCriticality(AlarmCondition.WARN), False)
        except Exception as e:
            self.fail("test_alarm_condition_comparision exception: " + str(e))



            

if __name__ == '__main__':
    unittest.main()

