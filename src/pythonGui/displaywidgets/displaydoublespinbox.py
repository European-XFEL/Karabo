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

from widget import DisplayWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DisplayDoubleSpinBox(DisplayWidget):
    category = "Digit"
    alias = "Float Field"
    
    def __init__(self, **params):
        super(DisplayDoubleSpinBox, self).__init__(**params)
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        self.__leDblValue = QLineEdit()
        self.__validator = QDoubleValidator(self.__leDblValue)
        self.__leDblValue.setValidator(self.__validator)
        self.__leDblValue.setReadOnly(True)
        
        self.__key = params.get('key')

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
        try:
            return float(self.__leDblValue.text())
        except ValueError:
            return 0
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
            self.__leDblValue.setText("{}".format(value))
            self.__leDblValue.blockSignals(False)
