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

__all__ = ["EditableSpinBox"]

import globals

from editablewidget import EditableWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["Digit","Integer Field","EditableSpinBox"]


class EditableSpinBox(EditableWidget):
    
    def __init__(self, **params):
        super(EditableSpinBox, self).__init__(**params)

        self.__spinBox = QSpinBox()
        #intMax = sys.maxint <--- This produces a bug in 64bit systems
        self.__spinBox.setRange(globals.MIN_INT32, globals.MAX_INT32)
        
        self.__spinBox.installEventFilter(self)
        self.__spinBox.valueChanged.connect(self.onEditingFinished)
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        # Set key
        self.__key = params.get('key')
        # Set value
        value = params.get('value')
        self.valueChanged(self.__key, value)

        #metricPrefixSymbol = params.get('metricPrefixSymbol')
        #unitSymbol = params.get('unitSymbol')
        # Append unit label, if available
        #unitLabel = str()
        #if metricPrefixSymbol:
        #    unitLabel += metricPrefixSymbol
        #if unitSymbol:
        #    unitLabel += unitSymbol
        #if len(unitLabel) > 0:
        #    self.__spinBox.setSuffix(" %s" %unitLabel)


    def eventFilter(self, object, event):
        # Block wheel event on QSpinBox
        if event.type() == QEvent.Wheel and object == self.__spinBox:
            return True
        return False


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


    def addParameters(self, **params):
        minInc = params.get('minInc')
        if minInc:
            self.__spinBox.blockSignals(True)
            self.__spinBox.setMinimum(minInc)
            self.__spinBox.blockSignals(False)

        maxInc = params.get('maxInc')
        if maxInc:
            self.__spinBox.blockSignals(True)
            self.__spinBox.setMaximum(maxInc)
            self.__spinBox.blockSignals(False)


    def _value(self):
        return self.__spinBox.value()
    value = property(fget=_value)


    def _setMinimum(self, min):
        self.__spinBox.blockSignals(True)
        self.__spinBox.setMinimum(min)
        self.__spinBox.blockSignals(False)
    minimum = property(fset=_setMinimum)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def _setMaximum(self, max):
        self.__spinBox.blockSignals(True)
        self.__spinBox.setMaximum(max)
        self.__spinBox.blockSignals(False)
    maximum = property(fset=_setMaximum)


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = 0
        
        self.__spinBox.blockSignals(True)
        self.__spinBox.setValue(value)
        self.__spinBox.blockSignals(False)
        
        if forceRefresh:
            # Needs to be called to update possible apply buttons
            self.onEditingFinished(value)


### slots ###
    def onEditingFinished(self, value):
        self.valueEditingFinished(self.__key, value)


    class Maker:
        def make(self, **params):
            return EditableSpinBox(**params)

