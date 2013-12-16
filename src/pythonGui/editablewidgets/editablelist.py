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
from editablewidget import EditableWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["List","Histogram","EditableList"]


class EditableList(EditableWidget):
    
    def __init__(self, **params):
        super(EditableList, self).__init__(**params)
        
        self.__label = Label(**params)
        self.__label.setMinimumWidth(160)
        self.__label.setMaximumHeight(24)
        self.__label.setFrameStyle(QFrame.Box)
        self.__label.signalEditingFinished.connect(self.onEditingFinished)
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        # Set key
        self.__key = params.get('key')
        # Set value
        value = params.get('value')
        self.valueChanged(self.__key, value)


    def _getCategory(self):
        category, alias, className = getCategoryAliasClassName()
        return category
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__label
    widget = property(fget=_getWidget)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return [self.__key]
    keys = property(fget=_getKeys)


    def addParameters(self, **params):
        print "EditableList.addParameters", params


    def _value(self):
        return self.__label.value #self.__list
    value = property(fget=_value)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


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
        self.valueEditingFinished(self.__key, value)


    class Maker:
        def make(self, **params):
            return EditableList(**params)

