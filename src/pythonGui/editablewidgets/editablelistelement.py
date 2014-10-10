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


from widget import DisplayWidget, EditableWidget
from karabo.hash import Hash
from stringlistedit import StringListEdit

from PyQt4.QtCore import pyqtSignal
from PyQt4.QtGui import QDialog, QPushButton


class EditableListElement(EditableWidget, DisplayWidget):
    category = "SelectionList"
    alias = "List Element Field"
    signalValueChanged = pyqtSignal(str, object) # key, value
    
    def __init__(self, box, parent):
        super(EditableListElement, self).__init__(box)
        
        self.__pushButton = QPushButton("Edit list", parent)
        self.__pushButton.setStyleSheet("QPushButton { text-align: left; }")
        
        self.__choiceItemList = [] # list with hidden possible items of listelement
        self.__choiceStringList = [] # list with names of possible listelements
        self.__selectedItemList = [] # list with already added items
        self.__selectedStringList = [] # list with selected listelements
        
        self.__isInit = False
        
        self.__pushButton.clicked.connect(self.onClicked)


    @property
    def widget(self):
        return self.__pushButton


    @property
    def value(self):
        return self.__selectedStringList # TODO: Hash(value) compare with EditableChoiceElement


    def copyListItem(self, values, arrayIndex=0):
        if isinstance(values, list):
            for v in values:
                self._addListItem(v, arrayIndex)
        else:
            self._addListItem(values, arrayIndex)


    def _addListItem(self, value, arrayIndex):
            index = self.__choiceStringList.index(value)
            if index < 0:
                return

            choiceItem = self.__choiceItemList[index]
            parentItem = choiceItem.parent()

            # Change full key name...
            newInternalKeyName = parentItem.box
            newInternalKeyName.append("[{}]".format(arrayIndex)) #[next]

            copyItem = choiceItem.copy(parentItem, newInternalKeyName)
            parentItem.setExpanded(True)
            self.__selectedItemList.append(copyItem)

            # Notify Manager about changes
            self.signalValueChanged.emit(copyItem.box, Hash())


    def valueChanged(self, box, value, timestamp=None):
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
