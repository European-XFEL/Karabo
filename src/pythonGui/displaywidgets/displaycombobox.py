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

__all__ = ["DisplayComboBox"]


from widget import DisplayWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DisplayComboBox(DisplayWidget):
    category = "Selection"
    alias = "Selection Field"
    
    def __init__(self, enumeration=None, **params):
        super(DisplayComboBox, self).__init__(**params)
        
        self.__comboBox = QComboBox()
        self.__comboBox.setFrame(False)
        self.__comboBox.setEnabled(False)
        
        self.addItems(enumeration)


    @property
    def widget(self):
        return self.__comboBox


    @property
    def value(self):
        return self.__comboBox.currentText()


    def addItems(self, texts):
        self.__comboBox.blockSignals(True)
        self.__comboBox.addItems(texts)
        self.__comboBox.blockSignals(False)


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return

        index = self.__comboBox.findText(str(value))
        if index < 0 :
            return
        
        if value != self.value:
            self.__comboBox.blockSignals(True)
            self.__comboBox.setCurrentIndex(index)
            self.__comboBox.blockSignals(False)
