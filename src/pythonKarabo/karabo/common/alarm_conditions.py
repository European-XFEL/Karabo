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
from enum import Enum
from functools import total_ordering


@total_ordering
class AlarmCondition(Enum):
    """A class for unified alarm conditions.

    Alarms are categorized by severity, which can either be none, warn, alarm
    or interlock. Interlock is the most severe condition.
    """

    NONE = "none"
    WARN = "warn"
    ALARM = "alarm"
    INTERLOCK = "interlock"

    @staticmethod
    def criticalityList():
        """Return the severity list

        severity increases from left to right
        :return: an list with alarm severities
        """
        return [
            AlarmCondition.NONE,
            AlarmCondition.WARN,
            AlarmCondition.ALARM,
            AlarmCondition.INTERLOCK,
        ]

    def criticalityLevel(self):
        """Return the severity level of this alarm condition"""
        for p in AlarmCondition.criticalityList():
            if p.value in self.value:
                return p
        return self

    @staticmethod
    def returnMostSignificant(conditionList):
        """Returns the most severe/significant condition

        Returns the most severe/significant condition in the given list of
        conditions
        :param conditionList: a list containing alarm conditions
        :return: The most severe condition in the list
        """
        if len(conditionList) == 0:
            return AlarmCondition.NONE
        else:
            s = conditionList[0]
            for c in conditionList:
                s = s.returnMoreSignificant(c)
            return s.criticalityLevel()

    def returnMoreSignificant(self, other):
        """Return the more significant of this and the other condition

        :param other: The condition to compare against
        """
        if self.criticalityList().index(
            self.criticalityLevel()
        ) > self.criticalityList().index(other.criticalityLevel()):
            return self
        else:
            return other

    @staticmethod
    def fromString(name):
        """Return an alarm condition from its stringified representation."""
        return AlarmCondition(name)

    def asString(self):
        """Return the string representation of the alarm condition"""
        return self.value

    def asBaseString(self):
        """Return the severity level, e.g. base of the alarmcondtions"""
        return self.parent().value

    def isSameCriticality(self, other):
        """Return whether *other* is of same criticality/severity

        Return whether this alarm condition is of same criticality/severity
        as *other*
        :param other: the alarm condition to compare against
        """
        return self.criticalityList().index(
            self.criticalityLevel()
        ) == self.criticalityList().index(other.criticalityLevel())

    def __lt__(self, other):
        return self.level < other.level


for i, condition in enumerate(AlarmCondition.__members__.values()):
    condition.level = i
