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

__all__ = ["EditableLineEdit"]


from util import SignalBlocker
from widget import EditableWidget

from PyQt4.QtGui import QLineEdit


class EditableLineEdit(EditableWidget):
    category = "String"
    alias = "Text Field"

    def __init__(self, box, parent):
        super(EditableLineEdit, self).__init__(box)
        
        self.widget = QLineEdit(parent)
        self.widget.textChanged.connect(self.onEditingFinished)

        # Needed for updates during input, otherwise cursor jumps to end of input
        self.lastCursorPos = 0


    @property
    def value(self):
        return self.widget.text()


    def valueChanged(self, box, value, timestamp=None):
        if value is None:
            value = ""
        
        if not isinstance(value, str):
            value = value.decode()
        
        with SignalBlocker(self.widget):
            self.widget.setText(value)
        
        self.widget.setCursorPosition(self.lastCursorPos)


    def onEditingFinished(self, value):
        self.lastCursorPos = self.widget.cursorPosition()
        EditableWidget.onEditingFinished(self, value)
