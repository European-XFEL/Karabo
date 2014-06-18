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
        self.__leDblValue.editingFinished.connect(self.onEditingFinished)
        self.__leDblValue.textChanged.connect(self.onTextChanged)

        # Needed for updates during input, otherwise cursor jumps to end of input
        self.__lastCursorPos = 0
        self.normalPalette = self.__leDblValue.palette()
        self.errorPalette = QPalette(self.normalPalette)
        self.errorPalette.setColor(QPalette.Text, Qt.red)


    def onTextChanged(self, text):
        self.__leDblValue.setPalette(self.normalPalette
                                     if self.__leDblValue.hasAcceptableInput()
                                     else self.errorPalette)


    @property
    def widget(self):
        return self.__leDblValue


    def typeChanged(self, box):
        min, max = box.descriptor.getMinMax()
        if min is not None:
            self.__validator.setBottom(min)
        if max is not None:
            self.__validator.setTop(max)


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


    def onEditingFinished(self):
        self.__lastCursorPos = self.__leDblValue.cursorPosition()
        self.signalEditingFinished.emit(self.boxes[0], self.value)
