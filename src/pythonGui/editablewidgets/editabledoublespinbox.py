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

from widget import EditableWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class EditableDoubleSpinBox(EditableWidget):
    category = "Digit"
    alias = "Float Field"

    def __init__(self, box, parent):
        super(EditableDoubleSpinBox, self).__init__(box)
        self.__leDblValue = QLineEdit(parent)
        self.__validator = QDoubleValidator(self.__leDblValue)
        self.__leDblValue.setValidator(self.__validator)
        self.__leDblValue.textChanged.connect(self.onEditingFinished)

        # Needed for updates during input, otherwise cursor jumps to end of input
        self.__lastCursorPos = 0


    @property
    def widget(self):
        return self.__leDblValue


    def addParameters(self,minInc=None, maxInc=None, **params):
        if minInc is not None:
            self.__validator.setBottom(minInc)

        if maxInc is not None:
            self.__validator.setTop(maxInc)


    @property
    def value(self):
        return float(self.__leDblValue.text())


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = 0.0
        
        self.__leDblValue.blockSignals(True)
        self.__leDblValue.setText("{}".format(value))
        self.__leDblValue.blockSignals(False)   
        
        self.__leDblValue.setCursorPosition(self.__lastCursorPos)
        
        if forceRefresh:
            # Needs to be called to update possible apply buttons
            self.onEditingFinished(value)


### slots ###
    def onEditingFinished(self, value):
        self.__lastCursorPos = self.__leDblValue.cursorPosition()
        self.signalEditingFinished.emit(self.boxes[0], float(value))
