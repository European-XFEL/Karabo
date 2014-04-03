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

import globals

from util import SignalBlocker
from widget import EditableWidget

from PyQt4.QtCore import QEvent
from PyQt4.QtGui import QSpinBox

from numpy import iinfo


class EditableSpinBox(EditableWidget):
    category = "Digit"
    alias = "Integer Field"

    def __init__(self, box, parent):
        super(EditableSpinBox, self).__init__(box)

        self.widget = QSpinBox(parent)
        self.widget.installEventFilter(self)
        self.widget.valueChanged.connect(self.onEditingFinished)
        box.addWidget(self)


    def eventFilter(self, object, event):
        # Block wheel event on QSpinBox
        return event.type() == QEvent.Wheel and object is self.widget


    @property
    def value(self):
        return self.widget.value()


    def typeChanged(self, box):
        min = getattr(box.descriptor, "minInc", None)
        if min is None:
            min = getattr(box.descriptor, "minExc", None)
        if min is None:
            info = iinfo(box.descriptor.numpy)
            min = info.min
        else:
            min += 1
        if min < -0x80000000:
            min = -0x80000000


        max = getattr(box.descriptor, "maxInc", None)
        if max is None:
            max = getattr(box.descriptor, "maxExc", None)
        if max is None:
            info = iinfo(box.descriptor.numpy)
            max = info.max
        else:
            max -= 1
        if max > 0x7fffffff:
            max = 0x7fffffff

        self.widget.setRange(min, max)



    def valueChanged(self, box, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = 0
        
        with SignalBlocker(self.widget):
            self.widget.setValue(value)
        
        if forceRefresh:
            # Needs to be called to update possible apply buttons
            self.onEditingFinished(value)
