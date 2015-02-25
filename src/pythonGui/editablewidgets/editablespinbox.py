#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class EditableWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["EditableSpinBox"]


from util import SignalBlocker
from widget import DisplayWidget, EditableWidget

from karabo.hashtypes import Integer

from PyQt4.QtCore import QEvent
from PyQt4.QtGui import QSpinBox


class EditableSpinBox(EditableWidget, DisplayWidget):
    category = Integer
    alias = "Integer Spin Box"

    def __init__(self, box, parent):
        super(EditableSpinBox, self).__init__(box)
        self.widget = QSpinBox(parent)


    def setReadOnly(self, ro):
        self.widget.setReadOnly(ro)
        if not ro:
            self.widget.installEventFilter(self)
            self.widget.valueChanged.connect(self.onEditingFinished)


    def eventFilter(self, object, event):
        # Block wheel event on QSpinBox
        return event.type() == QEvent.Wheel and object is self.widget


    @property
    def value(self):
        return self.widget.value()


    def typeChanged(self, box):
        rmin, rmax = box.descriptor.getMinMax()
        self.widget.setRange(max(-0x80000000, rmin), min(0x7fffffff, rmax))


    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            value = 0
        
        with SignalBlocker(self.widget):
            self.widget.setValue(value)

