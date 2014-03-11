#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on April 27, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which inherits from a QTreeWidgetItem.
   
   Inherited by: PropertyTreeWidgetItem, ImageTreeWidgetItem, CommandTreeWidgetItem,
                 AttributeTreeWidgetItem
"""

__all__ = ["BaseTreeWidgetItem"]


import const
import globals

from manager import Manager

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QCursor, QTreeWidgetItem


class BaseTreeWidgetItem(QTreeWidgetItem):
    
    def __init__(self, path, parent, parentItem=None):
        
        if parentItem:
            super(BaseTreeWidgetItem, self).__init__(parentItem)
        else:
            super(BaseTreeWidgetItem, self).__init__(parent)
        
        self.internalKey = path
        
        # The components can be defined in Subclasses
        self.__displayComponent = None
        self.__editableComponent = None
        
        self.mItem = None


    def setupContextMenu(self):
        raise NotImplementedError, "BaseTreeWidgetItem.setupContextMenu"


### getter and setter functions ###
    def _getEnabled(self):
        raise NotImplementedError, "BaseTreeWidgetItem._getEnabled"
    def _setEnabled(self, enabled):
        raise NotImplementedError, "BaseTreeWidgetItem._setEnabled"
    enabled = property(fget=_getEnabled, fset=_setEnabled)


    def _getClassAlias(self):
        return self.data(0, const.CLASS_ALIAS)
    def _setClassAlias(self, alias):
        self.setData(0, const.CLASS_ALIAS, alias)
    classAlias = property(fget=_getClassAlias, fset=_setClassAlias)


    # Returns the display component of the item
    def _displayComponent(self):
        return self.__displayComponent
    def _setDisplayComponent(self, component):
        self.__displayComponent = component
    displayComponent = property(fget=_displayComponent, fset=_setDisplayComponent)


    # Returns the editable component of the item
    def _editableComponent(self):
        return self.__editableComponent
    def _setEditableComponent(self, component):
        if not component:
            return
        
        self.__editableComponent = component
        
        self.setupContextMenu()
        self.treeWidget().setItemWidget(self, 2, self.editableComponent.widget)
        self.treeWidget().resizeColumnToContents(2)
    editableComponent = property(fget=_editableComponent, fset=_setEditableComponent)


    def _internalKey(self):
        return self.data(0, const.INTERNAL_KEY)
    def _setInternalKey(self, key):
        self.setData(0, const.INTERNAL_KEY, key)
    internalKey = property(fget=_internalKey, fset=_setInternalKey)


    def _updateNeeded(self):
        return self.data(0, const.UPDATE_NEEDED)
    def _setUpdateNeeded(self, updateNeeded):
        self.setData(0, const.UPDATE_NEEDED, updateNeeded)
    updateNeeded = property(fget=_updateNeeded, fset=_setUpdateNeeded)


    def _allowedStates(self):
        return self.data(0, const.ALLOWED_STATE)
    def _setAllowedStates(self, state):
        self.setData(0, const.ALLOWED_STATE, state)
    allowedStates = property(fget=_allowedStates, fset=_setAllowedStates)


    def _isChoiceElement(self):
        return self.data(0, const.IS_CHOICE_ELEMENT)
    def _setIsChoiceElement(self, isChoiceElemet):
        self.setData(0, const.IS_CHOICE_ELEMENT, isChoiceElemet)
    isChoiceElement = property(fget=_isChoiceElement, fset=_setIsChoiceElement)


    def _alias(self):
        return self.data(0, const.ALIAS)
    def _setAlias(self, index):
        self.setData(0, const.ALIAS, index)
    alias = property(fget=_alias, fset=_setAlias)


    def _tags(self):
        return self.data(0, const.TAGS)
    def _setTags(self, index):
        self.setData(0, const.TAGS, index)
    tags = property(fget=_tags, fset=_setTags)


    def _description(self):
        return self.data(0, const.DESCRIPTION)
    def _setDescription(self, index):
        self.setData(0, const.DESCRIPTION, index)
    description = property(fget=_description, fset=_setDescription)


    def _valueType(self):
        return self.data(0, const.VALUE_TYPE)
    def _setValueType(self, valueType):
        self.setData(0, const.VALUE_TYPE, valueType)
    valueType = property(fget=_valueType, fset=_setValueType)


    def _timestamp(self):
        return self.data(0, const.TIMESTAMP)
    def _setTimestamp(self, index):
        self.setData(0, const.TIMESTAMP, index)
    timestamp = property(fget=_timestamp, fset=_setTimestamp)


    def _requiredAccessLevel(self):
        return self.data(0, const.REQUIRED_ACCESS_LEVEL)
    def _setRequiredAccessLevel(self, requiredAccessLevel): # int value expected
        self.setData(0, const.REQUIRED_ACCESS_LEVEL, requiredAccessLevel)
        if requiredAccessLevel > globals.GLOBAL_ACCESS_LEVEL:
            self.setHidden(True)
    requiredAccessLevel = property(fget=_requiredAccessLevel, fset=_setRequiredAccessLevel)


    def _metricPrefixSymobl(self):
        return self.data(0, const.METRIC_PREFIX_SYMBOL)
    def _setMetricPrefixSymbol(self, metricPrefixSymbol):
        self.setData(0, const.METRIC_PREFIX_SYMBOL, metricPrefixSymbol)
    metricPrefixSymbol = property(fget=_metricPrefixSymobl, fset=_setMetricPrefixSymbol)


    def _unitSymbol(self):
        return self.data(0, const.UNIT_SYMBOL)
    def _setUnitSymbol(self, unitSymbol):
        self.setData(0, const.UNIT_SYMBOL, unitSymbol)
    unitSymbol = property(fget=_unitSymbol, fset=_setUnitSymbol)


    def _enumeration(self):
        return self.data(0, const.ENUMERATION)
    def _setEnumeration(self, enumeration):
        self.setData(0, const.ENUMERATION, enumeration)
    enumeration = property(fget=_enumeration, fset=_setEnumeration)


### public functions ###
    def unregisterEditableComponent(self):
        if self.editableComponent:
            Manager().unregisterEditableComponent(self.internalKey, self.editableComponent)


    def unregisterDisplayComponent(self):
        if self.displayComponent:
            Manager().unregisterDisplayComponent(self.internalKey, self.displayComponent)


    def updateState(self, state):
        if self.allowedStates is None or len(self.allowedStates) < 1:
            return
        
        if state in self.allowedStates:
            self.enabled = True
        else:
            self.enabled = False


    def showContextMenu(self):
        if self.mItem is None:
            return

        self.mItem.exec_(QCursor.pos())


    def setErrorState(self, isError):
        if self.displayComponent:
            self.displayComponent.setErrorState(isError)


    def setReadOnly(self, readOnly):
        if readOnly is True:
            self.setFlags(self.flags() & ~Qt.ItemIsEnabled)
        else:
            self.setFlags(self.flags() | Qt.ItemIsEnabled)


    def setToolTipDialogVisible(self, show):
        raise NotImplementedError, "BaseTreeWidget.setToolTipDialogVisible"

