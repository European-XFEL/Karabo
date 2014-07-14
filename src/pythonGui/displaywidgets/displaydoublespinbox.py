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

from PyQt4.QtGui import QLineEdit


class DisplayDoubleSpinBox(DisplayWidget):
    category = "Digit"
    alias = "Float Field"
    
    def __init__(self, box, parent):
        super(DisplayDoubleSpinBox, self).__init__(box)
        
        self.__leDblValue = QLineEdit(parent)
        self.__validator = QDoubleValidator(self.__leDblValue)
        self.__leDblValue.setValidator(self.__validator)
        self.__leDblValue.setReadOnly(True)


    @property
    def widget(self):
        return self.__leDblValue


    @property
    def value(self):
        try:
            return float(self.__leDblValue.text())
        except ValueError:
            return None


    def valueChanged(self, box, value, timestamp=None):
        if value != self.value:
            self.__leDblValue.blockSignals(True)
            self.__leDblValue.setText("{}".format(value))
            self.__leDblValue.blockSignals(False)
