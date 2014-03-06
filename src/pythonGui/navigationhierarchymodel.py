#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 1, 2013
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which represents a model to display a hierarchical
   navigation in a treeview."""

__all__ = ["NavigationHierarchyModel"]

import globals
from navigationhierarchynode import NavigationHierarchyNode
import manager

from PyQt4.QtCore import (QAbstractItemModel, QByteArray, QMimeData,
                          QModelIndex, Qt, pyqtSignal)
from PyQt4.QtGui import QItemSelectionModel, QIcon

from enums import NavigationItemTypes

class NavigationHierarchyModel(QAbstractItemModel):
    signalItemChanged = pyqtSignal(dict)


    def __init__(self, parent=None):
        super(NavigationHierarchyModel, self).__init__(parent)
        
        self.__rootItem = NavigationHierarchyNode("Hierarchical view")
        self.__currentConfig = None
        self.setSupportedDragActions(Qt.CopyAction)
        self.selection_model = QItemSelectionModel(self)
        self.selection_model.selectionChanged.connect(self.onSelectionChanged)


    def _currentConfig(self):
        return self.__currentConfig
    currentConfig = property(fget=_currentConfig)


    def updateData(self, config):
        #print "+++ NavigationHierarchyModel.updateData"
        #print config
        #print ""
        
        # Needed for GLOBAL_ACCESS_LEVEL changes
        if self.__currentConfig != config:
            self.__currentConfig = config

        selectedIndexes = self.selection_model.selectedIndexes()
        if selectedIndexes:
            lastSelectionPath = selectedIndexes[0].internalPointer().path
        else:
            lastSelectionPath = None
        
        self.beginResetModel()
        self.__rootItem.clearChildItems()
        
        # Get server data
        serverKey = "server"
        if config.has(serverKey):
            serverConfig = config.get(serverKey)
            serverIds = list()
            serverConfig.getKeys(serverIds)
            for serverId in serverIds:
                # Get attributes
                #serverAttributes = serverConfig.getAttributes(serverId)
                #version = serverConfig.getAttribute(serverId, "version")
                
                host = str("UNKNOWN")
                if serverConfig.hasAttribute(serverId, "host"):
                    host = serverConfig.getAttribute(serverId, "host")

                visibility = serverConfig.getAttribute(serverId, "visibility")
                if visibility > globals.GLOBAL_ACCESS_LEVEL:
                    continue
                
                # Host item already exists?
                hostItem = self.__rootItem.getItem(host)
                if not hostItem:
                    hostItem = NavigationHierarchyNode(host, host, self.__rootItem)
                    self.__rootItem.appendChildItem(hostItem)
                
                path = "server." + serverId
                serverItem = NavigationHierarchyNode(serverId, path, hostItem)
                hostItem.appendChildItem(serverItem)
            
                if serverConfig.hasAttribute(serverId, "deviceClasses"):
                    classes = serverConfig.getAttribute(serverId, "deviceClasses")
                    visibilities = serverConfig.getAttribute(serverId, "visibilities")
                    i = 0
                    for deviceClass in classes:
                        if visibilities[i] <= globals.GLOBAL_ACCESS_LEVEL:
                            path = "server." + serverId + ".classes." + deviceClass
                            classItem = NavigationHierarchyNode(deviceClass, path, serverItem)
                            serverItem.appendChildItem(classItem)
                        i = i + 1
        
        # Get device data
        deviceKey = "device"
        if config.has(deviceKey):
            deviceConfig = config.get(deviceKey)
            deviceIds = list()
            deviceConfig.getKeys(deviceIds)
            for deviceId in deviceIds:
                # Get attributes
                visibility = deviceConfig.getAttribute(deviceId, "visibility")
                if visibility > globals.GLOBAL_ACCESS_LEVEL:
                    continue
                
                host = deviceConfig.getAttribute(deviceId, "host")
                classId = deviceConfig.getAttribute(deviceId, "classId")
                serverId = deviceConfig.getAttribute(deviceId, "serverId")
                #version = deviceConfig.getAttribute(deviceId, "version")
                status = "ok"
                if deviceConfig.hasAttribute(deviceId, "status"):
                    status = deviceConfig.getAttribute(deviceId, "status")

                # Host item already exists?
                hostItem = self.__rootItem.getItem(host)
                if not hostItem:
                    hostItem = NavigationHierarchyNode(host, host, self.__rootItem)
                    self.__rootItem.appendChildItem(hostItem)
                
                # Server item already exists?
                serverItem = hostItem.getItem(serverId)
                if not serverItem:
                    if serverId == "__none__":
                        path = "server." + serverId
                        serverItem = NavigationHierarchyNode(serverId, path, hostItem)
                        hostItem.appendChildItem(serverItem)
                    else:
                        continue

                # Class item already exists?
                classItem = serverItem.getItem(classId)
                if not classItem:
                    if serverId == "__none__":
                        path = "server." + serverId + ".classes." + classId
                        classItem = NavigationHierarchyNode(classId, path, serverItem)
                        serverItem.appendChildItem(classItem)
                    else:
                        continue

                path = deviceId
                deviceItem = NavigationHierarchyNode(deviceId, path, classItem)
                deviceItem.status = status
                classItem.appendChildItem(deviceItem)

        self.endResetModel()
        if lastSelectionPath is not None:
            self.selectPath(lastSelectionPath)


    def onSelectionChanged(self, selected, deselected):
        selectedIndexes = selected.indexes()
        if len(selectedIndexes) < 1:
            return
        
        index = selectedIndexes[0]

        if not index.isValid():
            return

        level = self.getHierarchyLevel(index)

        classId = None
        path = ""

        if level == 0:
            type = NavigationItemTypes.HOST
        elif level == 1:
            type = NavigationItemTypes.SERVER
            path = "server." + index.data()
        elif level == 2:
            type = NavigationItemTypes.CLASS
            parentIndex = index.parent()
            serverId = parentIndex.data()
            classId = index.data()

            schema = manager.Manager().getClassSchema(serverId, classId)
            path = "{}.{}".format(serverId, classId)
            manager.Manager().onSchemaAvailable(dict(key=path, classId=classId,
                                                     type=type, schema=schema))
        elif level == 3:
            type = NavigationItemTypes.DEVICE
            deviceId = index.data()
            classIndex = index.parent()
            classId = classIndex.data()
            #serverIndex = classIndex.parent()
            #serverId = serverIndex.data()

            schema = manager.Manager().getDeviceSchema(deviceId)
            path = deviceId
            manager.Manager().onSchemaAvailable(dict(key=path, classId=classId,
                                                     type=type, schema=schema))

        itemInfo = dict(key=path, classId=classId, type=type)
        self.signalItemChanged.emit(itemInfo)


    def selectIndex(self, index):
        if not index:
            return

        path = index.internalPointer().path
        self.selection_model.setCurrentIndex(index,
                                             QItemSelectionModel.ClearAndSelect)


    def getHierarchyLevel(self, index):
        # Find out the hierarchy level of the selected item
        hierarchyLevel = 0
        seekRoot = index
        while seekRoot.parent() != QModelIndex():
            seekRoot = seekRoot.parent()
            hierarchyLevel += 1
        return hierarchyLevel


    def findIndex(self, path):
        # Recursive search
        return self._rFindIndex(self.__rootItem, path)


    def _rFindIndex(self, item, path):
        for i in xrange(item.childCount()):
            childItem = item.childItem(i)
            resultItem = self._rFindIndex(childItem, path)
            if resultItem:
                return resultItem

        if item.path != "" and path.startswith(item.path):
            return self.createIndex(item.row(), 0, item)
        return None


    def selectPath(self, path):
        index = self.findIndex(path)
        if index is not None:
            self.selection_model.select(index,
                                        QItemSelectionModel.ClearAndSelect)


    def rowCount(self, parent=QModelIndex()):
        
        if parent.column() > 0:
            #print "rowCount 1"
            return None

        if not parent.isValid():
            #print "root"
            parentItem = self.__rootItem
        else:
            #print "parent"
            parentItem = parent.internalPointer()

        #print "rowCount", parentItem.childCount()
        return parentItem.childCount()


    def columnCount(self, parentIndex=QModelIndex()):
        return 1


    def data(self, index, role=Qt.DisplayRole):
        
        #row = index.row()
        column = index.column()

        if role == Qt.DisplayRole:
            item = index.internalPointer()
            return item.data(column)
        elif (role == Qt.DecorationRole) and (column == 0):
            # Find out the hierarchy level of the selected item
            hierarchyLevel = self.getHierarchyLevel(index)
            if hierarchyLevel == 0:
                return QIcon(":host")
            elif hierarchyLevel == 1:
                #status = self.rawData(level, row, 3)
                #if status == "offline":
                #    return QIcon(":no")
                #elif status == "starting" or status == "online":
                return QIcon(":yes")
            elif hierarchyLevel == 2:
                return QIcon(":device-class")
            elif hierarchyLevel == 3:
                item = index.internalPointer()
                if item.status == "error":
                    return QIcon(":device-instance-error")
                else:
                    return QIcon(":device-instance")


    def flags(self, index):
        if not index.isValid():
            return None
        ret = Qt.ItemIsEnabled | Qt.ItemIsSelectable
        if self.getHierarchyLevel(index) > 0:
            ret |= Qt.ItemIsDragEnabled
        return ret


    def headerData(self, section, orientation, role):
        
        if role == Qt.DisplayRole:
            if (orientation == Qt.Horizontal) and (section == 0):
                    return "Hierarchical view"


    def index(self, row, column, parent=QModelIndex()):
        if not self.hasIndex(row, column, parent):
            return QModelIndex()

        if not parent.isValid():
            parentItem = self.__rootItem
        else:
            parentItem = parent.internalPointer()

        childItem = parentItem.childItem(row)
        if childItem:
            return self.createIndex(row, column, childItem)
        else:
            return QModelIndex()


    def parent(self, index):
        if not index.isValid():
            return QModelIndex()
        
        childItem = index.internalPointer()
        if not childItem:
            return QModelIndex()
        
        parentItem = childItem.parentItem
        if not parentItem:
            return QModelIndex()
        
        if parentItem == self.__rootItem:
            return QModelIndex()

        return self.createIndex(parentItem.row(), 0, parentItem)


    def indexInfo(self, index):
        if not index.isValid():
            return { }

        level = self.getHierarchyLevel(index)

        if level == 0:
            type = NavigationItemTypes.HOST
            return { }
        elif level == 1:
            type = NavigationItemTypes.SERVER
            serverId = index.data()
            path = serverId
            return dict(key=path, type=type, serverId=serverId)
        elif level == 2:
            type = NavigationItemTypes.CLASS
            parentIndex = index.parent()
            serverId = parentIndex.data()
            classId = index.data()
            path = "{}.{}".format(serverId, classId)
            return dict(key=path, type=type, serverId=serverId, classId=classId)
        elif level == 3:
            type = NavigationItemTypes.DEVICE
            parentIndex = index.parent()
            classId = parentIndex.data()
            deviceId = index.data()
            path = deviceId
            return dict(key=path, type=type, classId=classId, deviceId=deviceId)


    def mimeData(self, items):
        itemInfo = self.indexInfo(items[0])

        serverId  = itemInfo.get("serverId")
        navigationItemType = itemInfo.get("type")
        displayName = ""

        if navigationItemType == NavigationItemTypes.CLASS:
            displayName = itemInfo.get("classId")

        mimeData = QMimeData()

        mimeData.setData("sourceType", "NavigationTreeView")
        if navigationItemType:
            mimeData.setData("navigationItemType",
                             QByteArray.number(navigationItemType))
        if serverId:
            mimeData.setData("serverId", serverId)
        if displayName:
            mimeData.setData("displayName", displayName)

        return mimeData
