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

__all__ = ["DisplayCheckBox"]


from widget import DisplayWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DisplayCheckBox(DisplayWidget):
    category = "Switch"
    alias = "Toggle Field"

    
    def __init__(self, **params):
        super(DisplayCheckBox, self).__init__(**params)
        
        self.__checkBox = QCheckBox()
        self.__checkBox.setEnabled(False)


    @property
    def widget(self):
        return self.__checkBox


    @property
    def value(self):
        return self.__checkBox.checkState() == Qt.Checked


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return
        
        value = bool(value) # could be 0 or 1
        
        checkState = Qt.Checked
        if value is True:
            checkState = Qt.Checked
        else :
            checkState = Qt.Unchecked
        
        self.__checkBox.blockSignals(True)
        self.__checkBox.setCheckState(checkState)
        self.__checkBox.blockSignals(False)