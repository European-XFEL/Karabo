# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
import threading
import time
import socket

from karabo.bound import AlarmCondition



class  AlarmCondition_TestCase(unittest.TestCase):

    def test_alarm_condition_roundtrip(self):
        condition = AlarmCondition.ALARM_HIGH
        self.assertEqual(condition.asString(), "alarmHigh")
        condition = AlarmCondition.fromString(condition.asString())
        self.assertEqual(condition.asString(), "alarmHigh")
        self.assertEqual(condition, AlarmCondition.ALARM_HIGH)


    def test_alarm_condition_no_other_conditions(self):
        self.assertRaises(ValueError, AlarmCondition, "SOME_NEW_NAME")

    def test_alarm_condition_significance_evaluation(self):
        condition_list = [AlarmCondition.ALARM_LOW, AlarmCondition.WARN, AlarmCondition.INTERLOCK]
        most_significant = AlarmCondition.returnMostSignificant(condition_list)
        self.assertEqual(most_significant, AlarmCondition.INTERLOCK)
        condition_list = condition_list[:-1]
        most_significant = AlarmCondition.returnMostSignificant(condition_list)
        self.assertEqual(most_significant, AlarmCondition.ALARM)



    def test_alarm_condition_comparision(self):
        condition = AlarmCondition.ALARM_HIGH
        self.assertEqual(condition.isSameCriticality(AlarmCondition.ALARM), True)
        self.assertEqual(condition.isSameCriticality(AlarmCondition.ALARM_LOW), True)
        self.assertEqual(condition.isSameCriticality(AlarmCondition.WARN), False)



            

if __name__ == '__main__':
    unittest.main()

