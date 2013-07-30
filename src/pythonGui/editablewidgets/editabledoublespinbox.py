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

__all__ = ["EditableDoubleSpinBox"]

import sys

from editablewidget import EditableWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["Digit","Float Field","EditableDoubleSpinBox"]


class EditableDoubleSpinBox(EditableWidget):

    
    def __init__(self, **params):
        super(EditableDoubleSpinBox, self).__init__(**params)

        self.__leDblValue = QLineEdit()
        self.__validator = QDoubleValidator(self.__leDblValue)
        self.__leDblValue.setValidator(self.__validator)
        self.__leDblValue.textChanged.connect(self.onEditingFinished)
        
        # Needed for updates during input, otherwise cursor jumps to end of input
        self.__lastCursorPos = 0
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        # Set key
        self.__key = params.get(QString('key'))
        if self.__key is None:
            self.__key = params.get('key')
        # Set value
        value = params.get(QString('value'))
        if value is None:
            value = params.get('value')
        self.valueChanged(self.__key, value)


    def _getCategory(self):
        category, alias, className = getCategoryAliasClassName()
        return category
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__leDblValue
    widget = property(fget=_getWidget)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return [self.__key]
    keys = property(fget=_getKeys)


    def addParameters(self, **params):
        minInc = params.get(QString('minInc'))
        if minInc is None:
            minInc = params.get('minInc')
        
        if minInc:
            self.__validator.setBottom(minInc)

        maxInc = params.get(QString('maxInc'))
        if maxInc is None:
            maxInc = params.get('maxInc')
        
        if maxInc:
            self.__validator.setTop(maxInc)


    def _value(self):
        value, ok = self.__leDblValue.text().toDouble()
        if ok:
            return value
        return  0.0
    value = property(fget=_value)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = 0.0
        
        self.__leDblValue.blockSignals(True)
        self.__leDblValue.setText(QString("%1").arg(value))
        self.__leDblValue.blockSignals(False)   
        
        self.__leDblValue.setCursorPosition(self.__lastCursorPos)
        
        if forceRefresh:
            # Needs to be called to update possible apply buttons
            self.onEditingFinished(value)


### slots ###
    def onEditingFinished(self, value):
        self.__lastCursorPos = self.__leDblValue.cursorPosition()
        self.valueEditingFinished(self.__key, value)


    class Maker:
        def make(self, **params):
            return EditableDoubleSpinBox(**params)

