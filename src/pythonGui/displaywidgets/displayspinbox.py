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

import globals

from widget import DisplayWidget

from karabo.karathon import Types

from PyQt4.QtGui import QSpinBox

from numpy import iinfo

class DisplaySpinBox(DisplayWidget):
    category = "Digit"
    alias = "Integer Field"

    def __init__(self, valueType=None, **params):
        super(DisplaySpinBox, self).__init__(**params)

        self.__spinBox = QSpinBox()

        if valueType is not None:
            info = iinfo(valueType.numpy)
            self.__spinBox.setRange(info.min, info.max)
        self.__spinBox.setReadOnly(True)
        
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


    @property
    def widget(self):
        return self.__spinBox


    @property
    def value(self):
        return self.__spinBox.value()


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


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return
        
        if value != self.value:
            self.__spinBox.blockSignals(True)
            self.__spinBox.setValue(value)
            self.__spinBox.blockSignals(False)
