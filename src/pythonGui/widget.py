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
        """ Create a widget with one box.

        For widgets that support more than one box, pass None for the
        box and do the managment of boxes yourself.

        Note that valueChanged will be called with and initial value for
        the box, so make sure __init__ is only called after everything is
        set up so that this poses no problem."""
        super(Widget, self).__init__()
        self.valueType = None
        if box is not None:
            self.boxes = [box]
            self.connectBox(box)


    @classmethod
    def register(cls, name, dict):
        super(Widget, cls).register(name, dict)
        if "menu" in dict:
            cls.factories[name] = cls
            cls.factory = cls


    @classmethod
    def getClass(cls, alias):
        # Get module and class name as tuple (moduleName,className)
        return cls.aliasConcreteClass[alias]


    @classmethod
    def getAliasesViaCategory(cls, category):
        return cls.categoryToAliases.get(category, [ ])


    def connectBox(self, box):
        """ connects this widget to the signals of the box """
        if self.typeChanged is not None:
            box.signalNewDescriptor.connect(self.typeChanged)
            if box.descriptor is not None:
                self.typeChanged(box)
        if self.valueChanged is not None:
            box.signalUpdateComponent.connect(self.valueChanged)
            if box.hasValue():
                self.valueChanged(box, box.value, box.timestamp)


    def addBox(self, box):
        """adds the *box* to the displayed or edited box.

        Return whether that is possible. The default implementation returns
        False, as multi-box support has to be programmed by the user."""
        return False


    def save(self, element):
        """Saves the widget into the ElementTree element"""
        return


    def load(self, element):
        """Loads the widgets from the ElementTree element"""
        return


class DisplayWidget(Widget):
    menu = "Change display widget"
    factories = { }
    categoryToAliases = { }
    aliasConcreteClass = { }


    @classmethod
    def register(cls, name, dict):
        super(DisplayWidget, cls).register(name, dict)
        if "alias" in dict:
            DisplayWidget.categoryToAliases.setdefault(cls.category,
                                                       [ ]).append(cls.alias)
            DisplayWidget.aliasConcreteClass[cls.alias] = cls


    def setReadOnly(self, ro):
        assert ro, "combined Editable and Display widgets: set setReadOnly!"


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
    categoryToAliases = { }
    aliasConcreteClass = { }


    @classmethod
    def register(cls, name, dict):
        super(EditableWidget, cls).register(name, dict)
        if "alias" in dict:
            EditableWidget.categoryToAliases.setdefault(cls.category,
                                                        [ ]).append(cls.alias)
            EditableWidget.aliasConcreteClass[cls.alias] = cls


    def addParameters(self, **params):
        pass


    def setReadOnly(self, ro):
        assert not ro, "combined Editable and Display widgets: set setReadOnly!"


    def onEditingFinished(self, value):
        self.signalEditingFinished.emit(self.boxes[0], value)
