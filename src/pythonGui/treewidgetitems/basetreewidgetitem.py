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


from collections import OrderedDict

from .popupwidget import PopupWidget

from karabo.hashtypes import Type

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QCursor, QTreeWidgetItem


class BaseTreeWidgetItem(QTreeWidgetItem):
    
    def __init__(self, box, parent, parentItem=None):
        
        if parentItem:
            super(BaseTreeWidgetItem, self).__init__(parentItem)
        else:
            super(BaseTreeWidgetItem, self).__init__(parent)
        
        self.box = box
        
        # The components can be defined in Subclasses
        self.__displayComponent = None
        self.__editableComponent = None
        
        self.mItem = None
        
        # Popup widget for tooltip info
        self.popupWidget = None


    def setupContextMenu(self):
        raise NotImplementedError("BaseTreeWidgetItem.setupContextMenu")


    # Returns the display component of the item
    def _displayComponent(self):
        return self.__displayComponent
    def _setDisplayComponent(self, component):
        self.__displayComponent = component
    displayComponent = property(fget=_displayComponent, fset=_setDisplayComponent)
    isChoiceElement = False
    isListElement = False
    description = None

    # Returns the editable component of the item
    def _editableComponent(self):
        return self.__editableComponent
    def _setEditableComponent(self, component):
        if component is None:
            return
        
        self.__editableComponent = component
        
        self.setupContextMenu()
        self.treeWidget().setItemWidget(self, 2, self.editableComponent.widget)
        self.treeWidget().resizeColumnToContents(2)
    editableComponent = property(fget=_editableComponent, fset=_setEditableComponent)


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
        if not self.popupWidget:
            self.popupWidget = PopupWidget(self.treeWidget())

        if show:
            info = self.updateToolTipDialog()
            
            pos = QCursor.pos()
            pos.setX(pos.x() + 10)
            pos.setY(pos.y() + 10)
            self.popupWidget.move(pos)
            self.popupWidget.show()
        else:
            self.popupWidget.hide()


    def updateToolTipDialog(self):
            info = OrderedDict()
            
            if len(self.text(0)) > 0:
                info["Property"] = self.text(0)
            d = self.box.descriptor
            if d.description is not None:
                info["Description"] = d.description

            info["Key"] = '.'.join(self.box.path)
            if isinstance(d, Type):
                info["Value Type"] = d.hashname()
            if d.defaultValue is not None:
                info["Default Value"] = d.defaultValue
            if d.alias is not None:
                info["Alias"] = d.alias
            if d.tags is not None:
                info["Tags"] = ", ".join(d.tags)
            if self.box.timestamp is not None:
                info["Timestamp"] = self.box.timestamp.toLocal()
            if d.displayType and d.displayType.startswith('bin|'):
                info["Bits"] = d.displayType[4:]
            if self.box.configuration.type == "device":
                info["Value on device"] = self.box.value

            self.popupWidget.setInfo(info)

