""" This module contains a mechanism to register classes that
inherit from a certain parent. """

from PyQt4.QtCore import QObject
from karabo.middlelayer import Registry


def Monkey(name, bases, dict):
    """ This is a Monkey-patcher class

    A class with this metaclass does not actually define a new class, but
    updates an already preexisting class. """
    for k, v in dict.items():
        setattr(bases[0], k, v)
    return bases[0]


class MetaRegistry(type(Registry), type(QObject)):
    pass


class Registry(Registry, metaclass=MetaRegistry):
    """ This is a special case of a registry which inherits QObject """
