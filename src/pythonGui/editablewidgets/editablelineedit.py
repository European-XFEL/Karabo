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


from widget import EditableWidget

from PyQt4.QtGui import QLineEdit


class EditableLineEdit(EditableWidget):
    category = "String"
    alias = "Text Field"

    def __init__(self, box, parent):
        super(EditableLineEdit, self).__init__(box)
        
        self.__lineEdit = QLineEdit(parent)
        self.__lineEdit.textChanged.connect(self.onEditingFinished)

        # Needed for updates during input, otherwise cursor jumps to end of input
        self.__lastCursorPos = 0


    @property
    def widget(self):
        return self.__lineEdit


    @property
    def value(self):
        return self.__lineEdit.text()


    def valueChanged(self, box, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = ""
        
        self.__lineEdit.blockSignals(True)
        self.__lineEdit.setText(value)
        self.__lineEdit.blockSignals(False)
        
        self.__lineEdit.setCursorPosition(self.__lastCursorPos)
        
        if forceRefresh:
            # Needs to be called to update possible apply buttons
            self.onEditingFinished(value)


    def onEditingFinished(self, value):
        self.__lastCursorPos = self.__lineEdit.cursorPosition()
        EditableWidget.onEditingFinished(self, value)
