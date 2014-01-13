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

from widget import EditableWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class EditableSpinBox(EditableWidget):
    category = "Digit"
    alias = "Integer Field"

    def __init__(self, value=None, **params):
        super(EditableSpinBox, self).__init__(**params)

        self.__spinBox = QSpinBox()
        #intMax = sys.maxint <--- This produces a bug in 64bit systems
        self.__spinBox.setRange(globals.MIN_INT32, globals.MAX_INT32)
        
        self.__spinBox.installEventFilter(self)
        self.__spinBox.valueChanged.connect(self.onEditingFinished)
        
        self.valueChanged(self.keys[0], value)

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
        return event.type() == QEvent.Wheel and object == self.__spinBox


    @property
    def widget(self):
        return self.__spinBox


    def addParameters(self, minInc=None, maxInc=None, **params):
        if minInc is not None:
            self.__spinBox.blockSignals(True)
            self.__spinBox.setMinimum(minInc)
            self.__spinBox.blockSignals(False)

        if maxInc is not None:
            self.__spinBox.blockSignals(True)
            self.__spinBox.setMaximum(maxInc)
            self.__spinBox.blockSignals(False)


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
        self.valueEditingFinished(self.keys[0], value)
