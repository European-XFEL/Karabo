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

__all__ = ["DisplayLineEdit"]


from widget import DisplayWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DisplayLineEdit(DisplayWidget):
    category = "String"
    alias = "Text Field"

    def __init__(self, **params):
        super(DisplayLineEdit, self).__init__(**params)
        
        self.__lineEdit = QLineEdit()
        self.__lineEdit.setMinimumSize(160, 24)
        self.__lineEdit.setReadOnly(True)
        

    @property
    def widget(self):
        return self.__lineEdit


    @property
    def value(self):
        return self.__lineEdit.text()


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return
        
        if value != self.value:
            self.__lineEdit.blockSignals(True)
            self.__lineEdit.setText(value)
            self.__lineEdit.blockSignals(False)
