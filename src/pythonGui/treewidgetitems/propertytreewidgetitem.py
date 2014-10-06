#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 11, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which inherits from a BaseTreeWidgetItem and represents
   an attribute item and its parameters.
"""

__all__ = ["PropertyTreeWidgetItem"]


from .basetreewidgetitem import BaseTreeWidgetItem
from components import DisplayComponent
import icons

from PyQt4.QtCore import Qt, QSize
from PyQt4.QtGui import QAction, QCursor, QMenu

class PropertyTreeWidgetItem(BaseTreeWidgetItem):


    def __init__(self, box, parent, parentItem=None):
        super(PropertyTreeWidgetItem, self).__init__(box, parent, parentItem)
        
        self.setData(0, Qt.SizeHintRole, QSize(200, 32))
        self.setIcon(0, icons.folder)

        self.displayComponent = DisplayComponent("Value Field", self.box, self.treeWidget())
        self.treeWidget().setItemWidget(self, 1, self.displayComponent.widget)
        self.treeWidget().resizeColumnToContents(1)

        box.signalUpdateComponent.connect(self.onDisplayValueChanged)


    def setupContextMenu(self):
        # item specific menu
        # add actions from attributeWidget
        if self.editableComponent is None:
            return
        
        self.mItem = QMenu()
        text = "Reset to default"
        self.__acResetToDefault = QAction(icons.revert, text, None)
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


### public functions ###
    def setReadOnly(self, readOnly):
        if self.editableComponent is not None:
            self.editableComponent.setEnabled(not readOnly)
        BaseTreeWidgetItem.setReadOnly(self, readOnly)


    def onSetToDefault(self):
        self.box.descriptor.setDefault(self.box)


    def onDisplayValueChanged(self, box, value):
        # Update tooltip dialog, if visible
        if self.popupWidget and self.popupWidget.isVisible():
            self.updateToolTipDialog()

