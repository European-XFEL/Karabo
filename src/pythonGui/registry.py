""" This module contains a mechanism to register classes that
inherit from a certain parent. """

from PyQt4.QtCore import Qt, QObject
from PyQt4.QtGui import QAction, QIcon

from xml.etree import ElementTree


ns_svg = "{http://www.w3.org/2000/svg}"
ns_karabo = "{http://karabo.eu/scene}"
ElementTree.register_namespace("svg", ns_svg[1:-1])
ElementTree.register_namespace("krb", ns_karabo[1:-1])


Registry = None


class Metaclass(QObject.__class__):
    def __init__(self, name, bases, dict):
        super(Metaclass, self).__init__(name, bases, dict)
        if Registry is not None:
            super(self, self).register(name, dict)


class Registry(object):
    __metaclass__ = Metaclass


    @classmethod
    def register(cls, name, dict):
        pass

        
class Loadable(Registry):
    """The registry for all loadable objects.

    The XML element either has to have a krb:class attribute, which
    is registered here under subclasses, or it is an SVG element, which
    is registered under xmltags. Unknown elements are ignored."""
    __metaclass__ = Metaclass
    xmltags = { }
    krbclasses = { }


    @classmethod
    def register(cls, name, dict):
        super(Loadable, cls).register(name, dict)
        if "xmltag" in dict:
            Loadable.xmltags[cls.xmltag] = cls
        else:
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
