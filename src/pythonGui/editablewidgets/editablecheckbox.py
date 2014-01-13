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

__all__ = ["EditableCheckBox"]


from widget import EditableWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class EditableCheckBox(EditableWidget):
    category = "Switch"
    alias = "Toggle Field"

    def __init__(self, value=None, **params):
        super(EditableCheckBox, self).__init__(**params)
        
        self.__checkBox = QCheckBox()
        self.__checkBox.stateChanged.connect(self.onEditingFinished)
        
        self.valueChanged(self.keys[0], value)


    @property
    def widget(self):
        return self.__checkBox


    def addParameters(self, **params):
        print "addParameters", params


    @property
    def value(self):
        return self.__checkBox.checkState() == Qt.Checked


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            value = False
        
        checkState = Qt.Checked
        if (value is True) or (value == "true") or (value == 1):
            checkState = Qt.Checked
        else:
            checkState = Qt.Unchecked
        if value != self.value:
            self.__checkBox.blockSignals(True)
            self.__checkBox.setCheckState(checkState)
            self.__checkBox.blockSignals(False)
        
        if forceRefresh:
            # Needs to be called to update possible apply buttons
            self.onEditingFinished(checkState)
        

### slots ###
    def onEditingFinished(self, value):
        self.valueEditingFinished(self.keys[0], value == Qt.Checked)
