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


class Widget(QObject, Registry):
    def __init__(self, **kwargs):
        # This method should not be necessary. It is, because we
        # get kwargs which are not empty.
        QObject.__init__(self)


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


class DisplayWidget(Widget):
    menu = "Change display widget"
    factories = { }

    def __init__(self, key=None, **kwargs):
        Widget.__init__(self, **kwargs)
        self.valueType = None
        if key is not None:
            self.keys = [key]

    def removeKey(self, key):
        if key in self.keys:
            self.keys = [ ]


class VacuumWidget(DisplayWidget):
    menu = "Change vacuum widget"
    category = "State"

    def __init__(self, value=None, **params):
        super(VacuumWidget, self).__init__(**params)

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
    signalEditingFinished = pyqtSignal(str, object)

    
    def __init__(self, key=None, **kwargs):
        Widget.__init__(self, **kwargs)
        self.valueType = None
        if key is not None:
            self.keys = [key]
            
    
    def addParameters(self, **params):
        pass

    
    def valueEditingFinished(self, key, value):
        self.signalEditingFinished.emit(key, value)
