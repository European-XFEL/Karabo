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

from widget import DisplayWidget

from PyQt4.QtGui import QSpinBox

from numpy import iinfo

class DisplaySpinBox(DisplayWidget):
    category = "Digit"
    alias = "Integer Field"

    def __init__(self, box, parent):
        super(DisplaySpinBox, self).__init__(box)

        self.__spinBox = QSpinBox(parent)
        self.__spinBox.setReadOnly(True)
        box.addWidget(self)


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
