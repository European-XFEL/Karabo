#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the treewidget of the configuration
   panel containing the parameters of a device class/instance.
"""

__all__ = ["ParameterTreeWidget"]


import treewidgetitems.attributetreewidgetitem
from editableapplylatercomponent import EditableApplyLaterComponent
from enums import *
import globals
from karabo.karathon import *
from manager import Manager

from PyQt4.QtCore import *
from PyQt4.QtGui import *


class ParameterTreeWidget(QTreeWidget):


    def __init__(self, configPanel, path=str(), classId=str()):
        # configPanel - save parent widget for toolbar buttons
        # path - full path of navigationItem
        super(ParameterTreeWidget, self).__init__()
        
        self.__classId = classId # DeviceClass name stored for XML save
        self.__instanceKey = path
        self.__configPanel = configPanel
        self.__currentItem = None

        self.setWordWrap(True)
        self.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.setSelectionMode(QAbstractItemView.ExtendedSelection)
        #self.setSortingEnabled(True)
        self.sortByColumn(0, Qt.AscendingOrder)
        
        self._setupActions()
        self._setupContextMenu()
        
        #self.setMouseTracking(True)
        #self.itemEntered.connect(self.onItemEntered)
        self.customContextMenuRequested.connect(self.onCustomContextMenuRequested)


### protected ###
    def mousePressEvent(self, event):
        QTreeWidget.mousePressEvent(self, event)
        
        if event.buttons() != Qt.LeftButton:
            return
        
        self._performDrag()


    def _performDrag(self):
        item = self.currentItem()
        if item is None:
            return
        
        # Attributes can not be dropped
        if isinstance(item, treewidgetitems.attributetreewidgetitem.AttributeTreeWidgetItem):
            return
        
        mimeData = QMimeData()        

        # Put necessary data in MimeData:
        # Source type
        mimeData.setData("sourceType", "ParameterTreeWidget")
        # Internal key
        mimeData.setData("internalKey", QString(item.internalKey).toAscii())
        # Display name
        displayName = item.text(0)
        # Use DeviceClass/DeviceInstance-Key if no displayName is set
        if len(item.text(0)) == 0:
            keys = item.internalKey.split('.')
            displayName = keys[1]
        mimeData.setData("displayName", displayName.toAscii())
        
        # Get NavigationItemType
        navigationItemType = self.__configPanel.getNavigationItemType()
        
        # Display component?
        hasDisplayComponent = navigationItemType == NavigationItemTypes.DEVICE
        mimeData.setData("hasDisplayComponent", QString("%1").arg(hasDisplayComponent).toAscii())
        # Editable component?
        hasEditableComponent = item.editableComponent is not None
        mimeData.setData("hasEditableComponent", QString("%1").arg(hasEditableComponent).toAscii())
        # Navigation item type
        mimeData.setData("navigationItemType", QString("%1").arg(navigationItemType).toAscii())
        # Class alias
        if item.classAlias:
            mimeData.setData("classAlias", QString("%1").arg(item.classAlias).toAscii())

        drag = QDrag(self)
        drag.setMimeData(mimeData)
        if drag.exec_(Qt.MoveAction) == Qt.MoveAction:
            pass


### getter & setter functions ###
    def _getInstanceKey(self):
        return self.__instanceKey
    def _setInstanceKey(self, instanceKey):
        self.__instanceKey = instanceKey
    instanceKey = property(fget=_getInstanceKey, fset=_setInstanceKey)


### public functions ###
    def checkApplyButtonsEnabled(self):
        # Returns a tuple containing the enabled and the conflicted state
        return self._r_applyButtonsEnabled(self.invisibleRootItem())


    def getParameterTreeWidgetItemByKey(self, key):
        return self.__configPanel.getParameterTreeWidgetItemByKey(key)


    def stateUpdated(self, state):
        self._r_updateParameters(self.invisibleRootItem(), state)


    def setActionsVisible(self, visible):
        # Called when NavigationItem changed
        self.__acFileOpen.setVisible(visible)
        self.__acFileSaveAs.setVisible(visible)
        self.__acItemsVisibility.setVisible(visible)


    def addItemDataToHash(self, item, config):
        editableComponent = item.editableComponent
        if not isinstance(editableComponent, EditableApplyLaterComponent):
            return
        
        if (editableComponent is not None) and (editableComponent.applyEnabled is True):
            config.set(str(item.internalKey), editableComponent.value)
            editableComponent.changeApplyToBusy(True)


    def applyRemoteChanges(self, item):
        editableComponent = item.editableComponent
        if not isinstance(editableComponent, EditableApplyLaterComponent):
            return
        
        if (editableComponent is not None) and (editableComponent.applyEnabled is True):
            editableComponent.onApplyRemoteChanges(item.internalKey)


    def nbSelectedApplyEnabledItems(self):
        # Return only selected items for not applied yet
        counter = 0
        for item in self.selectedItems():
            editableComponent = item.editableComponent
            if (editableComponent is None) or (not isinstance(editableComponent, EditableApplyLaterComponent)):
                continue
            
            if editableComponent.applyEnabled is True:
                counter += 1
        return counter


    def setErrorState(self, isError):
        self._r_setErrorStateItem(self.invisibleRootItem(), isError)


    def setReadOnly(self, readOnly):
        self._r_setReadOnlyItem(self.invisibleRootItem(), readOnly)


### private functions ###
    def _r_setErrorStateItem(self, item, isError):
        for i in range(item.childCount()):
            childItem = item.child(i)
            childItem.setErrorState(isError)
            self._r_setErrorStateItem(childItem, isError)


    def _r_setReadOnlyItem(self, item, readOnly):
        for i in range(item.childCount()):
            childItem = item.child(i)
            childItem.setReadOnly(readOnly)
            self._r_setReadOnlyItem(childItem, readOnly)


    def _setupActions(self):
        text = "Open configuration (*.xml)"
        self.__acFileOpen = QAction(QIcon(":open"), "&Open configuration", self)
        self.__acFileOpen.setStatusTip(text)
        self.__acFileOpen.setToolTip(text)
        self.__acFileOpen.triggered.connect(self.onFileOpen)
        self.__configPanel.addActionToToolBar(self.__acFileOpen)

        text = "Save configuration as (*.xml)"
        self.__acFileSaveAs = QAction(QIcon(":save-as"), "Save &As...", self)
        self.__acFileSaveAs.setStatusTip(text)
        self.__acFileSaveAs.setToolTip(text)
        self.__acFileSaveAs.triggered.connect(self.onFileSaveAs)
        self.__configPanel.addActionToToolBar(self.__acFileSaveAs)

        text = "Show expert values"
        self.__acItemsVisibility = QAction(QIcon(":enum"), text, self)
        self.__acItemsVisibility.setToolTip(text)
        self.__acItemsVisibility.setStatusTip(text)
        self.__acItemsVisibility.setCheckable(True)
        self.__acItemsVisibility.setChecked(False)
        self.__acItemsVisibility.toggled.connect(self.onAllItemsVisibility)
        self.__configPanel.addActionToToolBar(self.__acItemsVisibility)
        

    def _setupContextMenu(self):
        self.setContextMenuPolicy(Qt.CustomContextMenu)
        
        # add toolbar menu also in context menu
        self.__mFile = QMenu(self)
        self.__mFile.addAction(self.__acFileOpen)
        self.__mFile.addAction(self.__acFileSaveAs)
        self.__mFile.addAction(self.__acItemsVisibility)
        self.__mFile.addSeparator()
        # other action from configurationPanel gets added via addConfigAction


    def addConfigAction(self, action):
        self.__mFile.addAction(action)


    def addConfigMenu(self, menu):
        self.__mFile.addMenu(menu)


    def _r_updateParameters(self, parentItem, state):
        for i in range(parentItem.childCount()):
            childItem = parentItem.child(i)
            childItem.updateState(state)
            self._r_updateParameters(childItem, state)


    def _r_setItemVisibility(self, item, show):
        if show == False:
            if item.requiredAccessLevel > globals.GLOBAL_ACCESS_LEVEL:
                item.setHidden(True)
            else:
                item.setHidden(False)
        else:
            item.setHidden(False)

        if (item.isChoiceElement == False) and (item.isListElement == False):
            for i in range(item.childCount()):
                self._r_setItemVisibility(item.child(i), show)


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
            if type(childItem) == treewidgetitems.propertytreewidgetitem.PropertyTreeWidgetItem:
                result = self._r_applyButtonsEnabled(childItem)
                if result[0] is True: # Bug: returns but
                    return result

        if (type(item) != treewidgetitems.propertytreewidgetitem.PropertyTreeWidgetItem) or (item.editableComponent is None) or \
           (not isinstance(item.editableComponent, EditableApplyLaterComponent)):
            return (False,False)
        return (item.editableComponent.applyEnabled, item.editableComponent.hasConflict)


### slots ###
    def onItemEntered(self, item, column):
        
        if self.__currentItem and  (self.__currentItem != item):
            self.__currentItem.setToolTipDialogVisible(False)
        self.__currentItem = item
        self.__currentItem.setToolTipDialogVisible(True)


    def onApplyChanged(self, enable):
        # Called when apply button of editableComponent changed
        # Check if no apply button in tree is enabled/conflicted anymore
        result = self.checkApplyButtonsEnabled()
        self.__configPanel.onApplyChanged(result[0], result[1])


    def onApplyAll(self, config):
        # go trough tree and save changes into config
        self._r_applyAll(self.invisibleRootItem(), config)


    def onApplyAllRemoteChanges(self):
        self._r_applyAllRemoteChanges(self.invisibleRootItem())


    def onFileOpen(self):
        type = self.__configPanel.getNavigationItemType()
        configChangeType = None
        if type is NavigationItemTypes.CLASS:
            configChangeType = ConfigChangeTypes.DEVICE_CLASS_CONFIG_CHANGED
        elif type is NavigationItemTypes.DEVICE:
            configChangeType = ConfigChangeTypes.DEVICE_INSTANCE_CONFIG_CHANGED
        
        if self.__classId is None:
            print "onFileOpen classId not set"
            #keys = self.instanceKey.split('+', 1)
            #if len(keys) is 2:
            #    self.__classId = str(keys[1])
        
        # TODO: Remove dirty hack for scientific computing again!!!
        croppedClassId = self.__classId.split("-")
        self.__classId = croppedClassId[0]
        
        Manager().onFileOpen(configChangeType, str(self.instanceKey), str(self.__classId))


    def onFileSaveAs(self):
        Manager().onSaveAsXml(str(self.__classId), self.instanceKey)


    def onAllItemsVisibility(self, show):
        if show == True :
            text = "Hide expert values"
            self.__acItemsVisibility.setToolTip(text)
            self.__acItemsVisibility.setStatusTip(text)
        else :
            text = "Show expert values"
            self.__acItemsVisibility.setToolTip(text)
            self.__acItemsVisibility.setStatusTip(text)

        rootItem = self.invisibleRootItem()
        for i in range(rootItem.childCount()):
            self._r_setItemVisibility(rootItem.child(i), show)


    def onCustomContextMenuRequested(self, pos):
        item = self.itemAt(pos)
        if item is None:
            # Show standard context menu
            self.__mFile.exec_(QCursor.pos())
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

