__author__="Steffen Hauf <steffen.hauf at xfel.eu>"
__date__ ="$June 30, 2016 2:17:13 PM$"


from enum import Enum

class AlarmCondition(Enum):
    """
    A class for unified alarm conditions. Alarms
    are categorized by severity, which can either
    be none, warn, alarm or interlock. Interlock
    is the most severe condition.
    """
    NONE = "none"
    WARN = "warn"
    WARN_LOW = "warnLow"
    WARN_HIGH = "warnHigh"
    WARN_VARIANCE_LOW = "warnVarianceLow"
    WARN_VARIANCE_HIGH = "warnVarianceHigh"
    ALARM = "alarm"
    ALARM_LOW = "alarmLow"
    ALARM_HIGH = "alarmHigh"
    ALARM_VARIANCE_LOW = "alarmVarianceLow"
    ALARM_VARIANCE_HIGH = "alarmVarianceHigh"
    INTERLOCK = "interlock"

    @staticmethod
    def criticalityList():
        """
        Returns the severity list, with severity increasing
        from left to right
        :return: an list with alarm severities
        """

        return [AlarmCondition.NONE, AlarmCondition.WARN,
             AlarmCondition.ALARM, AlarmCondition.INTERLOCK]



    def criticalityLevel(self):
        """
        Returns the severity level of this alarm conditoon
        :return:
        """
        for p in AlarmCondition.criticalityList():
            if p.value in self.value:
                return p
        return self

    @staticmethod
    def returnMostSignificant(conditionList):
        """
        Returns the most severe/significant condition in
        the given list of conditions
        :param conditionList: an iterable containing alarm
        conditions
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
        """
        Return the more signifcant of this and the other condition
        :param other: The condition to compare against
        :return:
        """
        return self if self.criticalityList().index(self.criticalityLevel()) \
                 > self.criticalityList().index(other.criticalityLevel()) else other

    @staticmethod
    def fromString(name):
        """
        Return an alarm condition from its stringified representation.
        :param name:
        :return:
        """
        return AlarmCondition(name)


    def asString(self):
        """
        Return the string representation of the alarm condition
        :return:
        """
        return self.value

    def asBaseString(self):
        """
        Return the severity level, e.g. base of the alarmcondtions
        :return:
        """
        return self.parent().value

    def isSameCriticality(self, other):
        """
        Returns if this alarm condition is of same criticality/severity
        as other
        :param other: the alarm condition to compare against
        :return:
        """
        return self.criticalityList().index(self.criticalityLevel()) \
               == self.criticalityList().index(other.criticalityLevel())
