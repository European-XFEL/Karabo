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

from collections import OrderedDict
from basetreewidgetitem import BaseTreeWidgetItem
from choicecomponent import ChoiceComponent
from displaycomponent import DisplayComponent
from popupwidget import PopupWidget

from PyQt4.QtCore import Qt, QSize
from PyQt4.QtGui import QAction, QCursor, QIcon, QMenu

class PropertyTreeWidgetItem(BaseTreeWidgetItem):


    def __init__(self, path, parent, parentItem=None):
        super(PropertyTreeWidgetItem, self).__init__(path, parent, parentItem)
        
        # Popup widget for tooltip info
        self.__popupWidget = None
        self.__currentValueOnDevice = None
        
        self.setData(0, Qt.SizeHintRole, QSize(200, 32))
        self.setIcon(0, QIcon(":folder"))

        self.displayComponent = DisplayComponent("Value Field", key=self.internalKey)
        self.treeWidget().setItemWidget(self, 1, self.displayComponent.widget)
        self.treeWidget().resizeColumnToContents(1)
        # Connect to DisplayComponent to get current value on device for tooltip update
        self.displayComponent.signalValueChanged.connect(self.onDisplayValueChanged)


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


    def _defaultValue(self):
        return self.data(0, const.DEFAULT_VALUE)
    def _setDefaultValue(self, default):
        self.setData(0, const.DEFAULT_VALUE, default)
    defaultValue = property(fget=_defaultValue, fset=_setDefaultValue)


    def _setEnabled(self, enable):
        if self.editableComponent is not None:
            self.editableComponent.setEnabled(enable)
    enabled = property(fset=_setEnabled)


### public functions ###
    def setReadOnly(self, readOnly):
        self._setEnabled(not readOnly)
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
            if self.__currentValueOnDevice is not None:
                # Key consists of deviceId, property key
                _, property = str(self.internalKey).split(".", 1)
            else:
                # Key consists of serverId, classId, property key
                _, _, property = str(self.internalKey).split(".", 2)
            info["Key"] = property
            if self.valueType is not None:
                info["Value Type"] = self.valueType
            if self.defaultValue is not None:
                info["Default Value"] = self.defaultValue
            if self.alias is not None:
                info["Alias"] = self.alias
            if self.tags is not None:
                tagString = str()
                nbTags = len(self.tags)
                for i in xrange(nbTags):
                    tagString += self.tags[i]
                    if i < (nbTags-1):
                        tagString += ", "
                info["Tags"] = tagString
            if self.timestamp is not None:
                info["Timestamp"] = self.timestamp
            if self.__currentValueOnDevice is not None:
                info["Value on device"] = self.__currentValueOnDevice

            self.__popupWidget.setInfo(info)


### slots ###
    def onSetToDefault(self):
        if self.editableComponent:
            #self.editableComponent.value = self.defaultValue
            self.editableComponent.onValueChanged(self.internalKey, self.defaultValue)
            if not isinstance(self.editableComponent, ChoiceComponent):
                self.editableComponent.onEditingFinished(self.internalKey, self.defaultValue)


    def onDisplayValueChanged(self, key, value):
        self.__currentValueOnDevice = value
        # Update tooltip dialog, if visible
        if self.__popupWidget and self.__popupWidget.isVisible():
            self._updateToolTipDialog()

