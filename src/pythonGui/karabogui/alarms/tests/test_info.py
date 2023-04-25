# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from karabo.common.api import AlarmCondition

from ..info import AlarmInfo


def test_append_alarm_type():
    al = AlarmInfo(alarm_dict={})
    al.append_alarm_type('choochness', AlarmCondition.ALARM_LOW)
    assert al.alarm_dict[AlarmCondition.ALARM_LOW] == {'choochness'}
    al.append_alarm_type('choochability', 'warn')
    assert al.alarm_dict[AlarmCondition.WARN] == {'choochability'}
    al.append_alarm_type('choochity', 'warn')
    assert al.alarm_dict[AlarmCondition.WARN] == {'choochity', 'choochability'}


def test_remove_alarm_type():
    data = {AlarmCondition.ALARM_LOW: {'a1', 'a2'}}
    al = AlarmInfo(alarm_dict=data)
    al.remove_alarm_type('a1', AlarmCondition.ALARM_LOW)
    assert al.alarm_dict[AlarmCondition.ALARM_LOW] == {'a2'}
    data = {AlarmCondition.ALARM_LOW: {'a1', 'a2'}}
    al = AlarmInfo(alarm_dict=data)
    al.remove_alarm_type('a1', 'alarmLow')
    assert al.alarm_dict[AlarmCondition.ALARM_LOW] == {'a2'}


def test_alarm_type():
    data = {AlarmCondition.ALARM_LOW: {'a1', 'a2'},
            AlarmCondition.WARN_LOW: {'w1', 'w2'},
            }
    al = AlarmInfo(alarm_dict=data)
    assert al.alarm_type == AlarmCondition.ALARM_LOW.asString()
    al.remove_alarm_type('a1', AlarmCondition.ALARM_LOW)
    al.remove_alarm_type('a2', AlarmCondition.ALARM_LOW)
    assert al.alarm_type == AlarmCondition.WARN_LOW.asString()
    al.remove_alarm_type('w1', AlarmCondition.WARN_LOW)
    al.remove_alarm_type('w2', AlarmCondition.WARN_LOW)
    assert al.alarm_type == ''
