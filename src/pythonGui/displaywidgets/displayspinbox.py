#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
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

__all__ = ["DisplaySpinBox"]

import sys

from displaywidget import DisplayWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["Digit","Integer Field","DisplaySpinBox"]


class DisplaySpinBox(DisplayWidget):
    
    def __init__(self, **params):
        super(DisplaySpinBox, self).__init__(**params)

        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>

        self.__spinBox = QSpinBox()
        intMax = sys.maxint
        self.__spinBox.setRange(-intMax, intMax)
        self.__spinBox.setEnabled(False)
        
        self.__key = params.get('key')
        
        # Set value
        value = params.get('value')
        if value is not None:
            self.valueChanged(self.__key, value)


    def _getCategory(self):
        category, alias, className = getCategoryAliasClassName()
        return category
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__spinBox
    widget = property(fget=_getWidget)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return [self.__key]
    keys = property(fget=_getKeys)


    def _value(self):
        return self.__spinBox.value()
    value = property(fget=_value)


    def _setMinimum(self, min):
        self.__spinBox.blockSignals(True)
        self.__spinBox.setMinimum(min)
        self.__spinBox.blockSignals(False)
    minimum = property(fset=_setMinimum)


    def _setMaximum(self, max):
        self.__spinBox.blockSignals(True)
        self.__spinBox.setMaximum(max)
        self.__spinBox.blockSignals(False)
    maximum = property(fset=_setMaximum)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return
        
        if value != self.value:
            self.__spinBox.blockSignals(True)
            self.__spinBox.setValue(value)
            self.__spinBox.blockSignals(False)


    class Maker:
        def make(self, **params):
            return DisplaySpinBox(**params)

