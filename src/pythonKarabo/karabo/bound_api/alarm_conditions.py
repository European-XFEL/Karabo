__author__="Steffen Hauf <steffen.hauf at xfel.eu>"
__date__ ="$June 30, 2016 2:17:13 PM$"

<<<<<<< HEAD

=======
>>>>>>> 8bc7c061f9e49a59c10482fbc561e0b459561d19
class AlarmCondition:

    NONE = None
    WARN = None
    WARN_LOW = None
    WARN_HIGH = None
    WARN_VARIANCE_LOW = None
    WARN_VARIANCE_HIGH = None
    ALARM = None
    ALARM_LOW = None
    ALARM_HIGH = None
    ALARM_VARIANCE_LOW = None
    ALARM_VARIANCE_HIGH = None
    INTERLOCK = None


    def __init__(self, name, rank = None, parent=None):

        if rank is not None:
           self.rank = rank
        elif parent is not None:
            self.rank = parent.rank
        else:
            raise AttributeError("Either a rank or a parent needs to be given for alarm condition {}!".format(name))

        #protect against new conditions:
        if name not in [k for k,d in AlarmCondition.__dict__.items() if isinstance(d,AlarmCondition) or d is None]:
            raise AttributeError("Alarm condition {} may not be declared, only predeclared conditions are allowed!".format(name))

        self.name = name
        self.parent = parent

    @staticmethod
    def returnMostSignificant(conditionList):
        if len(conditionList) == 0:
            return AlarmCondition.NONE

        else:
            s = conditionList[0]
            for c in conditionList:
                s = s.returnMoreSignificant(c)
            return s if s.parent is None else s.parent


    def returnMoreSignificant(self, other):
        return self if self.rank > other.rank else other

    @classmethod
    def fromString(cls, name):
        if name not in cls.__dict__:
            raise AttributeError("Alarm condition of name {} does not exist!")
        return cls.__dict__[name]


    def asString(self):
        return self.name

    def asBaseString(self):
        return self.parent.name if self.parent is not None else self.name

    def isSameCriticality(self, other):
        return self.rank == other.rank

    def __repr__(self):
        return self.name


AlarmCondition.NONE = AlarmCondition("NONE", rank=0)
AlarmCondition.WARN = AlarmCondition("WARN", rank=1)
AlarmCondition.WARN_LOW = AlarmCondition("WARN_LOW", parent=AlarmCondition.WARN)
AlarmCondition.WARN_HIGH = AlarmCondition("WARN_LOW", parent=AlarmCondition.WARN)
AlarmCondition.WARN_VARIANCE_LOW = AlarmCondition("WARN_VARIANCE_LOW", parent=AlarmCondition.WARN)
AlarmCondition.WARN_VARIANCE_HIGH = AlarmCondition("WARN_VARIANCE_HIGH", parent=AlarmCondition.WARN)
AlarmCondition.ALARM = AlarmCondition("ALARM", rank=2)
AlarmCondition.ALARM_LOW = AlarmCondition("ALARM_LOW", parent=AlarmCondition.ALARM)
AlarmCondition.ALARM_HIGH = AlarmCondition("ALARM_HIGH", parent=AlarmCondition.ALARM)
AlarmCondition.ALARM_VARIANCE_LOW = AlarmCondition("ALARM_VARIANCE_LOW", parent=AlarmCondition.ALARM)
AlarmCondition.ALARM_VARIANCE_HIGH = AlarmCondition("ALARM_VARIANCE_HIGH", parent=AlarmCondition.ALARM)
AlarmCondition.INTERLOCK = AlarmCondition("INTERLOCK", rank=3) #interlock is always the highest

