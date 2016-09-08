"""This module is a registry for all classes which may be serialized and
deserialized, in addition to the types already known to Hash.
"""

known_classes = {}


def register_class(cls):
    global known_classes
    known_classes[cls.__name__] = cls
