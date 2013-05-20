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

from manager import Manager

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class BaseTreeWidgetItem(QTreeWidgetItem):
    
    def __init__(self, key, parent, parentItem=None):
        
        if parentItem:
            super(BaseTreeWidgetItem, self).__init__(parentItem)
        else:
            super(BaseTreeWidgetItem, self).__init__(parent)
        
        self.internalKey = key
        print ""
        print "-----", key
        print ""

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
        self.__editableComponent = component
        
        self.setupContextMenu()
        self.treeWidget().setItemWidget(self, 2, self.editableComponent.widget)
        self.treeWidget().resizeColumnToContents(2)
    editableComponent = property(fget=_editableComponent, fset=_setEditableComponent)


    def _internalKey(self):
        return self.data(0, const.INTERNAL_KEY).toPyObject()
    def _setInternalKey(self, key):
        self.setData(0, const.INTERNAL_KEY, key)
    internalKey = property(fget=_internalKey, fset=_setInternalKey)


    def _updateNeeded(self):
        return self.data(0, const.UPDATE_NEEDED).toPyObject()
    def _setUpdateNeeded(self, updateNeeded):
        self.setData(0, const.UPDATE_NEEDED, updateNeeded)
    updateNeeded = property(fget=_updateNeeded, fset=_setUpdateNeeded)


    def _descriptionIndex(self):
        index = self.data(0, const.DESCRIPTION_INDEX).toPyObject()
        return index
    def _setDescriptionIndex(self, index):
        self.setData(0, const.DESCRIPTION_INDEX, index)
    descriptionIndex = property(fget=_descriptionIndex, fset=_setDescriptionIndex)


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

