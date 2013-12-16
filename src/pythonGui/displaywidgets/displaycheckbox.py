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

__all__ = ["DisplayCheckBox"]


from displaywidget import DisplayWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["Switch","Toggle Field","DisplayCheckBox"]


class DisplayCheckBox(DisplayWidget):
    
    def __init__(self, **params):
        super(DisplayCheckBox, self).__init__(**params)
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        self.__checkBox = QCheckBox()
        self.__checkBox.setEnabled(False)

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
        return self.__checkBox
    widget = property(fget=_getWidget)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return [self.__key]
    keys = property(fget=_getKeys)


    def _value(self):
        if self.__checkBox.checkState() == Qt.Checked:
            return True
        else:
            return False
    value = property(fget=_value)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return
        
        value = bool(value) # could be 0 or 1
        
        checkState = Qt.Checked
        if value is True:
            checkState = Qt.Checked
        else :
            checkState = Qt.Unchecked
        
        self.__checkBox.blockSignals(True)
        self.__checkBox.setCheckState(checkState)
        self.__checkBox.blockSignals(False)


    class Maker:
        def make(self, **params):
            return DisplayCheckBox(**params)

