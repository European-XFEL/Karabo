#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 11, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which inherits from a BaseTreeWidgetItem and represents
   an attribute item and its parameters.
"""

__all__ = ["PropertyTreeWidgetItem"]

from collections import OrderedDict
from basetreewidgetitem import BaseTreeWidgetItem
from components import ChoiceComponent, DisplayComponent
import icons
from popupwidget import PopupWidget

from PyQt4.QtCore import Qt, QSize
from PyQt4.QtGui import QAction, QCursor, QMenu

class PropertyTreeWidgetItem(BaseTreeWidgetItem):


    def __init__(self, box, parent, parentItem=None):
        super(PropertyTreeWidgetItem, self).__init__(box, parent, parentItem)
        
        # Popup widget for tooltip info
        self.__popupWidget = None
        self.__currentValueOnDevice = None
        
        self.setData(0, Qt.SizeHintRole, QSize(200, 32))
        self.setIcon(0, icons.folder)

        self.displayComponent = DisplayComponent(
            "Value Field", self.internalKey, self.treeWidget())
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


    def setToolTipDialogVisible(self, show):
        if not self.__popupWidget:
            self.__popupWidget = PopupWidget(self.treeWidget())

        if show:
            info = self._updateToolTipDialog()
            
            pos = QCursor.pos()
            pos.setX(pos.x() + 10)
            pos.setY(pos.y() + 10)
            self.__popupWidget.move(pos)
            self.__popupWidget.show()
        else:
            self.__popupWidget.hide()


    def _updateToolTipDialog(self):
            info = OrderedDict()
            info["Property"] = self.text(0)
            if self.description is not None:
                info["Description"] = self.description

            info["Key"] = '.'.join(self.internalKey.path)
            d = self.internalKey.descriptor
            info["Value Type"] = d.hashname()
            if d.defaultValue is not None:
                info["Default Value"] = d.defaultValue
            if d.alias is not None:
                info["Alias"] = d.alias
            if d.tags is not None:
                info["Tags"] = ", ".join(d.tags)
            if self.internalKey.timestamp is not None:
                info["Timestamp"] = self.internalKey.timestamp.toLocal()
            if d.displayType and d.displayType.startswith('bin|'):
                info["Bits"] = d.displayType[4:]
            if self.__currentValueOnDevice is not None:
                info["Value on device"] = self.__currentValueOnDevice

            self.__popupWidget.setInfo(info)


    def onSetToDefault(self):
        self.internalKey.descriptor.setDefault(self.internalKey)


    def onDisplayValueChanged(self, key, value):
        self.__currentValueOnDevice = value
        # Update tooltip dialog, if visible
        if self.__popupWidget and self.__popupWidget.isVisible():
            self._updateToolTipDialog()

