#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 28, 2012
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

__all__ = ["EditableChoiceElement"]


from editablewidget import EditableWidget
from libkarabo import *

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["Choice","Choice Element","EditableChoiceElement"]


class EditableChoiceElement(EditableWidget):
    
    def __init__(self, **params):
        super(EditableChoiceElement, self).__init__(**params)
        
        self.__comboBox = QComboBox()
        self.__comboBox.setFrame(False)
        
        self.__comboBox.installEventFilter(self)
        self.__comboBox.currentIndexChanged.connect(self.onEditingFinished)
        
        self.childItemList = []
        
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
        item = params.get(QString('itemToBeAdded'))
        if item is None:
            item = params.get('itemToBeAdded')
        if item is not None:
            self.__comboBox.blockSignals(True)
            self.__comboBox.addItem(item.text(0))
            self.childItemList.append(item)
            self.__comboBox.blockSignals(False)


    def _value(self):
        return Hash(str(self.__comboBox.currentText()))
    value = property(fget=_value)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def _updateChoiceItems(self, index):
        for i in range(len(self.childItemList)):
            item = self.childItemList[i]
            item.setHidden(True)
            item.updateNeeded = False
        
        selectedItem = self.childItemList[index]
        selectedItem.setHidden(False)
        selectedItem.updateNeeded = True
        self._r_updateChildItems(selectedItem)


    def _r_updateChildItems(self, parentItem):
        for i in range(parentItem.childCount()):
            childItem = parentItem.child(i)
            if parentItem.updateNeeded == True:
                if parentItem.isChoiceElement == True:
                    if parentItem.defaultValue == childItem.text(0) :
                        childItem.updateNeeded = True
                        childItem.setHidden(False)
                    else:
                        childItem.updateNeeded = False
                else:
                    childItem.updateNeeded = True
                    childItem.setHidden(False)
            else:
                childItem.updateNeeded = False
            self._r_updateChildItems(childItem)


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return
        
        if isinstance(value, Hash):
            keys = value.keys()
            if keys > 0:
                value = keys[0]
        
        index = self.__comboBox.findText(value)
        if index < 0:
            return

        self.__comboBox.blockSignals(True)
        self.__comboBox.setCurrentIndex(index)
        self.__comboBox.blockSignals(False)
        self._updateChoiceItems(index)


### slots ###
    def onEditingFinished(self, index):
        if index > -1 and index < len(self.childItemList):
            self._updateChoiceItems(index)
        self.valueEditingFinished(self.__key, self.value)


    def copy(self, item):
        copyWidget = EditableChoiceElement(item=item)

        if self.item.isChoiceElement == False :
            for i in range(self.__comboBox.count()) :
                copyWidget.comboBox.addItem(self.__comboBox.itemText(i))

        return copyWidget


    class Maker:
        def make(self, **params):
            return EditableChoiceElement(**params)

