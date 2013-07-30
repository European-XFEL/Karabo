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
import choicecomponent
from displaycomponent import DisplayComponent
from popupwidget import PopupWidget

from PyQt4.QtCore import *
from PyQt4.QtGui import *

class PropertyTreeWidgetItem(BaseTreeWidgetItem):


    def __init__(self, path, parent, parentItem=None):
        super(PropertyTreeWidgetItem, self).__init__(path, parent, parentItem)
        
        # Popup widget for tooltip info
        self.__popupWidget = None
        
        self.setData(0, Qt.SizeHintRole, QSize(200, 32))
        self.setIcon(0, QIcon(":folder"))

        self.displayComponent = DisplayComponent("Value Field", key=self.internalKey)
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


    def _defaultValue(self):
        return self.data(0, const.DEFAULT_VALUE).toPyObject()
    def _setDefaultValue(self, default):
        self.setData(0, const.DEFAULT_VALUE, default)
    defaultValue = property(fget=_defaultValue, fset=_setDefaultValue)


    def _setEnabled(self, enable):
        if self.editableComponent is not None:
            self.editableComponent.setEnabled(enable)
    enabled = property(fset=_setEnabled)


    def setToolTipDialogVisible(self, show):
        if not self.__popupWidget:
            self.__popupWidget = PopupWidget()
        
        if show:
            info = OrderedDict()
            info["Property"] = self.text(0)
            paramKey = str(self.internalKey).split(".configuration.")
            info["Parameter key"] = paramKey[1]
            if self.valueType:
                info["Value type"] = self.valueType
            if self.description:
                info["Description"] = self.description
            if self.defaultValue:
                info["Default Value"] = self.defaultValue
            if self.alias:
                info["Alias"] = self.alias
            if self.tags:
                tagString = str()
                nbTags = len(self.tags)
                for i in xrange(nbTags):
                    tagString += self.tags[i]
                    if i < (nbTags-1):
                        tagString += ", "
                info["Tags"] = tagString
            if self.timestamp:
                info["Timestamp"] = self.timestamp
            
            self.__popupWidget.setInfo(info)
            
            pos = QCursor.pos()
            pos.setX(pos.x() + 10)
            self.__popupWidget.move(pos)
            self.__popupWidget.show()
            
            #pos = QCursor.pos()
            #width = self.__popupWidget.width()
            #height = self.__popupWidget.height()
            
            #animation = QPropertyAnimation(self.__popupWidget, "geometry")
            #animation.setDuration(500)
            #animation.setDirection(QAbstractAnimation.Forward)
            #animation.setStartValue(QRect(pos.x(), pos.y(), width, height))
            #animation.setEndValue(QRect(pos.x(), pos.y(), width, height))
            #animation.start()
        else:
            self.__popupWidget.hide()


### slots ###
    def onSetToDefault(self):
        if self.editableComponent:
            #self.editableComponent.value = self.defaultValue
            self.editableComponent.onValueChanged(self.internalKey, self.defaultValue)
            if type(self.editableComponent) is not choicecomponent.ChoiceComponent:
                self.editableComponent.onEditingFinished(self.internalKey, self.defaultValue)

