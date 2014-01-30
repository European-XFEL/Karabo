#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents the treewidget of the configuration
   panel containing the parameters of a device class/instance.
"""

__all__ = ["ParameterTreeWidget"]


from editableapplylatercomponent import EditableApplyLaterComponent
from editablepathapplylatercomponent import EditablePathApplyLaterComponent
from enums import NavigationItemTypes
import globals
from manager import Manager
from treewidgetitems.propertytreewidgetitem import PropertyTreeWidgetItem
from treewidgetitems.attributetreewidgetitem import AttributeTreeWidgetItem

from PyQt4.QtCore import QMimeData, QRect, Qt
from PyQt4.QtGui import QAbstractItemView, QMenu, QTreeWidget


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
        
        self.setContextMenuPolicy(Qt.CustomContextMenu)
        self.__mContext = QMenu(self) # Actions from configurationPanel are added via addContextAction
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

        if self.__currentItem and (self.__currentItem is not item):
            # Hide tooltip of former item
            self.__currentItem.setToolTipDialogVisible(False)
        
        # Now check where the press event took place and handle it correspondingly
        if iconRect.contains(event.pos()):
            self.__currentItem = item
            self.__currentItem.setToolTipDialogVisible(True)
            
        QTreeWidget.mousePressEvent(self, event)


    def mimeData(self, items):
        item = items[0]

        # Attributes can not be dropped
        if isinstance(item, AttributeTreeWidgetItem):
            return
        
        mimeData = QMimeData()        

        # Put necessary data in MimeData:
        # Source type
        mimeData.setData("sourceType", "ParameterTreeWidget")
        # Internal key
        mimeData.setData("internalKey", item.internalKey)
        # Display name
        displayName = item.text(0)
        # Use DeviceClass/DeviceInstance-Key if no displayName is set
        if len(item.text(0)) == 0:
            keys = item.internalKey.split('.')
            displayName = keys[1]
        mimeData.setData("displayName", displayName)
        
        # Get NavigationItemType
        navigationItemType = self.__configPanel.getNavigationItemType()
        
        # Display component?
        hasDisplayComponent = navigationItemType == NavigationItemTypes.DEVICE
        mimeData.setData("hasDisplayComponent", "{}".format(hasDisplayComponent))
        # Editable component?
        hasEditableComponent = item.editableComponent is not None
        mimeData.setData("hasEditableComponent", "{}".format(hasEditableComponent))
        
        # TODO: HACK to get apply button disabled
        if hasEditableComponent:
            mimeData.setData("currentValue", "{}".format(item.editableComponent.value))
        
        if item.unitSymbol:
            mimeData.setData("unitSymbol", "{}".format(item.unitSymbol))
        if item.metricPrefixSymbol:
            mimeData.setData("metricPrefixSymbol", "{}".format(item.metricPrefixSymbol))

        if item.enumeration:
            enumerationString = str()
            nbEnums = len(item.enumeration)
            for i in xrange(nbEnums):
                enumerationString += item.enumeration[i]
                if i != (nbEnums-1):
                    enumerationString += ","
            mimeData.setData("enumeration", enumerationString)
            
        # Navigation item type
        mimeData.setData("navigationItemType", "{}".format(navigationItemType))
        # Class alias
        if item.classAlias:
            mimeData.setData("classAlias", "{}".format(item.classAlias))

        return mimeData


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


    def addItemDataToHash(self, item, config):
        editableComponent = item.editableComponent
        if editableComponent is None:
            return

        if not isinstance(editableComponent, EditableApplyLaterComponent) and \
           not isinstance(editableComponent, EditablePathApplyLaterComponent):
            return
        
        if editableComponent.applyEnabled:
            config.set(str(item.internalKey), editableComponent.value)
            editableComponent.changeApplyToBusy(True)


    def applyRemoteChanges(self, item):
        editableComponent = item.editableComponent
        if editableComponent is None:
            return

        if not isinstance(editableComponent, EditableApplyLaterComponent) and \
           not isinstance(editableComponent, EditablePathApplyLaterComponent):
            return
        
        if editableComponent.applyEnabled:
            editableComponent.onApplyRemoteChanges(item.internalKey)


    def nbSelectedApplyEnabledItems(self):
        # Return only selected items for not applied yet
        counter = 0
        for item in self.selectedItems():
            editableComponent = item.editableComponent
            if editableComponent is None:
                continue
            if not isinstance(editableComponent, EditableApplyLaterComponent) and \
               not isinstance(editableComponent, EditablePathApplyLaterComponent):
                continue
            
            if editableComponent.applyEnabled is True:
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
        self.__mContext.addAction(action)


    def addContextMenu(self, menu):
        self.__mContext.addMenu(menu)


    def addContextSeparator(self):
        self.__mContext.addSeparator()


    def _r_updateParameters(self, parentItem, state):
        for i in range(parentItem.childCount()):
            childItem = parentItem.child(i)
            childItem.updateState(state)
            self._r_updateParameters(childItem, state)


    def globalAccessLevelChanged(self):
        rootItem = self.invisibleRootItem()
        for i in range(rootItem.childCount()):
            self._r_globalAccessLevelChanged(rootItem.child(i))


    def _r_globalAccessLevelChanged(self, item):
        if item.requiredAccessLevel > globals.GLOBAL_ACCESS_LEVEL:
            item.setHidden(True)
        else:
            item.setHidden(False)

        if (item.isChoiceElement == False) and (item.isListElement == False):
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
                if result[0] is True: # Bug: returns but
                    return result

        if not isinstance(item, PropertyTreeWidgetItem):
            return (False, False)

        if (item.editableComponent is None) or \
           (not isinstance(item.editableComponent, EditableApplyLaterComponent) \
            and (not isinstance(item.editableComponent, EditablePathApplyLaterComponent))):
            return (False,False)

        return (item.editableComponent.applyEnabled, item.editableComponent.hasConflict)


### slots ###
    def onApplyChanged(self, key, enable):
        # Called when apply button of editableComponent changed
        # Check if no apply button in tree is enabled/conflicted anymore
        result = self.checkApplyButtonsEnabled()
        self.__configPanel.onApplyChanged(key, result[0], result[1])


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
        
        Manager().onFileOpen(configChangeType, str(self.instanceKey + ".configuration"), str(self.__classId))


    def onFileSaveAs(self):
        Manager().onSaveAsXml(str(self.__classId), str(self.instanceKey + ".configuration"))


    def onCustomContextMenuRequested(self, pos):
        item = self.itemAt(pos)
        if item is None:
            # Show standard context menu
            self.__mContext.exec_(QCursor.pos())
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

