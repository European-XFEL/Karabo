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
from scientificdoublespinbox import ScientificDoubleSpinBox

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["Digit","Float Field","EditableDoubleSpinBox"]


class EditableDoubleSpinBox(EditableWidget):
    
    EPSILON_MIN = 1e-4
    EPSILON_MAX = 1e+4
    
    def __init__(self, **params):
        super(EditableDoubleSpinBox, self).__init__(**params)

        # Using new scientific doublespinbox
        self.__scientificSpinBox = ScientificDoubleSpinBox()
        self.__scientificSpinBox.setVisible(False)
        
        self.__scientificSpinBox.installEventFilter(self)
        self.__scientificSpinBox.valueChanged.connect(self.onEditingFinished)
        
        # Using normal doublespinbox
        self.__normalSpinBox = QDoubleSpinBox()
	# Set Range to maximum possible values
	doubleMax = sys.float_info.max
	self.__normalSpinBox.setRange(-doubleMax, doubleMax)
        self.__normalSpinBox.setDecimals(6)
        self.__normalSpinBox.setSingleStep(0.000001)
        
        self.__normalSpinBox.installEventFilter(self)
        self.__normalSpinBox.valueChanged.connect(self.onEditingFinished)
        
        self.__spinBoxWidget = QWidget()
        layout = QHBoxLayout(self.__spinBoxWidget)
        layout.addWidget(self.__scientificSpinBox)
        layout.addWidget(self.__normalSpinBox)
        
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


    def eventFilter(self, object, event):
        # Block wheel event on QDoubleSpinBox
        if self.__scientificSpinBox.isVisible():
            doubleSpinBox = self.__scientificSpinBox
        else:
            doubleSpinBox = self.__normalSpinBox
        
        if (event.type() == QEvent.Wheel) and (object == doubleSpinBox):
            return True
        return False


    def _getCategory(self):
        category, alias, className = getCategoryAliasClassName()
        return category
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__spinBoxWidget
    widget = property(fget=_getWidget)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return [self.__key]
    keys = property(fget=_getKeys)


    def addParameters(self, **params):
        min = params.get(QString('minimum'))
        if min is None:
            min = params.get('minimum')
        if min is not None:
            self.minimum = min

        max = params.get(QString('maximum'))
        if max is None:
            max = params.get('maximum')
        if max is not None:
            self.maximum = max


    def _value(self):
        return self.__normalSpinBox.value()
    value = property(fget=_value)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def _setMinimum(self, min):
        self.__scientificSpinBox.blockSignals(True)
        self.__scientificSpinBox.setMinimum(min)
        self.__scientificSpinBox.blockSignals(False)

        self.__normalSpinBox.blockSignals(True)
        self.__normalSpinBox.setMinimum(min)
        self.__normalSpinBox.blockSignals(False)
    minimum = property(fset=_setMinimum)


    def _setMaximum(self, max):
        self.__scientificSpinBox.blockSignals(True)
        self.__scientificSpinBox.setMaximum(max)
        self.__scientificSpinBox.blockSignals(False)
        
        self.__normalSpinBox.blockSignals(True)
        self.__normalSpinBox.setMaximum(max)
        self.__normalSpinBox.blockSignals(False)
    maximum = property(fset=_setMaximum)


    def _setWidgetVisibility(self, value):
        if ((value > EditableDoubleSpinBox.EPSILON_MIN)
        and (value < EditableDoubleSpinBox.EPSILON_MAX)):
            self.__scientificSpinBox.setVisible(False)
            self.__normalSpinBox.setVisible(True)
        else:
            self.__scientificSpinBox.setVisible(True)
            self.__normalSpinBox.setVisible(False)


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            return
        
        # Show either normal or scientific view
        self._setWidgetVisibility(value)

        self.__scientificSpinBox.blockSignals(True)
        self.__scientificSpinBox.setValue(value)
        self.__scientificSpinBox.blockSignals(False)  
        
        self.__normalSpinBox.blockSignals(True)
        self.__normalSpinBox.setValue(value)
        self.__normalSpinBox.blockSignals(False)      
        
        if forceRefresh:
            # Needs to be called to update possible apply buttons
            self.onEditingFinished(value)


### slots ###
    def onEditingFinished(self, value):
        # Show either normal or scientific view
        self._setWidgetVisibility(value)
        
        self.valueEditingFinished(self.__key, value)


    class Maker:
        def make(self, **params):
            return EditableDoubleSpinBox(**params)

