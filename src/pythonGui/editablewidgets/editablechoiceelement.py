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


from widget import EditableWidget
from karabo.hash import Hash
from schema import Schema

from PyQt4.QtCore import QEvent
from PyQt4.QtGui import QComboBox


class EditableChoiceElement(EditableWidget):
    category = "Choice"
    alias = "Choice Element"

    def __init__(self, box, parent):
        self.__comboBox = QComboBox(parent)
        self.__comboBox.setFrame(False)

        super(EditableChoiceElement, self).__init__(box)

        self.__comboBox.installEventFilter(self)
        self.__comboBox.currentIndexChanged.connect(self.onEditingFinished)
        
        self.childItemList = []
        
    def eventFilter(self, object, event):
        # Block wheel event on QComboBox
        if event.type() == QEvent.Wheel and object == self.__comboBox:
            return True
        return False


    @property
    def widget(self):
        return self.__comboBox


    def addParameters(self, itemToBeAdded=None):
        if itemToBeAdded is None: return

        self.__comboBox.blockSignals(True)
        self.__comboBox.addItem(itemToBeAdded.text(0))
        self.childItemList.append(itemToBeAdded)
        self.__comboBox.blockSignals(False)


    @property
    def value(self):
        return self.__comboBox.currentText()


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
                    if parentItem.defaultValue is None or \
                       parentItem.defaultValue == childItem.text(0):
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


    def valueChanged(self, box, value, timestamp=None, forceRefresh=False):
        if not isinstance(value, basestring):
            value = box.current

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
        EditableWidget.onEditingFinished(self, self.value)


    def copy(self, item):
        copyWidget = EditableChoiceElement(item=item)

        if self.item.isChoiceElement == False :
            for i in range(self.__comboBox.count()) :
                copyWidget.comboBox.addItem(self.__comboBox.itemText(i))

        return copyWidget
