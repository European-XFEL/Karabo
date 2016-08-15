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

import os.path

from PyQt4.QtCore import QObject, pyqtSignal, pyqtSlot
from PyQt4.QtGui import QLabel, QPixmap

from karabo.middlelayer import String
from karabo_gui import background
from karabo_gui.const import OK_COLOR, ERROR_COLOR
from karabo_gui.registry import Registry
from karabo_gui.util import generateObjectName
import karabo_gui.gui as gui


class Widget(Registry, QObject):
    """ This is the parent class for all widget factories in the GUI """
    widgets = {}
    displayType = None
    priority = 0

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
        self.deferred = False

    @classmethod
    def register(cls, name, dict):
        super(Widget, cls).register(name, dict)
        if "alias" in dict:
            Widget.widgets[name] = cls
        if "menu" in dict:
            cls.factories[name] = cls
            cls.factory = cls

    @classmethod
    def isCompatible(cls, box, readonly):
        """ Return wether this widget may be used with a given box and
        read-only-ness."""
        return ((readonly and issubclass(cls, DisplayWidget) or
                 not readonly and issubclass(cls, EditableWidget)) and
                isinstance(box.descriptor, cls.category) and
                cls.displayType in (None, box.descriptor.displayType))

    @classmethod
    def getClass(cls, box):
        p = -1
        for c in cls.getClasses(box):
            if c.priority > p:
                winner = c
                p = c.priority
        return winner

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

    @pyqtSlot(object, object, object)
    @pyqtSlot()
    def updateStateSlot(self, box=None, value=None, ts=None):
        self.updateState()

    def updateState(self):
        """This sets the widget to be enabled or disabled, depending on
        whether the user is allowed to make changes. It simply calls
        setEnabled on the widget. Overwrite this method if that's not
        how it works for your widget."""
        self.widget.setEnabled(self.boxes[0].isAccessible())

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
        self.updateState()

    def updateLater(self):
        """call longer-running code at a later time

        If the widget has code that takes a while to run, it should be
        moved to *deferredUpdate*, and the widget should call *updateLater*
        to schedule this update. This keeps the GUI responsive.

        This is especially important for values which change often, as
        *deferredUpdate* will only be called once the many updates finished."""
        def updater():
            self.deferredUpdate()
            self.deferred = False
        if not self.deferred:
            background.executeLater(updater, background.Priority.BACKGROUND)
        self.deferred = True

    def deferredUpdate(self):
        """actually update widget as ordered by *updateLater*

        Overwrite this method to run long-running code, and call
        *updateLater* instead. The default implementation does nothing."""

    @property
    def project(self):
        # XXX: this is crazy - hopefully can be removed
        return self.widget.parent().parent().parent().project


class DisplayWidget(Widget):
    """All widgets displaying a value should inherit from this subclass
    of :class:`Widget`."""
    menu = "Change display widget"
    factories = {}

    @staticmethod
    def getClasses(box):
        return [v for v in Widget.widgets.values()
                if v.isCompatible(box, True)]

    def setReadOnly(self, ro):
        assert ro, "combined Editable and Display widgets: set setReadOnly!"

    def updateState(self):
        pass


class VacuumWidget(DisplayWidget):
    menu = "Change vacuum widget"
    category = String

    def __init__(self, box, parent):
        DisplayWidget.__init__(self, box)

        self.widget = QLabel(parent)

        objectName = generateObjectName(self)
        self._styleSheet = ("QLabel#{}".format(objectName) +
                            " {{ background-color : rgba{}; }}")
        self.widget.setObjectName(objectName)
        self.setErrorState(False)

    value = None

    @classmethod
    def isCompatible(cls, box, readonly):
        return box.path == ("state",) and super().isCompatible(box, readonly)

    def _setPixmap(self, name):
        p = QPixmap(os.path.join(os.path.dirname(__file__),
                    "icons", "vacuum", name))
        self.widget.setPixmap(p)
        self.widget.setMaximumWidth(p.width())
        self.widget.setMaximumHeight(p.height())

    def setErrorState(self, isError):
        color = ERROR_COLOR if isError else OK_COLOR
        ss = self._styleSheet.format(color)
        self.widget.setStyleSheet(ss)


class EditableWidget(Widget):
    """All widgets with which one can edit value should inherit from
    this subclass of :class:`Widget`."""
    menu = "Change widget"
    factories = {}
    signalEditingFinished = pyqtSignal(object, object)

    def __init__(self, box):
        Widget.__init__(self, box)
        box.configuration.boxvalue.state.signalUpdateComponent.connect(
            self.updateStateSlot)
        gui.window.signalGlobalAccessLevelChanged.connect(self.updateStateSlot)

    @staticmethod
    def getClasses(box):
        return [v for v in Widget.widgets.values()
                if v.isCompatible(box, False)]

    def setReadOnly(self, ro):
        assert not ro, "combined Editable and Display widgets: set setReadOnly!"

    def onEditingFinished(self, value):
        self.signalEditingFinished.emit(self.boxes[0], value)
