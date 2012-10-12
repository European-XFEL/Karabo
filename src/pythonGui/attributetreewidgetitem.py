#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which inherits from a BaseTreeWidgetItem and represents
   an attribute item and its parameters.
"""

__all__ = ["AttributeTreeWidgetItem"]


import const

from basetreewidgetitem import BaseTreeWidgetItem
import choicecomponent
from displaycomponent import DisplayComponent
from manager import Manager

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class AttributeTreeWidgetItem(BaseTreeWidgetItem):


    def __init__(self, key, parent, parentItem=None):
        
        super(AttributeTreeWidgetItem, self).__init__(key, parent, parentItem)
        
        self.setData(0, Qt.SizeHintRole, QSize(200, 32))
        self.setIcon(0, QIcon(":folder"))
        
        self.displayComponent = DisplayComponent("Value Field", key=self.internalKey)
        self.treeWidget().setItemWidget(self, 1, self.displayComponent.widget)
        self.treeWidget().resizeColumnToContents(1)
        
        self.valueType = "undefined"
        self.accessType = None
        self.defaultValue = None
        self.expertLevel = 0
        self.isChoiceElement = False
        self.isListElement = False


    # TODO: complete implementation
    def copy(self, parentItem, keyName=str()):
        copyItem = AttributeTreeWidgetItem(self.internalKey, self.treeWidget(), parentItem)
        #copyItem.setIcon(0, self.icon(0))
        copyItem.setText(0, self.text(0))
        
        keys = str(self.internalKey).split(".")
        if len(keyName) < 1:
            copyItem.internalKey = parentItem.internalKey + "." + keys[len(keys)-1]
        else:
            copyItem.internalKey = keyName + "." + keys[len(keys)-1]
        
        if self.editableComponent is not None:
            copyItem.setEditableComponent(self.editableComponent.copy())
        
        copyItem.defaultValue = self.defaultValue
        copyItem.expertLevel = self.expertLevel
        
        copyItem.isChoiceElement = self.isChoiceElement
        copyItem.isListElement = self.isListElement
        copyItem.updateNeeded = self.updateNeeded

        # copying children as well
        for i in range(self.childCount()) :
            self.child(i).copy(copyItem, str())

        if parentItem.isChoiceElement == True :
            if parentItem.defaultValue == copyItem.text(0):
                copyItem.setHidden(False)
            else:
                copyItem.setHidden(True)
            parentItem.updateNeeded = True
            parentItem.editableComponent.addParameters(itemToBeAdded=copyItem)
            parentItem.onSetToDefault()
        else:
            copyItem.setHidden(False)

        copyItem.onSetToDefault()
        return copyItem


    def _setupContextMenu(self):
        # item specific menu
        # add actions from attributeWidget
        if self.editableComponent is None:
            return
        
        self.mItem = QMenu()
        text = "Reset to default"
        self.__acResetToDefault = QAction(QIcon(":revert"), text, None)
        self.__acResetToDefault.setStatusTip(text)
        self.__acResetToDefault.setToolTip(text)
        self.__acResetToDefault.setIconVisibleInMenu(True)
        self.__acResetToDefault.triggered.connect(self.onSetToDefault)
        
        self.mItem.addAction(self.__acResetToDefault)
        self.mItem.addSeparator()
        #self.mItem.addAction(self.editableComponent.acApply)
        #self.mItem.addAction(self.editableComponent.acReset)


### getter & setter functions ###
    def _setEnabled(self, enable):
        if self.editableComponent is not None:
            self.editableComponent.setEnabled(enable)
    enabled = property(fset=_setEnabled)


    def _setText(self, text):
        self.setText(0, text)
        self.treeWidget().resizeColumnToContents(0)
    displayText = property(fset=_setText)


    def setEditableComponent(self, component):
        self.editableComponent = component
        self._setupContextMenu()
        self.treeWidget().setItemWidget(self, 2, self.editableComponent.widget)
        self.treeWidget().resizeColumnToContents(2)


    def _accessType(self):
        return self.data(0, const.ACCESS_TYPE).toPyObject()
    def _setAccessType(self, accessType):
        self.setData(0, const.ACCESS_TYPE, accessType)
    accessType = property(fget=_accessType, fset=_setAccessType)


    def _valueType(self):
        return self.data(0, const.VALUE_TYPE).toPyObject()
    def _setValueType(self, valueType):
        self.setData(0, const.VALUE_TYPE, valueType)
    valueType = property(fget=_valueType, fset=_setValueType)


    def _defaultValue(self):
        return self.data(0, const.DEFAULT_VALUE).toPyObject()
    def _setDefaultValue(self, default):
        self.setData(0, const.DEFAULT_VALUE, default)
    defaultValue = property(fget=_defaultValue, fset=_setDefaultValue)


    def _expertLevel(self):
        level = self.data(0, const.EXPERT_LEVEL).toPyObject()
        return level
    def _setExpertLevel(self, expertLevel): # int value expected
        self.setData(0, const.EXPERT_LEVEL, expertLevel)
        if expertLevel > 0:
            self.setHidden(True)
    expertLevel = property(fget=_expertLevel, fset=_setExpertLevel)


    def _isListElement(self):
        return self.data(0, const.IS_LIST_ELEMENT).toPyObject()
    def _setIsListElement(self, isListElement):
        self.setData(0, const.IS_LIST_ELEMENT, isListElement)
    isListElement = property(fget=_isListElement, fset=_setIsListElement)


### public functions ###
    def setReadOnly(self, readOnly):
        self.enabled = not readOnly
        BaseTreeWidgetItem.setReadOnly(self, readOnly)


    def initRefresh(self, value):
        if self.editableComponent is not None:
            self.editableComponent.value = value
            self.editableComponent.busyState = False


    def changeApplyToBusy(self, isApplyBusy, changeTimer=True):
        if self.editableComponent is None:
            return
        self.editableComponent.changeApplyToBusy(isApplyBusy, changeTimer)


    def onSetToDefault(self):
        if (self.editableComponent is not None) and (self.defaultValue is not None):
            #self.editableComponent.value = self.defaultValue
            self.editableComponent.onValueChanged(self.internalKey, self.defaultValue)
            if type(self.editableComponent) is not choicecomponent.ChoiceComponent:
                self.editableComponent.onEditingFinished(self.internalKey, self.defaultValue)


    def setColorMandatory(self):
        f = self.font(0)
        f.setBold(True)
        self.setFont(0, f)

