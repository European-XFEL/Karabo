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

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class EditableLineEdit(EditableWidget):
    category = "String"
    alias = "Text Field"

    def __init__(self, value=None, **params):
        super(EditableLineEdit, self).__init__(**params)
        
        self.__lineEdit = QLineEdit()
        self.__lineEdit.textChanged.connect(self.onEditingFinished)
        
        # Needed for updates during input, otherwise cursor jumps to end of input
        self.__lastCursorPos = 0
        
        self.valueChanged(self.keys[0], value)


    @property
    def widget(self):
        return self.__lineEdit


    @property
    def value(self):
        return self.__lineEdit.text()


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = str()
        
        self.__lineEdit.blockSignals(True)
        self.__lineEdit.setText(value)
        self.__lineEdit.blockSignals(False)
        
        self.__lineEdit.setCursorPosition(self.__lastCursorPos)
        
        if forceRefresh:
            # Needs to be called to update possible apply buttons
            self.onEditingFinished(value)


### slots ###
    def onEditingFinished(self, value):
        #if "." in value:
         #   QMessageBox.critical(None, "Invalid input", "Your input contains '.' characters.<br>Please choose something else.")
         #   self.__lineEdit.setText("")
         #   return
        self.__lastCursorPos = self.__lineEdit.cursorPosition()
        self.valueEditingFinished(self.keys[0], value)
