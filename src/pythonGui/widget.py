#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 30, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a factory class to create
   display widgets.
"""

__all__ = ["DisplayWidget"]


from PyQt4.QtCore import QObject, pyqtSignal
from PyQt4.QtGui import QLabel
from registry import Loadable, Registry


class Widget(Registry, QObject):
    valueChanged = None
    typeChanged = None

    def __init__(self, box):
        super(Widget, self).__init__()
        self.valueType = None
        if box is not None:
            self.boxes = [box]


    @classmethod
    def register(cls, name, dict):
        super(Widget, cls).register(name, dict)
        if "menu" in dict:
            cls.factories[name] = cls
            cls.factory = cls
            cls.categoryToAliases = { } # <category, [alias1,alias2,..]>
            cls.aliasToCategory = { } # <alias, category>
            cls.aliasConcreteClass = { } # dict of actual classes
        elif "alias" in dict:
            if cls.category in cls.categoryToAliases:
                cls.categoryToAliases[cls.category].append(cls.alias)
            else:
                cls.categoryToAliases[cls.category] = [cls.alias]
            cls.aliasToCategory[cls.alias] = cls.category
            cls.aliasConcreteClass[cls.alias] = cls


    @classmethod
    def get_class(cls, alias):
        # Get module and class name as tuple (moduleName,className)
        return cls.aliasConcreteClass[alias]


    @classmethod
    def getAliasesViaCategory(cls, category):
        return cls.categoryToAliases.get(category, [ ])


    def addKey(self, key):
        """adds the *key* to the displayed or edited keys.

        Return `True` if that is possible, `False` otherwise."""
        return False


class DisplayWidget(Widget):
    menu = "Change display widget"
    factories = { }


class VacuumWidget(DisplayWidget):
    menu = "Change vacuum widget"
    category = "State"

    def __init__(self, box=None):
        DisplayWidget.__init__(box)

        self.label = QLabel()
        self.setErrorState(False)
        if value is not None:
            self.valueChanged(self.keys[0], value)
            self.value = value


    @property
    def widget(self):
        return self.label


    value = None


    def _setPixmap(self, pixmap):
        self.label.setPixmap(pixmap)
        self.label.setMaximumWidth(pixmap.width())
        self.label.setMaximumHeight(pixmap.height())


    def setErrorState(self, isError):
        if isError is True:
            self.label.setStyleSheet( # light red
                "QLabel { background-color : rgba(255,155,155,128); }")
        else:
            self.label.setStyleSheet( # light green
                "QLabel { background-color : rgba(225,242,225,128); }")


class EditableWidget(Widget):
    menu = "Change widget"
    factories = { }
    signalEditingFinished = pyqtSignal(object, object)


    def addParameters(self, **params):
        pass


    def onEditingFinished(self, value):
        self.signalEditingFinished.emit(self.boxes[0], value)
