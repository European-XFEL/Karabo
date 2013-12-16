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


from editablewidget import EditableWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["String","Text Field","EditableLineEdit"]


class EditableLineEdit(EditableWidget):
  
    def __init__(self, **params):
        super(EditableLineEdit, self).__init__(**params)
        
        self.__lineEdit = QLineEdit()
        self.__lineEdit.textChanged.connect(self.onEditingFinished)
        
        # Needed for updates during input, otherwise cursor jumps to end of input
        self.__lastCursorPos = 0
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        # Set key
        self.__key = params.get('key')
        # Set value
        value = params.get('value')
        self.valueChanged(self.__key, value)


    def _getCategory(self):
        category, alias, className = getCategoryAliasClassName()
        return category
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__lineEdit
    widget = property(fget=_getWidget)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return [self.__key]
    keys = property(fget=_getKeys)


    def addParameters(self, **params):
        print "addParameters", params


    def _value(self):
        return str(self.__lineEdit.text())
    value = property(fget=_value)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


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
        self.valueEditingFinished(self.__key, value)


    class Maker:
        def make(self, **params):
            return EditableLineEdit(**params)

