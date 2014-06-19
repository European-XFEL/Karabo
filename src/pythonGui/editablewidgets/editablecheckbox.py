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

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QCheckBox


class EditableCheckBox(EditableWidget):
    category = "Switch"
    alias = "Toggle Field"

    def __init__(self, box, parent):
        super(EditableCheckBox, self).__init__(box)
        
        self.__checkBox = QCheckBox(parent)
        self.__checkBox.stateChanged.connect(self.onEditingFinished)


    @property
    def widget(self):
        return self.__checkBox


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
        self.signalEditingFinished.emit(self.boxes[0], value == Qt.Checked)
