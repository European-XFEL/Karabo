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

__all__ = ["EditableListElement"]


from editablewidget import EditableWidget
from libpyexfelportable import *
from manager import Manager
from stringlistedit import StringListEdit

from PyQt4.QtCore import *
from PyQt4.QtGui import *


def getCategoryAliasClassName():
    return ["SelectionList","List Element Field","EditableListElement"]


class EditableListElement(EditableWidget):
    # signals
    signalValueChanged = pyqtSignal(str, object) # key, value
    
    def __init__(self, **params):
        super(EditableListElement, self).__init__(**params)
        
        self.__pushButton = QPushButton("Edit list")
        self.__pushButton.setStyleSheet("QPushButton { text-align: left; }")
        
        self.__choiceItemList = [] # list with hidden possible items of listelement
        self.__choiceStringList = QStringList() # list with names of possible listelements
        self.__selectedItemList = [] # list with already added items
        self.__selectedStringList = [] # list with selected listelements
        
        self.__isInit = False
        
        isDeviceInstance = params.get(QString('isDevIns'))
        if isDeviceInstance is None:
            isDeviceInstance = params.get('isDevIns')
        if isDeviceInstance:
            self.signalValueChanged.connect(Manager().onDeviceInstanceValueChanged)
        else:
            self.signalValueChanged.connect(Manager().onDeviceClassValueChanged)
        
        self.__pushButton.clicked.connect(self.onClicked)
        
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


    def _getCategory(self):
        category, alias, className = getCategoryAliasClassName()
        return category
    category = property(fget=_getCategory)


    # Returns the actual widget which is part of the composition
    def _getWidget(self):
        return self.__pushButton
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
            item.needsUpdate = False
            self.__choiceItemList.append(item)
            self.__choiceStringList.append(item.text(0))


    def _value(self):
        return self.__selectedStringList # TODO: Hash(value) compare with EditableChoiceElement
    value = property(fget=_value)


    def addKeyValue(self, key, value):
        self.__key = key # TODO: Overwritten - unregistering in Manager...
        self.valueChanged(key, value)


    def removeKey(self, key):
        self.__key = None


    def copyListItem(self, values, arrayIndex=0):
        if isinstance(values, list):
            for v in values:
                self._addListItem(v, arrayIndex)
        else:
            self._addListItem(values, arrayIndex)


    def _addListItem(self, value, arrayIndex):
            index = self.__choiceStringList.indexOf(value)
            if index < 0:
                return

            choiceItem = self.__choiceItemList[index]
            parentItem = choiceItem.parent()

            # Change full key name...
            newInternalKeyName = parentItem.internalKey
            newInternalKeyName.append(QString("[%1]").arg(arrayIndex)) #[next]

            copyItem = choiceItem.copy(parentItem, newInternalKeyName)
            parentItem.setExpanded(True)
            self.__selectedItemList.append(copyItem)

            # Notify Manager about changes
            self.signalValueChanged.emit(copyItem.internalKey, Hash())


    def valueChanged(self, key, value, timestamp=None):
        if value is None:
            return
        
        self.__selectedStringList = value
        
        if self.__isInit is False:
            # Copy item
            self.copyListItem(value)
            self.__isInit = True


### slots ###
    # slot called when changes need to be sent to Manager
    def onClicked(self):
        listEdit = StringListEdit(True, self.value)
        listEdit.setTexts("Add", "&Name", "Edit")
        listEdit.setAllowedChoices(self.__choiceStringList, self.__choiceItemList)

        if listEdit.exec_() == QDialog.Accepted:
            # Remove old items
            for i in range(len(self.__selectedItemList)):
                item = self.__selectedItemList[i]
                parentItem = item.parent()
                if parentItem is not None:
                    parentItem.removeChild(item)
            self.__selectedStringList = []

            for i in range(listEdit.getListCount()):

                value = listEdit.getListElementAt(i)
                self.__selectedStringList.append(value)

                # TODO: don't copy already existing item..
                self.copyListItem(listEdit.getListElementAt(i), i)


    class Maker:
        def make(self, **params):
            return EditableListElement(**params)

