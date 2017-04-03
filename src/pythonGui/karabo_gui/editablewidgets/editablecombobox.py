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

__all__ = ["EditableComboBox"]


from karabo_gui.util import SignalBlocker
from karabo_gui.widget import EditableWidget

from PyQt4.QtCore import QEvent
from PyQt4.QtGui import QComboBox


class EditableComboBox(EditableWidget):
    priority = 20
    alias = "Selection Field"

    def __init__(self, box, parent):
        super(EditableComboBox, self).__init__(box)
        
        self.widget = QComboBox(parent)
        self.widget.setFrame(False)

        self.widget.installEventFilter(self)
        self.widget.currentIndexChanged[str].connect(self.onEditingFinished)


    @classmethod
    def isCompatible(cls, box, readonly):
        return not readonly and box.descriptor.options is not None


    def typeChanged(self, box):
        with SignalBlocker(self.widget):
            self.widget.clear()
            self.widget.addItems([str(o) for o in box.descriptor.options])

    def eventFilter(self, object, event):
        # Block wheel event on QComboBox
        return event.type() == QEvent.Wheel and object is self.widget

    @property
    def value(self):
        return self.boxes[0].descriptor.options[self.widget.currentIndex()]

    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            return

        try:
            index = next(i for i, v in enumerate(box.descriptor.options)
                         if v == value)
        except StopIteration:
            return

        with SignalBlocker(self.widget):
            self.widget.setCurrentIndex(index)

