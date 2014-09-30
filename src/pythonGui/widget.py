#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on July 30, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a factory class to create
   display widgets.

.. autoclass:: Widget
   :members:

.. autoclass:: DisplayWidget

.. autoclass:: EditableWidget
"""

__all__ = ["DisplayWidget"]


from PyQt4.QtCore import QObject, pyqtSignal, pyqtSlot
from PyQt4.QtGui import QLabel, QPixmap
from registry import Registry
import os.path


class Widget(Registry, QObject):
    """ This is the parent class for all widget factories in the GUI """
    widgets = { }

    def __init__(self, box):
        """ Create a widget with one box.

        For widgets that support more than one box, pass ``None`` for the
        box and do the managment of boxes yourself.

        Note that `valueChanged` will be called with an initial value for
        the box, so make sure `__init__` is only called after everything is
        set up so that this poses no problem."""
        super(Widget, self).__init__()
        if box is not None:
            self.boxes = [box]


    @classmethod
    def register(cls, name, dict):
        super(Widget, cls).register(name, dict)
        if "alias" in dict:
            Widget.widgets[name] = cls
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


    def addBox(self, box):
        """adds the *box* to the displayed or edited box.

        Return whether that is possible. The default implementation returns
        False, as multi-box support has to be programmed by the user."""
        return False


    def save(self, element):
        """Saves the widget into the :class:`~xml.etree.ElementTree.Element`
        element"""


    def load(self, element):
        """Loads the widgets from the :class:`~xml.etree.ElementTree.Element`
        element """


    def valueChanged(self, box, value, timestamp=None):
        """ notify the widget about a new value

        *value* is the value to be shown, it might be different from
        the value in the *box*."""


    def typeChanged(self, box):
        """ notify the widget that the type it's displaying has changed

        this is also used to inform the widget about its type initially,
        so put your type-dependent initialization code here. """


    @pyqtSlot(object, object, object)
    def valueChangedSlot(self, box, value, timestamp=None):
        # avoid having to declare valueChanged a slot in every widget
        self.valueChanged(box, value, timestamp)


    @pyqtSlot(object)
    def typeChangedSlot(self, box):
        # avoid having to declare typeChanged a slot in every widget
        self.typeChanged(box)


    @property
    def project(self):
        return self.widget.parent().parent().parent().project


class DisplayWidget(Widget):
    """All widgets displaying a value should inherit from this subclass
    of :class:`Widget`."""
    menu = "Change display widget"
    factories = { }
    displayCTA = categoryToAliases = { }
    displayACC = aliasConcreteClass = { }


    @classmethod
    def register(cls, name, dict):
        super(DisplayWidget, cls).register(name, dict)
        if "alias" in dict:
            cls.displayCTA.setdefault(cls.category, [ ]).append(cls.alias)
            cls.displayACC[cls.alias] = cls


    def setReadOnly(self, ro):
        assert ro, "combined Editable and Display widgets: set setReadOnly!"


class VacuumWidget(DisplayWidget):
    menu = "Change vacuum widget"
    category = "State"
    displayCTA = categoryToAliases = { }
    displayACC = aliasConcreteClass = { }

    def __init__(self, box, parent):
        DisplayWidget.__init__(self, box)

        self.widget = QLabel(parent)
        self.setErrorState(False)


    value = None


    def _setPixmap(self, name):
        p = QPixmap(os.path.join(os.path.dirname(__file__),
                    "icons", "vacuum", name))
        self.widget.setPixmap(p)
        self.widget.setMaximumWidth(p.width())
        self.widget.setMaximumHeight(p.height())


    def setErrorState(self, isError):
        if isError:
            self.widget.setStyleSheet( # light red
                "QLabel { background-color : rgba(255,155,155,128); }")
        else:
            self.widget.setStyleSheet( # light green
                "QLabel { background-color : rgba(225,242,225,128); }")


class EditableWidget(Widget):
    """All widgets with which one can edit value should inherit from
    this subclass of :class:`Widget`."""
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


    def setReadOnly(self, ro):
        assert not ro, "combined Editable and Display widgets: set setReadOnly!"


    def onEditingFinished(self, value):
        self.signalEditingFinished.emit(self.boxes[0], value)
