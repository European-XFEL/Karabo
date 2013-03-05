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

__all__ = ["EditableComboBox"]


from editablewidget import EditableWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["Selection","Selection Field","EditableComboBox"]


class EditableComboBox(EditableWidget):
    
    def __init__(self, **params):
        super(EditableComboBox, self).__init__(**params)
        
        self.__comboBox = QComboBox()
        self.__comboBox.setFrame(False)
        
        enumeration = params.get(QString('enumeration'))
        if enumeration is None:
            enumeration = params.get('enumeration')
        self.addItems(enumeration)
        
        self.__valueType = params.get(QString('valueType'))
        if self.__valueType is None:
            self.__valueType = params.get('valueType')
        
        self.__comboBox.installEventFilter(self)
        self.connect(self.__comboBox, SIGNAL("currentIndexChanged(QString)"), self.onEditingFinished)
        
        # Minimum and maximum number of associated keys, 1 by default for each
        self.__minMaxAssociatedKeys = (1,1) # tuple<min,max>
        
        # Set key
        self.__key = params.get(QString('key'))
        if self.__key is None:
            self.__key = params.get('key')
        # Set value
        value = params.get(QString('value'))
        if value is None:
            value = params.get('value')
        self.valueChanged(self.__key, value)


    def eventFilter(self, object, event):
        # Block wheel event on QComboBox
        if event.type() == QEvent.Wheel and object == self.__comboBox:
            return True
        return False


    def _getCategory(self):
        category, alias, className = getCategoryAliasClassName()
        return category
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__comboBox
    widget = property(fget=_getWidget)


    # Returns a tuple of min and max number of associated keys with this component
    def _getMinMaxAssociatedKeys(self):
        return self.__minMaxAssociatedKeys
    minMaxAssociatedKeys = property(fget=_getMinMaxAssociatedKeys)


    def _getKeys(self):
        return [self.__key]
    keys = property(fget=_getKeys)


    def addParameters(self, **params):
        print "addParameters", params


    def _value(self):
        if self.__valueType == "int":
            return int(self.__comboBox.currentText())
        elif (self.__valueType == "float") or (self.__valueType == "double"):
            return float(self.__comboBox.currentText())
        return str(self.__comboBox.currentText())
    value = property(fget=_value)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


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
        
        self.__comboBox.blockSignals(True)
        self.__comboBox.setCurrentIndex(index)
        self.__comboBox.blockSignals(False)

        # Needs to be called to update possible apply buttons
        self.onEditingFinished(value)


### slots ###
    def onEditingFinished(self, value):
        self.valueEditingFinished(self.__key, value)


    class Maker:
        def make(self, **params):
            return EditableComboBox(**params)

