#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the treewidget of the configuration
   panel containing the parameters of a device class/instance.
"""

__all__ = ["ParameterTreeWidget"]

from components import EditableApplyLaterComponent
import globals
from network import Network
from treewidgetitems.propertytreewidgetitem import PropertyTreeWidgetItem

from PyQt4.QtCore import pyqtSignal, QByteArray, QMimeData, QRect, Qt
from PyQt4.QtGui import QAbstractItemView, QCursor, QMenu, QTreeWidget


class ParameterTreeWidget(QTreeWidget):
    signalApplyChanged = pyqtSignal(object, bool, bool) # box, enable, hasConflicts

    def __init__(self, conf=None):
        super(ParameterTreeWidget, self).__init__()
        
        self.conf = conf
        # Store previous selected item for tooltip handling
        self.prevItem = None

        self.setWordWrap(True)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.setSelectionMode(QAbstractItemView.ExtendedSelection)
        #self.setSortingEnabled(True)
        #self.sortByColumn(0, Qt.AscendingOrder)
        
        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.mContext = QMenu(self) # Actions from configurationPanel are added via addContextAction
        self.customContextMenuRequested.connect(self.onCustomContextMenuRequested)

        self.model().setSupportedDragActions(Qt.CopyAction)
        self.setDragEnabled(True)


### protected ###
    def mousePressEvent(self, event):
        item = self.itemAt(event.pos())
        
        # Make sure the event was on a valid item
        if not item:
           return

        # Get the tree widget's x position
        treeX = self.header().sectionViewportPosition(0)

        # Get the x coordinate of the root item. It is required in order to calculate
        # the identation of the item
        rootX = self.visualItemRect(self.invisibleRootItem()).x()

        # Get the rectangle of the viewport occupied by the pressed item
        vRect = self.visualItemRect(item)

        # Calculate the x coordinate of the item
        itemX = treeX + vRect.x() - rootX

        # Get the rect surrounding the icon
        iconRect = QRect(itemX, vRect.y(), vRect.height(), vRect.height())      

        if self.prevItem and (self.prevItem is not item):
            # Hide tooltip of former item
            self.prevItem.setToolTipDialogVisible(False)
        
        # Now check where the press event took place and handle it correspondingly
        if iconRect.contains(event.pos()):
            self.prevItem = item
            self.prevItem.setToolTipDialogVisible(True)
            
        QTreeWidget.mousePressEvent(self, event)


    def mimeData(self, items):
        mimeData = QMimeData()
        # Source type
        mimeData.setData("sourceType", "ParameterTreeWidget")
        return mimeData


    def checkApplyButtonsEnabled(self):
        # Returns a tuple containing the enabled and the conflicted state
        return self._r_applyButtonsEnabled(self.invisibleRootItem())


    def applyItem(self, item):
        """return a list of pairs (box, value) to be applied """

        editableComponent = item.editableComponent
        if (editableComponent is None or
                not isinstance(editableComponent, EditableApplyLaterComponent)
                or not editableComponent.applyEnabled):
            return [ ]

        return [(box, editableComponent.widgetFactory.value)
                for box in editableComponent.boxes]


    def applyRemoteChanges(self, item):
        editableComponent = item.editableComponent
        if editableComponent is None:
            return

        if not isinstance(editableComponent, EditableApplyLaterComponent):
            return

        editableComponent.onApplyRemoteChanges(item.box)


    def resetAll(self):
        nbSelectedItems = self.nbSelectedApplyEnabledItems()
        if nbSelectedItems > 0:
            selectedItems = self.selectedItems()
            for item in selectedItems:
                self.applyRemoteChanges(item)
        else:
            self.onApplyAllRemoteChanges()


    def nbSelectedApplyEnabledItems(self):
        # Return only selected items for not applied yet
        counter = 0
        for item in self.selectedItems():
            editableComponent = item.editableComponent
            if editableComponent is None:
                continue
            if not isinstance(editableComponent, EditableApplyLaterComponent):
                continue
            
            if editableComponent.applyEnabled:
                counter += 1
        return counter


    def setErrorState(self, inErrorState):
        self._r_setErrorStateItem(self.invisibleRootItem(), inErrorState)


    def setReadOnly(self, readOnly):
        self._r_setReadOnlyItem(self.invisibleRootItem(), readOnly)


### private functions ###
    def _r_setErrorStateItem(self, item, inErrorState):
        for i in range(item.childCount()):
            childItem = item.child(i)
            childItem.setErrorState(inErrorState)
            self._r_setErrorStateItem(childItem, inErrorState)


    def _r_setReadOnlyItem(self, item, readOnly):
        for i in range(item.childCount()):
            childItem = item.child(i)
            childItem.setReadOnly(readOnly)
            self._r_setReadOnlyItem(childItem, readOnly)


    def addContextAction(self, action):
        self.mContext.addAction(action)


    def addContextMenu(self, menu):
        self.mContext.addMenu(menu)


    def addContextSeparator(self):
        self.mContext.addSeparator()


    def globalAccessLevelChanged(self):
        rootItem = self.invisibleRootItem()
        for i in range(rootItem.childCount()):
            self._r_globalAccessLevelChanged(rootItem.child(i))


    def _r_globalAccessLevelChanged(self, item):
        if item.requiredAccessLevel > globals.GLOBAL_ACCESS_LEVEL:
            item.setHidden(True)
        else:
            item.setHidden(False)

        if not item.isChoiceElement and not item.isListElement:
            for i in range(item.childCount()):
                self._r_globalAccessLevelChanged(item.child(i))


    def _r_applyAll(self, item, config):
        # recursive function for tree to update the apply buttons
        for i in range(item.childCount()):
            childItem = item.child(i)
            self.addItemDataToHash(childItem, config)
            self._r_applyAll(childItem, config)


    def _r_applyAllRemoteChanges(self, item):
        # recursive function for tree to update the apply buttons
        for i in range(item.childCount()):
            childItem = item.child(i)
            self.applyRemoteChanges(childItem)
            self._r_applyAllRemoteChanges(childItem)


    def _r_applyButtonsEnabled(self, item):
        for i in range(item.childCount()):
            childItem = item.child(i)
            if isinstance(childItem, PropertyTreeWidgetItem):
                result = self._r_applyButtonsEnabled(childItem)
                if result[0]: # Bug: returns but
                    return result

        if (not isinstance(item, PropertyTreeWidgetItem) or
            item.editableComponent is None or
            not isinstance(item.editableComponent,
                           EditableApplyLaterComponent)):
            return False, False

        return (item.editableComponent.applyEnabled,
                item.editableComponent.hasConflict)


    def onApplyChanged(self, box, enable):
        # Called when apply button of editableComponent changed
        # Check if no apply button in tree is enabled/conflicted anymore
        result = self.checkApplyButtonsEnabled()
        self.signalApplyChanged.emit(box, result[0], result[1])


    def allItems(self):
        stack = [ ]
        item = self.invisibleRootItem()
        while True:
            stack.extend(item.child(i) for i in xrange(item.childCount()))
            if not stack:
                return
            item = stack.pop()
            yield item



    def onApplyAll(self):
        nbSelectedItems = self.nbSelectedApplyEnabledItems()
        if nbSelectedItems > 0:
            selectedItems = self.selectedItems()
        else:
            selectedItems = self.allItems()
        boxes = sum([self.applyItem(item) for item in selectedItems], [ ])
        Network().onReconfigure(boxes)


    def onApplyAllRemoteChanges(self):
        self._r_applyAllRemoteChanges(self.invisibleRootItem())


    def onCustomContextMenuRequested(self, pos):
        item = self.itemAt(pos)
        if item is None:
            # Show standard context menu
            self.mContext.exec_(QCursor.pos())
            return

        item.showContextMenu()


    def onApplyCurrentItemChanges(self):
        editableComponent = self.currentItem().editableComponent
        if editableComponent is None:
            return
        editableComponent.onApplyClicked()


    def onApplyCurrentItemRemoteChanges(self):
        editableComponent = self.currentItem().editableComponent
        if editableComponent is None:
            return
        editableComponent.onApplyRemoteChanges()

