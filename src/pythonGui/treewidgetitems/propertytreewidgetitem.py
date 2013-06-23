#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 11, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which inherits from a BaseTreeWidgetItem and represents
   an attribute item and its parameters.
"""

__all__ = ["PropertyTreeWidgetItem"]

import const

from basetreewidgetitem import BaseTreeWidgetItem
#import choicecomponent
from displaycomponent import DisplayComponent
#from manager import Manager

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class PropertyTreeWidgetItem(BaseTreeWidgetItem):


    def __init__(self, path, parent, parentItem=None):
        super(PropertyTreeWidgetItem, self).__init__(path, parent, parentItem)
        
        self.setData(0, Qt.SizeHintRole, QSize(200, 32))
        self.setIcon(0, QIcon(":folder"))

        self.displayComponent = DisplayComponent("Value Field", path=path)
        self.treeWidget().setItemWidget(self, 1, self.displayComponent.widget)
        self.treeWidget().resizeColumnToContents(1)


    def setupContextMenu(self):
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


### getter and setter functions ###
    def _setText(self, text):
        self.setText(0, text)
        self.treeWidget().resizeColumnToContents(0)
    displayText = property(fset=_setText)


    def _isChoiceElement(self):
        return self.data(0, const.IS_CHOICE_ELEMENT).toPyObject()
    def _setIsChoiceElement(self, isChoiceElemet):
        self.setData(0, const.IS_CHOICE_ELEMENT, isChoiceElemet)
    isChoiceElement = property(fget=_isChoiceElement, fset=_setIsChoiceElement)


    def _setEnabled(self, enable):
        if self.editableComponent is not None:
            self.editableComponent.setEnabled(enable)
    enabled = property(fset=_setEnabled)


### slots ###
    def onSetToDefault(self):
        return # needs to be correctly implemented
        #if (self.editableComponent is not None) and (self.defaultValue is not None):
            #self.editableComponent.value = self.defaultValue
        #    self.editableComponent.onValueChanged(self.internalKey, self.defaultValue)
        #    if type(self.editableComponent) is not choicecomponent.ChoiceComponent:
        #        self.editableComponent.onEditingFinished(self.internalKey, self.defaultValue)

