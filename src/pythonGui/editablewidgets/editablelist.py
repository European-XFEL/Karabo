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

__all__ = ["EditableList"]


from label import Label
from widget import EditableWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class EditableList(EditableWidget):
    category = "List"
    alias = "Histogram"

    def __init__(self, box):
        super(EditableList, self).__init__(box)
        
        self.__label = Label(value="", valueType=box.descriptor.valueType)
        self.__label.setMinimumWidth(160)
        self.__label.setMaximumHeight(24)
        self.__label.setFrameStyle(QFrame.Box)
        self.__label.signalEditingFinished.connect(self.onEditingFinished)

        if hasattr(box, 'value'):
            self.valueChanged(box, box.value)


    @property
    def widget(self):
        return self.__label


    @property
    def value(self):
        return self.__label.value


    def valueChanged(self, key, value, timestamp=None, forceRefresh=False):
        if value is None:
            return
        
        #self.__list = value
        self.__label.value = value
        
        #listLen = len(self.__list)
        #maxLen = 10
        #valueAsString = ""
        #for i in range(listLen):
        #    if maxLen < 1:
        #        valueAsString += ".."
        #        break
            
        #    index = self.__list[i]
        #    valueType = type(index)
        #    if valueType is float:
        #        index = str("%.6f" %index)
        #    valueAsString += str(index)
            
        #    if i != (listLen-1):
        #        valueAsString += ", "
        #    maxLen -= 1
        
        #self.__label.blockSignals(True)
        #self.__label.blockSignals(False)


### slots ###
    def onEditingFinished(self, value):
        self.__label.value = value
        self.signalEditingFinished.emit(self.keys[0], value)
