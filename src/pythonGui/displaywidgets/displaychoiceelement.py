#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on August 6, 2012
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

__all__ = ["DisplayChoiceElement"]


from widget import DisplayWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class DisplayChoiceElement(DisplayWidget):
    category = "Choice"
    alias = "Choice Element"
    
    def __init__(self, **params):
        super(DisplayChoiceElement, self).__init__(**params)

        self.__comboBox = QComboBox()
        self.__comboBox.setFrame(False)
        self.__comboBox.currentIndexChanged.connect(self.onEditingFinished)
        
        self.childItemList = []

    @property
    def widget(self):
        return self.__comboBox


    def addParameters(self, item=None, **params):
        if item is not None:
            self.__comboBox.blockSignals(True)
            self.__comboBox.addItem(item.text(0))
            self.childItemList.append(item)
            self.__comboBox.blockSignals(False)


    @property
    def value(self):
        return self.__comboBox.currentText()


    def _r_updateChildItems(self, parentItem):
        
        for i in range(parentItem.childCount()):

            childItem = parentItem.child(i)
            if parentItem.updateNeeded == True :
                if parentItem.isChoiceElement == True :
                    if parentItem.defaultValue == childItem.text(0) :
                        childItem.updateNeeded = True
                        childItem.setHidden(False)
                    else :
                        childItem.updateNeeded = False
                else:
                    childItem.updateNeeded = True
                    childItem.setHidden(False)
            else :
                childItem.updateNeeded = False
            self._r_updateChildItems(childItem)


    def valueChanged(self, key, value, timestamp=None):
        if not isinstance(value, str):
            return
        
        index = self.__comboBox.findText(value)
        if index < 0 :
            return

        #self.__comboBox.blockSignals(True)
        self.__comboBox.setCurrentIndex(index)
        #self.__comboBox.blockSignals(False)


### slots ###
    def onEditingFinished(self, index):
        #if self._applyVisible() is True:
            # device instance
        #    self.applyEnabled = True
        #else:
            # device class
        #    self.signalDeviceClassValueChanged.emit(self.item.internalKey, self.value)
        
        if index > -1 and index < len(self.childItemList) :
            for i in range(len(self.childItemList)) :
                item = self.childItemList[i]
                item.setHidden(True)
                item.updateNeeded = False

            selectedItem = self.childItemList[index]
            selectedItem.setHidden(False)
            selectedItem.updateNeeded = True
            self._r_updateChildItems(selectedItem)
