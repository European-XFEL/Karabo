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


from util import SignalBlocker
from widget import EditableWidget

from PyQt4.QtCore import QEvent
from PyQt4.QtGui import QComboBox


class EditableComboBox(EditableWidget):
    category = "Selection"
    alias = "Selection Field"

    def __init__(self, box, parent):
        super(EditableComboBox, self).__init__(box)
        
        self.widget = QComboBox(parent)
        self.widget.setFrame(False)

        self.widget.installEventFilter(self)
        self.widget.currentIndexChanged[str].connect(self.onEditingFinished)


    def typeChanged(self, box):
        with SignalBlocker(self.widget):
            self.widget.clear()
            self.widget.addItems(box.descriptor.options)


    def eventFilter(self, object, event):
        # Block wheel event on QComboBox
        return event.type() == QEvent.Wheel and object is self.widget


    @property
    def value(self):
        return self.widget.currentText()


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            return

        index = self.widget.findText(unicode(value))
        if index < 0:
            return

        with SignalBlocker(self.widget):
            self.widget.setCurrentIndex(index)

        if forceRefresh:
            # Needs to be called to update possible apply buttons
            self.onEditingFinished(value)
