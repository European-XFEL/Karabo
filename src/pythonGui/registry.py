""" This module contains a mechanism to register classes that
inherit from a certain parent. """

from PyQt4.QtCore import Qt, QObject
from PyQt4.QtGui import QAction
from karabo.api_2 import Registry
from const import ns_karabo


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


class Loadable(Registry):
    """The registry for all loadable objects.

    The XML element either has to have a krb:class attribute, which
    is registered here under subclasses, or it is an SVG element, which
    is registered under xmltags. Unknown elements are ignored."""
    xmltags = { }
    krbclasses = { }


    @classmethod
    def register(cls, name, dict):
        super(Loadable, cls).register(name, dict)
        if "xmltag" in dict:
            Loadable.xmltags[cls.xmltag] = cls
        cls.krbclasses[name] = cls


    @staticmethod
    def load(element, layout):
        krb = element.get(ns_karabo + "class")
        ret = None
        if krb is not None:
            ret = Loadable.krbclasses[krb].load(element, layout)
        if ret is None:
            try:
                return Loadable.xmltags[element.tag].load(element, layout)
            except KeyError:
                return
        return ret
