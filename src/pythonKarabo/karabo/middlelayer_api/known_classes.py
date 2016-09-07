"""This module is a registry for all classes which may be serialized and
deserialized, in addition to the types already known to Hash.
"""

from karabo.common.states import State
from karabo.common.alarm_conditions import AlarmCondition


known_classes = {}


def register_class(cls):
    global known_classes
    known_classes[cls.__name__] = cls


register_class(State)
register_class(AlarmCondition)
