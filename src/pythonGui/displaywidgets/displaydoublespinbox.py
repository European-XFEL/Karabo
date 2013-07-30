#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a widget plugin for attributes
   and is created by the factory class DisplayWidget.
   
   Each plugin needs to implement the following interface:
   
   def getCategoryAliasClassName():
       pass
   
    class Maker:
        def make(self, **params):
            return Attribute*(**params)
"""

__all__ = ["DisplayDoubleSpinBox"]

#import sys

from displaywidget import DisplayWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["Digit","Float Field","DisplayDoubleSpinBox"]


class DisplayDoubleSpinBox(DisplayWidget):
    
    def __init__(self, **params):
        super(DisplayDoubleSpinBox, self).__init__(**params)
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        self.__leDblValue = QLineEdit()
        self.__validator = QDoubleValidator(self.__leDblValue)
        self.__leDblValue.setValidator(self.__validator)
        self.__leDblValue.setEnabled(False)
        
        self.__key = params.get(QString('key'))
        if self.__key is None:
            self.__key = params.get('key')
        
        # Set value
        value = params.get(QString('value'))
        if value is None:
            value = params.get('value')
        if value is not None:
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


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return
        
        if value != self.value:
            self.__leDblValue.blockSignals(True)
            self.__leDblValue.setText(QString("%1").arg(value))
            self.__leDblValue.blockSignals(False)


    class Maker:
        def make(self, **params):
            return DisplayDoubleSpinBox(**params)

