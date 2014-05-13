""" This module contains a mechanism to register classes that
inherit from a certain parent. """

from PyQt4.QtCore import Qt, QObject
from PyQt4.QtGui import QAction
from karabo.registry import Registry
from const import ns_karabo


class Monkey(object):
    """ This is a Monkey-patcher class

    A class with this metaclass does not actually define a new class, but
    updates an already preexisting class. """
    def __new__(cls, name, bases, dict):
        for k, v in dict.iteritems():
            setattr(bases[0], k, v)
        return bases[0]


class Registry(Registry):
    """ This is a special case of a registry which inherits QObject """
    class __metaclass__(type(Registry), type(QObject)):
        pass


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
        if krb is not None:
            return Loadable.krbclasses[krb].load(element, layout)
        try:
            return Loadable.xmltags[element.tag].load(element, layout)
        except KeyError:
            return
