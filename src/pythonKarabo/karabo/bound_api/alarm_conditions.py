__author__="Steffen Hauf <steffen.hauf at xfel.eu>"
__date__ ="$June 30, 2016 2:17:13 PM$"


from enum import Enum

class AlarmCondition(Enum):
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

    @classmethod
    def parentList(cls):
     return [cls.NONE, cls.WARN, cls.ALARM, cls.INTERLOCK] #also defines trumping order


    def parent(self):
        for p in AlarmCondition.parentList():
            if p.value in self.value:
                return p
        return self

    @staticmethod
    def returnMostSignificant(conditionList):
        if len(conditionList) == 0:
            return AlarmCondition.NONE

        else:
            s = conditionList[0]
            for c in conditionList:
                s = s.returnMoreSignificant(c)
            return s.parent()


    def returnMoreSignificant(self, other):
        return self if AlarmCondition.parentList().index(self.parent()) > AlarmCondition.parentList().index(other.parent()) else other

    @classmethod
    def fromString(cls, name):
         return AlarmCondition(name)


    def asString(self):
        return self.value

    def asBaseString(self):
        return self.parent().value

    def isSameCriticality(self, other):
        return AlarmCondition.parentList().index(self.parent()) == AlarmCondition.parentList().index(other.parent())
