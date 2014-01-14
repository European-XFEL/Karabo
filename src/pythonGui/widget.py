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

class MetaWidget(QObject.__class__):
    def __init__(self, name, bases, dict):
        super(MetaWidget, self).__init__(name, bases, dict)
        if "menu" in dict:
            self.factories[name] = self
            self.factory = self
            self.categoryToAliases = { } # <category, [alias1,alias2,..]>
            self.aliasToCategory = { } # <alias, category>
            self.aliasConcreteClass = { } # dict of actual classes
        elif "alias" in dict:
            if self.category in self.categoryToAliases:
                self.categoryToAliases[self.category].append(self.alias)
            else:
                self.categoryToAliases[self.category] = [self.alias]
            self.aliasToCategory[self.alias] = self.category
            self.aliasConcreteClass[self.alias] = self

class Widget(QObject):
    __metaclass__ = MetaWidget

    def __init__(self, **kwargs):
        # This method should not be necessary. It is, because we
        # get kwargs which are not empty.
        QObject.__init__(self)

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
